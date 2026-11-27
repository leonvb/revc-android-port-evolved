# 06 — Android & Mobile UX Polish

## Overview & Goals
On modern Android flagships (such as the **OPPO Find X7** with its 120 Hz LTPO AMOLED display, X-axis linear haptic motor, and gyroscope), `reVC` already features a custom native-backed virtual gamepad (`TouchControlsView.java` ↔ `TouchControls.cpp`), floating look-stick, menu touch support, and runtime render-scale control.

To elevate the mobile port from "virtual PS2 pad overlaid on screen" to a **first-class modern mobile experience**, this plan adds:
1. **Granular Sub-Context Touch Layouts & Smooth Alpha Transitions** (On-Foot Unarmed vs. Aiming vs. Car vs. Bike vs. Boat/Heli vs. Map).
2. **Tactile X-Axis Linear Haptics (`VibrationEffect`)** bridged from game events (gunfire, crashes, explosions, gear shifts, damage).
3. **Gyroscope ADS Fine-Aiming & Optional Tilt Steering**.
4. **Analog Throttle/Brake Slide & Direct HUD Tap Shortcuts** (tap radar to open map, tap weapon icon for Weapon Wheel, swipe radio).
5. **120 Hz Display Pacing & Battery/Thermal Presets**.

---

## Target Files in Codebase

| File | Role |
| :--- | :--- |
| `android/launcher/app/src/main/java/com/revc/game/TouchControlsView.java` | Sub-context button sets, smooth fade animations, HUD tap zones, analog pedal slider, gyro toggle |
| `android/launcher/app/src/main/java/com/revc/game/GameActivity.java` | `Vibrator` / `VibrationEffect` manager, `SensorManager` (`TYPE_GYROSCOPE`), 60/90/120 Hz `preferredRefreshRate` API |
| `src/skel/android/TouchControls.h` & `.cpp` | Expose rich game state bitmask (`nativeGetDetailedContext`), analog throttle/brake floats (`0..1`), gyro delta pitch/yaw |
| `src/skel/android/JavaWrapper.h` & `.cpp` | JNI `TriggerHaptic(int effectType, float intensity)` calls from native game thread |
| `src/skel/sdl2/sdl2.cpp` | Merge analog touch triggers, gyro camera deltas, and frame-rate pacing cap |
| `src/core/Pad.cpp` | Consume analog gas/brake (`0..255` instead of binary `0` or `255` on touch) and `ShakePad` → Android Haptics hook |

---

## Detailed Design & Implementation Steps

### 1. Sub-Context Touch Controls & Direct HUD Shortcuts (`TouchControlsView.java`, `TouchControls.cpp`)
Currently `nativeGetGameContext()` returns 4 coarse states (`0 = menu`, `1 = on foot`, `2 = vehicle`, `3 = cutscene`).

#### Expand `nativeGetDetailedContext()` Bitmask / Enum:
- **`ON_FOOT_UNARMED` vs. `ON_FOOT_ARMED` vs. `ON_FOOT_AIMING` vs. `ON_FOOT_SWIMMING`**:
  - Hide Aim/Fire reticle buttons when completely unarmed and out of combat (optional "Clean Exploration HUD"), fading them in (`150 ms` alpha lerp) the moment a weapon is drawn or combat starts.
  - Show a dedicated **Shoulder-Swap** & **Crouch** micro-button while in Over-the-Shoulder ADS (`ON_FOOT_AIMING`).
  - Show **Dive / Climb** prompt only when swimming or facing a climbable ledge.
- **`VEHICLE_CAR` vs. `VEHICLE_BIKE` vs. `VEHICLE_HELI_PLANE` vs. `VEHICLE_TURRET`**:
  - **Bikes (`CBike`)**: Automatically show lean-forward / wheelie control or map left-stick vertical axis cleanly without cluttering car driving.
  - **Helicopters / Planes (`CHeli`, `CPlane`, RC Baron/Raider)**: Present intuitive twin-stick flight controls (Ascend/Descend + Yaw/Bank) instead of confusing car buttons.
  - **Firetruck / Rhino Tank**: Show turret aim/water-cannon fire control.
- **Direct Native HUD Tap Zones**:
  - **Tap Top-Right Weapon HUD** → cycle weapon; **Hold** → open Slow-Mo Weapon Wheel ([Doc 02](02-camera-aiming-and-combat.md)).
  - **Tap Bottom-Left Minimap** → jump straight into the Fullscreen Pause Map (to drop a GPS waypoint in 1 tap instead of Pause → Map).

---

### 2. Analog Throttle / Brake Gesture (`TouchControlsView.java`, `Pad.cpp`)
- On PS2/Gamepad, `CPad` supports analog pressure/triggers (`0..255`), but touch buttons currently send binary `pressed ? 255 : 0`, making low-speed parking or smooth cornering throttle modulation harder.
- **Optional Vertical Slide Pedal**:
  - Holding the **Accelerate** or **Brake** button and sliding the thumb slightly upward/downward modulates analog pressure (`30%..100%`), or combines gas+brake into an optional vertical throttle slider for racing fans.

---

### 3. Tactile Linear Haptics via `CPad::StartShake` (`Pad.cpp`, `JavaWrapper.cpp`, `GameActivity.java`)
Vice City already calls `CPad::GetPad(0)->StartShake(duration, intensity)` on every gunshot, car collision, explosion, landing, and damage hit! On Android SDL2, rumble is often unmapped unless wired to Android's `Vibrator`.

#### Implementation:
1. Hook `CPad::StartShake` (and custom crisp events like weapon fire & gear shift) on `#ifdef ANDROID` to call `CJavaWrapper::Vibrate(durationMs, amplitude, presetId)` with rate-limiting (`<= 1` call per `25 ms` to avoid binder overhead).
2. On Android API 29+ (`Build.VERSION_CODES.Q` / Android 14+ on Find X7):
   - **Gunshots**: Crisp `VibrationEffect.EFFECT_TICK` / `12 ms` high-sharpness pulse scaled by weapon caliber (light tap for 9mm/Uzi, punchy thud for Shotgun/Python/M4).
   - **Collisions & Explosions**: `VibrationEffect.createOneShot(duration, amplitude)` scaled by collision impulse.
   - **Menu Taps & Weapon Wheel Ticks**: Subtle `EFFECT_CLICK` micro-feedback.
3. Expose **Haptics Intensity: Off / Subtle / Full** in Options.

---

### 4. Gyroscope Aiming (`GameActivity.java`, `TouchControls.cpp`, `Cam.cpp`)
- Register `Sensor.TYPE_GYROSCOPE` at `SENSOR_DELAY_GAME` **only while the app is resumed and Gyro is enabled** (zero battery drain when disabled).
- Modes: **Off / While Aiming (ADS Only — Recommended) / Always On**.
- Transform device angular velocity (`rad/s`) according to current landscape display rotation (`Surface.ROTATION_90` vs `ROTATION_270` so flipping the phone 180° never inverts pitch/yaw!) and feed sub-pixel deltas directly into OTS / Sniper / M16 camera pitch & yaw.
- Makes headshots and Sniper/M4 missions (*Phnom Penh '86*, *The Shootist*, *Death Row*) feel effortless on touchscreen.

---

### 5. High-Refresh Display Pacing & Thermal Profiles (`GameActivity.java`, `sdl2.cpp`)
- Expose a clean **Target Frame Rate** selector in Display Settings: **`30 FPS (Classic)` / `60 FPS (Balanced)` / `90 FPS` / `120 FPS (Smooth)`**.
- On Android 11+ (`API 30+`), call `Surface.setFrameRate()` / `WindowManager.LayoutParams.preferredDisplayModeId` to match the chosen target so LTPO screens (like the Find X7's 1–120 Hz panel) lock cleanly at 60 Hz or 120 Hz without micro-stutter, saving significant battery and heat when 60 FPS is selected.
- Pair with existing `nativeSetRenderScale` (`50%..100%`) to offer 1-click **Performance / Balanced / Ultra Quality** presets.

---

## Verification & Testing Criteria
- Verify landscape orientation flip (`ROTATION_90` vs `ROTATION_270`) keeps gyro aiming axes correct.
- Confirm physical Bluetooth/USB controller connection (`nativeIsControllerConnected()`) still auto-hides touch overlays cleanly while retaining optional haptics/HUD improvements.
- Profile JNI call frequency during heavy gunfire (Minigun / Uzi + explosions) to ensure haptics rate-limiting causes zero frame-time spikes at 120 FPS.
