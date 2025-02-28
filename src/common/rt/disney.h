#ifndef DESNEY_H
#define DESNEY_H

#ifdef __cplusplus
// include vec & mat types (same namings as in GLSL)
#include "common.h"
#endif  // __cplusplus

struct Disney {
    vec3 _base_color;
    float _roughness;

    float _subsurface;
    float _anisotropic;
    float _metallic;
    float _clearcoat_gloss;

    int _is_refractive;
    float _eta;  // internal IOR / externalIOR, IOR(index of refraction)
    float _sheen_tint;
    float _specular_transmission;

    float _specular_tint;
    float _clearcoat;
    float _sheen;
    float _specular;
};

struct DisneyParam {
    Disney _disney;
    int _id;
};

#endif  // !DESNEY_H