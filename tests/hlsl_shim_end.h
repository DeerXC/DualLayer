// hlsl_shim_end.h - takes back the macros of tests/hlsl_shim.h (pow, abs, max, min) once shaders/lighting.hlsli has
// been included.
#pragma once

#undef pow
#undef abs
#undef max
#undef min
