# Architecture

Covers milestones M0 and M1.

## One frame (M0 understanding gate, notes: `docs/understanding/M0-frame-flow.md`)

```
WinMain
 └─ RunApp
     ├─ Log.OpenFile, Config.LoadFile            (once)
     ├─ Window.Create                            (DPI awareness first)
     ├─ Device.Init  → adapter list → D3D11 device (+ debug layer) → flip swap chain → back-buffer sRGB view
     ├─ Triangle.Init → compile shaders from shaders/test_triangle.hlsl → buffers
     ├─ Models.Init   → compile shaders/models.hlsl → input layout → render states → MeshData (CPU) → vertex + index buffers (GPU)
     └─ loop, per frame:
         1. Window.PumpMessages        (WM_SIZE / F5 / Esc set flags; the loop reads them)
         2. F5      → reload config.ini, apply vsync
         3. resized → Device.Resize     (release view → ResizeBuffers → create view)
         4. minimised → sleep, skip
         5. Update  → time step, spin angle, camera, keys Z X C (wireframe, detail, spin), Models.Update
         6. Render  → GpuEvent "Frame"
                         Device.BeginFrame     bind RTV + DEPTH, viewport, clear colour (linear, written as sRGB) and depth (1.0)
                         GpuEvent "Opaque.Models"
                             Models.Draw       FrameCB (V*P, eye, light) → per object: World = S*R*T, normal matrix → ObjectCB → DrawIndexed
                         GpuEvent "Opaque.TestTriangle"      (only with [test_triangle] show = 1)
                             Triangle.Draw     W*V*P → constant buffer → IA/VS/PS state → Draw(3)
         7. Device.EndFrame → Present(vsync) → device-lost check
     └─ shutdown: Models.Shutdown → Triangle.Shutdown → Device.Shutdown (reports leaked objects in Debug) → Log.Close
```

## Modules and the allowed direction of dependencies

```
main
 ├─ render   (Device, Triangle, Models, MeshData, Projection, GpuDebug)   → core
 └─ core     (Log, Assert, Config, Window)  → standard library and Windows only
```

`core` must never include `render` or anything above it. `render` is the only place that talks to Direct3D. New layers (`scene`, `physics`, `game`, `audio`, `ui`) are added by the milestone that needs them, always on top.
Pure-logic code (`Config`, `Log`, `Assert`, `Camera`, `MeshData`) has no Windows or Direct3D dependency and is tested without a window (`tests/`).
`MeshData` makes the shapes on the CPU; `Models` only copies them to the GPU and draws them. `Projection.h` is the one lens that every pass uses.

## Debugging facilities that exist since M0

- Every GPU object has a name `<System>.<Resource>`; every pass is wrapped in a `GpuEvent` (RenderDoc / PIX / VS Graphics Debugger show them).
- Debug builds: D3D11 + DXGI debug layers, break on error when a debugger is attached, a leak report at exit.
- `logs/dual.log` and the ring buffer (`log::Recent()`, shown by a debug panel planned for a later milestone).
