# 03 — Player Movement & Traversal

## Overview & Goals
Two iconic limitations of 2002 Vice City make traversal feel dated compared to GTA San Andreas and modern open-world games:
1. **Instant Drowning in Water**: Vice City is an archipelago of islands, marinas, and beaches, yet touching deep water immediately triggers `PED_DEAD` / drowning, turning minor boat mishaps into mission failures.
2. **No Ledge Vaulting / Climbing**: Waist-high fences, dock edges, and low walls block the player unless a specific ramp is nearby.
3. **Stiff Sprint & Movement Transitions**: Sprinting requires rapid button-mashing (or abrupt binary stamina depletion), and falling out of fast-moving cars lacks smooth bail-out transitions.

This plan designs **Surface Swimming & Dock/Shore Recovery**, **Waist-to-Chest Ledge Mantling**, and **Modernized Analog Locomotion & Sprint Options** using procedural bone/physics blending so no external proprietary animation files are required (with optional custom `.ifp` animation support if present).

---

## Target Files in Codebase

| File | Role |
| :--- | :--- |
| `src/core/config.h` | Add `#define PLAYER_SWIMMING`, `#define LEDGE_MANTLING`, `#define MODERN_SPRINT_CONTROLS` |
| `src/peds/Ped.h` & `Ped.cpp` | Add `PED_SWIM` and `PED_CLIMB` states in `ePedState` (guarded at end of enum so vanilla enum indices never shift) |
| `src/peds/PlayerPed.cpp` | Water buoyancy/swimming controller, stamina drain in water, ledge detection raycasts, hold-to-sprint option |
| `src/renderer/WaterLevel.cpp` | Query exact wave height & surface normal (`CWaterLevel::GetWaterLevel`), water splash/wake particles |
| `src/animation/AnimManager.cpp` & `AnimBlendAssociation.cpp` | Synthesize/blend swim & climb poses from existing `ped.ifp` animations (`JUMP_GLIDE`, `FALL_GLIDE`, `CLIMB_JUMP` / `EV_DIVE`) + optional `modern_traversal.ifp` override |
| `src/core/Cam.cpp` | Replace `MODE_PLAYER_FALLEN_WATER` death-cam with normal `MODE_FOLLOWPED` elevated slightly above water surface when `PLAYER_SWIMMING` is active |

---

## Detailed Design & Implementation Steps

### 1. Surface Swimming & Water Climb-Out (`PLAYER_SWIMMING`)

#### A. Intercepting the Drowning Trigger (`Ped.cpp` / `PlayerPed.cpp`)
- Currently, when `CWaterLevel::GetWaterLevel(pos.x, pos.y, pos.z, &waterZ, false)` exceeds chest depth (`waterZ - pos.z > ~0.7f`), `CPed::ProcessBuoyancy()` transitions the player into `MODE_PLAYER_FALLEN_WATER` and drains health rapidly to zero.
- **Modern Swimming State (`PED_SWIM`)**:
  - When `bEnableSwimming` is on and `CPlayerPed` enters deep water:
    1. Holster active weapon temporarily (`WEAPONTYPE_UNARMED`).
    2. Keep `TheCamera` in `MODE_FOLLOWPED` (clamping minimum camera Z to `waterZ + 0.65f` so the camera never dips underwater where no underwater geometry exists).
    3. Pin player vertical position smoothly to wave height (`targetZ = waterZ - 0.55f`) using buoyant spring damping (`CPhysical::ApplyMoveForce`).

#### B. Swimming Locomotion & Stamina
- **Treading Water (Stick Neutral)**: Gentle bobbing synced to `CWaterLevel` sine waves, zero stamina drain, periodic water ripple (`PARTICLE_WATER_SPARK` / `WATER_HYDRANT` subtle ring).
- **Breaststroke / Freestyle Swim (Stick Active)**:
  - Move speed `~2.4 m/s` normal, `~3.8 m/s` sprint-swim (holding Sprint).
  - Sprint-swimming consumes `m_fCurrentStamina`; when stamina hits `0`, Tommy slows to normal swim speed (or optionally drains health slowly if exhausted in open ocean, configurable in `reVC.ini`).
- **Procedural / Blended Swim Pose (Zero Asset Dependency)**:
  - Without requiring external `.ifp` mods, pitch the root `RpClump` forward `~65°` while moving, blend `FALL_GLIDE` / `RUN_PLAYER` arm/leg cycles at `0.65x` speed, and keep head pitched above water via `CPedIK`.
  - If an optional `gamefiles/anim/swim.ifp` is bundled later, `CAnimManager` automatically loads true breaststroke/tread animations.

#### C. Boat Boarding & Dock/Shore Climb-Out
- **Shore**: As ground `z` rises (`waterZ - groundZ < 0.65f`), smoothly transition back to `PED_IDLE` / `PED_WALK`.
- **Docks, Seawalls & Boats**:
  - Pressing **Jump (`Square`)** near a dock edge (`height above water <= 1.8m`) or pressing **Enter Vehicle (`Triangle`)** beside an empty boat (`CBoat`) triggers a mantle/board transition onto the deck or dock, making fallen boat missions effortless to recover from.

---

### 2. Ledge Vaulting & Waist-High Wall Mantling (`LEDGE_MANTLING`)

*(Note: `reVC` already contains a basic `WALLCLIMB_CHEAT` reference in `config.h`; this replaces cheat-like upward teleportation with grounded physical detection.)*

#### A. Two-Ray Forward Ledge Probe (`PlayerPed.cpp`)
When the player presses **Jump (`Square`)** while moving toward an obstacle (or mid-jump against a wall):
1. **Chest/Waist Ray**: Horizontal `CWorld::ProcessLineOfSight` forward `1.0 m` from `pos + (0, 0, 0.3f)` to detect a vertical wall face.
2. **Head/Clearance Ray**: Horizontal ray at `pos + (0, 0, 1.9f)` to confirm open space above the wall, followed by a downward vertical ray from `wallPoint + forward * 0.45f + (0, 0, 1.9f)` to find exact `ledgeTopZ`.

#### B. Two Height Tiers
- **Low Vault (`0.6 m <= ledgeHeight <= 1.25 m`)** — fences, highway dividers, hoods:
  - Smooth `350 ms` parabolic arc translating `CPlayerPed` over/onto `ledgeTopZ + 1.0f` using `JUMP_LAUNCH` / `JUMP_GLIDE` while temporarily disabling capsule horizontal collision with the vaulted prop.
- **High Mantle (`1.25 m < ledgeHeight <= 2.15 m`)** — dock walls, shipping containers, low roofs:
  - Snap hands to ledge edge (`600 ms` pull-up curve lifting Z to `ledgeTopZ`, then stepping forward `0.5 m` onto solid ground).

---

### 3. Modernized Analog Locomotion & Sprint (`MODERN_SPRINT_CONTROLS`)
- **Hold-to-Sprint (No Tap-Mashing)**:
  - Option to make **Hold Cross / Shift** sprint continuously at full speed (instead of PS2 tap-mashing), with smooth stamina bar regeneration.
- **Smooth Turn-Radius & Momentum**:
  - Replace instant 180° velocity reversal when flicking the left stick with a brief `120 ms` pivot deceleration so Tommy's feet feel planted on the ground without feeling sluggish.
- **High-Speed Vehicle Bail-Out Polish**:
  - Vice City already lets Tommy dive out of moving cars above a speed threshold (`EV_DIVE`), taking minor roll damage. Smooth camera tracking during the roll and allow quicker recovery to aiming (`Aim` input cancels the ground-lay delay slightly earlier).

---

## Verification & Testing Criteria
- Test falling into water at Ocean Beach Marina, Leaf Links bridges, Viceport docks, and Cortez's Yacht; verify player can swim back to shore, mantle onto a `1.5 m` dock wall, or board a nearby boat without getting stuck inside pier collision meshes.
- Verify missions that intentionally kill NPCs in water only enable swimming for `FindPlayerPed()` (unless NPC swimming is explicitly desired), preserving scripted mission kills.
- Test mantling around gated mission areas (e.g., Starfish Island bridges before unlock, Fort Baxter) to ensure `2.15 m` max mantle height does not accidentally bypass high security walls.
