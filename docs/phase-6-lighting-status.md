# Phase 6 lighting and road-reflection status

The Modern options `DynamicPointLights` and `WetRoadReflections` are master
switch gated. Simple world atomics use a separate modern shader when either
effect applies. GL/GLES also handles meshes without vertex normals, using a
triangle normal derived in the fragment shader on GL3/GLES3; D3D9 still
requires vertex normals. GL2 does not assume optional derivatives and leaves
local lighting disabled on meshes without normals, though they can still use
other supported world effects. The shader selects up to eight overlapping
point or directional lights per atomic, evaluated in the fragment shader;
fog-only and darkening lights remain on the classic path. Marked road atomics
use weather wetness, a sky-color fallback or the existing Neo environment map
when it has a valid rendered frame, and a point-light specular term. Environment
map rendering itself uses the classic pipeline to avoid feedback. Missing
shaders or unsupported geometry fall back to the existing renderer, and Classic is unchanged.

Both backends also support the rim-ped (including skinned) and Neo-vehicle
pipes, and opaque meshes in the alternate `NEW_RENDERER` building path.
This is not yet full M6.3/M6.4: vehicles using the classic MatFX pipe and
deferred transparent building meshes do not yet receive these pixel lights,
and the road reflection is a local environment-map/sky approximation rather
than a planar scene reflection. If the alternate renderer's legacy 2,000-entry
per-pass deferred list fills, further transparent entries are dropped instead
of writing past its bounds. GPU/image/performance testing remains open.
M6.5 has an initial **GL/GLES only** directional shadow pass. With Modern
and `RealtimeShadows` set to 1 or 2, an outdoor centered-perspective gameplay
camera builds a texel-snapped light fit. The pass packs depth into an RGBA8
camera texture (GLES z rasters are not sampleable), alpha-tests texture
cutouts, and samples a 3x3 PCF filter on the modern world pipe and the opaque
alternate building pipe. Medium uses 512 pixels over a 70-unit view slice;
High uses 1024 pixels over 100 units. A provisional opposite-sun moon and
horizon fade are used; the moon is not synchronized to an independent moon.
Classic shadows stay enabled. If shaders, resources, or framebuffer setup fail,
the pass is skipped. Interiors, cutscenes, offset and parallel cameras skip it.

This is **not a complete implementation**: only opaque, non-skinned atomic
entities from the existing visibility lists are eligible casters (up to 512,
nearest first); clump-based vehicles/peds, vertex-alpha/fading meshes, many
offscreen casters, and deferred translucent buildings are not covered. Only
modern world-pipe receivers receive the map, not peds or vehicles. Weather
strength, cascades, D3D9, device/context-loss recovery and real-device quality
and performance testing remain open. Basis transport reduces rotation jumps
but can still fall back discontinuously for large light-direction changes.
