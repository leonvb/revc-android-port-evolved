# M6.1 draw distance: suspended

The Modern extended draw-distance feature is suspended after reports of distant
flicker even at low settings and alternating low/high detail nearby at high
settings. Its slider is hidden, and the active Modern multiplier is always
1.0, including when an older configuration contains a higher saved value.
The Modern preset no longer selects 2.2×. The existing Display draw-distance
control and the camera's normal FOV/height scaling are unchanged. Other Modern
visual features remain independently available.

The previous experimental policy and the old saved setting are retained for
possible future development; they do not extend the active camera LOD, far
clip, fog, population horizon, or priority scan while suspended. The Modern
streaming cache policy remains independent and never reduces the Classic cache
budget. Android's synchronized streamer and scripted scene-load timer fix are
unchanged.

## Before considering re-enabling

- Reproduce and identify the cause of high/low model alternation and distant
  flicker. The previous priority-scan mitigation did not resolve the reports.
- Verify Classic and Modern on real devices, including older saved settings,
  different weather, live preset changes, interiors, cutscenes and island
  transitions. Check the bottom-left radar, GPS route and touch-control overlap.
- Measure streaming churn, RAM/VRAM, frame time and thermal behavior. Confirm
  renderer lists and model/streaming limits at the full intended range.

Suspending this feature does not establish that other sources of flicker are
fixed. M6.1 remains incomplete; see the other phase-6 status documents for the
separate lighting and post-processing work. Real-time directional sun/moon
shadow mapping remains unimplemented.
