# M6.2 post-processing

The Modern master switch gates FXAA 3.11 Quality and Dual-Kawase bloom on
GL/GLES and D3D9. `[Modern] FXAA` selects FXAA; the old experimental edge AA
remains an independently selectable fallback and is not FXAA. If both are
selected, FXAA takes precedence. `[Modern] NeonBloom` has Off, Low and High
settings. The Modern preset selects FXAA and High bloom; Classic selects off.
Turning the master switch on while the preset is Classic now applies the full
Modern preset (including its non-visual settings). Switching it back on from a
Custom preset preserves the user's selections. Existing Custom configurations
are not automatically migrated; select the Modern preset to reset them.
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
may still use the copy path. When the scene camera changes size, PostFX
buffers are reopened even if legacy motion blur is disabled; the effect of
this change on Android depends on whether a resize occurs after initial setup.
Unsupported shader models, unavailable shaders, invalid capture bounds and
failed target creation bypass the new effects.
The budget estimates tightly packed RGBA8 memory, not actual driver allocation.

## Android runtime diagnostics

Android writes `[ModernRender]` entries to logcat and, when the game folder is
writable, to `log.txt` in that folder (normally `reVC/log.txt` on shared
storage). A session-start marker separates launches. Each effect/status pair
is logged at most once per launch, including disabled or bypass reasons and a
`draw submitted` status; this avoids per-frame disk writes. A submitted draw
or selected world pipeline only means the CPU reached that path, not that the
GPU produced the expected pixels. Shadow detail is the initial caster count;
bloom detail is the selected bloom level; AA detail is 1 for FXAA and 0 for
the experimental fallback. World detail bits report vertex normals (1), road
(2), and active shadow map (4) for the first selected world atomic. These are
initial observations, not frame-by-frame telemetry.

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
