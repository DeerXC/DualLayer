# 理解メモ M1: モデルを描く / drawing models
Date: 【 】   Self-rating 1-5: 【 】

Written by myself after reading `src/render/Models.cpp` and stepping through `Models::Draw` with the debugger.
"I do not understand" is a valid answer: it is written down and then asked about.

## 1. The flow (the arrows are given, the words are my own)
```
Init   Models.Init
         ├─ CompileShader ×2      models.hlsl -> a vertex shader and a pixel shader        (what is a shader?                         )
         ├─ CreateInputLayout     "12 bytes of position, then 12 bytes of normal"          (why does the GPU need to be told?         )
         ├─ CreateStates          rasterizer, depth test, no blending                      (what does the depth test do?              )
         └─ CreateMeshes          MeshData (CPU) -> vertex + index buffers (GPU)           (why indices?                              )
Frame  Device.BeginFrame          bind the colour AND the depth buffer, clear both         (why is depth cleared to 1.0?              )
       Models.Draw
         ├─ FrameCB               View * Projection, the eye, the light                    (what is the view matrix for?              )
         └─ for every object      World = Scale * Rotation * Translation -> ObjectCB -> DrawIndexed
                                  VS: position * World * ViewProjection   PS: the light    (what does the pixel shader return?        )
```

## 2. Three questions
1. A cube has 8 corners. Why does `MakeCube` make **24** vertices?
   My answer:
2. Turn the order of the three indices of one triangle round (swap two of them). What appears on the screen, and why? (Try it on the cube; undo it afterwards.)
   My answer:
3. `World = Scale * Rotation * Translation`. What would change with `Translation * Rotation` (first move, then turn)?
   My answer:

## 3. Two experiments (no recompile: just press the keys)
- [ ] `X` a few times: the sphere goes from 8 to 64 segments. What happens to the outline and to the highlight? Why does the light look worse with few segments?
  Observed:
- [ ] In `Device::BeginFrame` pass `nullptr` instead of `m_depthView.Get()` to `OMSetRenderTargets` (one line), run, walk round the shapes. What goes wrong? Put it back.
  Observed:

## 4. What I changed
-
