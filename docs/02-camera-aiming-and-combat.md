# 02 — Camera, Aiming & Combat Overhaul

## Implementation status

**Phase 3 implementation is in progress and requires gameplay verification.**
Modern road-vehicle follow now supports manual-orbit delay, speed/reverse FOV,
slope look-ahead and reverse swing. Supported on-foot firearms have OTS framing,
D-pad-right shoulder swap, existing strafe/aim animation integration, a dynamic
reticle, and camera-to-world/muzzle-to-world rays. Modern Assisted adds a short
snap on aim and right-stick flick target selection; both modern modes have
controller/touch reticle friction over visible hostile peds. The weapon and radio
wheels use tap-to-cycle, hold-to-open and right-stick/touch selection, and give
up time-scale ownership if another system changes it. Police/taxi radio,
cutscenes, replays, scoped/special weapons and scripted cameras keep their
existing paths. These integrations have not been tested in actual gameplay or on
Android. The OTS route covers unscoped firearms including the minigun and flamethrower;
the weapon wheel groups eight slots and leaves utility
slots accessible through quick cycling. Flamethrower area-effect propagation
still uses the original engine's collision behavior after the muzzle check.
Treat the remaining acceptance criteria,
including all weapons, crouch/strafe, interiors and special camera transitions,
as unverified rather than complete.

## Overview & Goals
Vice City's on-foot gunplay and camera feel dated primarily because:
1. **Rifles & Heavy Weapons (`M4`, `Ruger`, `M60`, `Rocket`, `Sniper`)** force a stationary 1st-person mode where Tommy cannot walk or strafe (`MODE_M16_1STPERSON` / `PlayerControlM16`).
2. **Pistols, Shotguns & SMGs** rely either on rigid PS2 lock-on (`MODE_SYPHON`) or hip-fire run-and-gun tied to character facing rather than camera direction.
3. **Vehicle Camera (`MODE_BEHINDCAR` / `MODE_CAM_ON_A_STRING`)** snaps back aggressively against manual right-stick / touch panning and lacks speed-sensitive framing.
4. **Weapon & Radio Switching** requires cycling sequentially through every slot.

This plan introduces a **GTA San Andreas / GTA IV / GTA V-style Over-the-Shoulder (OTS) 3rd-person combat system**, **Hybrid Aim-Assist**, a **Modern Vehicle Camera**, and **Slow-Motion Radial Weapon & Radio Wheels**.

---

## Target Files in Codebase

| File | Role |
| :--- | :--- |
| `src/core/config.h` | Add `#define MODERN_OTS_AIMING`, `#define MODERN_VEHICLE_CAMERA`, `#define WEAPON_RADIO_WHEELS` |
| `src/core/Camera.h` & `Cam.cpp` | Add `CCam::MODE_OTS_AIMING` (`Process_OverTheShoulder`), vehicle camera auto-center timer, dynamic FOV & pitch look-ahead |
| `src/peds/PlayerPed.cpp` & `.h` | Strafe locomotion while aiming, upper-body torso pitch/yaw IK (`CPedIK`), hybrid soft target-assist |
| `src/peds/PedIK.cpp` & `.h` | Extend `PointGunInDirection` pitch clamping and smooth interpolation for OTS camera |
| `src/weapons/Weapon.cpp` & `WeaponInfo.cpp` | Raycast crosshair-to-world target point so bullets travel toward the center screen reticle without parallax |
| `src/renderer/Hud.cpp` & `.h` | Dynamic bloom/spread crosshair reticle, hit-marker option, radial Weapon Wheel & Radio Wheel rendering |
| `src/core/Pad.cpp` & `.h` | Hold-vs-tap weapon/radio wheel input detection, smooth right-stick response curves (deadzone + power curve) |
| `src/core/MenuScreensCustom.cpp` & `IniFile.cpp` | Aim Mode (`Classic`, `Modern Free-Aim`, `Modern Assisted`), Shoulder Swap, Auto-Center Delay, Reticle Style |

---

## Detailed Design & Implementation Steps

### 1. Over-the-Shoulder (OTS) Third-Person Aiming (`Cam.cpp`, `PlayerPed.cpp`, `Weapon.cpp`)

#### A. Camera Mode (`CCam::Process_OverTheShoulder`)
- Triggered when holding **Aim (`R1` / Right Mouse / Touch Aim)** with any firearm (except scoped Sniper/Rocket Launcher, where Sniper retains zoom scope or optional toggle).
- **Framing & Offset**:
  - Anchor relative to `CPlayerPed` pelvis/clavicle bone: default offset `(+0.55f, -1.85f, +0.55f)` in camera-local space (configurable left/right **Shoulder Swap** via `L3` or `D-Pad` tap while aiming).
  - Smoothly interpolate FOV from `70°` (exploration) → `52°` (ADS) over `~140 ms` using `CTimer::ExpSmooth`.
  - **Wall Collision Avoidance**: Reuse `CWorld::ProcessLineOfSight` sphere/ray cast from player spine to desired camera position so aiming near walls pulls the camera inward cleanly without clipping inside geometry.

#### B. Move & Strafe While Aiming (`CPlayerPed::PlayerControlModernAim`)
- Replace stationary `PlayerControlM16` when `Modern Aiming` is enabled:
  - Lock `CPlayerPed::m_fRotationDest` (character yaw) to match `TheCamera.Orientation` so Tommy always faces downrange.
  - Drive lower-body movement velocity from Left Stick `(leftX, leftY)` projected onto camera forward/right vectors, blending existing run/walk/strafe animations (or directional blend via `CAnimManager`) while keeping upper-body weapon aim active via `CPedIK::PointGunInDirection`.
  - Allow **crouch-aiming** (`bIsDucking`) for tighter weapon spread.

#### C. Parallax-Free Crosshair Raycasting (`Weapon.cpp`)
- Because an over-the-shoulder camera is offset ~0.55 m to the right of Tommy's gun barrel, firing straight along camera forward would miss close targets.
- **Two-Stage Aim Ray**:
  1. Cast ray from `CameraPos` along `CameraForward` up to weapon range (`WeaponInfo->m_fRange`) to find `WorldHitPoint` (ignoring the player entity).
  2. Aim weapon muzzle vector from `MuzzleWorldPos` → `WorldHitPoint` (clamped if an obstacle is directly between Tommy's chest and `WorldHitPoint` so bullets never shoot backward).

---

### 2. Hybrid Target-Assist for Gamepads & Touch (`PlayerPed.cpp`, `Cam.cpp`)
Pure PC mouse free-aim is hard on touchscreens, while classic PS2 hard lock-on prevents headshots or shooting vehicle tires.

**Three User-Selectable Aim Modes:**
1. **Classic**: Original Vice City `MODE_SYPHON` / `MODE_M16_1STPERSON`.
2. **Modern Free-Aim**: Pure OTS reticle with subtle **reticle friction** (slows camera sensitivity by 35% when the crosshair passes over a hostile `CPed`).
3. **Modern Assisted (Default on Android & Gamepad)**:
   - **Snap-on-ADS**: Pressing Aim within a `12°` cone of a nearby threat gently slews the OTS camera onto the target's center-mass over `100 ms`.
   - **Free Micro-Adjustment**: Moving the right stick or touch look-pad immediately offsets aim from chest → head or knees without breaking OTS mode, plus flicking the stick left/right switches to neighboring targets.

---

### 3. Modern Vehicle Camera (`Cam.cpp` — `Process_BehindCar_Modern`)
- **Manual Look Auto-Center Delay**:
  - Currently, releasing or holding manual camera orbit fights the spring camera immediately.
  - Add `m_fManualCamHoldTimer`: any right-stick or touch camera drag resets a configurable timer (default **`1.75 seconds`**). Only after the timer expires does the camera smoothly ease back behind the vehicle's velocity vector.
- **Velocity-Based Dynamic Framing & FOV**:
  - Subtle FOV expansion from `70°` at rest → `80°` at top speed, paired with slight camera height/pitch lift (`+4°` look-ahead over crests and downhill slopes using vehicle pitch velocity) so hills don't blind the player to oncoming traffic.
- **Reverse Camera**:
  - Smoothly swing or widen rear view when reversing (`m_vecMoveSpeed` dot `GetForward() < -0.05f`) without jarring instant 180° snaps.

---

### 4. Slow-Motion Radial Weapon Wheel & Radio Wheel (`Hud.cpp`, `Pad.cpp`, `DMAudio.cpp`)

#### Interaction Flow
- **Tap** Weapon/Radio button (`< 180 ms`): Quick-cycle to next/previous weapon or station (classic behavior preserved).
- **Hold** Weapon (`L2`/`R2` or Touch Weapon Icon) or Radio (`L1` or Touch Radio Icon) (`>= 180 ms`):
  - Opens a clean 8-sector **Weapon Wheel** (grouped by Vice City weapon slots: Melee, Pistols, Shotguns, SMGs, Assault Rifles, Heavy, Thrown, Sniper) or 10-sector **Radio Wheel** (displaying existing `CRadar::Radio*Sprite` station logos!).
  - Applies smooth time-dilation (`CTimer::ms_fTimeScale` eased to `0.25f`, restoring to `1.0f` on release) — disabled during active cutscenes or replays.
  - Right Stick / Touch drag highlights a sector with ammo count and weapon silhouette; releasing equips immediately.

---

## Verification & Testing Criteria
- Test every weapon class (`WEAPONTYPE_PISTOL`, `UZI`, `STUBBY_SHOTGUN`, `M4`, `RUGER`, `SNIPERRIFLE`, `ROCKETLAUNCHER`, `MINIGUN`, `GRENADE`) on foot, crouched, and near tight interior walls (Malibu Club, Ocean View Hotel, Print Works).
- Confirm drive-by shooting (`Look Left` / `Look Right` + Fire, or optional free-aim drive-by) still works cleanly in cars, boats, and bikes.
- Verify scripted camera modes (`MODE_FIXED`, `MODE_FLYBY`, cutscenes, RC vehicles, helicopter cannon `MODE_HELICANNON_1STPERSON`) are completely unaffected.
