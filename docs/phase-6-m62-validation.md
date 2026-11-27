# M6.2 post-processing

The Modern master switch gates FXAA 3.11 Quality and Dual-Kawase bloom on
GL/GLES and D3D9. `[Modern] FXAA` selects FXAA; the old experimental edge AA
remains an independently selectable fallback and is not FXAA. If both are
selected, FXAA takes precedence. `[Modern] NeonBloom` has Off, Low and High
settings. The Modern preset selects FXAA and High bloom; Classic selects off.
See `docs/fxaa-third-party.md` for FXAA provenance and binary notices.

Bloom captures after motion blur and before HUD, extracts highlights into a
three-level half/quarter/eighth-resolution pyramid, downsamples and upsamples
using Dual-Kawase kernels, and adds the result to the scene before FXAA. The
peak-channel LDR soft-knee threshold retains saturated red/blue; it is not an
HDR or emissive-mask pass and can also pick up bright non-emissive surfaces.
Intensity is reduced during daytime. Targets are color-only, released with the
postfx lifecycle, bounded by device maximum size and a conservative 24 MiB
minimum-size budget, and bypassed after allocation or target-binding failure.
The renderer resumes the scene camera and its depth buffer on failure. D3D9
ends each scene before beginning the next; GL checks FBO completeness.

The scene copy path currently uses camera-raster dimensions at reduced GL/SDL
render scale. Both modern effects skip at reduced scale; other existing effects
may still use the copy path. Unsupported shader models, unavailable shaders,
invalid capture bounds and failed target creation bypass the new effects.
The budget estimates tightly packed RGBA8 memory, not actual driver allocation.

## Evidence and remaining checks

- Standalone tests exercise pairing, bounds, target budget, pass ordering,
  day/night factor and a CPU reference of the bloom filter math; they do not
  execute rendering. Shader CI compiles/links desktop GL and GLES and compiles
  HLSL SM3; it also verifies embedded bytecode and generated GLSL includes.
- Real-device visual and performance behavior has not been independently
  verified here. In particular, exercise GL/D3D orientation, odd dimensions,
  half-pixel offsets, target switches, resize/device loss, allocation failures,
  screenshots, motion blur, HUD, menus, mobile bandwidth, and driver behavior.
- M6.3–M6.5 still require per-pixel point lights, wet-road reflections and
  directional real-time shadows; M6.2 is not a claim that all Phase 6 is done.
