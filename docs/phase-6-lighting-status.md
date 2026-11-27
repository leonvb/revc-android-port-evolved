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

GL/GLES also supports the rim-ped (including skinned) and Neo-vehicle pipes.
This is not yet full M6.3/M6.4: D3D9 ped/vehicle shaders are being compiled,
vehicles using the classic MatFX pipe do not yet receive these pixel lights,
the alternate `NEW_RENDERER` building path bypasses these pipes, and the road
reflection is a local environment-map/sky approximation rather than a planar
scene reflection. GPU/image/performance testing remains open.
M6.5 directional shadow mapping remains unimplemented.
