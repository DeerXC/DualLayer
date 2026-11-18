# Architecture

Covers milestones M0, M1 and M2 so far.

## One frame

```
WinMain
 └─ RunApp
     ├─ Log.OpenFile, Config.LoadFile            (once)
     ├─ Window.Create                            (DPI awareness first)
     ├─ Gpu.Init (main.cpp: Device, Triangle and Models together, so that they can be made again together)
     │   ├─ Device.Init  → adapter list → D3D11 device (+ debug layer) → flip swap chain → back-buffer sRGB view
     │   ├─ Triangle.Init → compile shaders from shaders/test_triangle.hlsl → buffers
     │   ├─ Models.Init   → compile shaders/models.hlsl and shaders/ripple.hlsl → input layout → render states → MeshData (CPU) → vertex + index buffers (GPU)
     │   └─ Models.LoadThemes                    (the looks of layer A and B and the ripple settings: built in, changed by the config)
     └─ loop, per frame:
         1. Window.PumpMessages        (WM_SIZE / F5 / Esc set flags; the loop reads them)
         2. lost device (or F9 test) → Gpu.Recover: everything on the GPU is released and made again on a new Device; the models keep layer, ripple and spin; after 5 tries the program says so and ends
         3. F5      → reload config.ini, Models.LoadThemes, apply vsync
         4. resized → Device.Resize     (release view → ResizeBuffers → create view)
         5. minimised → sleep, skip
         6. Update  → time step, spin angle, camera, keys Z X C Tab (wireframe, detail, spin, layer), Models.Update (spin, ripple clock)
         7. Render  → Models.ClearColor (the background of the layer on the screen), GpuEvent "Frame"
                         Device.BeginFrame     bind RTV + DEPTH, viewport, clear colour (linear, written as sRGB) and depth (1.0)
                         GpuEvent "Opaque.Models"
                             Models.Draw       idle:   FrameCB (V*P, eye, light and sky of the layer) → per object of that layer: World = S*R*T, normal matrix → ObjectCB → DrawIndexed
                                               ripple: the same twice into two off-screen pictures, then one triangle on the back buffer that picks a picture per pixel
                         GpuEvent "Opaque.TestTriangle"      (only with [test_triangle] show = 1)
                             Triangle.Draw     W*V*P → constant buffer → IA/VS/PS state → Draw(3)
         8. Device.EndFrame → Present(vsync) → device-lost check (sets Lost(); step 2 of the next frame handles it)
     └─ shutdown: Gpu.Shutdown = Models.Shutdown → Triangle.Shutdown → Device.Shutdown (reports leaked objects in Debug) → Log.Close
```

## Device recovery

`Present` reports a removed or reset device (driver update, a driver reset after a long GPU stall, a GPU that is switched off). `Device` only sets `Lost()`; it never repairs itself. `main.cpp` owns the three GPU users in a `Gpu` struct: on `Lost()` it shuts all of them down, makes a **new** `Device` object (a lost one is never reused) and initialises all three again. The camera, the layer, the ripple, the spin and the wireframe switch live outside the GPU objects or are not touched by `Init`, so the player sees a flicker and the same scene. The two ripple pictures are made again the next time a ripple needs them. The level of detail goes back to `[models] segments`. The `F9` key (config `[debug] device_reset_key = 1`) runs the same path without a real device loss, so that it can be tested.

## Layers and the ripple (M2)

Every object of the scene has a mask (A = 1, B = 2, both = 3). A draw call is skipped when the mask does not contain the layer that is being drawn. The objects are not moved or copied, and each layer is always drawn with its own theme (light, sky, background).

`LayerRipple` (header only, no Direct3D) keeps the timing: which layer is outside the circle, which one is inside, and how far the circle has opened. `Tab` starts a ripple from the idle state; `Tab` during a ripple turns it round, so the circle closes again and the old layer stays. The state has no jump in any case, because the circle radius is the only thing that changes.

While the circle is closed (idle) the layer is drawn straight to the back buffer: no extra cost. During a ripple, `Models::DrawRipple` does three passes:

1. the outer layer into picture 0 (its own background colour, its own depth buffer),
2. the inner layer into picture 1,
3. back on the back buffer: `ripple.hlsl` draws one triangle that covers the screen and takes each pixel from picture 1 inside the circle and from picture 0 outside, with a bright ring and a small bend of the picture at the ring.

The two pictures are made the first time they are needed and made again when the window size changes. If they cannot be made, the layer is shown without a ripple and the log says why. `StateGuard` puts back the render target, the viewport and every other state that the ripple changes.

## Modules and the allowed direction of dependencies

```
main
 ├─ render   (Device, Triangle, Models, MeshData, LayerRipple, Projection, GpuDebug)   → core
 └─ core     (Log, Assert, Config, Window)  → standard library and Windows only
```

`core` must never include `render` or anything above it. `render` is the only place that talks to Direct3D. New layers (`scene`, `physics`, `game`, `audio`, `ui`) are added by the milestone that needs them, always on top.

Pure-logic code (`Config`, `Log`, `Assert`, `Camera`, `MeshData`, `LayerRipple`) has no Windows or Direct3D dependency and can be tested without a window.

`MeshData` makes the shapes on the CPU; `Models` only copies them to the GPU and draws them. `Projection.h` is the one lens that every pass uses.

## Debugging facilities that exist since M0

- Every GPU object has a name `<System>.<Resource>` (the ripple ones: `Models.Ripple.*`); every pass is wrapped in a `GpuEvent` (RenderDoc / PIX / VS Graphics Debugger show them).
- Debug builds: D3D11 + DXGI debug layers, break on error when a debugger is attached, a leak report at exit.
- `logs/dual.log` and the ring buffer (`log::Recent()`, shown by a debug panel planned for a later milestone).
