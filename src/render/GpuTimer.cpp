#include "render/GpuTimer.h"

#include "core/Log.h"

#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace dl
{

namespace
{
constexpr float kSmoothing = 0.08f; // the part of a new frame that goes into the mean: about the last dozen frames
}

bool GpuTimer::Init(ID3D11Device* device)
{
    Shutdown();
    if (device == nullptr)
    {
        return false;
    }
    m_device = device;
    D3D11_QUERY_DESC disjoint{};
    disjoint.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    D3D11_QUERY_DESC stamp{};
    stamp.Query = D3D11_QUERY_TIMESTAMP;
    for (Slot& slot : m_slots)
    {
        if (FAILED(m_device->CreateQuery(&disjoint, &slot.disjoint)))
        {
            DL_LOG_WARN("GpuTimer", "the device gives no timestamp queries: the GPU time is not measured");
            Shutdown();
            return false;
        }
        for (Stamp& s : slot.stamps)
        {
            if (FAILED(m_device->CreateQuery(&stamp, &s.query)))
            {
                DL_LOG_WARN("GpuTimer", "a timestamp query could not be made: the GPU time is not measured");
                Shutdown();
                return false;
            }
        }
    }
    m_ok = true;
    DL_LOG_INFO("GpuTimer", "ready: %d frames in flight, %d timestamps each", kFrames, kStamps);
    return true;
}

void GpuTimer::Shutdown()
{
    for (Slot& slot : m_slots)
    {
        slot = Slot{};
    }
    m_device.Reset();
    m_current = 0;
    m_oldest = 0;
    m_recording = false;
    m_ok = false;
    m_ms.fill(0.0f);
    m_readFrames = 0;
}

void GpuTimer::AddStamp(ID3D11DeviceContext* context, Part part, bool begin)
{
    if (!m_recording)
    {
        return;
    }
    Slot& slot = m_slots[static_cast<std::size_t>(m_current)];
    if (slot.count >= kStamps)
    {
        return; // a part that has no room is lost, not an error
    }
    Stamp& stamp = slot.stamps[static_cast<std::size_t>(slot.count++)];
    stamp.part = part;
    stamp.begin = begin;
    context->End(stamp.query.Get()); // a timestamp query has only an End
}

void GpuTimer::BeginFrame(ID3D11DeviceContext* context)
{
    m_recording = false;
    if (!m_ok)
    {
        return;
    }
    Slot& slot = m_slots[static_cast<std::size_t>(m_current)];
    if (slot.pending)
    {
        return; // the GPU is more than three frames behind: this frame is not timed
    }
    slot.count = 0;
    context->Begin(slot.disjoint.Get());
    m_recording = true;
    AddStamp(context, kFrame, true);
}

void GpuTimer::Begin(ID3D11DeviceContext* context, Part part)
{
    AddStamp(context, part, true);
}

void GpuTimer::End(ID3D11DeviceContext* context, Part part)
{
    AddStamp(context, part, false);
}

void GpuTimer::EndFrame(ID3D11DeviceContext* context)
{
    if (!m_recording)
    {
        return;
    }
    // the end of the frame always has room: the last stamp is made even when the list is full
    Slot& slot = m_slots[static_cast<std::size_t>(m_current)];
    if (slot.count >= kStamps)
    {
        slot.count = kStamps - 1;
    }
    AddStamp(context, kFrame, false);
    context->End(slot.disjoint.Get());
    slot.pending = true;
    m_current = (m_current + 1) % kFrames;
    m_recording = false;
}

bool GpuTimer::ReadSlot(ID3D11DeviceContext* context, Slot& slot)
{
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT clock{};
    if (context->GetData(slot.disjoint.Get(), &clock, sizeof(clock), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
    {
        return false; // not ready yet
    }
    std::array<std::uint64_t, kStamps> times{};
    for (int i = 0; i < slot.count; ++i)
    {
        if (context->GetData(slot.stamps[static_cast<std::size_t>(i)].query.Get(), &times[static_cast<std::size_t>(i)],
                             sizeof(std::uint64_t), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
        {
            return false;
        }
    }
    slot.pending = false;
    if (clock.Disjoint != FALSE || clock.Frequency == 0)
    {
        return true; // the clock changed during the frame: it is dropped
    }
    std::array<float, kPartCount> sum{};
    std::array<std::uint64_t, kPartCount> opened{};
    std::array<bool, kPartCount> isOpen{};
    for (int i = 0; i < slot.count; ++i)
    {
        const Stamp& stamp = slot.stamps[static_cast<std::size_t>(i)];
        const std::size_t p = static_cast<std::size_t>(stamp.part);
        const std::uint64_t t = times[static_cast<std::size_t>(i)];
        if (stamp.begin)
        {
            opened[p] = t;
            isOpen[p] = true;
        }
        else if (isOpen[p] && t >= opened[p])
        {
            sum[p] +=
                static_cast<float>(static_cast<double>(t - opened[p]) * 1000.0 / static_cast<double>(clock.Frequency));
            isOpen[p] = false;
        }
    }
    for (std::size_t p = 0; p < sum.size(); ++p)
    {
        m_ms[p] = m_readFrames == 0 ? sum[p] : m_ms[p] + (sum[p] - m_ms[p]) * kSmoothing;
    }
    ++m_readFrames;
    return true;
}

void GpuTimer::Collect(ID3D11DeviceContext* context)
{
    if (!m_ok)
    {
        return;
    }
    // the oldest first, as many as are ready
    for (int n = 0; n < kFrames; ++n)
    {
        Slot& slot = m_slots[static_cast<std::size_t>(m_oldest)];
        if (!slot.pending || !ReadSlot(context, slot))
        {
            break;
        }
        m_oldest = (m_oldest + 1) % kFrames;
    }
}

} // namespace dl
