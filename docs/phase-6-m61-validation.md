# M6.1 draw distance: validation status

This is a first implementation, not a completed milestone. It adds a 1–4×
Modern draw-distance slider (default 2.2× in the Modern preset), preserves the
Classic/master-off setting at 1×, extends the outdoor far clip and fog, and
moderately extends selected population generation/removal distances. The
original generation multiplier is unchanged for other systems. Alternate traffic
spawn batches retain the original near horizon; the initial traffic batch and
wanted-car spawns use their original distances. Pedestrian spawn batches retain
the near radius; forced car generators retain their original range.
The slider compounds the existing Display draw-distance setting. The actual
far clip is capped at 2000 world units unless the original clip is already
larger; interiors, cutscenes, scripted/cinematic and top-down cameras keep
their normal distances. Existing pool,
model and renderer limits can still cause pop-in or dropped distant entities.

On supported 64-bit PC/Android builds, Modern uses a streaming cache budget
based on one quarter of startup free physical memory, with the existing 65 MiB
minimum and a 512 MiB cap. Zero/unknown memory falls back to the Classic
budget. This is a cache limit, not a reservation or a live memory-pressure
measurement. Switch and 32-bit builds retain the existing budget. The budget
is never reduced below the previous platform-derived Classic budget. A capable
GPU and actual device memory pressure still need assessment.

## Required before accepting M6.1

- CI builds have passed for desktop GL3, Android GLES3, Switch GLES3, and
  Windows D3D9 on the initial M6.1 commit. Run those targets on actual devices;
  neither builds nor standalone tests replace in-game verification.
- Compare Classic/master-off and Modern at 1×, 2.2× and 4× across clear, foggy
  and rainy weather, including interiors, cutscenes, scripted cameras, aircraft,
  bridges and island transitions. Check clipping, fade/pop-in, fog and missing
  geometry as well as save/reload and live menu changes.
- Inspect traffic and pedestrians near and far (including player surroundings),
  mission/script behavior, culling, startup spawning and pool pressure. The
  current change does not independently configure each generator or guarantee
  zero pop-in.
- Measure streaming churn, RAM/VRAM usage, CPU/GPU frame time and thermal
  behavior on both constrained and high-memory target devices. Verify budget
  changes and recovery when the master toggle is changed in game. Lower
  settings or redesign limits if memory or frame-time budgets are exceeded.

M6.2–M6.5 (bloom/FXAA, per-pixel lights, wet roads and directional shadows)
are not implemented by this change and each requires separate shader/backend
and device validation.
