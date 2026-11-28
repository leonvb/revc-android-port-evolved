# 04 — Rendering, Lighting & World Visuals

> Design targets below are not all implemented. Modern extended draw distance is
> currently suspended due to reported flickering; see
> [M6.1 status](phase-6-m61-validation.md). Directional shadow mapping remains
> unimplemented.

## Overview & Goals
`reVC` uses `librw` (`vendor/librw`), which already abstracts RenderWare pipelines over **OpenGL 3.3+**, **OpenGL ES 3.0+** (Android / Switch), and **Direct3D 9** (Windows), and already includes `EXTENDED_PIPELINES` (Neo vehicle gloss, world lightmaps, rim lighting) and `EXTENDED_COLOURFILTER` (`CPostFX`).

However, Vice City's rendering still exhibits classic 2002 constraints:
1. **PS2 Memory Streaming Budgets**: Short LOD switch distances, aggressive vehicle/ped despawning, and small streaming memory caps designed for 32 MB RAM.
2. **Flat Blob / Projected Planar Shadows**: Buildings and palm trees cast no real dynamic sun shadows, and vehicles/peds use simple projected textures (`Shadows.cpp`).
3. **Vertex-Only Point Lights**: Car headlights, neon signs, sirens, and muzzle flashes mostly rely on ground coronas/projected blobs rather than per-pixel dynamic lighting on world/ped meshes.
4. **No HDR Neon Bloom, Wet-Road Reflections, or Post-Process AA**: Vice City's signature 1986 Ocean Drive neon aesthetic benefits enormously from bloom and wet-asphalt reflections, while mobile GPUs (Mali / Adreno) handle FXAA/SMAA far more efficiently than MSAA.

---

## Target Files in Codebase

| File | Role |
| :--- | :--- |
| `src/core/config.h` | Add `#define EXTENDED_DRAW_DISTANCE`, `#define POSTFX_BLOOM_AA`, `#define DYNAMIC_PERPIXEL_LIGHTS`, `#define REALTIME_SHADOW_MAPS`, `#define WET_ROAD_REFLECTIONS` |
| `src/core/Streaming.cpp` & `Renderer.cpp` | Streaming memory budget (`ms_memoryAvailable`), LOD distance multiplier (`CRenderer::ms_lodDistScale`), offscreen despawn radii |
| `src/extras/postfx.cpp` & `.h` | Multi-pass FBO chain for Dual-Kawase HDR Bloom, FXAA 3.11 / SMAA, and subtle vignette/chromatic aberration options |
| `src/extras/shaders/` | Add GLSL (Desktop + ES 3.0) and HLSL shaders (`bloom_threshold`, `bloom_downsample`, `bloom_upsample`, `fxaa`, `shadow_depth`, `wet_road`) + regenerate `.inc` headers via `make_glsl.sh` / `make_hlsl.cmd` |
| `src/extras/custompipes.cpp`, `custompipes_gl.cpp`, `custompipes_d3d9.cpp` | Upload dynamic light arrays, shadow matrices, and wet-road reflection uniforms to world/vehicle/ped shaders |
| `src/renderer/PointLights.cpp` & `Lights.cpp` | Forward top $N$ (`8–16`) dynamic point lights (headlights, sirens, explosions, muzzle flashes) to fragment shaders |
| `src/renderer/Shadows.cpp` & `ShadowCamera.cpp` | Optional 2-cascade directional sun/moon shadow map FBO |
| `vendor/librw/src/gl/` | Depth-texture FBO support (`GL_DEPTH_COMPONENT24`) for shadow maps and screen-space effects on GL3 & GLES3 |

---

## Detailed Design & Implementation Steps

### 1. Extended Draw Distance, Streaming & Zero Pop-In (`Streaming.cpp`, `Renderer.cpp`, `Population.cpp`)
*(Lowest GPU cost, massive immediate visual upgrade)*
- **LOD Distance Multiplier**:
  - Expose `fLodDistScale` slider (`1.0x` Classic up to `4.0x` Ultra; default `2.2x` on modern hardware) in Display Settings.
  - Scale `CModelInfo::m_fLodDistance` comparisons in `CRenderer::SetupEntityVisibility` and `SetupBigBuildingVisibility` so high-detail buildings stay rendered across Vice Beach and Mainland simultaneously instead of swapping to blocky low-poly LODs two blocks away.
- **Streaming Memory Budget**:
  - Ensure `CStreaming::ms_memoryAvailable` defaults to at least `256–512 MB` on 64-bit desktop/Android builds so high-detail TXDs and DFFs remain cached in RAM without thrashing `CdStream`.
- **Vehicle & Ped Horizon Persistence**:
  - Pair existing `PED_CAR_DENSITY_SLIDERS` with configurable generation/cull distances (`CTheCarGenerators`, `CCarCtrl::GenerateRandomCars`, `CPopulation::ManagePopulation`) so traffic extends smoothly down Ocean Drive and bridges instead of popping in 60 meters ahead.

---

### 2. Dual-Kawase Neon Bloom & FXAA Post-Processing (`postfx.cpp`, `src/extras/shaders/`)
Tile-based mobile GPUs (like the Immortalis-G720 on OPPO Find X7) are bandwidth-sensitive to 4x MSAA on full-resolution framebuffers, whereas half/quarter-res Dual-Kawase bloom + single-pass FXAA costs `< 0.6 ms` at 1080p/1440p.

#### Pipeline in `CPostFX::Render`:
1. **Scene Capture**: Reuse `CPostFX::pBackBuffer` raster (already captured for colour filter & screen droplets).
2. **Bright-Pass + Dual-Kawase Pyramid (3 Mips: 1/2, 1/4, 1/8 res)**:
   - Extract emissive highlights (neon signs, coronas, sun glare, taillights, explosions) above soft knee threshold (`luminance > 0.78`), modulated by `CTimeCycle` night/dusk factor so daytime whites don't over-bloom while nighttime Ocean Drive glows richly.
   - Downsample & upsample with 5-tap Dual-Kawase tent filter.
3. **Composite + Colour Filter + FXAA**:
   - Combine scene + Vice City PS2/PC colour grading + bloom in one pass, followed by **FXAA 3.11 Quality** (with luma in alpha) to eliminate jagged powerlines, palm fronds, and car panel edges.

---

### 3. Dynamic Per-Pixel Point Lights (`PointLights.cpp`, `custompipes_gl.cpp`)
- Currently, `CPointLights::aLights` (max 32 per frame: car headlights, taillights, police sirens, weapon muzzle flashes, fire/explosions) only affect vertex colors on nearby dynamic objects and draw flat ground decals on roads.
- **Shader Upgrade**:
  - Upload a uniform array `u_DynamicLights[8]` (position + radius, color + intensity) to `neoWorldVC.frag`, `neoVehicle.frag`, and `neoRimSkin.frag` (culling/sorting the 8 strongest lights affecting each draw call's bounding sphere, or global top 12 near camera).
  - Compute smooth inverse-square attenuation + Lambert/Blinn-Phong specular in the fragment shader.
  - Result: Police red/blue sirens flash realistically across building facades and car paint at night, muzzle flashes light up alleyways, and car headlights illuminate walls smoothly.

---

### 4. Wet-Road & Neon Reflections (`custompipes_gl.cpp`, `Weather.cpp`)
- Vice City already tracks road wetness via `CWeather::WetRoads` (`0.0..1.0` during/after rain) and already renders `Neo` vehicle reflections.
- **Road Material Detection & Planar/Env Pass**:
  - Tag asphalt/sidewalk road materials (or use surface normal `N.z > 0.9f` + road model flags in `neoWorldVC.frag`) modulated by `CWeather::WetRoads` (plus optional "Night Dampness" aesthetic setting).
  - Perturb road normal with a scrolling micro-ripple normal/procedural noise function and blend a mirrored sky/neon environment reflection + specular streaks from `u_DynamicLights` and `CCoronas` (streetlights, traffic lights, neon hotel signs).

---

### 5. Real-Time Directional Sun/Moon Shadow Mapping (`Shadows.cpp`, `ShadowCamera.cpp`)
- **Architecture**:
  - Render a `2048x2048` (or `2x 1536x1536` 2-cascade) orthographic depth map from the `CTimeCycle` sun/moon direction (`CCoronas` sun vector) covering `~80 m` around the player camera.
  - Include nearby buildings, palm trees, vehicles, and peds in the depth pass (skipping tiny props `< 0.5 m` and updating distant static cascade on alternating frames if needed on mobile).
  - Sample depth texture in `neoWorldVC.frag` and `neoVehicle.frag` with **4-tap / 9-tap Poisson PCF** soft filtering, fading smoothly into ambient lighting at dusk/dawn transitions so shadows never pop.
  - Automatically disable classic planar blob shadows for entities casting real shadow maps.

---

## Verification & Testing Criteria
- **Shader Parity & Fallbacks**:
  - Verify GLSL shaders compile cleanly on **Desktop OpenGL 3.3 Core** AND **Android OpenGL ES 3.0 / 3.2** (explicit `precision highp float;` guards on GLES, no desktop-only implicit casts) as well as HLSL SM3.0 on D3D9.
- **Mobile GPU Performance Budget**:
  - Target locked **60 FPS / 120 FPS** on Snapdragon 8 Gen 2/3 & Dimensity 9300 (OPPO Find X7) at `100%` render scale, with individual Graphics Menu toggles (`Bloom: Off/Low/High`, `Anti-Aliasing: Off/FXAA/MSAA`, `Dynamic Shadows: Classic/Medium/High`, `Wet Reflections: Off/On`).
