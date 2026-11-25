// ShaderCompiler.h - compiles one entry point of an HLSL file at run time, with the flags of the project.
// Flags:     row-major matrices (docs/CONVENTIONS.md, "HLSL") and strict mode; Debug adds debug information and turns
// the
//            optimiser off, Release optimises at level 3.
// Path:      UTF-8, relative to the working directory (the project folder under the debugger; the folder of the exe
//            otherwise, because the post-build step copies shaders\ next to it).
// Errors:    the compiler's message (or the reason the file could not be read) is logged under `logModule`; the
//            function then returns false and `out` stays empty.
// Threading: main thread only (it logs).
#pragma once

#include <d3dcommon.h>
#include <wrl/client.h>

namespace dl
{

bool CompileShader(const char* path, const char* entry, const char* target, Microsoft::WRL::ComPtr<ID3DBlob>& out,
                   const char* logModule = "Shader");

} // namespace dl
