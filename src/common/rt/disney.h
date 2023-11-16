#ifndef DESNEY_H
#define DESNEY_H

#ifdef __cplusplus
// include vec & mat types (same namings as in GLSL)
#include "common.h"
#endif // __cplusplus

#ifndef __cplusplus
layout(std430)
#endif // !__cplusplus
struct Disney {
    vec3 _base_color;
    float _roughness;
    float _subsurface;
    float padding1, padding2, padding3;
};

#ifndef __cplusplus
layout(std430)
#endif // !__cplusplus
struct DisneyParam {
    Disney _disney;
    int _id;
};

#endif // !DESNEY_H