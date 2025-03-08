#ifndef _RT_KNIGHTCRAWLER25_MAIN_GLSL_
#define _RT_KNIGHTCRAWLER25_MAIN_GLSL_

#include "../random.glsl"
#include "struct.glsl"
#include "disney.glsl"

// input : direction_in is normalized, to camera
// output: direction    is normalized, to light
void sample_disney(in Disney disney, inout uint wseed, in vec3 normal, in vec3 direction_in, out vec3 direction,
                   out float pdf, out vec3 bsdf) {
    bool inner = dot(direction_in, normal) < 0;
    State state = buildState(disney, inner);
    normal = inner ? -normal : normal;

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
    normal = inner ? -normal : normal;

    float pdf = 1.0f;
    bsdf = DisneyEval(state, direction_in, normal, direction_out, pdf);
}

#endif