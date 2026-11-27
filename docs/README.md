# reVC Modernization Roadmap & Implementation Plan

This folder contains the architectural design and phased engineering plan for modernizing **reVC** (Grand Theft Auto: Vice City) across Android (e.g., 120 Hz OLED flagships like OPPO Find X7) and desktop/console platforms (Windows, Linux, macOS, Nintendo Switch) without breaking compatibility with existing Vice City assets, mission scripts (`main.scm`), or save files.

---

## Core Architectural Principles

1. **Opt-In Compile-Time & Runtime Toggles**
   - Every modernization feature must be guarded behind a preprocessor macro in `src/core/config.h` (disabled under `VANILLA_DEFINES` so `Vanilla` builds remain faithful to original `gta-vc.exe`).
   - Every gameplay-altering or GPU-intensive feature must also expose a runtime setting in `reVC.ini` (`src/core/IniFile.cpp`) and in the Custom Frontend Options menu (`src/core/MenuScreensCustom.cpp` via `src/extras/frontendoption.h`), allowing players to switch between **Classic** and **Modern** behavior at any time without rebuilding.

2. **Frame-Rate Independence First (`CTimer::GetTimeStep()`)**
   - Original Vice City normalized time around **30 FPS** (`1.0f` time step = `1/50s` internal tick scaled so 30 FPS ≈ `1.667` time step units).
   - Modern displays run at **60, 90, 120, or 144+ Hz**. All new physics forces, damping curves, camera interpolations, UI animations, and input smoothing must use exponential decay (`powf(factor, CTimer::GetTimeStep())` or `1.0f - expf(-rate * dt)`) rather than raw per-frame multiplication.

3. **Save-File & Script Compatibility**
   - Keep `COMPATIBLE_SAVES` (`src/save/PCSave.cpp`, `GenericGameStorage.cpp`) intact. Avoid changing serialized struct layouts unless gated by a new save chunk or transient runtime state.
   - Ensure mission critical scripts (`src/control/Script*.cpp`) remain completable. Where modern physics or traversal could break tight mission assumptions (e.g., scripted boat chases, stunt jumps, RC missions), provide automatic fallback or tuned parameters.

4. **Cross-Platform Shader Parity (`librw`)**
   - Rendering enhancements in `src/extras/` and `vendor/librw/` must support OpenGL 3.3+ (Desktop), OpenGL ES 3.0+ (Android / Switch), and Direct3D 9 (Windows), with graceful fallbacks on tile-based mobile GPUs (Mali / Immortalis / Adreno).

---

## Documentation Index

| Document | Scope | Key Engine Subsystems |
| :--- | :--- | :--- |
| [01 — High-FPS Physics & Driving Modernization](01-physics-and-driving.md) | 60/120 FPS frame-rate-independent physics fixes, progressive tire grip/drifting, weight transfer, transmission upgrades, mid-air vehicle control | `Physical.cpp`, `Automobile.cpp`, `Bike.cpp`, `Boat.cpp`, `Transmission.cpp`, `HandlingMgr.cpp` |
| [02 — Camera, Aiming & Combat Overhaul](02-camera-aiming-and-combat.md) | Over-the-shoulder 3rd-person ADS camera, move-while-aiming/shooting, hybrid target-assist + free-aim, modern vehicle camera, slow-mo weapon & radio wheels | `Cam.cpp`, `Camera.cpp`, `PlayerPed.cpp`, `Weapon.cpp`, `Hud.cpp`, `Pad.cpp` |
| [03 — Player Movement & Traversal](03-movement-and-traversal.md) | Surface swimming & water recovery, ledge vaulting/mantling, smooth analog locomotion & sprint model, high-speed vehicle bail-out | `PlayerPed.cpp`, `Ped.cpp`, `WaterLevel.cpp`, `AnimBlendAssociation.cpp` |
| [04 — Rendering, Lighting & World Visuals](04-rendering-and-visuals.md) | Dynamic shadow mapping, neon HDR bloom, wet-road & screen-space reflections, FXAA/SMAA, dynamic point lights, extended draw distance & zero pop-in | `postfx.cpp`, `custompipes*.cpp`, `Shadows.cpp`, `PointLights.cpp`, `Streaming.cpp`, `src/extras/shaders/` |
| [05 — Navigation, Checkpoints & Quality of Life](05-navigation-and-qol.md) | GTA IV/V-style 3D/minimap GPS route line (`ThePaths` A*), automatic post-mission saves, mid-mission stage checkpoints, faster interior/cutscene fades | `Radar.cpp`, `PathFind.cpp`, `PCSave.cpp`, `GameLogic.cpp`, `Script.cpp` |
| [06 — Android & Mobile UX Polish](06-android-and-mobile-ux.md) | Context-aware touch HUD transitions, rich `VibrationEffect` haptics via JNI, gyroscope ADS fine-aiming, analog triggers/throttle gestures, thermal/FPS pacing | `TouchControlsView.java`, `GameActivity.java`, `TouchControls.cpp`, `JavaWrapper.cpp`, `sdl2.cpp` |

---

## Recommended Implementation Phases

To minimize regression risk and deliver playable value incrementally on `dev`, implement in the following order:

### Phase 1 — Foundation & High-FPS Physics (Low Risk, Immediate Feel)
1. Centralize modern feature toggles (`MODERN_PHYSICS`, `MODERN_CAMERA`, `GPS_ROUTE`, etc.) in `src/core/config.h`, `IniFile.cpp`, and a dedicated **"Modernization"** submenu in `MenuScreensCustom.cpp`.
2. Implement **60/120 FPS time-step fixes** in `CPhysical`, `CAutomobile`, `CBike`, and `CBoat` ([Doc 01](01-physics-and-driving.md)).
3. Add **progressive driving dynamics & mid-air control** ([Doc 01](01-physics-and-driving.md)).

### Phase 2 — Navigation & Quality of Life (High Utility, Self-Contained)
1. Implement **A* GPS route pathfinding & radar polyline rendering** using `ThePaths` ([Doc 05](05-navigation-and-qol.md)).
2. Add **Autosave Slot** after mission completion and property purchases ([Doc 05](05-navigation-and-qol.md)).

### Phase 3 — Camera, Combat & Mobile Controls (Transformative Gameplay)
1. Implement **Vehicle Camera auto-center delay & speed FOV** ([Doc 02](02-camera-aiming-and-combat.md)).
2. Implement **Over-the-Shoulder 3rd-person free-aim + strafe-while-shooting** for rifles/heavy weapons and optional pistols/SMGs ([Doc 02](02-camera-aiming-and-combat.md)).
3. Add **Weapon Wheel & Radio Wheel** with time-dilation ([Doc 02](02-camera-aiming-and-combat.md)).
4. Pair with **Android Gyro Aiming, Haptics & Context Touch Polish** ([Doc 06](06-android-and-mobile-ux.md)).

### Phase 4 — Traversal Expansion (Medium-High Complexity, Animation-Bound)
1. Implement **Water Treading / Surface Swimming & Ledge Climb-Out** so falling in water is no longer an instant "Wasted" ([Doc 03](03-movement-and-traversal.md)).
2. Refine **Ledge Vaulting / Waist-High Wall Mantling** ([Doc 03](03-movement-and-traversal.md)).

### Phase 5 — Modern Graphics & Shader Pipeline (GPU-Focused)
1. Expand streaming budgets & LOD distances (`Streaming.cpp`, `Renderer.cpp`) ([Doc 04](04-rendering-and-visuals.md)).
2. Add **HDR Neon Bloom + FXAA** post-processing pass in `CPostFX` ([Doc 04](04-rendering-and-visuals.md)).
3. Add **Wet-Road Planar/Screen-Space Reflections** and **Dynamic Per-Pixel Point Lights** ([Doc 04](04-rendering-and-visuals.md)).
4. Implement **Cascaded / Directional Shadow Mapping** ([Doc 04](04-rendering-and-visuals.md)).
