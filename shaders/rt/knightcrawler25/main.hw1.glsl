#ifndef _RT_KNIGHTCRAWLER25_MAIN_HW1_H_
#define _RT_KNIGHTCRAWLER25_MAIN_HW1_H_

#include "../random.glsl"
#include "helper.glsl"
#include "struct.glsl"

void TintColors_hw1(Material mat, float eta, out float F0, out vec3 Csheen, out vec3 Cspec0) {
    float lum = Luminance(mat.baseColor);
    vec3 ctint = lum > 0.0 ? mat.baseColor / lum : vec3(1.0);

    F0 = (1.0 - eta) / (1.0 + eta);
    F0 *= F0;

    vec3 Ks = mix(vec3(1.0), ctint, mat.specularTint);
    Cspec0 = mix(mat.specular * F0 * Ks, mat.baseColor, mat.metallic);
    Csheen = mix(vec3(1.0), ctint, mat.sheenTint);
}

vec3 EvalDisneyDiffuse_hw1(Material mat, vec3 Csheen, vec3 V, vec3 L, vec3 H, out float pdf) {
    pdf = 0.0;
    if (L.z <= 0.0) {
        return vec3(0.0);
    }

    float LDotH = dot(L, H);

    float Rr = mat.roughness * LDotH * LDotH;

    // Diffuse
    float Fd90 = 0.5 + 2.0 * Rr;
    float FL = SchlickWeight(L.z);
    float FV = SchlickWeight(V.z);
    float diffuse = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // Fake subsurface
    float Fss90 = Rr;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (L.z + V.z) - 0.5) + 0.5);

    // Sheen
    float FH = SchlickWeight(LDotH);
    vec3 Fsheen = FH * mat.sheen * Csheen;

    pdf = L.z * INV_PI;
    return INV_PI * mat.baseColor * mix(diffuse, ss, mat.subsurface) + Fsheen;
}

vec3 EvalMicrofacetReflection_hw1(Material mat, vec3 V, vec3 L, vec3 H, vec3 F, out float pdf) {
    pdf = 0.0;
    if (L.z <= 0.0) {
        return vec3(0.0);
    }

    float D = GTR2Aniso(H.z, H.x, H.y, mat.ax, mat.ay);
    float G1 = SmithGAniso(abs(V.z), V.x, V.y, mat.ax, mat.ay);
    float G2 = G1 * SmithGAniso(abs(L.z), L.x, L.y, mat.ax, mat.ay);

    pdf = G1 * D / (4.0 * V.z);
    return mat.baseColor * F * (D * G2 / (4.0 * L.z * V.z));
}

vec3 EvalMicrofacetRefraction_hw1(Material mat, float eta, vec3 V, vec3 L, vec3 H, vec3 F, out float pdf) {
    pdf = 0.0;
    if (L.z >= 0.0) {
        return vec3(0.0);
    }

    float LDotH = dot(L, H);
    float VDotH = dot(V, H);

    float D = GTR2Aniso(H.z, H.x, H.y, mat.ax, mat.ay);
    float G1 = SmithGAniso(abs(V.z), V.x, V.y, mat.ax, mat.ay);
    float G2 = G1 * SmithGAniso(abs(L.z), L.x, L.y, mat.ax, mat.ay);
    float denom = LDotH + VDotH * eta;
    denom *= denom;
    float jacobian = abs(LDotH) / denom;

    pdf = G1 * max(0.0, VDotH) * D * jacobian / V.z;
    return pow(mat.baseColor, vec3(0.5)) * (1.0 - F) * (D * G2 * abs(VDotH) * jacobian / abs(L.z * V.z));
}

vec3 EvalClearcoat_hw1(Material mat, vec3 V, vec3 L, vec3 H, out float pdf) {
    pdf = 0.0;
    if (L.z <= 0.0) {
        return vec3(0.0);
    }

    float VDotH = dot(V, H);

    // eta = 1.5 => R0 = 0.04
    float F = mix(0.04, 1.0, SchlickWeight(VDotH));
    float D = GTR1(H.z, mat.clearcoatRoughness);
    float G = SmithG(L.z, 0.25) * SmithG(V.z, 0.25);
    float jacobian = 1.0 / (4.0 * VDotH);

    pdf = D * H.z * jacobian;
    return vec3(F) * D * G;
}

void BuildWtAndPr_hw1(Material mat, float NDotV, out float diffWt, out float metalWt, out float glassWt, out float clearCtWt,
                      out float diffPr, out float metalPr, out float glassPr, out float clearCtPr) {
    // Model weights
    diffWt = (1.0 - mat.metallic) * (1.0 - mat.specTrans);
    metalWt = (1 - mat.specTrans * (1.0 - mat.metallic));
    glassWt = (1.0 - mat.metallic) * mat.specTrans;
    clearCtWt = 0.25 * mat.clearcoat;

    // Lobe probabilities
    float schlickWt = SchlickWeight(NDotV);

    diffPr = diffWt;
    metalPr = metalWt;
    glassPr = glassWt;
    clearCtPr = clearCtWt;

    // Normalize probabilities
    float invTotalWt = 1.0 / (diffPr + metalPr + glassPr + clearCtPr);
    diffPr *= invTotalWt;
    metalPr *= invTotalWt;
    glassPr *= invTotalWt;
    clearCtPr *= invTotalWt;
}

// make sure V, N is on the same side
vec3 DisneyEval_hw1(State state, vec3 V, vec3 N, vec3 L, out float pdf) {
    pdf = 0.0;
    vec3 f = vec3(0.0);

    // TODO: Tangent and bitangent should be calculated from mesh (provided, the mesh has proper uvs)
    vec3 T, B;
    BuildOnb(N, T, B);

    // Transform to shading space to simplify operations (NDotL = L.z; NDotV = V.z; NDotH = H.z)
    V = ToLocal(T, B, N, V);
    L = ToLocal(T, B, N, L);

    vec3 H;
    if (L.z > 0.0) {
        H = normalize(L + V);
    } else {
        H = normalize(L + V * state.eta);
    }

    if (H.z < 0.0) {
        H = -H;
    }

    // Tint colors
    vec3 Csheen, Cspec0;
    float F0;
    TintColors_hw1(state.mat, state.eta, F0, Csheen, Cspec0);

    float diffWt, metalWt, glassWt, clearCtWt;
    float diffPr, metalPr, glassPr, clearCtPr;
    BuildWtAndPr_hw1(state.mat, V.z, diffWt, metalWt, glassWt, clearCtWt, diffPr, metalPr, glassPr, clearCtPr);

    bool reflect = L.z * V.z > 0;

    float tmpPdf = 0.0;
    float VDotH = abs(dot(V, H));

    // Diffuse
    if (diffPr > 0.0 && reflect) {
        f += EvalDisneyDiffuse_hw1(state.mat, Csheen, V, L, H, tmpPdf) * diffWt;
        pdf += tmpPdf * diffPr;
    }

    // Metallic Reflection
    if (metalPr > 0.0 && reflect) {
        vec3 F = mix(Cspec0, vec3(1.0), SchlickWeight(VDotH));

        f += EvalMicrofacetReflection_hw1(state.mat, V, L, H, F, tmpPdf) * metalWt;
        pdf += tmpPdf * metalPr;
    }

    // Glass/Specular BSDF
    if (glassPr > 0.0) {
        // Dielectric fresnel (achromatic)
        float F = DielectricFresnel(VDotH, state.eta);

        if (reflect) {
            f += EvalMicrofacetReflection_hw1(state.mat, V, L, H, vec3(F), tmpPdf) * glassWt;
            pdf += tmpPdf * glassPr * F;
        } else {
            f += EvalMicrofacetRefraction_hw1(state.mat, state.eta, V, L, H, vec3(F), tmpPdf) * glassWt;
            pdf += tmpPdf * glassPr * (1.0 - F);
        }
    }

    // Clearcoat
    if (clearCtPr > 0.0 && reflect) {
        f += EvalClearcoat_hw1(state.mat, V, L, H, tmpPdf) * clearCtWt;
        pdf += tmpPdf * clearCtPr;
    }

    return f * abs(L.z);
}

// make sure V, N is on the same side
vec3 DisneySample_hw1(State state, vec3 V, vec3 N, in vec3 rnd, out vec3 L, out float pdf) {
    pdf = 0.0;

    float r1 = rnd.x;
    float r2 = rnd.y;

    // TODO: Tangent and bitangent should be calculated from mesh (provided, the mesh has proper uvs)
    vec3 T, B;
    BuildOnb(N, T, B);

    // Transform to shading space to simplify operations (NDotL = L.z; NDotV = V.z; NDotH = H.z)
    V = ToLocal(T, B, N, V);

    // Tint colors
    vec3 Csheen, Cspec0;
    float F0;
    TintColors_hw1(state.mat, state.eta, F0, Csheen, Cspec0);

    float diffWt, metalWt, glassWt, clearCtWt;
    float diffPr, metalPr, glassPr, clearCtPr;
    BuildWtAndPr_hw1(state.mat, V.z, diffWt, metalWt, glassWt, clearCtWt, diffPr, metalPr, glassPr, clearCtPr);

    // CDF of the sampling probabilities
    float cdf[4];
    cdf[0] = diffPr;
    cdf[1] = cdf[0] + metalPr;
    cdf[2] = cdf[1] + glassPr;
    cdf[3] = cdf[2] + clearCtPr;

    // Sample a lobe based on its importance
    float r3 = rnd.z;

    // Diffuse
    if (r3 < cdf[0]) {
        L = CosineSampleHemisphere(r1, r2);
    }
    // Dielectric + Metallic reflection
    else if (r3 < cdf[1]) {
        vec3 H = SampleGGXVNDF(V, state.mat.ax, state.mat.ay, r1, r2);

        if (H.z < 0.0) {
            H = -H;
        }

        L = normalize(reflect(-V, H));
    }
    // Glass
    else if (r3 < cdf[2]) {
        vec3 H = SampleGGXVNDF(V, state.mat.ax, state.mat.ay, r1, r2);
        float F = DielectricFresnel(abs(dot(V, H)), state.eta);

        if (H.z < 0.0) {
            H = -H;
        }

        // Rescale random number for reuse
        r3 = (r3 - cdf[1]) / (cdf[2] - cdf[1]);

        // Reflection
        if (r3 < F) {
            L = normalize(reflect(-V, H));
        }
        // Transmission
        else {
            L = normalize(refract(-V, H, state.eta));
        }
    }
    // Clearcoat
    else {
        vec3 H = SampleGTR1(state.mat.clearcoatRoughness, r1, r2);

        if (H.z < 0.0) {
            H = -H;
        }

        L = normalize(reflect(-V, H));
    }

    L = ToWorld(T, B, N, L);
    V = ToWorld(T, B, N, V);

    return DisneyEval_hw1(state, V, N, L, pdf);
}

// input : direction_in is normalized, to camera
// output: direction    is normalized, to light
void sample_disney_hw1(in Disney disney, inout uint wseed, in vec3 normal, in vec3 direction_in, out vec3 direction,
                       out float pdf, out vec3 bsdf) {
    bool inner = dot(direction_in, normal) < 0;
    State state = buildState(disney, inner);
    normal = inner ? -normal : normal;

    vec3 rnd = vec3(RandomFloat(wseed), RandomFloat(wseed), RandomFloat(wseed));

    bsdf = DisneySample_hw1(state, direction_in, normal, rnd, direction, pdf);
}

#endif