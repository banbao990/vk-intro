// modified from: https://github.com/woAIxuexiSR/SRT/blob/master/src/device_include/scene/bxdf.h

#ifndef DISNEY_GLSL
#define DISNEY_GLSL

#define METAL_ALPHA_MIN 1e-3f
#define DISNEY_EPS 1e-6f

#include "../random.glsl"

struct Onb {
    vec3 normal;     // z
    vec3 tangent;    // x
    vec3 bitangent;  // y
};

vec3 obn_to_local(in Onb onb, in vec3 v) {
    return vec3(dot(v, onb.tangent), dot(v, onb.bitangent), dot(v, onb.normal));
}

vec3 obn_to_world(in Onb onb, in vec3 v) {
    return onb.tangent * v.x + onb.bitangent * v.y + onb.normal * v.z;
}

// normal is normalized
Onb make_onb(in vec3 normal) {
    Onb onb;
    onb.normal = normal;
    vec3 t = (abs(onb.normal.x) > abs(onb.normal.y)) ? vec3(0, 1, 0) : vec3(1, 0, 0);
    onb.tangent = normalize(cross(t, onb.normal));
    onb.bitangent = cross(onb.normal, onb.tangent);
    return onb;
}

float luminance(in vec3 color) {
    return dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
}

float schlick_fresnel(in float cos_theta, in float eta) {
    float r0 = (eta - 1.0f) / (eta + 1.0f);
    r0 = r0 * r0;
    float m = clamp(1.0f - cos_theta, 0.0f, 1.0f);
    return r0 + (1.0f - r0) * m * m * m * m * m;
}

// eta = 1.0f
float schlick_fresnel(in float cos_theta) {
    float m = clamp(1.0f - cos_theta, 0.0f, 1.0f);
    return m * m * m * m * m;
}

float fresnel_mix(in float metallic, in float eta, in float cos_theta) {
    return mix(schlick_fresnel(cos_theta, eta), schlick_fresnel(cos_theta), metallic);
}

float GTR1(in float n_h, in float a) {
    if (a >= 1.0f) {
        return M_1_PI;
    }
    float a2 = a * a;
    float t = 1.0f + (a2 - 1.0f) * n_h * n_h;
    return (a2 - 1.0f) * M_1_PI / (log(a2) * t);
}

float GTR2(in float n_h, in float a) {
    float a2 = a * a;
    float t = 1.0f + (a2 - 1.0f) * n_h * n_h;
    return a2 * M_1_PI / (t * t);
}

float GTR2_aniso(in float n_h, in float h_x, in float h_y, in float ax, in float ay) {
    // n_h : cos(theta), h_x : sin(theta)cos(phi), h_y : sin(theta)sin(phi)
    float a = h_x / ax;
    float b = h_y / ay;
    float t = a * a + b * b + n_h * n_h;
    return M_1_PI / (ax * ay * t * t);
}

float smithG_GGX(in float n_v, in float alphaG) {
    float a = alphaG * alphaG;
    float b = n_v * n_v;
    return (2.0f * n_v) / (n_v + sqrt(a + b - a * b));
}

vec3 sample_GTR2(in float a, in vec2 rnd) {
    float a2 = a * a;
    float phi = 2.0f * M_PI * rnd.x;
    float sin_theta = sqrt(a2 / (1.0f / rnd.y - 1.0f + a2));
    float cos_theta = clamp(sqrt(1.0f - sin_theta * sin_theta), 0.0f, 1.0f);
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

vec3 sample_GTR2_aniso(in float ax, in float ay, in vec2 rnd) {
    float phi = 2.0f * M_PI * rnd.x;
    float sin_phi = ay * sin(phi);
    float cos_phi = ax * cos(phi);
    float tan_theta = sqrt(rnd.y / (1.0f - rnd.y));
    return normalize(vec3(tan_theta * cos_phi, tan_theta * sin_phi, 1.0f));
}

//__host__ __device__ inline float3 sample_GGXVNDF(float3 V, float a, float2 sample) {
//    float3 Vh = normalize(make_float3(a * V.x, a * V.y, V.z));
//
//    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
//    float3 T1 = (lensq > 0.0f) ? make_float3(-Vh.y, Vh.x, 0.0f) * rsqrtf(lensq) : make_float3(1.0f, 0.0f, 0.0f);
//    float3 T2 = cross(Vh, T1);
//
//    float r = sqrt(sample.x);
//    float phi = 2.0f * (float)M_PI * sample.y;
//    float t1 = r * cos(phi);
//    float t2 = r * sin(phi);
//    float s = 0.5f * (1.0f + Vh.z);
//    t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;
//
//    float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;
//
//    return normalize(make_float3(a * Nh.x, a * Nh.y, max(0.0f, Nh.z)));
//}

vec3 sample_GGXVNDF(in vec3 V, in float a, in vec2 rnd) {
    vec3 Vh = normalize(vec3(a * V.x, a * V.y, V.z));

    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = (lensq > 0.0f) ? vec3(-Vh.y, Vh.x, 0.0f) * inversesqrt(lensq) : vec3(1.0f, 0.0f, 0.0f);
    vec3 T2 = cross(Vh, T1);

    float r = sqrt(rnd.x);
    float phi = 2.0f * M_PI * rnd.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + Vh.z);
    t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;

    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;

    return normalize(vec3(a * Nh.x, a * Nh.y, max(0.0f, Nh.z)));
}

vec3 sample_GTR1(float a, vec2 rnd) {
    if (a >= 1.0f) {
        return random_cosine_direction(rnd);
    }
    float a2 = a * a;
    float phi = 2.0f * M_PI * rnd.x;
    float cos_theta = sqrt((1.0f - pow(a2, 1.0f - rnd.y)) / (1.0f - a2));
    float sin_theta = clamp(sqrt(1.0f - (cos_theta * cos_theta)), 0.0f, 1.0f);
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// bsdf = BSDF * cos
// wi, wo: world space
// wi: direction_out, view direction   (normalized)
// wo: direction_in,  light direction  (normalized)
void eval_disney(in Disney disney, in vec3 normal, in vec3 direction_out, in vec3 direction_in,
                 out vec3 bsdf) {
    Onb onb = make_onb(normal);  // always the shading normal

    vec3 V = obn_to_local(onb, direction_in);
    vec3 L = obn_to_local(onb, direction_out);

    const bool inner = V.z <= 0.0f;
    if (inner) {
        V = -V;
        L = -L;
    }
    const float ior = disney._eta;
    float eta = inner ? ior : 1.0f / ior;

    vec3 H = (L.z > 0.0f) ? normalize(L + V) : normalize(L + V * eta);
    if (H.z <= 0.0f) {
        H = -H;
    }
    float V_H = dot(V, H);
    float L_H = dot(L, H);

    float lum = luminance(disney._base_color);
    vec3 ctint = (lum > 0.0f) ? disney._base_color / lum : vec3(1.0f);
    // float F0 = (1.0f - eta) / (1.0f + eta);
    // vec3 spec_color = mix(F0 * F0 * mix(vec3(1.0f), ctint, disney._specular_tint),
    // disney._base_color, disney._metallic);
    vec3 spec_color = mix(disney._specular * 0.08f * mix(vec3(1.0f), ctint, disney._specular_tint),
                          disney._base_color, disney._metallic);
    vec3 sheen_color = mix(vec3(1.0f), ctint, disney._sheen_tint);

    float FM = fresnel_mix(disney._metallic, eta, V_H);
    float diffuse_w = lum * (1.0f - disney._metallic) * (1.0f - disney._specular_transmission);
    float spec_reflect_w = luminance(mix(spec_color, vec3(1.0f), FM));
    float spec_refract_w = (1.0f - FM) * (1.0f - disney._metallic) * disney._specular_transmission * lum;
    float clearcoat_w = disney._clearcoat * (1.0f - disney._metallic);

    // diffuse
    vec3 f_diffuse = vec3(0.0f);
    if (diffuse_w > 0.0f && L.z > 0.0f) {
        // diffuse
        float FL = schlick_fresnel(L.z);
        float FV = schlick_fresnel(V.z);
        float FH = schlick_fresnel(L_H);
        float Fd90 = 0.5f + 2.0f * L_H * L_H * disney._roughness;
        float Fd = mix(1.0f, Fd90, FL) * mix(1.0f, Fd90, FV);
        // sub-surface
        float Fss90 = L_H * L_H * disney._roughness;
        float Fss = mix(1.0f, Fss90, FL) * mix(1.0f, Fss90, FV);
        float ss = 1.25f * (Fss * (1.0f / (L.z + V.z) - 0.5f) + 0.5f);
        // sheen
        vec3 Fsheen = FH * disney._sheen * sheen_color;
        f_diffuse = (M_1_PI * mix(Fd, ss, disney._subsurface) * disney._base_color + Fsheen) *
                    (1.0f - disney._metallic) * (1.0f - disney._specular_transmission);
    }

    // specular reflection
    vec3 f_spec_reflect = vec3(0.0f);
    if (spec_reflect_w > 0.0f && L.z > 0.0f && V.z > 0.0f) {
        float _FM = fresnel_mix(disney._metallic, eta, L_H);
        vec3 F = mix(spec_color, vec3(1.0f), _FM);

        float aspect = sqrt(1.0f - disney._anisotropic * 0.9f);
        float a2 = disney._roughness * disney._roughness;
        float ax = max(0.001f, a2 / aspect);
        float ay = max(0.001f, a2 * aspect);

        // float D = GTR2(H.z, disney._roughness);
        float D = GTR2_aniso(H.z, H.x, H.y, ax, ay);
        float G = smithG_GGX(abs(L.z), disney._roughness) * smithG_GGX(abs(V.z), disney._roughness);

        f_spec_reflect = F * D * G / (4.0f * L.z * V.z);
    }

    // specular refraction
    vec3 f_spec_refract = vec3(0.0f);
    if (spec_refract_w > 0.0f && L.z < 0.0f) {
        float F = schlick_fresnel(abs(V_H), eta);
        float D = GTR2(H.z, disney._roughness);
        float denom = (L_H + V_H * eta) * (L_H + V_H * eta);
        float G = smithG_GGX(abs(L.z), disney._roughness) * smithG_GGX(abs(V.z), disney._roughness) *
                  abs(L_H) * abs(V_H) * eta * eta / denom;
        vec3 refract_color = sqrt(disney._base_color);

        f_spec_refract = refract_color * (1.0f - disney._metallic) * disney._specular_transmission *
                         (1.0f - F) * D * G / (abs(L.z) * abs(V.z));
    }
    // clearcoat
    vec3 f_clearcoat = vec3(0.0f);
    if (clearcoat_w > 0.0f && L.z > 0.0f && V.z > 0.0f) {
        float FH = schlick_fresnel(L_H, 1.0f / 1.5f);
        float F = mix(0.04f, 1.0f, FH);
        float D = GTR1(H.z, disney._clearcoat_gloss);
        float G = smithG_GGX(L.z, 0.25f) * smithG_GGX(V.z, 0.25f);
        f_clearcoat = vec3(0.25f) * disney._clearcoat * F * D * G / (4.0f * L.z * V.z);
    }

    bsdf = f_diffuse + f_spec_reflect + f_spec_refract + f_clearcoat;
    bsdf = bsdf * abs(L.z);
}

// input : direction_in is normalized, to camera
// output: direction    is normalized, to light
void sample_disney(in Disney disney, inout uint wseed, in vec3 normal, in vec3 direction_in, out vec3 direction,
                   out float pdf) {
    Onb onb = make_onb(normal);  // always the shading normal

    vec3 V = obn_to_local(onb, direction_in);
    const bool inner = V.z <= 0.0f;
    if (inner) {
        // normal = -normal; // useless after onb
        V = -V;
    }
    const float ior = disney._eta;
    float eta = inner ? ior : 1.0f / ior;

    float lum = luminance(disney._base_color);
    vec3 ctint = (lum > 0.0f) ? disney._base_color / lum : vec3(1.0f);
    vec3 spec_color = mix(disney._specular * 0.08f * mix(vec3(1.0f), ctint, disney._specular_tint),
                          disney._base_color, disney._metallic);
    vec3 sheen_color = mix(vec3(1.0f), ctint, disney._sheen_tint);

    float aspect = sqrt(1.0f - disney._anisotropic * 0.9f);
    float a2 = disney._roughness * disney._roughness;
    float ax = max(0.001f, a2 / aspect);
    float ay = max(0.001f, a2 * aspect);

    float FM = fresnel_mix(disney._metallic, eta, V.z);
    float diffuse_w = lum * (1.0f - disney._metallic) * (1.0f - disney._specular_transmission);
    float spec_reflect_w = luminance(mix(spec_color, vec3(1.0f), FM));
    float spec_refract_w = (1.0f - FM) * (1.0f - disney._metallic) * disney._specular_transmission * lum;
    float clearcoat_w = disney._clearcoat * (1.0f - disney._metallic);

    float total_w = diffuse_w + spec_reflect_w + spec_reflect_w + clearcoat_w;
    diffuse_w /= total_w;
    spec_reflect_w /= total_w;
    spec_refract_w /= total_w;
    clearcoat_w /= total_w;

    vec2 rnd = vec2(RandomFloat(wseed), RandomFloat(wseed));
    vec3 L = vec3(0.0f);

    bool invalid_sample = false;

    if (rnd.x < diffuse_w) {
        rnd.x /= diffuse_w;
        L = random_cosine_direction(rnd);
    } else if (rnd.x < diffuse_w + spec_reflect_w) {
        rnd.x = (rnd.x - diffuse_w) / spec_reflect_w;
        // vec3 H = sample_GTR2(disney._roughness, rnd);
        vec3 H = sample_GTR2_aniso(ax, ay, rnd);
        if (dot(V, H) <= 0.0f) {
            invalid_sample = true;
        }
        L = normalize(reflect(-V, H));
    } else if (rnd.x < diffuse_w + spec_reflect_w + spec_refract_w) {
        rnd.x = (rnd.x - diffuse_w - spec_reflect_w) / spec_refract_w;
        // vec3 H = sample_GTR2(disney._roughness, rnd);
        vec3 H = sample_GGXVNDF(V, disney._roughness, rnd);
        L = refract(-V, H, eta);
        if (dot(V, H) <= 0.0f || dot(L, L) == 0.0f) {
            invalid_sample = true;
        }
        L = normalize(L);
    } else {
        rnd.x = (rnd.x - diffuse_w - spec_reflect_w - spec_refract_w) / clearcoat_w;
        vec3 H = sample_GTR1(disney._clearcoat_gloss, rnd);
        if (dot(V, H) <= 0.0f) {
            invalid_sample = true;
        }
        L = normalize(reflect(-V, H));
    }

    // wi
    if (inner) {
        L = -L;
    }
    direction = obn_to_world(onb, L);

    pdf = 0.0f;

    if (invalid_sample) {
        return;
    }

    vec3 H_reflect = normalize(L + V);
    if (H_reflect.z < 0.0f) {
        H_reflect = -H_reflect;
    }
    vec3 H_refract = normalize(L + V * eta);
    if (H_refract.z < 0.0f) {
        H_refract = -H_refract;
    }

    if (diffuse_w > 0.0f && L.z > 0.0f) {
        pdf += diffuse_w * cosine_hemisphere_pdf(L.z);
    }
    if (spec_reflect_w > 0.0f && dot(V, H_reflect) > 0.0f) {
        // pdf += spec_reflect_w * GTR2(H_reflect.z, disney._roughness) * H_reflect.z / (4.0f * dot(V, H_reflect));
        pdf += spec_reflect_w * GTR2_aniso(H_reflect.z, H_reflect.x, H_reflect.y, ax, ay) * H_reflect.z / (4.0f * dot(V, H_reflect));
    }
    if (spec_refract_w > 0.0f && dot(V, H_refract) > 0.0f && dot(L, H_refract) < 0.0f) {
        float V_H = dot(V, H_refract);
        float L_H = dot(L, H_refract);
        float denom = (L_H + V_H * eta);
        // pdf += spec_refract_w * GTR2(H_refract.z, disney._roughness) * H_refract.z * abs(L_H) / (denom * denom);
        pdf += spec_refract_w * GTR2(H_refract.z, disney._roughness) * smithG_GGX(V.z, disney._roughness) * max(dot(V, H_refract), 0.0f) / V.z * abs(L_H) / (denom * denom);
    }
    if (clearcoat_w > 0.0f && dot(V, H_reflect) > 0.0f) {
        pdf += clearcoat_w * GTR1(H_reflect.z, disney._clearcoat_gloss) * H_reflect.z / (4.0f * dot(V, H_reflect));
    }
}

#endif