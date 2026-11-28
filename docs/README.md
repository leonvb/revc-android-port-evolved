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
| [00 — Phased Implementation Plan & Release Schedule](00-implementation-phases.md) | Rollout schedule, milestone breakdown, dependency graph, risk ratings, and CI/playtest acceptance gates; Phase 6 removed | All subsystems (`dev` → `miami` workflow) |
| [01 — High-FPS Physics & Driving Modernization](01-physics-and-driving.md) | 60/120 FPS frame-rate-independent physics fixes, progressive tire grip/drifting, weight transfer, transmission upgrades, mid-air vehicle control | `Physical.cpp`, `Automobile.cpp`, `Bike.cpp`, `Boat.cpp`, `Transmission.cpp`, `HandlingMgr.cpp` |
| [02 — Camera, Aiming & Combat Overhaul](02-camera-aiming-and-combat.md) | Over-the-shoulder 3rd-person ADS camera, move-while-aiming/shooting, hybrid target-assist + free-aim, modern vehicle camera, slow-mo weapon & radio wheels | `Cam.cpp`, `Camera.cpp`, `PlayerPed.cpp`, `Weapon.cpp`, `Hud.cpp`, `Pad.cpp` |
| [03 — Player Movement & Traversal](03-movement-and-traversal.md) | Surface swimming & water recovery, ledge vaulting/mantling, smooth analog locomotion & sprint model, high-speed vehicle bail-out | `PlayerPed.cpp`, `Ped.cpp`, `WaterLevel.cpp`, `AnimBlendAssociation.cpp` |
| [04 — Rendering and visuals](04-rendering-and-visuals.md) | Modern rendering work removed; existing renderer retained | — |
| [05 — Navigation, Checkpoints & Quality of Life](05-navigation-and-qol.md) | GTA IV/V-style 3D/minimap GPS route line (`ThePaths` A*), automatic post-mission saves, mid-mission stage checkpoints, faster interior/cutscene fades | `Radar.cpp`, `PathFind.cpp`, `PCSave.cpp`, `GameLogic.cpp`, `Script.cpp` |
| [06 — Android & Mobile UX Polish](06-android-and-mobile-ux.md) | Context-aware touch HUD transitions, rich `VibrationEffect` haptics via JNI, gyroscope ADS fine-aiming, analog triggers/throttle gestures, thermal/FPS pacing | `TouchControlsView.java`, `GameActivity.java`, `TouchControls.cpp`, `JavaWrapper.cpp`, `sdl2.cpp` |
| [07 — Fixed Controls, Camera, Autosave, Foot IK & Ragdolls](07-controls-camera-autosave-ragdoll-plan.md) | Current `modern`-based implementation plan for fixed modern gamepad controls, analog triggers, camera/aim, FPS settings, safe autosave, foot IK and ragdolls | Input backends, `Pad.cpp`, `Cam.cpp`, save/script systems, `PedIK.cpp`, ped physics |

---

## Recommended Implementation Phases

See **[00 — Phased Implementation Plan & Release Schedule](00-implementation-phases.md)** for earlier milestones, dependencies, risk levels, and acceptance gates. For the newer controller, FPS, autosave, camera, aim, foot IK and ragdoll requirements, use [Doc 07](07-controls-camera-autosave-ragdoll-plan.md), which supersedes conflicting earlier proposals for those topics and targets `modern`. Phase 6 was removed:

1. **Phase 0 — Foundation**: `config.h` + `reVC.ini` `[Modern]` settings, Pause Menu **"Modernization"** page, `CTimer` exponential time-step helpers, and Android `30 / 60 / 90 / 120 FPS` LTPO display pacing (`M0.1–M0.2`).
2. **Phase 1 — High-FPS Physics & Driving**: 60/120 FPS physics & boat drag parity fixes, Pacejka-lite progressive tire slip/drifting & weight transfer, and modern transmission + mid-air jump control (`M1.1–M1.3`, [Doc 01](01-physics-and-driving.md)).
3. **Phase 2 — Navigation & Quality of Life**: Non-intrusive A* GPS route line on minimap/Pause Map and dedicated non-destructive post-mission Autosave slot (`M2.1–M2.2`, [Doc 05](05-navigation-and-qol.md)).
4. **Phase 3 — Camera, Combat & Radial Wheels**: Modern vehicle camera, Over-the-Shoulder 3rd-person ADS + strafe-while-shooting, hybrid snap + free micro-aim assist, and slow-motion Weapon & Radio Wheels (`M3.1–M3.4`, [Doc 02](02-camera-aiming-and-combat.md)).
5. **Phase 4 — Android & Mobile UX Polish**: Linear X-axis `VibrationEffect` haptics, orientation-aware gyroscope ADS aiming, sub-context touch HUD transitions, analog throttle/brake slide, and 1-tap HUD shortcuts (`M4.1–M4.3`, [Doc 06](06-android-and-mobile-ux.md)).
6. **Phase 5 — Traversal Expansion**: Hold-to-sprint & smooth analog locomotion, surface swimming + shore/dock/boat climb-out, and two-ray physical ledge vaulting/mantling (`M5.1–M5.3`, [Doc 03](03-movement-and-traversal.md)).
7. **Phase 6 — Modern rendering**: Removed; see [Doc 04](04-rendering-and-visuals.md).
