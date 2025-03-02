#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#ifndef M_1_PI
#define M_1_PI 0.31830988618379067154f
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

uint InitRandomSeed(uint val0, uint val1) {
    uint v0 = val0, v1 = val1, s0 = 0;

    for (uint n = 0; n < 16; n++) {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

uint RandomInt(inout uint seed) {
    // LCG values from Numerical Recipes
    return (seed = 1664525 * seed + 1013904223);
}

float RandomFloat(inout uint seed) {
    //// Float version using bitmask from Numerical Recipes
    // const uint one = 0x3f800000;
    // const uint msk = 0x007fffff;
    // return uintBitsToFloat(one | (msk & (RandomInt(seed) >> 9))) - 1;

    // Faster version from NVIDIA examples; quality good enough for our use case.
    return (float(RandomInt(seed) & 0x00FFFFFF) / float(0x01000000));
}

vec3 random_cosine_direction(inout uint wseed) {
    float r1 = RandomFloat(wseed);
    float r2 = RandomFloat(wseed);
    float z = sqrt(1 - r2);

    float phi = M_PI * 2 * r1;
    float x = cos(phi) * sqrt(r2);
    float y = sin(phi) * sqrt(r2);
    return normalize(vec3(x, y, z));
}

vec3 random_cosine_direction(vec2 rnd) {
    float r1 = rnd.x;
    float r2 = rnd.y;
    float z = sqrt(1 - r2);
    float phi = M_PI * 2 * r1;
    float x = cos(phi) * sqrt(r2);
    float y = sin(phi) * sqrt(r2);
    return normalize(vec3(x, y, z));
}

float cosine_hemisphere_pdf(float cos_theta) {
    return cos_theta * M_1_PI;
}

#endif
