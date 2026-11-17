# Architecture

Covers milestones M0, M1 and the first steps of M2.

## One frame (M0 understanding gate, notes: `docs/understanding/M0-frame-flow.md`)

```
WinMain
 └─ RunApp
     ├─ Log.OpenFile, Config.LoadFile            (once)
     ├─ Window.Create                            (DPI awareness first)
     ├─ Gpu.Init (main.cpp: Device, Triangle and Models together, so that they can be made again together)
     │   ├─ Device.Init  → adapter list → D3D11 device (+ debug layer) → flip swap chain → back-buffer sRGB view
     │   ├─ Triangle.Init → compile shaders from shaders/test_triangle.hlsl → buffers
     │   ├─ Models.Init   → compile shaders/models.hlsl → input layout → render states → MeshData (CPU) → vertex + index buffers (GPU)
     │   └─ Models.LoadThemes                    (the looks of layer A and B: built in, changed by the config)
     └─ loop, per frame:
         1. Window.PumpMessages        (WM_SIZE / F5 / Esc set flags; the loop reads them)
         2. lost device (or F9 test) → Gpu.Recover: everything on the GPU is released and made again on a new Device; the models keep layer, fade and spin; after 5 tries the program says so and ends
         3. F5      → reload config.ini, Models.LoadThemes, apply vsync
         4. resized → Device.Resize     (release view → ResizeBuffers → create view)
         5. minimised → sleep, skip
         6. Update  → time step, spin angle, camera, keys Z X C Tab (wireframe, detail, spin, layer), Models.Update (spin, layer fade)
         7. Render  → Models.ClearColor (the background of this moment), GpuEvent "Frame"
                         Device.BeginFrame     bind RTV + DEPTH, viewport, clear colour (linear, written as sRGB) and depth (1.0)
                         GpuEvent "Opaque.Models"
                             Models.Draw       FrameCB (V*P, eye, mixed light and sky) → per object of the visible layer: World = S*R*T, normal matrix → ObjectCB → DrawIndexed
                         GpuEvent "Opaque.TestTriangle"      (only with [test_triangle] show = 1)
                             Triangle.Draw     W*V*P → constant buffer → IA/VS/PS state → Draw(3)
         8. Device.EndFrame → Present(vsync) → device-lost check (sets Lost(); step 2 of the next frame handles it)
     └─ shutdown: Gpu.Shutdown = Models.Shutdown → Triangle.Shutdown → Device.Shutdown (reports leaked objects in Debug) → Log.Close
```

## Device recovery

`Present` reports a removed or reset device (driver update, a driver reset after a long GPU stall, a GPU that is switched off). `Device` only sets `Lost()`; it never repairs itself. `main.cpp` owns the three GPU users in a `Gpu` struct: on `Lost()` it shuts all of them down, makes a **new** `Device` object (a lost one is never reused) and initialises all three again. The camera, the layer, the fade, the spin and the wireframe switch live outside the GPU objects or are not touched by `Init`, so the player sees a flicker and the same scene. The level of detail goes back to `[models] segments`. The `F9` key (config `[debug] device_reset_key = 1`) runs the same path without a real device loss, so that it can be tested.

## Layers (M2, first steps)

Every object of the scene has a mask (A = 1, B = 2, both = 3). `Models` keeps one number, the blend `m_layerBlend` (0 = A, 1 = B), that moves toward the layer chosen with `Tab`. The blend drives two things: the mix of the two themes (light, sky, background) and the visible layer, which changes when the blend passes 0.5. The objects are not moved or copied: a draw call is skipped when the mask does not contain the visible layer.

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
