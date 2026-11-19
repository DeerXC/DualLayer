# Architecture

Covers milestones M0, M1 and M2 so far (layers, ripple, collision, walking).

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
     ├─ Player.SetSettings, SetSpawn, Respawn     (the walking body: [player] and [collision] in the config)
     └─ loop, per frame:
         1. Window.PumpMessages        (WM_SIZE / F5 / Esc set flags; the loop reads them)
         2. lost device (or F9 test) → Gpu.Recover: everything on the GPU is released and made again on a new Device; the models keep layer, ripple and spin; after 5 tries the program says so and ends
         3. F5      → reload config.ini, Models.LoadThemes, Player settings, apply vsync
         4. resized → Device.Resize     (release view → ResizeBuffers → create view)
         5. minimised → sleep, skip
         6. Update  → time step, spin angle, keys Z X C Tab N F Space (wireframe, detail, spin, layer, collision, walk / fly, jump),
                      camera (walking: it only turns; flying: it also moves), Models.Update (spin, ripple clock), then
                      walking: Player.Update, the camera takes the eye of the body
                      flying + collision: ResolveSphere pushes the camera ball out of the solid objects of the layer the screen is going to show
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

`Present` reports a removed or reset device (driver update, a driver reset after a long GPU stall, a GPU that is switched off). `Device` only sets `Lost()`; it never repairs itself. `main.cpp` owns the three GPU users in a `Gpu` struct: on `Lost()` it shuts all of them down, makes a **new** `Device` object (a lost one is never reused) and initialises all three again. The camera, the layer, the ripple, the spin, the wireframe switch and the walking body live outside the GPU objects or are not touched by `Init`, so the player sees a flicker and the same scene. The two ripple pictures are made again the next time a ripple needs them. The level of detail goes back to `[models] segments`. The `F9` key (config `[debug] device_reset_key = 1`) runs the same path without a real device loss, so that it can be tested.

## Layers and the ripple (M2)

Every object of the scene has a mask (A = 1, B = 2, both = 3). A draw call is skipped when the mask does not contain the layer that is being drawn. The objects are not moved or copied, and each layer is always drawn with its own theme (light, sky, background).

`LayerRipple` (header only, no Direct3D) keeps the timing: which layer is outside the circle, which one is inside, and how far the circle has opened. `Tab` starts a ripple from the idle state; `Tab` during a ripple turns it round, so the circle closes again and the old layer stays. The state has no jump in any case, because the circle radius is the only thing that changes.

While the circle is closed (idle) the layer is drawn straight to the back buffer: no extra cost. During a ripple, `Models::DrawRipple` does three passes:

1. the outer layer into picture 0 (its own background colour, its own depth buffer),
2. the inner layer into picture 1,
3. back on the back buffer: `ripple.hlsl` draws one triangle that covers the screen and takes each pixel from picture 1 inside the circle and from picture 0 outside, with a bright ring and a small bend of the picture at the ring.

The two pictures are made the first time they are needed and made again when the window size changes. If they cannot be made, the layer is shown without a ripple and the log says why. `StateGuard` puts back the render target, the viewport and every other state that the ripple changes.

## Collision (M2)

The solid body is a ball (the camera, radius 0.35 m by default). `ResolveSphere` (`scene/Collision.h`) moves only the place of the ball out of everything that is solid in the layer the screen is going to show. There is no speed or physics inside it: one call, one correction.

The solids are the objects of the scene table `scene/SceneData.h`. The renderer (`Models.cpp`) reads the same table, so what is drawn and what is solid cannot disagree. An object is solid only in the layers of its mask, so the red cube and the grey cylinder exist for the ball only in A, the blue sphere and the yellow torus only in B, and the floor in both.

Every shape is exact: a box that can be stretched and turned, a ball, a cylinder with flat ends, and a torus (a tube bent into a ring, so that the hole can be flown through). For each solid the ball centre is moved into the local space of the object (the inverse of the world matrix of the renderer), pushed out along the shortest way, and moved back. The shapes turn with time, so the collision takes `Models::SpinSeconds()`, the same time that the renderer uses. Up to 16 rounds settle a ball that sits between two solids.

During a ripple the NEW layer is already the solid one (`Models::CollisionLayer()`). A ball that is inside an object of the new layer when `Tab` is pressed is pushed to its surface.

## Walking (M2)

`Player` (`scene/Player.h`, header only, no Windows) is a body of three balls of the collision radius, one at the feet, one at the eye and one halfway; the camera is the eye. Walking is the default; `F` switches to the free-flying camera, and `N` (collision off) also means flying, because a body needs something to stand on.

Per frame the time is cut into steps that move the body by less than a part of the radius (the floor is only 0.2 m thick). In each step: the horizontal speed comes from the keys and from where the camera looks (only its direction on the ground counts, so looking up does not slow the walk); gravity changes the vertical speed; a jump sets it; the body moves; `ResolveBody` pushes the three balls out of the solids, calling `ResolveSphere` for each ball and moving the whole body by each push. The direction of the push is the normal of what was hit: the part of the speed that goes into it is removed and the rest stays, so a floor ends a fall and a slope that is too steep turns the fall into a slide. Then a probe a few centimetres below the feet tells whether the body stands: the push must point up by at least 0.6 of its length (about 53 degrees). A standing body has no slide speed left (friction).

Feel: a jump still works for 0.08 s after leaving an edge (coyote time), and a jump pressed 0.1 s before the landing happens at the landing (jump buffer). A body that falls 20 m below the floor goes back to the spawn point.

The layer is a parameter of every `Update`, so a body that stands on an object that does not exist in the new layer falls at once. The turning shapes do not carry the body along; they only push it away.

## Modules and the allowed direction of dependencies

```
main
 ├─ render   (Device, Triangle, Models, MeshData, LayerRipple, Projection, GpuDebug)   → core, scene (SceneData only)
 ├─ scene    (Camera, SceneData, Collision, Player)                                   → standard library only
 └─ core     (Log, Assert, Config, Window)  → standard library and Windows only
```

`core` must never include `render` or anything above it. `render` is the only place that talks to Direct3D. New layers (`physics`, `game`, `audio`, `ui`) are added by the milestone that needs them, always on top.

Pure-logic code (`Config`, `Log`, `Assert`, `Camera`, `MeshData`, `LayerRipple`, `SceneData`, `Collision`, `Player`) has no Windows or Direct3D dependency and can be tested without a window.

`MeshData` makes the shapes on the CPU; `Models` only copies them to the GPU and draws them. `Projection.h` is the one lens that every pass uses.

## Debugging facilities that exist since M0

- Every GPU object has a name `<System>.<Resource>` (the ripple ones: `Models.Ripple.*`); every pass is wrapped in a `GpuEvent` (RenderDoc / PIX / VS Graphics Debugger show them).
- Debug builds: D3D11 + DXGI debug layers, break on error when a debugger is attached, a leak report at exit.
- `logs/dual.log` and the ring buffer (`log::Recent()`, shown by a debug panel planned for a later milestone).
