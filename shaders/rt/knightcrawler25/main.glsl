#ifndef _RT_KNIGHTCRAWLER25_MAIN_GLSL_
#define _RT_KNIGHTCRAWLER25_MAIN_GLSL_

#include "../random.glsl"
#include "disney.glsl"

State buildState(in Disney disney, in bool inner) {
    Material mat;
    mat.baseColor = disney._base_color;
    mat.anisotropic = disney._anisotropic;
    mat.metallic = disney._metallic;
    mat.roughness = disney._roughness;
    mat.subsurface = disney._subsurface;
    mat.specularTint = disney._specular_tint;
    mat.sheen = disney._sheen;
    mat.sheenTint = disney._sheen_tint;
    mat.clearcoat = disney._clearcoat;
    mat.clearcoatRoughness = mix(0.1, 0.001, disney._clearcoat_gloss);
    mat.specTrans = disney._specular_transmission;
    mat.ior = disney._eta;

    float aspect = sqrt(1.0 - mat.anisotropic * 0.9);
    mat.ax = max(0.001, mat.roughness / aspect);
    mat.ay = max(0.001, mat.roughness * aspect);

    State state;
    state.mat = mat;
    state.eta = inner ? 1.0 / disney._eta : disney._eta;

    return state;
}

// input : direction_in is normalized, to camera
// output: direction    is normalized, to light
void sample_disney(in Disney disney, inout uint wseed, in vec3 normal, in vec3 direction_in, out vec3 direction,
                   out float pdf, out vec3 bsdf) {
    bool inner = dot(direction_in, normal) < 0;
    State state = buildState(disney, inner);

    vec3 rnd = vec3(RandomFloat(wseed), RandomFloat(wseed), RandomFloat(wseed));

    bsdf = DisneySample(state, direction_in, normal, rnd, direction, pdf);
}

// bsdf = BSDF * cos
// wi, wo: world space
// wi: direction_out, view direction   (normalized)
// wo: direction_in,  light direction  (normalized)
void eval_disney(in Disney disney, in vec3 normal, in vec3 direction_out, in vec3 direction_in,
                 out vec3 bsdf) {
    bool inner = dot(direction_in, normal) < 0;
    State state = buildState(disney, inner);

    float pdf = 1.0f;
    bsdf = DisneyEval(state, direction_in, normal, direction_out, pdf);
}

#endif