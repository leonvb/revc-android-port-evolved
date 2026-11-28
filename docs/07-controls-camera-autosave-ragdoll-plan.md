# 07 — Fixed Modern Controls, Camera, Autosave, Foot IK and Ragdolls

## Purpose and status

Implementation plan only; none of the new work below is claimed complete. This plan targets the `modern` branch as of merge `f58f0ac`. For the topics covered here it supersedes conflicting earlier design proposals, not the removal of Modern rendering. Deliver milestones as small PRs **to `modern`**, with Android, Switch, Windows and CMake CI and device playtests. Do not merge an untested risky feature as though its acceptance gate has passed.

**Requirements:** a fixed, non-remappable modern gamepad layout; proportional vehicle triggers; a smoother reverse camera with a quicker return; configurable magnetic aim; one unambiguous FPS adjustment; reliable autosave recovery rather than exact position/vehicle restoration; foot IK; and GTA IV-*inspired* ragdoll reactions to death, vehicle bailouts and impacts. GTA IV parity or its proprietary technology/assets is not promised.

### Baseline observed in `modern`

- `CPad::GetAccelerate/GetBrake` provide 0–255 values, but normally read face buttons. Cars, bikes and boats already consume pedal values as fractions. SDL/GLFW currently reduce trigger axes to `mappedButtons` booleans, and the `lt != 0.0f` check can retain stale values at a midpoint. XInput has separate trigger-byte input. Other backends and Android physical controllers need to be traced rather than assumed equivalent.
- `ModernVehicleCameraState` and `Cam.cpp` already contain reverse hysteresis, reverse widening and manual-orbit hold. These are groundwork, not proof that the in-game transition is acceptable.
- Modern assisted aiming already has a hostile-target aim-down-sights snap and controller friction in `Cam.cpp`; it does not provide a configurable, persistent magnetic tracking behavior. Verify existing assist-mode gating while integrating.
- Graphics/Display and Modernization both expose the target FPS; Graphics also exposes the limiter toggle. `ModernConfig::ShouldFrameLimitGameplay` depends on both, and VSync/Android refresh requests have separate behavior.
- Autosave currently has a Modern setting but no active implementation found; the historical mission-script save block is disabled. `COMMAND_REGISTER_MISSION_PASSED` records an 8-byte text key, not a reliable saved world-space mission marker. Do not treat the option or prior roadmap as an implemented feature.
- `CPedIK` handles upper body/head aiming, not a ground-aware two-leg solver. Ped animation/collision and game scripts are not currently a general articulated ragdoll engine. Phase 5 swimming/mantling remains incomplete.

## Design decisions and scope

- **Fixed gamepad layout:** modern controller actions are not remappable and do not depend on saved gamepad rebindings or control-layout selectors. Show a read-only diagram and current platform button names; remove/disable *gamepad* remapping and conflicting legacy gamepad layouts. Keyboard/mouse rebinding and touch layout settings remain independent. Invert-look and vibration are preferences, not remapping. `VANILLA_DEFINES` keeps its build-compatible legacy behavior; for modern builds the fixed mapping is not switched back by the Classic gameplay preset. Document this deliberate exception to older opt-in guidance.
- **No exact-location promise:** autosave preserves progress and returns the player on foot to a validated, unlocked recovery location. A safe offset near the last completed mission's start is preferred only when its identity and placement are known; otherwise use a known safehouse/stock spawn. Do not restore an arbitrary active vehicle or player position, or pretend the last mission text key is a location.
- **One FPS control:** recommend placing one authoritative frame-rate selector in Graphics/Display and removing the duplicate from Modernization, while migrating existing settings. The existing limiter toggle must not remain an independent contradictory control. Precise migration and Native/Uncapped semantics are specified below.
- **Ragdolls:** develop one subsystem, not separate duplicate features for “ragdolls” and “ragdoll physics.” Keep its state transient; preserve script semantics, logical ped position, damage, death and vehicle entry/exit. Initially deploy to supported player/nonessential NPC cases only; scripted peds and unsupported contexts fall back to stock animations.

## M1 — Fixed controller layout and analog vehicle pedals

### Proposed fixed mapping

Use logical Xbox-style names below (PlayStation: A=Cross, B=Circle, X=Square, Y=Triangle; LT/LB/RT/RB=L2/L1/R2/R1). These are a concrete starting layout to validate against all actions before freezing, not a new remapping interface. For Nintendo prompts use platform-appropriate labels and menu confirm conventions, not a blind physical-label swap.

| Context | Control | Fixed action |
| --- | --- | --- |
| Common | Start / Options; left stick; right stick | Pause; move/steer; look/orbit |
| On foot | A; X; B; Y | Sprint; jump; crouch; enter/exit vehicle / context interaction |
| On foot/combat | LT; RT | Aim; fire/attack (weapon-specific behavior retained) |
| On foot/combat | LB tap/hold; RB | Previous weapon / weapon wheel; cycle/select eligible target |
| On foot | L3; R3; D-pad | Contextual special action; shoulder swap while aiming / look behind otherwise; phone and other contextual actions |
| Vehicle | RT; LT; RB | Analog throttle; analog brake and, once stopped, reverse; handbrake |
| Vehicle | LB tap/hold; B; Y | Next station / radio wheel; horn; exit vehicle |
| Vehicle | R3; D-pad | Look behind; camera/vehicle-specific actions |
| Menus | Bottom/right face buttons; D-pad/sticks; shoulders | Confirm/back per platform; navigate; tabs |

Complete an **action-by-context matrix** before code is frozen: include phone, camera mode, lock-on target cycling, drive-by fire/aim, melee, sniper zoom, helicopter/plane flight and weapons, RC vehicles, mission-specific prompts, boats/bikes, weapon/radio wheel selection, mission retries and frontend navigation. Explicitly resolve conflicts, including LT/RT for drive-bys and wheels consuming their hold/release inputs. Keep all essential actions reachable, without simultaneous conflicting commands; document any contextual modifier and add correct prompts/localization. Do not remove existing keyboard or touch functionality.

### Input pipeline and implementation

1. Audit `ControllerConfig.{h,cpp}`, `Pad.{h,cpp}`, SDL `src/skel/sdl2/sdl2.cpp` (including Android controller delivery), GLFW `src/skel/glfw/glfw.cpp`, Windows/XInput/DirectInput, and Switch's actual backend. Inventory analog capability and distinguish a physical gamepad from touch/keyboard before routing.
2. Normalize each supported physical trigger to `0.0..1.0` **every frame**, including exactly 0.5. Apply a small documented rest deadzone with linear rescaling; clamp invalid values, avoid sticky prior-frame values and preserve the other trigger independently. SDL's game-controller axes are `0..32767`, GLFW mapped axes are `-1..1`, and XInput trigger bytes are `0..255`; confirm hardware-specific ranges and missing-axis reporting per backend. Digital-only devices map released/pressed to 0/1 with a clear indication. Unrecognized raw devices must use a documented safe digital fallback rather than guessed axis indices.
3. Route fixed semantic actions into `CPad` without destroying the analog magnitude through `mappedButtons` or remapping code. A transient per-pad controller state is preferable to modifying serialized structures. Define a deterministic same-frame source policy: active physical pad wins its own actions while connected; keyboard and touch continue through their own existing paths when active, without stale or double inputs. Clear states on disconnect, focus loss, device switch, pause and reconnect. Test first capture and hotplug.
4. Return scaled values in `GetAccelerate/GetBrake` for the physical pad in vehicle context, then trace them through automobile, bike, boat, aircraft and reverse/gear logic. Do not use a single signed axis that loses simultaneous brake and gas. Full travel means full **pedal demand**, not guaranteed full speed; 50% means approximately 50% demand after calibration, not exactly half vehicle speed. Keep brake-to-reverse behavior stable near zero speed, including simultaneous pedals, hills and automatic transmission.
5. Hide/disable gamepad control-redefinition and legacy layout selectors, ignore old gamepad remap values, keep a read-only fixed diagram, and update contextual glyphs, tutorials and localized text. Confirm settings migration doesn't reset keyboard mappings. No change to NPC/AI pedal inputs.

**Acceptance:** automated tests cover 0/25/50/75/100%, midpoint, independent/simultaneous triggers, deadzones, axis conversion, disconnect/reconnect and digital fallback. On actual supported Android Bluetooth/USB and desktop controllers verify fractional pedal demand end-to-end on car/bike/boat, reverse, transitions and all action contexts; include Switch physical testing or report it separately. Keyboard, mouse, touch and Vanilla build regressions must pass.

## M2 — Reverse camera transition and return

**Implementation status:** reverse and forward-return yaw now use separate
bounded, substepped transitions for eligible Modern car/bike follow cameras.
Manual orbit and explicit look directions interrupt/reset automatic yaw; a
camera-source sweep supplements the existing target-to-camera collision checks.
The old reverse FOV widening remains independent. In the previous code it also
scaled `targetBeta`, while the legacy follower filtered and rate-limited that
moving target; it provided no separate forward-return target. Explicit view
reset flags also directly set `Beta`. The new controller replaces the first
path only and leaves explicit view resets intact. Compile with
`MODERN_VEHICLE_CAMERA_TRACE` for trajectory logs. Host tests are not a
substitute for collision and gameplay testing; entry/return tuning and actual
before/after traces remain unverified on device. M1 remains incomplete.

- Instrument current `src/core/ModernVehicleCamera.h` and `src/core/Cam.cpp` separately: measure yaw trajectory, angular velocity, reverse-entry/exit delay, interactions with `nextDirectionIsForward`, camera collision and the existing manual orbit hold. Find the actual snapping source before changing coefficients; current reverse widening is not itself a complete yaw controller.
- Introduce an explicit per-camera/player/vehicle transition state with separate reverse-entry and forward-return targets. Use shortest-angle interpolation with a bounded angular velocity/acceleration and time-step-independent smoothing; use speed hysteresis so braking, gear flicker and slopes do not oscillate. Tune initial targets as **playtest hypotheses**, e.g. smooth visible entry in about 0.5–0.8 s, then resume following forward in about 0.35–0.6 s after confirmed forward movement. Do not enforce an abrupt camera teleport to meet a timer.
- Manual right-stick/touch orbit remains authoritative. Its normal hold delay applies to user orbit, not an extra delay introduced solely by the reverse-to-forward transition. Reset on camera/vehicle change, cutscene, replay, first-person, look-behind and disabled Modern camera. Retain camera collision and avoid swings through walls.
- Add unit trajectory tests at 30/60/120 FPS, ±π wrap, neutral/oscillation, repeated direction changes, long frames, manual interruption and resets. On device test reversing out of parking spaces, tapping brakes while moving forward, immediate forward drive, hills, tunnels and camera collision. Record before/after time and maximum angular velocity. Classic camera must remain unchanged.

## M3 — Magnetic reticle / aim assist setting

**Implementation status:** the Modernization menu now exposes Off / Low /
Medium / High, independently of aim mode. Classic defaults to Off; Modern
presets default to Medium. Existing Modern-preset settings without the new key
migrate to Medium, while Classic/Custom settings without it migrate to Off.
The old Modern Assisted snap/flick and unconditional friction are replaced by
one adjustable system in both Modern aim modes, so they cannot stack. Free
Aim uses 65% of the selected strength; Assisted uses the full strength. Off adds
no magnetism or old assisted snap/flick. Mouse/keyboard are excluded; Android
touch uses lighter tuning than physical pads.

For supported on-foot instant-hit/shotgun weapons, the camera retains one
hostile ped using a generation-checked pool handle; unsuccessful acquisition
scans are rate-limited. It applies bounded camera
attraction and friction only with clear camera, chest-to-muzzle and muzzle
lines; an occluded target gets a short retention grace but no attraction. A
lost target is not silently replaced until a fresh aim or deliberate strong
stick input. Aim height follows the reticle within the body rather than forcing
head-level aim to the torso. Scoped, heavy/area, vehicle, scripted, replay,
Classic and disabled-control cases are excluded. Switch/handheld still lacks
its underlying Modern OTS aiming path and therefore has no magnetism. The
flamethrower's legacy area propagation remains an existing exception, not a
claimed reticle/muzzle-parity fix. The code and host tests still require device
gameplay validation and tuning.

- Add **Aim Magnetism: Off / Low / Medium / High** to the Modernization menu and persisted Modern config; specify defaults explicitly (Classic preset Off; Modern preset Medium) and localize/fallback text. It is distinct from the existing Classic / Free / Assisted aim-mode choice: magnetism operates only during supported Modern aiming with an eligible input device, and Off is exactly the unmodified camera/aim path. Existing Assisted snap and friction must not stack twice; define how those features are scaled or replaced for each value.
- Extend the existing target selection rather than scan every ped multiple times per frame. Candidate must be alive, eligible, in weapon range, near the reticle and visible to camera **and** not blocked from the muzzle. Use stable pool/reference handles with invalidation; no persistent raw dangling pointers. Include moving targets, friendly/mission targets, camera occlusion and target deletion. Do not acquire random bystanders just because they are closer.
- Implement bounded, time-step-independent angular friction plus gentle tracking/rotational attraction while the player aims; soft attraction fades with angular distance. Hysteresis and a short occlusion grace prevent hopping. Strong manual stick input always breaks/overrides the attraction and permits free aiming and headshots. No hidden bullet bending, through-wall aim, hard lock or unsolicited target switching. Calibrate gamepad, touch and mouse separately; default mouse Off unless explicitly supported. Disable/reset for cutscenes, scripted cameras, menus, free aim when setting is Off, unsupported weapons, vehicle/rail-shooter/scoped modes and invalid players.
- Verify real camera reticle, muzzle two-stage ray and actual hit location agree. Include existing heavy/weapon exceptions: do not claim all weapon families use the modern muzzle path until verified.

**Acceptance:** tests for strength ordering and Off identity; target scoring/retention, visibility, release/override, deletion, wrap-around, FPS invariance and no changes to the classic path. Device tests with moving/occluded targets, multiple enemies, headshots, tight interiors, touch/controller and scoped/drive-by missions. Inspect for reticle jitter and unwanted tracking.

## M4 — One FPS selector and migration

1. Use a single visible **Frame rate** selector in Graphics/Display (according to the actual frontend layout): `30 / 60 / 90 / 120 / Uncapped`. Remove the duplicate Modernization target and the separate on/off limiter entry from the same user flow. Specify `Uncapped` as **no software gameplay cap**; display refresh and VSync remain separate and may still limit output. Retain a separate VSync control where supported and do not promise that the OS display runs at any requested rate.
2. Make a single canonical persisted value and make desktop SDL/GLFW/Windows pacing, Android refresh hints, menus and graphics reset read the same effective state. Modern/Classic gameplay presets must not silently change the global FPS selection. Keep a safe 30-FPS/menu policy where required without changing the player's stored choice.
3. Version/precedence for one-time migration: if a valid new canonical key exists, use it; otherwise if legacy `[Graphics] FrameLimiter` is off use Uncapped; otherwise translate valid `[Modern] TargetFPS` (30/60/90/120, Native → Uncapped); on missing/invalid values use the old effective default, 30. Preserve independent VSync behavior. Write the new key, read old keys for backward compatibility but do not maintain contradictory live values; test upgrading files with both keys and downgrading where feasible. Do not reinterpret the old `Native` option as a guaranteed hardware-native frame cap.
4. Test cold startup, changing in both pause and main menu, graphics reset, preset changes, app background/resume, VSync, 30/60/90/120/Uncapped, old INIs and invalid keys, and Android display refresh requests. Verify the actual frame pacing on hardware, not just the selected label.

## M5 — Safe autosave and recovery spawn

### Trigger and progress consistency

- In `src/control/Script3.cpp`, `Script.cpp`, `src/save/GenericGameStorage.cpp`, `src/save/PCSave.cpp` and frontend/save code, add a queued event on a **confirmed mission pass** (and property purchase only after identifying a reliable success event). A mission ending or script termination alone is not proof of success. Coalesce duplicates, wait until mission cleanup has completed and no mission/cutscene, replay, death/arrest, loading, transition, frontend save or disabled-control state makes a save unsafe. If another mission starts, keep the event queued or cancel with an explicit policy; never save half-finished script state.
- Save current logical progress, stats, money, inventory and story/script state via the supported save path, without moving or deleting the live player/vehicle. Show success only after the entire write succeeds. Manual slots remain untouched. Inventory/vehicle outcomes follow tested stock save semantics; no promise to retain the precise vehicle, position, temporary entities or mission equipment.

### Spawn selection and safe fallback

- Preferred: record a **validated mission-start recovery anchor** when an eligible mission marker is entered, with mission identity, interior/level and a curated offset known to be outside that marker's actual trigger volume. Store a stable mission identifier and candidate anchor only if the script/marker association is reliable. Many missions have dynamic, moving, interior, absent or multiple markers; `LastMissionPassedName` alone cannot derive a unique marker/position. If identity, offset or zone accessibility cannot be proved, do not guess.
- Fallback: choose an unlocked, known safehouse/stock save spawn; finally use the game's established safe load spawn. Validate collision ground and clearance after required world/interior streaming, and reject water, unsafe heights, inaccessible/locked islands, active trigger volumes and occupied or hazardous locations. Use bounded alternative offsets and a deterministic fallback. Prevent immediate mission activation via placement outside trigger, not by globally disabling scripts; if necessary defer player control while streaming and validating.
- If the player was driving at save time, write the same progress snapshot and recover **on foot** at the safe anchor. Do not serialize or create a duplicate/mission vehicle. If exact stock serialization cannot safely save while the player is in a vehicle, explicitly defer the snapshot until a safe on-foot state or implement a proven safe snapshot representation; never mutate the live player just to trick the save routine. Explain the policy in the load menu.

### Storage, recovery and verification

- Reserve a genuinely dedicated autosave slot/file, with frontend load entry and no collision with stock slots or quicksave. Preserve compatible/manual save layout; place versioned recovery metadata in a separate, keyed, validated sidecar only if necessary. Couple metadata to the save generation/checksum so old metadata cannot be used with a newer/older save. Use staged writes, flush and atomic replacement where the platform supports it; retain last good generation and validate both files on restart. Define fallback on partial/missing/corrupt metadata or unsupported storage without losing manual saves. Do not claim a two-file rename is atomic.
- Test pass vs fail/aborted mission, delayed cleanup, repeated triggers, property purchase, phone calls, normal and locked islands, interiors, moving markers, several consecutive missions, in-car save, loading while a mission marker is nearby, crash/power loss at every write stage, corrupt/missing sidecar, old saves and unavailable storage. Verify inventory/progress, no automatic mission start and no duplicated vehicles. Device-test several real missions and the first accessible safehouse; host mocks cannot establish script correctness.

## M6 — Foot IK (independent of ragdoll)

- Prototype a post-animation visual IK pass in `src/peds/PedIK.{h,cpp}` and the animation hierarchy update path, without changing `CPedIK`/`CPed` save layouts. Apply after the base pose and before rendering, in the correct local/world space for skinned and non-skinned models. Resolve actual bone IDs and hierarchy order first; gracefully disable when bones/model/animation are unsupported.
- Raycast from each ankle to nearby ground using bounded queries, walkable normal and step-height limits; reject water, unreachable ledges, steep slopes, moving/unstable surfaces or missing collision. A two-bone hip–knee–ankle solve, bounded pelvis adjustment and foot tilt should smooth into/out of each planted foot according to animation contact. Clamp knee bend and reach to avoid flipping, stretching and foot sliding. Ensure IK never moves the logical ped collision capsule or gameplay root.
- Gate to visible near-camera peds, prioritize player, stagger NPC updates and define measured Android CPU/query budgets. Reset/disable in air, water, vehicles, animation transitions, ragdoll, cutscenes and unsupported poses. Handle stream-out, teleport, pause and scale/model changes.
- Test solver geometry, degenerate targets, slope/step limits, smoothing/reset and missing skeletons. Device visual tests on stairs, slopes, curbs, uneven terrain, interiors and moving platforms; verify no root penetration, script/collision changes or 30/60/120-FPS jitter. Measure CPU on low-end and target Android devices.

## M7 — GTA IV-inspired ragdoll: feasibility, integration and staged rollout

**Risk: high.** First produce a feasibility spike with a measurable prototype before choosing a library. Compare a small bounded in-engine articulated solver against a suitable cross-platform, permissively licensed rigid-body library. Audit size, build systems (Android NDK, Switch, Windows, desktop), license distribution, collision integration, performance, determinism and maintenance. No proprietary GTA IV/Euphoria assets or technology. If no option meets those constraints, document the blocker and retain stock animations rather than ship an unsafe imitation.

1. **Foundation:** model a limited per-ped skeleton (pelvis, torso, head, limbs) using bodies/constraints mapped to existing bones; initialize velocities from the animated pose. Fixed-step simulation with bounded substeps and clamped impulses, stable joints and simplified world/vehicle contacts. Define single ownership of the root: logical `CPed` stays coherent with the simulated pelvis/collision, and physics must not apply damage, impulses or collision twice. Broad phase, culling, sleeping and a fixed active-body/ped budget are mandatory.
2. **Trigger classification:** route only verified eligible events through the existing damage/death, vehicle hit and exit/bailout logic (`Ped.cpp`, `PedFight.cpp`, `PedAI.cpp`, vehicle collision paths). Preserve health, damage attribution, wanted/mission consequences, animation callbacks and timers exactly once. Use event thresholds/hysteresis to avoid re-triggering or flinging characters. Never start inside a vehicle, scripted attachment, mission choreography, cutscene, water or unsupported ped state; fallback to current stock animations.
3. **Death first:** blend from the last animation pose into physics, support corpse settling/sleep and cleanup; ensure death state, mission failure and respawn proceed normally even if simulation is culled, disabled or fails. Test NPC death/drowning and script deletion; do not accidentally make NPCs immortal or keep bodies alive indefinitely.
4. **Car hits and bailouts second:** distinguish actual car impact and genuine high-speed exit, preserve existing vehicle collision and rollout damage. Blend to ragdoll from the correct post-exit pose; do not disable vehicle collision globally or get stuck inside it. For survivors, settle to a collision-safe location, select front/back get-up animation, blend back and restore controls/aim once; abort safely if re-hit, killed, arrested or scripted. Do not interfere with the existing bailout-recovery logic until the replacement has equivalent tests.
5. **IV-inspired active reactions later:** optional bounded pose motors/bracing and varied impact responses based on direction/speed; tune using observed gameplay, not an assertion of GTA IV parity. Provide Off / On in Modernization (Classic Off), quality/budget defaults for mobile, and safe stock fallback if limits are reached. Foot IK must yield to ragdoll and resume only after get-up.
6. **Lifecycle/safety:** transient pool/side-table state, no raw-save/enum-layout changes; clear references on death, deletion, streaming, reload, restart, model change and vehicle destruction. Handle contact tunneling, collisions on steps/vehicles, explosions, world boundaries, paused time, time-scale changes, scene switches and unsupported skeletal models. Plan tests for scripts that query ped position/state during ragdoll and prove behavior remains compatible before enabling mission peds.

**Acceptance:** solver/constraint/energy and contact tests at 30/60/120 FPS and frame stalls; deterministic event-to-state tests; collision, deletion and recovery stress tests; save/load and `VALIDATE_SAVE_SIZE`; player and non-mission NPC scenarios for death, hit by slow/fast cars, fall, bailout, re-hit, head/limb obstruction, water and interiors. Validate scripted missions/cutscenes, NPC drowning and game-over; record CPU/memory, active-body limits and thermal performance on Android and other supported platforms. Behavior is not accepted solely because host tests or builds pass.

## Delivery order and release gates

| PR | Scope | Depends on | Exit gate |
| --- | --- | --- | --- |
| 1 | Fixed controller layout + analog triggers, read-only diagrams | Input/backend audit | Per-backend conversion tests; physical controller end-to-end matrix |
| 2 | Reverse camera tuning | Existing Modern camera | Trajectory tests plus in-vehicle device comparison |
| 3 | Magnetism and Modernization setting | Existing Modern aiming | Off/strength/target tests; actual controller/touch combat tests |
| 4 | FPS UI consolidation + migration | Existing frame pacing | Upgrade/restart/pacing/VSync tests on each backend |
| 5 | Autosave safe transaction, spawn and load UI | Save/script audit | Failure injection plus multiple real story reloads |
| 6 | Foot IK prototype and production gating | Animation/skeleton audit | Geometry tests, visual and CPU budget tests |
| 7a | Ragdoll feasibility prototype and go/no-go | Engine/collision/build audit | License/platform review, measured stability/performance |
| 7b–d | Ragdoll death, then impacts/bailouts, then active reactions | 7a; independent of foot IK | Event, mission, recovery and performance gates for each stage |

For each PR run relevant host tests and the full existing CI matrix, verify Classic and Vanilla behavior where applicable, and document any untested platform. Actual Android/desktop/Switch playtests, original game assets and representative saves are necessary for acceptance. CI and source-level tests are not substitutes. No render-overhaul work is required or authorized by this plan.
