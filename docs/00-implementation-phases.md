# 00 — Phased Implementation Plan & Release Schedule

## Purpose of This Document
While documents `01` through `06` detail the technical architecture of each subsystem, this document defines **how and in what order** those improvements will be built, tested, and merged on `dev` (and promoted to `miami`).

Each phase is broken into **small, self-contained Pull Requests (Milestones)** so that:
1. All 5 GitHub Actions CI workflows (`Android`, `Switch`, `MSVC amd64`, `MSVC x86`, `Conan+CMake`) stay green at every step.
2. Every milestone produces a **playable APK** that can be tested immediately on hardware (e.g., OPPO Find X7) before moving to the next milestone.
3. No single PR mixes risky subsystems (e.g., physics math is never mixed with GLSL shader changes or save-game serialization).

---

## High-Level Phase Dependency Graph

```text
Phase 0: Foundation (Settings, Toggles, TimeStep Math & 120 Hz Frame Pacing)
   │
   ├──► Phase 1: High-FPS Physics Fixes & Modern Driving Dynamics (Doc 01)
   │
   ├──► Phase 2: Navigation, Autosave & Quality of Life (Doc 05)
   │
   ├──► Phase 3: Camera, Over-the-Shoulder Combat & Radial Wheels (Doc 02)
   │       │
   │       └──► Phase 4: Android UX, Haptics, Gyro Aiming & Touch Polish (Doc 06)
   │
   ├──► Phase 5: Player Traversal — Swimming, Mantling & Locomotion (Doc 03)
   │
   └──► Phase 6: Modern rendering (removed; see Doc 04)
```

*(Note: Phases 1, 2, and early parts of Phase 4/6 are loosely coupled and can be reordered if priorities shift, whereas **Phase 0** is a prerequisite for everything, and **Phase 4's** gyro/ADS touch buttons build naturally on **Phase 3's** Over-the-Shoulder aiming.)*

---

## Phase 0 — Foundation: Settings Infrastructure, Time-Step Helpers & Frame Pacing
**Goal:** Build the shared scaffolding once so every subsequent feature has a clean home in `config.h`, `reVC.ini`, the Pause Menu, and frame-rate-independent math.

| Milestone / PR | Scope & Files | Deliverable |
| :--- | :--- | :--- |
| **M0.1 — Modernization Config & Menu Page** | `src/core/config.h`, `src/core/IniFile.cpp`, `src/core/MenuScreensCustom.cpp`, `src/text/Text.cpp` (fallback strings) | New **"Modernization"** submenu in Pause → Options with persistent `reVC.ini` `[Modern]` section and master preset (`Classic` vs. `Modern`). Zero behavior changes when set to `Classic`. |
| **M0.2 — Frame-Rate-Independent Math & Display Pacing** | `src/core/Timer.h`, `src/core/TimeStep.h`, `src/skel/sdl2/sdl2.cpp`, `GameActivity.java` | Standardized `CTimer::GetTimeStepRatio30()`, `PowTimeStep()`, and `ExpSmooth()` helpers + selectable **`30 / 60 / 90 / 120 FPS`** frame cap wired to Android `Surface.setFrameRate` / LTPO display mode. |

- **Risk Level:** Very Low.
- **Exit / Acceptance Gate:**
  - `Vanilla` x86 build compiles untouched (`#ifndef VANILLA_DEFINES` verified).
  - Toggling FPS cap (`30 / 60 / 90 / 120`) on Android locks frame pacing cleanly and persists across app restarts in `reVC.ini`.

---

## Phase 1 — High-FPS Physics Parity & Modern Driving (`Doc 01`)
**Goal:** Make 60–120 FPS physics match 30 FPS behavior accurately first, then layer optional modern vehicle physics on top.

| Milestone / PR | Scope & Files | Deliverable |
| :--- | :--- | :--- |
| **M1.1 — Core 60/120 FPS Physics & Boat Drag Fixes** | `src/entities/Physical.cpp`, `src/vehicles/Automobile.cpp`, `src/vehicles/Bike.cpp`, `src/vehicles/Boat.cpp`, `src/vehicles/Floater.cpp` | Fixes high-FPS off-throttle engine braking, stiff suspension damping, boat water-drag speed loss, angular spin decay, and particle/skidmark emission rates. |
| **M1.2 — Progressive Tire Grip, Weight Transfer & Drifting** | `src/vehicles/Automobile.cpp`, `src/vehicles/HandlingMgr.cpp` | Optional **Driving Style: Modern** toggle introducing Pacejka-lite slip angles, controllable powerslides, counter-steer self-aligning feel, and braking/acceleration load transfer. |
| **M1.3 — Modern Transmission & Mid-Air Vehicle Control** | `src/vehicles/Transmission.cpp`, `src/vehicles/Automobile.cpp`, `src/vehicles/Bike.cpp` | Per-vehicle transient RPM/shift simulation, smooth aero top-speed curve, optional sequential shift binding, and GTA V-style mid-air pitch/roll leveling on jumps. |

- **Risk Level:** Low–Medium (must not alter AI `AutoPilot` pathing or break timed races).
- **Exit / Acceptance Gate:**
  - 0→top-speed time, coasting distance, and `SQUALO` boat top speed at **120 FPS** match **30 FPS** within ±2%.
  - Missions *The Driver*, *The Fastest Boat*, *PCJ Playground*, and Unique Stunt Jumps tested and verified completable.
  - Existing save files load with zero struct size changes (`VALIDATE_SAVE_SIZE` passes).

---

## Phase 2 — Navigation, Autosave & Quality of Life (`Doc 05`)
**Goal:** Eliminate everyday open-world friction (pausing constantly to check the map, losing progress after tough missions) with isolated, high-value features.

| Milestone / PR | Scope & Files | Deliverable |
| :--- | :--- | :--- |
| **M2.1 — Minimap & Pause-Map A* GPS Route Line** | `src/control/PathFind.cpp`, `src/core/Radar.cpp` | Real-time A* pathfinding over `ThePaths` road graph (using isolated scratch arrays so NPC traffic AI is untouched). Draws a clipped purple route to user waypoints and yellow route to active mission blips on both the circular HUD radar and Pause Map, respecting locked story bridges. |
| **M2.2 — Dedicated Post-Mission Autosave & Retry Polish** | `src/save/PCSave.cpp`, `src/save/GenericGameStorage.cpp`, `src/control/Script.cpp`, `src/renderer/Hud.cpp`, `src/core/Camera.cpp` | Dedicated non-destructive **Autosave Slot** triggered 2 seconds after mission pass / property purchase (never overwriting manual slots 1–8), Pause Menu **"Restart Mission"** option, pre-mission armor/ammo restore option, and optional fast interior doorway fades. |

- **Risk Level:** Low–Medium (Autosave must strictly guard against saving while `IsPlayerOnAMission()` is true).
- **Exit / Acceptance Gate:**
  - GPS route updates in `< 0.2 ms`, clears automatically upon arrival, hides in boats/aircraft/interiors, and never routes across locked bridges before *Phnom Penh '86*.
  - Autosave tested across 5+ consecutive story missions + phone calls; reloading Autosave resumes script progression cleanly.

---

## Phase 3 — Camera, Over-the-Shoulder Combat & Radial Wheels (`Doc 02`)
**Goal:** Transform how Vice City plays on foot and in vehicles by replacing stationary 2002 aiming and slot-cycling with modern 3rd-person controls.

| Milestone / PR | Scope & Files | Deliverable |
| :--- | :--- | :--- |
| **M3.1 — Modern Vehicle Camera** | `src/core/Cam.cpp`, `src/core/Camera.h` | Configurable manual-orbit **auto-center delay** (`1.75s` default so camera stops fighting right-stick/touch), speed-based dynamic FOV (`70° → 80°`), hill crest pitch look-ahead, and smooth reverse camera. |
| **M3.2 — Over-the-Shoulder (OTS) 3rd-Person Aiming & Strafe** | `src/core/Cam.cpp`, `src/peds/PlayerPed.cpp`, `src/peds/PedIK.cpp`, `src/weapons/Weapon.cpp`, `src/renderer/Hud.cpp` | Smooth OTS ADS camera (`MODE_OTS_AIMING`) with left/right shoulder swap, full movement/strafing/crouching while aiming rifles & firearms, two-stage parallax-free muzzle raycasting, and dynamic spread reticle. |
| **M3.3 — Hybrid Target-Assist (`Snap + Free Micro-Aim`)** | `src/peds/PlayerPed.cpp`, `src/core/Cam.cpp`, `src/core/Pad.cpp` | Selectable `Classic` / `Modern Free-Aim` / `Modern Assisted` modes: soft ADS snap within `12°` cone + unrestricted stick/touch micro-aim for headshots or shooting tires + reticle friction. |
| **M3.4 — Slow-Motion Weapon Wheel & Radio Wheel** | `src/renderer/Hud.cpp`, `src/core/Pad.cpp`, `src/audio/DMAudio.cpp` | Tap to quick-cycle; hold (`>= 180 ms`) to open an 8-slot radial **Weapon Wheel** or 10-station **Radio Wheel** (with Vice City station icons) and smooth `0.25x` time-dilation. |

- **Risk Level:** Medium–High (`Cam.cpp` and `PlayerPed.cpp` have many weapon-specific state branches).
- **Exit / Acceptance Gate:**
  - Every weapon slot (Melee, Pistol, Shotgun, SMG, Assault Rifle, Heavy, Thrown, Sniper) tested standing, strafing, crouching, in tight interiors, and in vehicle drive-bys.
  - Scripted cutscene cameras, RC vehicles, and helicopter rail-shooter missions (`MODE_HELICANNON_1STPERSON`) verified unaffected.

---

## Phase 4 — Android & Mobile UX Polish (`Doc 06`)
**Goal:** Pair the modernized camera, driving, and weapon systems with flagship mobile controls, tactile haptics, and gyroscope precision.

| Milestone / PR | Scope & Files | Deliverable |
| :--- | :--- | :--- |
| **M4.1 — Linear X-Axis Haptics (`VibrationEffect`)** | `GameActivity.java`, `JavaWrapper.cpp`, `src/core/Pad.cpp` | Wires `CPad::StartShake` + weapon fire / gear shifts / UI ticks to Android `VibrationEffect` with `25 ms` JNI rate-limiting and `Off / Subtle / Full` intensity setting. |
| **M4.2 — Gyroscope ADS Aiming** | `GameActivity.java`, `TouchControls.cpp`, `src/skel/sdl2/sdl2.cpp`, `src/core/Cam.cpp` | Orientation-aware (`ROTATION_90` / `ROTATION_270`) `TYPE_GYROSCOPE` input active in `ADS Only` or `Always` mode with sensitivity slider. |
| **M4.3 — Sub-Context Touch HUD, Analog Pedals & Tap Shortcuts** | `TouchControlsView.java`, `TouchControls.cpp`, `src/core/Pad.cpp` | Smooth `150 ms` alpha transitions across granular contexts (Unarmed, Armed, OTS Aiming + Shoulder Swap/Crouch, Car, Bike, Heli/Plane, Swimming), vertical-slide analog gas/brake modulation, 1-tap Minimap → Map shortcut, and hold Weapon HUD → Weapon Wheel. |

- **Risk Level:** Low (isolated to `#ifdef ANDROID` and `android/launcher/`).
- **Exit / Acceptance Gate:**
  - Tested on Android 14+ (OPPO Find X7): zero frame-time spikes during rapid gunfire haptics, accurate gyro axes in both landscape orientations, and seamless auto-hide when a Bluetooth controller connects.

---

## Phase 5 — Player Movement & Traversal (`Doc 03`)
**Goal:** Remove Vice City's two harshest 2002 traversal barriers — instant water drowning and waist-high wall blocks — without breaking level gating.

| Milestone / PR | Scope & Files | Deliverable |
| :--- | :--- | :--- |
| **M5.1 — Hold-to-Sprint & Smooth Analog Locomotion** | `src/peds/PlayerPed.cpp`, `src/core/Pad.cpp` | Hold-to-sprint option (no tap-mashing), smooth stick-reversal pivot momentum, and faster recovery from high-speed car bail-outs (`EV_DIVE`). |
| **M5.2 — Surface Swimming & Shore/Dock/Boat Recovery** | `src/peds/Ped.h`, `src/peds/Ped.cpp`, `src/peds/PlayerPed.cpp`, `src/renderer/WaterLevel.cpp`, `src/core/Cam.cpp` | `PED_SWIM` state for the player: wave-locked buoyancy,procedural/blended breaststroke & treading water, stamina management, camera clamped above water, shore walk-out, dock pull-up (`<= 1.8 m`), and direct boat boarding from water. |
| **M5.3 — Physical Ledge Vaulting & Mantling** | `src/peds/PlayerPed.cpp`, `src/animation/AnimManager.cpp` | Two-ray forward + overhead clearance check enabling smooth **Low Vaults** (`0.6–1.25 m` fences/dividers) and **High Mantles** (`1.25–2.15 m` dock walls/containers). |

- **Risk Level:** Medium–High (collision edge cases near piers/bridges and animation blending).
- **Exit / Acceptance Gate:**
  - Player can fall off any boat/pier in Ocean Beach, Viceport, or Leaf Links, swim back, climb onto a dock or re-board a boat cleanly.
  - NPCs retain standard water behavior so missions requiring enemies to drown in water never soft-lock.
  - `2.15 m` mantle cap verified unable to bypass locked island bridge barricades early in the game.

---

## Phase 6 — Modern rendering removed

The Modern rendering work was removed at the project's request. The existing
renderer and ordinary display settings remain; see [Doc 04](04-rendering-and-visuals.md).

---

## Branching, CI & Release Workflow Per Phase

1. **Feature PRs → `dev`**:
   - Each milestone (`M0.1`, `M0.2`, `M1.1`, …) is developed on a working branch and merged into `dev` only after all 5 GitHub Actions workflows pass (`reVC Android`, `Nintendo Switch`, `MSVC amd64`, `MSVC x86`, `Conan+CMake`).
2. **Playtest Checkpoints on `dev`**:
   - Download the `revc-debug` / `revc-release` APK artifact from the `dev` workflow run and playtest on device at the end of each phase.
3. **Phase Promotion (`dev` → `miami`)**:
   - Once a full phase passes on-device playtesting on `dev`, merge `dev` into `miami` so `miami` always represents a stable, playtested release milestone.

---

## Summary Effort & Impact Matrix

| Phase | Milestones | Relative Complexity | Gameplay / Visual Impact | Dependencies |
| :--- | :---: | :---: | :---: | :--- |
| **Phase 0 — Foundation & FPS Pacing** | 2 PRs (`M0.1–M0.2`) | Small | Medium (120 Hz pacing + menu) | None |
| **Phase 1 — High-FPS Physics & Driving** | 3 PRs (`M1.1–M1.3`) | Medium | **Very High** (fixes 120 Hz + driving feel) | Phase 0 |
| **Phase 2 — GPS Route & Autosave QoL** | 2 PRs (`M2.1–M2.2`) | Small–Medium | **Very High** (modern open-world UX) | Phase 0 |
| **Phase 3 — OTS Combat, Camera & Wheels** | 4 PRs (`M3.1–M3.4`) | Large | **Transformative** (modernizes core gameplay) | Phase 0 |
| **Phase 4 — Android Haptics, Gyro & Touch** | 3 PRs (`M4.1–M4.3`) | Medium | **High** (flagship mobile feel) | Phase 0, Phase 3 |
| **Phase 5 — Swimming & Ledge Mantling** | 3 PRs (`M5.1–M5.3`) | Medium–Large | **High** (removes 2002 frustrations) | Phase 0 |
