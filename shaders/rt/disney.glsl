#ifndef DISNEY_GLSL
#define DISNEY_GLSL

#define METAL_ALPHA_MIN 1e-3f
#define DISNEY_EPS 1e-6f

// TODO: can be optimized, because in local frame, we know normal is (0, 0, 1)
float fd(in float n_dot_d_a, in float f90) {
    return (1.0f + (f90 - 1.0f) * pow(1.0f - n_dot_d_a, 5));
}

float G(in vec3 d, in float alpha_x, in float alpha_y) {
    float tx = d[0] * alpha_x;
    float ty = d[1] * alpha_y;
    float tz = max(abs(d[2]), DISNEY_EPS);
    float lambda_d = 0.5f * (sqrt((tx * tx + ty * ty) / (tz * tz)) - 1);
    return 1.0f / (1 + lambda_d);
}

// G = 1 / (1 + Lambda(omega))
// Lambda(omega) = (-1 + sqrt(1 + alpha^2 * tan^2(theta))) / 2
// tan^2(theta) = sqrt(1 - v_z^2) / v_z
float smith_GGX(float v_z, float roughness) {
    float a = roughness * roughness;
    float b = v_z * v_z;
    return (2 * v_z) / (v_z + sqrt(a + b - a * b));
}

float R0(in float eta) {
    float t = (eta - 1.0f) / (1.0f + eta);
    return t * t;
}

float luminance(in vec3 color) {
    return dot(color, vec3(0.212671f, 0.715160f, 0.072169f));
}

void eval_dm_dot_gm(out float dm_dot_gm, in float anisotropic, in float roughness, in vec3 in_l,
                    in vec3 h_l, in vec3 out_l) {
    // [2.2] Normal distribution function
    // anisotropic Trowbridge-Reitz distribution (GGX: Ground Glass X)
    const float aspect = sqrt(1 - 0.9f * anisotropic);
    const float roughness_2 = roughness * roughness;
    float alpha_x = max(METAL_ALPHA_MIN, roughness_2 / aspect);
    float alpha_y = max(METAL_ALPHA_MIN, roughness_2 * aspect);
    // float dm_sub_denominator = h_l[0] * h_l[0] / (alpha_x * alpha_x) + h_l[1] * h_l[1] / (alpha_y
    // * alpha_y) + h_l[2] * h_l[2];
    vec3 t_sub = h_l / vec3(alpha_x, alpha_y, 1.0f);
    float dm_sub_denominator = dot(t_sub, t_sub);
    float dm = 1.0f / max(DISNEY_EPS,
                          (BB_PI * alpha_x * alpha_y * dm_sub_denominator * dm_sub_denominator));

    // [2.3] Geometric shadowing function
    // Smith's method
    // float gm = G(in_l, alpha_x, alpha_y) * G(out_l, alpha_x, alpha_y);
    float gm = smith_GGX(abs(in_l[2]), roughness) * smith_GGX(abs(out_l[2]), roughness);
    dm_dot_gm = dm * gm;
}

// bsdf = BSDF * cos
void eval_disney(in Disney disney, in vec3 normal, in vec3 direction_out, in vec3 direction_in,
                 out vec3 bsdf) {
    mat3 local2world;
    const vec3 a = (abs(normal[0]) > 0.9f) ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f);
    local2world[2] = normal;  // local's z-axis is normal (description in world frame)
    local2world[1] = normalize(cross(normal, a));
    local2world[0] = normalize(cross(normal, local2world[1]));  // left hand
    local2world = transpose(local2world);                       // col majar, so we need transpose

    vec3 in_l = normalize(local2world * direction_in);
    vec3 out_l = normalize(local2world * direction_out);
    const vec3 n_l = vec3(0, 0, 1.0f);

    vec3 h_l;
    if (in_l[2] * out_l[2] < 0) {
        // refraction
        if (in_l[2] < 0) {
            // in -> out
            disney._eta = 1.0f / disney._eta;
        }
        // "Generalized half-vector" from Walter et al.
        // See "Microfacet Models for Refraction through Rough Surfaces"
        h_l = normalize(in_l + out_l * disney._eta);
        if (h_l[2] < 0) {
            // flip half-vector if it's below surface
            // TODO: h_l[2] = -h_l[2]?
            h_l = -h_l;
        }
    } else {
        h_l = normalize(out_l + in_l);
    }

    float h_dot_o = dot(h_l, out_l);
    float h_dot_o_a = clamp(abs(h_dot_o), 0, 1.0f);
    // float h_dot_o_a = max(DISNEY_EPS, abs(h_dot_o));
    // h_dot_o = sign(h_dot_o) * h_dot_o_a;
    float h_dot_o_2 = h_dot_o_a * h_dot_o_a;

    float n_dot_o = dot(n_l, out_l);
    float n_dot_o_a = clamp(abs(n_dot_o), 0, 1.0f);
    // float n_dot_o_a = max(DISNEY_EPS, abs(n_dot_o));
    // n_dot_o = sign(n_dot_o) * n_dot_o_a;

    float n_dot_i = dot(n_l, in_l);
    float n_dot_i_a = clamp(abs(n_dot_i), 0, 1.0f);
    // float n_dot_i_a = max(DISNEY_EPS, abs(n_dot_i));
    // n_dot_i = sign(n_dot_i) * n_dot_i_a;

    const bool is_outgoing = (n_dot_i < 0) || (n_dot_o < 0);
    vec3 diffuse = vec3(0);

    // [1] diffuse
    if (!is_outgoing) {
        // outgoing: No light below the surface
        // [1.1] base diffuse
        float fd90 = 0.5f + 2 * disney._roughness * h_dot_o_2;
        float fd_o = fd(n_dot_o_a, fd90);
        float fd_i = fd(n_dot_i_a, fd90);
        float base_diffuse_factor = fd_o * fd_i;

        // [1.2] subsurface
        float fss90 = disney._roughness * h_dot_o_2;
        float fss_o = fd(n_dot_o_a, fss90);
        float fss_i = fd(n_dot_i_a, fss90);
        float subsurface_factor =
            1.25f * (0.5f + fss_o * fss_i * (1.0f / (n_dot_i_a + n_dot_o_a) - 0.5f));

        float diffuse_factor =
            (1 - disney._subsurface) * base_diffuse_factor + disney._subsurface * subsurface_factor;
        diffuse_factor *= n_dot_o * BB_PI_INV;  // n_dot_o: cos
        diffuse = diffuse_factor * disney._base_color;
    }

    // [2] metal
    vec3 metal = vec3(0);
    float dm_dot_gm = 0;
    if (!is_outgoing) {
        // Cook-Torrance microfacet BRDF
        // f_metal = f_m * d_m * g_m / (4 * |n_dot_i| * n_dot_o)
        //  note: The cosine term in the denominator cancels out with the cosine.(BSDF * cos)

        // [2.1] Fresnel term
        // vec3 fm = pow(1 - h_dot_o_a, 5) * (1.0f - disney._base_color) + disney._base_color;

        // modified for the whole BSDF
        // const vec3 c_specular = vec3(1.0f, 1.0f, 1.0f);
        // vec3 ks = (1 - disney._specular_tint) + disney._specular_tint * c_specular;
        // vec3 c0 = disney._specular * R0(disney._eta) * (1 - disney._metallic) * ks +
        // disney._metallic * disney._base_color; vec3 fm_hat = pow(1 - h_dot_o_a, 5) * (1.0f - c0)
        // + c0;
        float fm1 = pow(1 - h_dot_o_a, 5);
        float r0 = R0(disney._eta);
        float fm2 = r0 + (1 - r0) * fm1;
        float fm_hat_factor = (1 - disney._metallic) * fm2 + disney._metallic * fm1;
        // vec3 fm_hat = fm_hat_factor * vec3(1.0f, 1.0f, 1.0f) + (1 - fm_factor) *
        // disney._base_color;
        vec3 fm_hat =
            disney._base_color - fm_hat_factor * (vec3(1.0f, 1.0f, 1.0f) - disney._base_color);

        // [2.2 & 2.3]
        eval_dm_dot_gm(dm_dot_gm, disney._anisotropic, disney._roughness, in_l, h_l, out_l);
        metal = (0.25f * dm_dot_gm / max(n_dot_i_a, DISNEY_EPS)) * fm_hat;  // *fm;
    }

    // [3] Closecoat
    float clearcoat_factor = 0;
    if (!is_outgoing) {
        // f_closecoat = f_c * d_c * g_c / (4 * |n_dot_i| * n_dot_o)
        //  note: The cosine term in the denominator cancels out with the cosine.(BSDF * cos)
        float r0 = R0(disney._eta);
        float fc = r0 + (1 - r0) * pow(1 - h_dot_o_a, 5);

        float alpha_g = (1 - disney._clearcoat_gloss) * 0.1f + disney._clearcoat_gloss * 0.001f;
        float alpha_g_2 = alpha_g * alpha_g;
        float dc =
            (alpha_g_2 - 1) / (BB_PI * log(alpha_g_2) * (1 + (alpha_g_2 - 1) * h_l[2] * h_l[2]));
        // float gc = G(in_l, 0.25f, 0.25f) * G(out_l, 0.25f, 0.25f);
        float gc = smith_GGX(in_l[2], 0.25f) * smith_GGX(out_l[2], 0.25f);

        clearcoat_factor = 0.25f * fc * dc * gc / n_dot_i_a;
    }

    // [4] Glass
    // TODO: there remains errors
    vec3 glass = vec3(0);
    if (disney._is_refractive > 0) {
        float h_dot_i = dot(h_l, in_l);
        float fg = 1.0f;
        if (disney._eta * disney._eta + n_dot_i * n_dot_i >= 1) {
            // not total reflection
            float rs = (h_dot_i - disney._eta * h_dot_o) /
                       max(h_dot_i + disney._eta * h_dot_o, DISNEY_EPS);
            float rp = (disney._eta * h_dot_i - h_dot_o) /
                       max(disney._eta * h_dot_i + h_dot_o, DISNEY_EPS);
            fg = 0.5f * (rs * rs + rp * rp);
            // fg = clamp(fg, 0.0f, 1.0f); // too large?
        }

        if (dm_dot_gm == 0) {
            eval_dm_dot_gm(dm_dot_gm, disney._anisotropic, disney._roughness, in_l, h_l, out_l);
        }
        if (n_dot_i * n_dot_o > 0) {
            // reflection
            glass = (0.25f * fg * dm_dot_gm / max(n_dot_i_a, DISNEY_EPS)) * disney._base_color;
        } else {
            // refraction
            float gf_sub_dominantor = h_dot_i + disney._eta * h_dot_o;
            gf_sub_dominantor *= gf_sub_dominantor;
            glass = ((1 - fg) * dm_dot_gm / max(n_dot_i_a * gf_sub_dominantor, DISNEY_EPS)) *
                    sqrt(disney._base_color);
        }
    }

    // [5] Sheen
    vec3 sheen = vec3(0);
    if (!is_outgoing) {
        float s_lu = luminance(disney._base_color);
        float sheen_factor = pow(1 - h_dot_o_a, 5) * n_dot_o_a;  // n_dot_o_a: |cos|
        // if (s_lu > 0) {
        // sheen = disney._base_color / s_lu; // assert(s_lu > 0)
        // } else {
        //     sheen = vec3(1.0f, 1.0f, 1.0f);
        // }
        sheen = sheen_factor * ((1 - disney._sheen_tint) +
                                ((disney._sheen_tint / s_lu) *
                                 disney._base_color));  // the last item is the `if clause` above
    }
    bsdf = disney._metallic * metal + (1 - disney._metallic) * diffuse;  // TODO
                                                                         // bsdf = clearcoat_factor * disney._base_color;
                                                                         // bsdf = glass;
                                                                         // bsdf = sheen;
                                                                         // bsdf = (1 - disney._specular_transmission) * (1 - disney._metallic) * diffuse
                                                                         //     + (1 - disney._metallic) * disney._sheen * sheen
                                                                         //     + (1 - disney._specular_transmission * (1 - disney._metallic)) * metal
                                                                         //     + 0.25f * disney._clearcoat * clearcoat_factor * disney._base_color;
                                                                         //+ (1 - disney._metallic) * disney._specular_transmission * glass;
}

#endif