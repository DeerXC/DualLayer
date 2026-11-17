# 理解メモ M0: 1フレームの流れ / the flow of one frame
Date: 【 】   Self-rating 1-5: 【 】

Written by myself after reading `docs/ARCHITECTURE.md` and stepping through `RunApp` with the debugger.

## 1. Draw the frame (hand-drawn photo or a diagram is fine)

## 2. In my own words
- What does `Present` do, and why does the swap chain have two buffers?
- Why is there an sRGB view on a UNORM buffer, and what would the picture look like without it?
- Why must the back-buffer view be released before `ResizeBuffers`?
- What is the difference between a "debug layer message" and an `ASSERT` in this project?
- Why is `SetProcessDpiAwarenessContext` called before the window exists?

## 3. An experiment I ran myself (result + screenshot)
(example: set `spin_deg_per_sec = 45`, then temporarily transpose the matrix in `Triangle::Draw` and describe what is seen)
  Observed:

## 4. What I do not understand yet
-
