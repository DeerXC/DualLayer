// GpuDebug.h - names for GPU objects and RAII event markers, so RenderDoc / PIX / VS Graphics Debugger show readable
// captures. Naming rule: "<System>.<Resource>", e.g. "Frame.BackBufferRTV", "TestTriangle.VertexBuffer". Event rule:
// one GpuEvent per render pass; named "<Group>.<Pass>" (Frame > Opaque.Models > ...).
#pragma once

#include <d3d11_1.h>
#include <cstring>

namespace dl
{
/// Works for any D3D object that has SetPrivateData (resources, views, shaders, states, the device itself).
template <class T> void SetDebugName(T* object, const char* name)
{
    if (object != nullptr && name != nullptr)
    {
        object->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(std::strlen(name)), name);
    }
}

class GpuEvent
{
  public:
    GpuEvent(ID3DUserDefinedAnnotation* annotation, const wchar_t* name) : m_annotation(annotation)
    {
        if (m_annotation != nullptr)
        {
            m_annotation->BeginEvent(name);
        }
    }
    ~GpuEvent()
    {
        if (m_annotation != nullptr)
        {
            m_annotation->EndEvent();
        }
    }
    GpuEvent(const GpuEvent&) = delete;
    GpuEvent& operator=(const GpuEvent&) = delete;

  private:
    ID3DUserDefinedAnnotation* m_annotation; // BORROWS: owned by Device
};
} // namespace dl
