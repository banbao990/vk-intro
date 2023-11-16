#ifndef DISNEY_GLSL
#define DISNEY_GLSL

float fd(in vec3 normal, const in float f90, in vec3 direction) {
    float n_dot_d = dot(normal, direction);
    return (1.0f + (f90 - 1.0f) * (1.0f - pow(n_dot_d, 5.0f)));
}

void eval_disney(inout uint wseed, in Disney disney,
    in vec3 normal, in vec3 direction_out, in vec3 direction_in,
    out vec3 bsdf
) {
    vec3 h = normalize(direction_out + direction_in);

    // [1] diffuse
    // [1.1] base diffuse
    float h_dot_o_2 = dot(h, direction_out);
    h_dot_o_2 *= h_dot_o_2;
    float n_dot_o = dot(normal, direction_out);

    float fd90 = 0.5f + 2 * disney._roughness * h_dot_o_2;
    float fd_o = fd(normal, fd90, direction_out);
    float fd_i = fd(normal, fd90, direction_in);
    float base_diffuse_factor = fd_o * fd_i;

    // [1.2] subsurface
    float fss90 = disney._roughness * h_dot_o_2;
    float fss_o = fd(normal, fss90, direction_out);
    float fss_i = fd(normal, fss90, direction_in);
    float subsurface_factor = 1.25f * fss_o * fss_i;

    float diffuse_factor = (1 - disney._subsurface) * base_diffuse_factor
        + disney._subsurface * subsurface_factor;
    diffuse_factor *= n_dot_o * BB_PI_INV;
    bsdf = diffuse_factor * disney._base_color;

    //vec3 h = normalize(l + v);
    //float ndotl = max(dot(n, l), 0.0f);
    //float ndotv = max(dot(n, v), 0.0f);
    //float ndoth = max(dot(n, h), 0.0f);
    //float ldoth = max(dot(l, h), 0.0f);
    //float vdoth = max(dot(v, h), 0.0f);

    //float alpha = roughness * roughness;
    //float alpha2 = alpha * alpha;
    //float d = (alpha2) / (BB_PI * pow(ndoth * ndoth * (alpha2 - 1.0f) + 1.0f, 2.0f));
    //float g = min(1.0f, min(2.0f * ndoth * ndotv / vdoth, 2.0f * ndoth * ndotl / ldoth));
    //float f = 0.5f + 2.0f * ldoth * ldoth * alpha2;
    //float f0 = pow(1.0f - vdoth, 5.0f);
    //float f90 = 0.5f + 2.0f * vdoth * vdoth * alpha2;
    //float fss90 = f0 + (f90 - f0) * pow(1.0f - ndotl, 5.0f);

}

#endif