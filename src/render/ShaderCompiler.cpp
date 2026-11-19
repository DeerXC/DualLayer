#include "render/ShaderCompiler.h"

#include "core/Log.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3dcompiler.h>

namespace dl
{

bool CompileShader(const char* path, const char* entry, const char* target, Microsoft::WRL::ComPtr<ID3DBlob>& out,
                   const char* logModule)
{
    UINT flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS; // docs/CONVENTIONS.md, as common.hlsli
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    wchar_t widePath[MAX_PATH];
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, widePath, MAX_PATH) == 0)
    {
        DL_LOG_ERROR(logModule, "the shader path is too long or not UTF-8: %s", path);
        return false;
    }
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompileFromFile(widePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, target, flags, 0,
                                          &out, &errors);
    if (FAILED(hr))
    {
        if (errors != nullptr)
        {
            DL_LOG_ERROR(logModule, "shader %s (%s) in %s failed:\n%s", entry, target, path,
                         static_cast<const char*>(errors->GetBufferPointer()));
        }
        else
        {
            DL_LOG_ERROR(logModule, "cannot read %s (0x%08X): is the working directory the project folder?", path,
                         static_cast<unsigned>(hr));
        }
        out.Reset();
        return false;
    }
    return true;
}

} // namespace dl
