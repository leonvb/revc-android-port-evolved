# M6.2 post-processing: experimental edge smoothing

This branch contains FXAA 3.11 Quality for GL/GLES and D3D9, plus an independent
**opt-in experimental edge-smoothing pass**. It is off by default, including in
the Modern preset; enable Modern features and select `EDGE AA (EXPERIMENTAL)`
in the Graphics menu (or Display menu on builds without a separate Graphics
page). The persistent setting is `[Modern] ExperimentalEdgeAA`; disabling the
Modern master switch bypasses it.

The pass captures the rendered game scene after motion blur and before HUD/menu
rendering and samples a bounded, contrast-adaptive neighborhood. It supports
GL3/GLES and D3D9 shader model 3, skips the effect if shader creation fails or
the framebuffer is unavailable, and does not require an additional raster.
It uses the existing power-of-two backbuffer and clamps samples to the copied
area. Source, generated GLSL include, D3D9 bytecode and an include generated
from the bytecode are checked in. The shader workflow compiles/links the actual
librw vertex and fragment headers for desktop GL and GLES and compiles the HLSL
with the Windows SDK; it also verifies the generated assets.

## Still required

- Validate visually on actual GL3, Android/Switch GLES and D3D9 devices: verify
  image orientation, aspect ratios, non-power-of-two resolutions, resize/device
  loss, menus, HUD, cutscenes, motion blur, droplets and master toggle changes.
- Measure GPU/frame time and texture-copy cost, especially on mobile GPUs.
- Integrate Dual-Kawase bloom with a safe render-target lifecycle and backend
  parity. Experimental AA alone does not complete M6.2.
- M6.3–M6.5 remain unimplemented: per-pixel point lights, wet-road material
  reflections, and directional real-time shadows.

## Render-target groundwork (in progress)

The GL backend now permits a camera with no depth raster, maintains reciprocal
color/depth raster links during replacement or removal, and uses the GLES
renderbuffer detach operation when appropriate. A standalone test covers the
pairing logic, but it does **not** create a GL context or verify framebuffer
completeness, raster destruction, or real driver behavior. Before using this
for multipass bloom, test color-only targets and depth reuse/destruction on
both desktop GL and GLES, including the Android render-scale path. D3D9 target
switching must separately account for BeginScene/EndScene and device loss.

The existing framebuffer copy uses camera-raster dimensions even when Android
render scaling changes the actual target size. Validate capture and sampling
at reduced scale before relying on bloom there; the bookkeeping fix does not
address that copy path. Experimental AA now skips its own capture at reduced
GL/SDL render scale. Other existing effects may still use the copy path. Bloom
and FXAA remain unimplemented.

## Dual-Kawase groundwork

`modernBloom.frag` and `modernBloom_PS.hlsl` define matching bright/down and
up kernels, and `ModernBloomPlan.h` proposes a three-level, five-pass chain.
These are **not connected to the renderer**: the bloom setting still does
nothing. The reference uses an LDR peak-channel soft-knee threshold rather
than luminance, to keep saturated red and blue highlights eligible; it is not
an HDR or emissive-mask implementation and can also pick up bright non-emissive
surfaces. The bright pass must use a 0.5-source-texel radius to preserve isolated
highlights on even-sized 2:1 downsampling; the other pass radii and the threshold
still need visual tuning. Odd dimensions and bilinear filtering can reduce
subpixel highlights. The proposed chain uses ceiling-halves, valid-rectangle
texel-center clamping, and distinct input/output targets; the caller must
supply UVs in each source texture's native orientation. Its target byte count
is a tightly packed RGBA8 lower bound, not a VRAM or allocation guarantee.

`tests/modern_bloom_plan.cpp` checks sizing, budgets, bounds and pass hazards;
`tests/modern_bloom_reference.py` exercises a CPU reference of the formulas.
Neither test executes the shader. Shader CI is configured to compile/link the
GLSL and compile HLSL as SM3, but does not compare rendered pixels. Before
wiring this into gameplay, validate actual render-target creation/completeness,
GL/D3D texture orientation and half-pixel offsets, viewport restoration,
state/cache restoration, resize/device loss, failed allocation/shader fallback,
night modulation, and target GPU bandwidth. The current experimental AA must
continue to skip its capture at reduced GL/SDL scale until the copy is fixed.
