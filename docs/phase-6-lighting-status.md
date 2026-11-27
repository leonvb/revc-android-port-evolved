# Phase 6 lighting and road-reflection status

The Modern options `DynamicPointLights` and `WetRoadReflections` are master
switch gated. On GL/GLES and D3D9, simple world atomics with normals use a separate
modern shader when either effect applies. It selects up to eight overlapping
point or directional lights per atomic, evaluated in the fragment shader;
fog-only and darkening lights remain on the classic path. Marked road atomics
use weather wetness, a sky-color fallback or the existing Neo environment map
when it has a valid rendered frame, and a point-light specular term. Environment
map rendering itself uses the classic pipeline to avoid feedback. Missing
shaders/normals fall back to the existing renderer, and Classic is unchanged.

Both backends also support the rim-ped (including skinned) and Neo-vehicle
pipes, and opaque meshes in the alternate `NEW_RENDERER` building path.
This is not yet full M6.3/M6.4: vehicles using the classic MatFX pipe and
deferred transparent building meshes do not yet receive these pixel lights,
and the road reflection is a local environment-map/sky approximation rather
than a planar scene reflection. If the alternate renderer's legacy 2,000-entry
per-pass deferred list fills, further transparent entries are dropped instead
of writing past its bounds. GPU/image/performance testing remains open.
M6.5 directional shadow mapping remains unimplemented.
