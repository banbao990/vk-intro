#ifndef _RT_KNIGHTCRAWLER25_STRUCT_H_
#define _RT_KNIGHTCRAWLER25_STRUCT_H_

// struct
struct Material {
    vec3 baseColor;
    // float anisotropic;
    float specular;

    float metallic;
    float roughness;
    float subsurface;
    float specularTint;

    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatRoughness;

    float specTrans;
    float ior;
    float ax;
    float ay;
};

struct State {
    Material mat;
    float eta;
};

State buildState(in Disney disney, in bool inner) {
    Material mat;
    mat.baseColor = disney._base_color;
    // mat.anisotropic = disney._anisotropic;
    mat.specular = disney._specular;
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

    float aspect = sqrt(1.0 - disney._anisotropic * 0.9);
    mat.ax = max(0.001, mat.roughness / aspect);
    mat.ay = max(0.001, mat.roughness * aspect);

    State state;
    state.mat = mat;
    state.eta = inner ? disney._eta : (1.0 / disney._eta);

    return state;
}

#endif