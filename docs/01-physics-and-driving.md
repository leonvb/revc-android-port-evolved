# 01 — High-FPS Physics & Driving Modernization

## Overview & Goals
Vice City's physics engine was tuned for a fixed **30 FPS** PS2 frame rate (`CTimer::GetTimeStep() ≈ 1.667`). When the frame limiter is disabled on modern 60 Hz, 90 Hz, or 120 Hz displays, several places in `CPhysical`, `CAutomobile`, `CBike`, and `CBoat` apply damping, drag, or suspension impulses **once per rendered frame** without proper exponential time-step scaling.

In addition, Vice City's car handling uses hard lateral traction clamping and arcade velocity-based gear tables, making cars either snap-oversteer abruptly or feel glued to rails.

This plan covers two layers:
1. **True Frame-Rate-Independent Physics (30 / 60 / 90 / 120+ FPS parity)**
2. **Optional "Modern Driving Dynamics" Mode** (progressive slip curves, weight transfer, transmission improvements, and GTA V-style mid-air vehicle control).

---

## Target Files in Codebase

| File | Role |
| :--- | :--- |
| `src/core/config.h` | Add `#define FIX_HIGH_FPS_PHYSICS` and `#define MODERN_VEHICLE_PHYSICS` |
| `src/core/TimeStep.h` / `Timer.h` | Helper inline math (`TimeStepPow`, `TimeStepLerp`) normalized to 30 FPS baseline (`dt30 = CTimer::GetTimeStep() / (50.0f / 30.0f)`) |
| `src/entities/Physical.cpp` | Air resistance, surface/rotational damping, and collision restitution scaling |
| `src/vehicles/Automobile.cpp` | Wheel suspension spring/damper math, engine braking, burnout, skidmarks, mid-air torque, and lateral tire slip |
| `src/vehicles/Bike.cpp` | Lean interpolation, wheelie/stoppie torque damping, high-FPS stability |
| `src/vehicles/Boat.cpp` & `Floater.cpp` | Water drag & buoyancy impulse normalization (fixes slow boats at 60/120 FPS) |
| `src/vehicles/Transmission.cpp` & `.h` | RPM/clutch simulation, rev-matching, manual/semi-auto gear option, smoother top-speed curve |
| `src/vehicles/HandlingMgr.cpp` & `.h` | Runtime multipliers and optional extended handling profiles |
| `src/core/MenuScreensCustom.cpp` & `IniFile.cpp` | Expose toggles & tuning sliders in Pause Menu → Options |

---

## Part A: 60 / 120 FPS Frame-Rate-Independent Physics Fixes

### 1. Standardized Time-Step Math Helpers (`src/core/Timer.h`)
Original R* code frequently writes:
```cpp
m_vecMoveSpeed *= 0.99f; // Runs 4x as often at 120 FPS as at 30 FPS!
```
Even when multiplied linearly (`1.0f - (1.0f - k) * timeStep`), linear approximations diverge at high frame rates or variable frame times.

**Planned Helper Functions:**
- Define `constexpr float BASE_TIMESTEP_30FPS = 50.0f / 30.0f;` (`1.6666667f`).
- `CTimer::GetTimeStepRatio30()` → returns `CTimer::GetTimeStep() / BASE_TIMESTEP_30FPS` (`1.0f` at 30 FPS, `0.5f` at 60 FPS, `0.25f` at 120 FPS).
- `CTimer::PowTimeStep(float per30FpsFactor)` → `powf(per30FpsFactor, CTimer::GetTimeStepRatio30())`.
- `CTimer::ExpSmooth(float current, float target, float ratePer30Fps)` → frame-rate-independent exponential smoothing.

### 2. Core Rigid-Body Damping (`src/entities/Physical.cpp`)
Audit `CPhysical::ApplyAirResistance()`, `CPhysical::ApplyFriction()`, and `CPhysical::ProcessControl()`:
- **Air resistance**: Replace per-frame `pow(m_fAirResistance, timeStep)` edge cases where `m_fAirResistance` is applied unscaled when speed exceeds thresholds.
- **Angular velocity (`m_vecTurnSpeed`) damping**: Ensure rotational drag decays via `PowTimeStep` so spinning cars/objects stop in the exact same world distance at 120 FPS as at 30 FPS.
- **Static sleep counter (`m_nStaticFrames`)**: Convert frame-count threshold into elapsed milliseconds (`m_fStaticTimeMs`) so physics bodies don't fall asleep 4× faster at 120 FPS.

### 3. Automobile High-FPS Fixes (`src/vehicles/Automobile.cpp`)
- **Off-Throttle Engine Braking & Rolling Resistance**:
  - At high FPS, releasing the accelerator slows cars down dramatically faster than at 30 FPS because rolling/axle drag is applied per frame. Scale rolling resistance impulse by `CTimer::GetTimeStep()`.
- **Suspension Spring & Damper (`ProcessCarWheel`)**:
  - `m_aSuspensionSpringRatioPrev` vs `m_aSuspensionSpringRatio` computes damper velocity as `(curr - prev)` over a single frame. At 120 FPS, `(curr - prev)` is 1/4th as large per tick unless divided by `CTimer::GetTimeStep()`, altering rebound stiffness and curb bounce. Normalize suspension velocity to units per time-step.
- **Skidmark & Tire Smoke Emission**:
  - Gate particle emission (`CParticle::AddParticle`) and skidmark vertex insertion (`CSkidmarks::RegisterOne`) using a per-wheel timer accumulator rather than every frame, preventing particle pool exhaustion and overly dark/dense smoke at 120 FPS.
- **Reverse / Burnout & Steering Return**:
  - Normalize `m_fSteerAngle` centering speed and `m_doingBurnout` timers by `CTimer::GetTimeStep()`.

### 4. Boat & Water Drag Fixes (`src/vehicles/Boat.cpp`, `Floater.cpp`)
- Boats are notoriously affected by unlocked FPS in GTA III/VC: `cBuoyancy::ProcessBuoyancy` and `CBoat::ProcessControl` apply water surface drag per frame, cutting top speed by 25–40% at 120 FPS (which can make missions like *The Fastest Boat* or *Supply & Demand* much harder).
- Normalize drag multipliers in `CBoat::ProcessControl` and `Floater.cpp` with `PowTimeStep` and verify top speed of `SQUALO` / `SPEEDER` matches 30 FPS within ±1%.

---

## Part B: Modern Vehicle Physics Mode (`MODERN_VEHICLE_PHYSICS`)

Behind a user toggle (**Driving Style: Classic / Modern** in Options):

### 1. Progressive Tire Slip & Controllable Drifting (`Automobile.cpp`)
- **Current behavior**: Lateral wheel friction (`ProcessCarWheel`) computes lateral impulse and clamps it abruptly at a flat `fTractionLoss` limit. Once exceeded, rear wheels lose grip suddenly.
- **Planned Pacejka-Lite Slip Curve**:
  - Compute wheel **slip angle** $\alpha = \arctan2(v_{\text{lat}}, |v_{\text{long}}| + \epsilon)$ for each wheel.
  - Evaluate lateral grip multiplier via a smooth 3-zone curve (linear grip build-up → peak grip around $6^\circ\text{–}10^\circ$ → progressive slide plateau at $75\%\text{–}85\%$ of peak rather than cliff drop-off).
  - Modulate rear traction dynamically by longitudinal drive/handbrake slip (allowing throttle-steered powerslides and predictable counter-steering).
  - Add subtle **self-aligning torque** on front steering wheels when counter-steering into a slide so gamepad/touch sticks feel natural rather than twitchy.

### 2. Dynamic Weight Transfer & Body Roll (`Automobile.cpp`)
- Shift effective normal load between front/rear and left/right wheels based on chassis longitudinal and lateral acceleration ($\Delta F_z \propto m \cdot a \cdot h_{\text{CG}} / L$).
- Trail-braking into a corner loads the front tires (increasing turn-in bite) and unloads the rear, while hard acceleration shifts weight rearward.
- Optionally soften visual chassis roll/pitch limits slightly for muscle cars/SUVs while keeping sports cars taut.

### 3. Modernized Transmission & RPM Model (`Transmission.cpp`)
- **Current behavior**: `cTransmission::CalculateDriveAcceleration` statelessly picks a gear from vehicle ground speed (`fCurVelocity`) and applies flat force per gear.
- **Planned enhancements**:
  - Track per-vehicle `m_fCurrentRPM` (normalized `0.0..1.0` from idle to redline) and `m_fShiftTimer` (~120–200 ms clutch shift pause with exhaust pop/audio pitch dip).
  - Torque curve peaking in the mid-to-upper RPM band rather than flat force, plus wheelspin RPM flare during burnouts/wet roads.
  - **Optional Manual / Sequential Gear Shift** binding (e.g., D-Pad Up/Down or L1/R1 modifier) for enthusiasts.
  - Smooth aerodynamic drag (`F_drag = k * v^2`) near top speed instead of hard velocity clipping.

### 4. Mid-Air Vehicle Control (`Automobile.cpp`, `Bike.cpp`)
- When `m_nWheelsOnGround == 0` and airborne for `> 150 ms` (GTA V style):
  - Map Left Stick X/Y (or Pitch/Steer inputs) to gentle angular acceleration (`ApplyTurnForce` around local right and forward axes), scaled by `CTimer::GetTimeStep()` and clamped to realistic angular rates so cars can level out before landing stunt jumps without feeling weightless.
  - Keep `CBike` lean/pitch responsive mid-air while preventing accidental 360° flips from minor curb hops.

---

## Verification & Testing Criteria
1. **30 vs. 60 vs. 120 FPS Determinism Test**:
   - Straight-line 0→top-speed time, coast-down stopping distance, and boat top speed must match within **±2%** across 30, 60, and 120 FPS.
2. **Mission Regression Suite**:
   - Verify *The Driver* (Hilary AI race — AI uses `AutoPilot` / `CarAI` while player uses full physics), *The Fastest Boat*, *PCJ Playground*, *Cone Crazy*, and Unique Stunt Jumps remain fair and completable in both Classic and Modern modes.
3. **Save Compatibility**:
   - Store any new per-vehicle runtime state (like `m_fCurrentRPM` or slip state) outside serialized save structs or in transient runtime side-tables indexed by `CPools::GetVehicleRef`, keeping `PCSave` binary-compatible.
