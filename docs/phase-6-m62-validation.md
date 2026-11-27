# M6.2 post-processing: experimental edge smoothing

This branch contains an **opt-in experimental edge-smoothing pass**, not the
roadmap's FXAA 3.11 Quality implementation. It is off by default, including in
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
- Implement and validate FXAA 3.11 Quality (the existing FXAA setting is still
  a placeholder) and Dual-Kawase bloom with a safe render-target lifecycle and
  backend parity. This experimental AA does not complete M6.2.
- M6.3–M6.5 remain unimplemented: per-pixel point lights, wet-road material
  reflections, and directional real-time shadows.
