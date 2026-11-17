# 理解メモ M2: 二つのレイヤー / two layers

Date: 【 】   Self-rating 1-5: 【 】

Written by myself after reading `src/render/Models.cpp` (`kScene`, `Update`, `Draw`, `BlendedTheme`) and stepping through `Models::Draw` with the debugger.
"I do not understand" is a valid answer: it is written down and then asked about.

## 1. The flow (the arrows are given, the words are my own)

```
Tab pressed   main.cpp: PressedOnce(VK_TAB)  ->  Models.ToggleLayer      m_layer goes 0 <-> 1               (why a number and not a bool?          )
Every frame   Models.Update                  m_layerBlend moves toward m_layer by dt / fade_seconds        (why dt?                               )
              Models.ClearColor              the two backgrounds, mixed by smoothstep(blend)               (why smoothstep?                       )
              Models.Draw
                ├─ FrameCB                   the two lights and skies, mixed in the same way               (what is mixed, what is not?           )
                └─ for every object          skip when (layerMask & (1 << visible layer)) == 0             (what does & do here?                  )
```

## 2. Three questions

1. Why is a layer a label on an object (`layerMask`), and not "move the other layer's objects far away"?
   My answer:
2. The objects swap when the blend is 0.5, not when `Tab` is pressed. What would the picture look like if they swapped at once? And if they swapped only at the end of the fade?
   My answer:
3. The mask of the floor is `kLayerBoth` (3). What is `3 & (1 << 0)` and what is `3 & (1 << 1)`? Why does the floor never disappear?
   My answer:

## 3. Experiments (change a number in `config/default.ini`, press F5)

- [ ] Add `[layers]` and `fade_seconds = 0`, then `fade_seconds = 2`. What changes in the feeling of the switch?
  Observed:
- [ ] Add `[layer_b]` and `light_x = -0.55`. Where do the bright sides of the shapes go? What happens to the highlight on the sphere?
  Observed:
- [ ] Set `[layer_a] clear_r = 0.3`. Why does the floor not change colour at all, although it is in both layers?
  Observed:

## 4. What I changed

-

## 5. What I do not understand yet

-
