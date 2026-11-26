// hlsl_shim.h - the vector types and the functions of HLSL that shaders/lighting.hlsli uses, written in C++, so that
// the test (tests/test_lighting.cpp) can include that file and run its functions. Only what lighting.hlsli needs: three
// floats, the arithmetic of a shader, saturate, step, smoothstep, lerp, frac, dot, length, normalize, pow, max, min,
// abs, floor, sin and sqrt. Use: include every standard and project header FIRST, then this file, then
// shaders/lighting.hlsli, then hlsl_shim_end.h. The names that the C++ library has too (pow, abs, max, min, floor, sin
// and sqrt: MSVC and GCC declare some of them in the global namespace) are not defined here as functions but as macros
// for the length of the include of the .hlsli, so that they cannot clash with anything.
#pragma once

#include <cmath>

struct float3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float3() = default;
    float3(float a, float b, float c) : x(a), y(b), z(c)
    {
    }
};

inline float3 operator+(const float3& a, const float3& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
inline float3 operator-(const float3& a, const float3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline float3 operator*(const float3& a, float s)
{
    return {a.x * s, a.y * s, a.z * s};
}
inline float3 operator*(float s, const float3& a)
{
    return {a.x * s, a.y * s, a.z * s};
}
inline float3 operator*(const float3& a, const float3& b)
{
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}
inline float3 operator/(const float3& a, float s)
{
    return {a.x / s, a.y / s, a.z / s};
}

inline float saturate(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}
inline float step(float edge, float v)
{
    return v >= edge ? 1.0f : 0.0f;
}
inline float dot(const float3& a, const float3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline float length(const float3& a)
{
    return std::sqrt(dot(a, a));
}
inline float3 normalize(const float3& a)
{
    return a / length(a);
}
inline float hlsl_pow(float a, float b)
{
    return std::pow(a, b);
}
inline float hlsl_abs(float v)
{
    return v < 0.0f ? -v : v;
}
inline float hlsl_max(float a, float b)
{
    return a > b ? a : b;
}
inline float hlsl_min(float a, float b)
{
    return a < b ? a : b;
}

inline float hlsl_floor(float v)
{
    return std::floor(v);
}
inline float hlsl_sin(float v)
{
    return std::sin(v);
}
inline float hlsl_sqrt(float v)
{
    return std::sqrt(v);
}
inline float frac(float v)
{
    return v - std::floor(v);
}
inline float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}
inline float3 lerp(const float3& a, const float3& b, float t)
{
    return a + (b - a) * t;
}
inline float smoothstep(float e0, float e1, float x)
{
    const float t = saturate((x - e0) / (e1 - e0));
    return t * t * (3.0f - 2.0f * t);
}

#define pow(a, b) hlsl_pow((a), (b))
#define floor(a) hlsl_floor((a))
#define sin(a) hlsl_sin((a))
#define sqrt(a) hlsl_sqrt((a))
#define abs(a) hlsl_abs((a))
#define max(a, b) hlsl_max((a), (b))
#define min(a, b) hlsl_min((a), (b))
