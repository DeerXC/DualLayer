// GpuTimer.h - how long the GPU takes for the parts of a frame, read from timestamp queries.
// Why:       the title bar shows the time of a frame, and a frame that sits at the same 5.6 ms for 14 draws and for 202
//            says that the frame rate is capped, not what the GPU spends. A timestamp before and after a part of the
//            work says what that part costs (the shadow pass, the scene, the ink lines, the rain, the HUD), so that a
//            new pass is judged against a number.
// Use:       BeginFrame before the first command of the frame, Begin / End round a part (or a GpuScope), EndFrame after
//            the last command and before Present, Collect once after it. A part that is begun and ended more than once
//            in a frame adds up (the ripple draws the scene twice). Ms(part) is the mean of the last frames, 0 until
//            the first one has been read.
// Reading:   the queries of a frame are read some frames later and never waited for, so the timer costs no stall. When
//            the device does not give timestamps (or the clock jumps: a "disjoint" frame) the frame is dropped; Valid()
//            says whether there is a number.
// Owns:      the queries (a ring of four frames, at most 48 timestamps each). Main thread only.
#pragma once

// WHY here: this header is included before the headers that use std::min and std::max (LightingSettings.h, Lights.h),
// and <d3d11_1.h> brings in <windows.h>, whose min and max macros would break them. The project has no global
// NOMINMAX: each file sets it itself.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <d3d11_1.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>

namespace dl
{

class GpuTimer
{
  public:
    enum Part
    {
        kFrame = 0, ///< from BeginFrame to EndFrame: all the work of the frame on the GPU (not the wait of Present)
        kShadow,    ///< the shadow pass
        kScene,     ///< the picture: sky, town, objects, water, halos (the lines and the rain are inside it)
        kLines,     ///< the ink lines
        kRain,      ///< the rain
        kHud,       ///< the overlay
        kPartCount
    };

    GpuTimer() = default;
    GpuTimer(const GpuTimer&) = delete;
    GpuTimer& operator=(const GpuTimer&) = delete;

    /// Makes the queries. False (the timer then does nothing, and says so once in the log) when the device cannot.
    bool Init(ID3D11Device* device);
    void Shutdown();

    void BeginFrame(ID3D11DeviceContext* context);
    void Begin(ID3D11DeviceContext* context, Part part);
    void End(ID3D11DeviceContext* context, Part part);
    void EndFrame(ID3D11DeviceContext* context);
    /// Reads the frames whose queries are ready (never waits).
    void Collect(ID3D11DeviceContext* context);

    /// The mean of the last frames, in milliseconds.
    float Ms(Part part) const
    {
        return m_ms[static_cast<std::size_t>(part)];
    }
    /// True once at least one frame has been read.
    bool Valid() const
    {
        return m_readFrames > 0;
    }

  private:
    static constexpr int kFrames = 4;  ///< frames in flight
    static constexpr int kStamps = 48; ///< timestamps in a frame
    struct Stamp
    {
        Microsoft::WRL::ComPtr<ID3D11Query> query;
        Part part = kFrame;
        bool begin = true;
    };
    struct Slot
    {
        Microsoft::WRL::ComPtr<ID3D11Query> disjoint;
        std::array<Stamp, kStamps> stamps;
        int count = 0;
        bool pending = false; ///< the frame has been ended and not yet read
    };

    void AddStamp(ID3D11DeviceContext* context, Part part, bool begin);
    bool ReadSlot(ID3D11DeviceContext* context, Slot& slot);

    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    std::array<Slot, kFrames> m_slots;
    int m_current = 0; ///< the slot being recorded
    int m_oldest = 0;  ///< the oldest slot that is pending
    bool m_recording = false;
    bool m_ok = false;
    std::array<float, kPartCount> m_ms{};
    int m_readFrames = 0;
};

/// Begins a part when made and ends it when it goes out of scope. A null timer does nothing.
class GpuScope
{
  public:
    GpuScope(GpuTimer* timer, ID3D11DeviceContext* context, GpuTimer::Part part)
        : m_timer(timer), m_context(context), m_part(part)
    {
        if (m_timer != nullptr)
        {
            m_timer->Begin(m_context, m_part);
        }
    }
    ~GpuScope()
    {
        if (m_timer != nullptr)
        {
            m_timer->End(m_context, m_part);
        }
    }
    GpuScope(const GpuScope&) = delete;
    GpuScope& operator=(const GpuScope&) = delete;

  private:
    GpuTimer* m_timer;              // BORROWS
    ID3D11DeviceContext* m_context; // BORROWS
    GpuTimer::Part m_part;
};

} // namespace dl
