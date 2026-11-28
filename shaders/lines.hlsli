// lines.hlsli - the maths of the ink lines of docs/DESIGN.md 6.10 (S2): which pixels of a picture are on an outline or on an inner
//               edge, and how a line is laid over the picture. From the depth buffer alone, one pixel and its four neighbours.
// Why:          the town is drawn with flat tones and a line round its shapes. A line found in the picture (and not drawn with the
//               meshes) is the same for the town, the props and the traveller, needs no second draw and no smooth normals, and
//               tears nowhere on a hard-edged low-poly mesh.
// What:         (1) the outer silhouette: a pixel that is in front of the ones beside it by more than a plane can explain, seen as a
//               ridge of the inverse depth; (2) the inner edge: a pixel round which the surface turns by more than a set angle,
//               seen as the angle between the normals of the four triangles that meet at the pixel. Both are independent of the
//               distance and of the angle at which a plane is seen: a plane, at any distance or slant, has no edge.
// Units:        depth buffer values are D3D ones (0 at the near plane, 1 at the far, not reversed, as Projection.h). 1 / z (metres)
//               is the "inverse depth" w: it is an affine function of the screen position on any plane, which is why it is used.
// Plain HLSL that is also C++ through tests/hlsl_shim.h (nothing here needs a texture or a constant buffer): the tests of
//               tests/test_style.cpp (TestLines) run these functions on a depth buffer that they make. The names `near` and `far`
//               are not used (windows.h defines both as macros).
// Borders:      the pass gives no line to the pixels on the outermost row and column of the picture (a neighbour that is not there is
//               the pixel itself, and a plane seen at a slant would show a false edge there).
// Not here:     the pass itself (the full-screen draw that reads the depth, calls these and lays the ink over the picture).

// The inverse depth (1 / z, 1 / metres) of a depth buffer value d.
float InverseViewDepth(float d, float nearZ, float farZ)
{
    const float a = 1.0f / nearZ;
    return a - d * (a - 1.0f / farZ);
}

// The size of one pixel on the plane at distance 1 of the camera: 2 tan(fov / 2) / height of the picture in pixels.
// `projection11` is the element of the projection matrix that holds 1 / tan(fov / 2) of the height.
float PixelSlope(float projection11, float heightPixels)
{
    return 2.0f / (projection11 * heightPixels);
}

// The point in view space of the pixel whose view-plane position is (sx, sy) (the view ray is (sx, sy, 1)) and whose inverse
// depth is w.
float3 LineViewPoint(float sx, float sy, float w)
{
    const float z = 1.0f / w;
    return float3(sx * z, sy * z, z);
}

float3 LineCross(float3 a, float3 b)
{
    return float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// A vector of length 1 (zero for a vector of no length: a triangle of no area, as at the border of the picture, has no turn to tell).
float3 LineUnit(float3 v)
{
    const float length2 = dot(v, v);
    return length2 > 1.0e-30f ? v * (1.0f / sqrt(length2)) : float3(0.0f, 0.0f, 0.0f);
}

// How well two normals agree: their cosine; a normal that is zero (LineUnit of nothing) agrees with everything.
float LineAgree(float3 a, float3 b)
{
    return (dot(a, a) < 0.5f || dot(b, b) < 0.5f) ? 1.0f : dot(a, b);
}

// How much the surface turns round a pixel: 1 - cos of the largest angle between the normals of the four triangles that
// meet at it (0: flat, 0.18: 35 degrees, 1: a right angle). A plane gives 0 at any distance and slant.
float LineTurn(float sx, float sy, float s, float wC, float wL, float wR, float wU, float wD)
{
    const float3 c = LineViewPoint(sx, sy, wC);
    const float3 r = LineViewPoint(sx + s, sy, wR) - c;
    const float3 u = LineViewPoint(sx, sy + s, wU) - c;
    const float3 l = LineViewPoint(sx - s, sy, wL) - c;
    const float3 d = LineViewPoint(sx, sy - s, wD) - c;
    const float3 n0 = LineUnit(LineCross(r, u));
    const float3 n1 = LineUnit(LineCross(u, l));
    const float3 n2 = LineUnit(LineCross(l, d));
    const float3 n3 = LineUnit(LineCross(d, r));
    const float lowest = min(min(min(LineAgree(n0, n1), LineAgree(n0, n2)), min(LineAgree(n0, n3), LineAgree(n1, n2))),
                             min(LineAgree(n1, n3), LineAgree(n2, n3)));
    return 1.0f - lowest;
}

// How far the inverse depth of a pixel stands over the mean of its neighbours' on the worse axis, in units of the pixel's own
// size on the screen (a second difference: zero on a plane). Large and positive for a pixel in front of what is beside it.
float LineRidge(float wC, float wL, float wR, float wU, float wD, float s)
{
    const float kx = (wL + wR - 2.0f * wC) / (s * wC);
    const float ky = (wU + wD - 2.0f * wC) / (s * wC);
    return max(-kx, -ky);
}

// The edge at one pixel. (sx, sy): its view-plane position; s: PixelSlope; wC, wL, wR, wU, wD: the inverse depth of the pixel and of
// its left, right, upper and lower neighbours (the upper one has the larger sy).
// silhouetteLevel: the ridge from which a pixel is on an outline (the line is full at twice that);
// creaseLo, creaseHi: the turn (LineTurn) from which an inner edge starts and at which it is full;
// jump: a pixel with a neighbour in front of it by more than this (relative, in inverse depth) is the far side of an edge and has
// no line: the line is on the near side only.
// Returns x: the outline, y: the inner edge, both 0 .. 1; z: the depth of view of the pixel in metres (for the fade).
float3 LineEdge(float sx, float sy, float s, float wC, float wL, float wR, float wU, float wD, float silhouetteLevel,
                float creaseLo, float creaseHi, float jump)
{
    const float nearest = max(max(wL, wR), max(wU, wD));
    if (nearest > wC * (1.0f + jump))
    {
        return float3(0.0f, 0.0f, 1.0f / wC);
    }
    const float silhouette = smoothstep(silhouetteLevel, 2.0f * silhouetteLevel, LineRidge(wC, wL, wR, wU, wD, s));
    const float crease = smoothstep(creaseLo, creaseHi, LineTurn(sx, sy, s, wC, wL, wR, wU, wD));
    return float3(silhouette, crease, 1.0f / wC);
}

// The inner edges fade out with distance (the depth is too coarse there to tell a turn from noise): 1 up to fadeNear, 0 from fadeFar.
float LineCreaseFade(float viewZ, float fadeNear, float fadeFar)
{
    return 1.0f - smoothstep(fadeNear, fadeFar, viewZ);
}

// The outlines keep their strength to fadeFar and then thin to farAlpha at five times that distance.
float LineSilhouetteFade(float viewZ, float fadeFar, float farAlpha)
{
    return lerp(1.0f, farAlpha, smoothstep(fadeFar, 5.0f * fadeFar, viewZ));
}

// How much of a pixel at `dist` pixels from the middle of an edge pixel a line `width` pixels wide covers.
float InkReach(float dist, float width)
{
    return saturate(0.5f * width + 0.5f - dist);
}

// The ink of one edge pixel (`edge` from LineEdge) at `dist` pixels from the pixel that is being drawn: the pass takes the largest over a
// square of pixels round it.
float LineCoverage(float3 edge, float dist, float widthSilhouette, float widthCrease, float fadeNear, float fadeFar, float farAlpha)
{
    const float silhouette = edge.x * LineSilhouetteFade(edge.z, fadeFar, farAlpha) * InkReach(dist, widthSilhouette);
    const float crease = edge.y * LineCreaseFade(edge.z, fadeNear, fadeFar) * InkReach(dist, widthCrease);
    return max(silhouette, crease);
}

// The line is a dark of the local colour and not black (6.10): the colour of the line of the moment, with a part of the picture's own
// colour darkened to 30%.
float3 InkColor(float3 lineColor, float3 scene, float localMix)
{
    return lerp(lineColor, scene * 0.3f, localMix);
}

// The picture with the line laid over it. `coverage` 0 .. 1 (LineCoverage, the largest over the pixels round), `amount` 0 .. 1: the master.
float3 LineComposite(float3 scene, float coverage, float3 lineColor, float localMix, float amount)
{
    return lerp(scene, InkColor(lineColor, scene, localMix), saturate(coverage * amount));
}
