# Standalone GPS path-search tests

The road-search core is independent of RenderWare and can be exercised without game assets:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  tests/gps_path_search.cpp -o /tmp/gps_path_search
/tmp/gps_path_search
```

These tests cover disabled and water nodes, disconnected groups, road direction, height-aware nearest-node selection, bounds and output capacity, circular clipping, and compare the result with Dijkstra on randomly generated directed graphs. They do not replace in-game bridge, overpass, performance, or rendering tests.

## Radar and rendering removal contracts

```sh
python3 tests/test_radar_distance_contract.py
python3 tests/test_classic_render_contract.py
```

These source-level checks verify the bottom-left radar placement and guard
against reintroducing removed Modern rendering hooks. They do not render a frame.

## Android CD streamer and scripted scene loads

The host-side test compiles the actual Android streamer using minimal JNI/log
headers; it does not require game assets or an Android SDK. It runs repeated
concurrent-channel reads, completion waits, worker restarts and a short-read
error case. It does not replace an on-device hotel/interior transition test.

```sh
INCLUDES=$(find src -type d -printf ' -I%p')
c++ -w -std=c++11 -pthread -ffunction-sections -fdata-sections \
  -DANDROID -DRW_GL3 -DLIBRW_SDL2 -DLIBRW -Itests/android_stubs \
  $INCLUDES -Ivendor/librw -Ivendor/sdl2/include tests/android_cdstream.cpp \
  -Wl,--gc-sections -o /tmp/android_cdstream
timeout 30 /tmp/android_cdstream
python3 tests/test_scene_load_contract.py
```

## Custom frontend text fallback

These host tests exercise missing custom menu keys, main/mission translation
precedence and simultaneous fallback strings. The source check also verifies all
reVC additions match `utils/gxt/american.txt`. No game assets are overwritten.

```sh
python3 tests/test_frontend_text_fallback.py
INCLUDES=$(find src -type d -printf ' -I%p')
for MODE in enabled disabled; do
  OPTS=""
  if [ "$MODE" = disabled ]; then OPTS=-DTEST_WITHOUT_MODERN_FEATURES; fi
  c++ -w -std=c++11 -ffunction-sections -fdata-sections \
    -DRW_GL3 -DLIBRW_SDL2 -DLIBRW $OPTS $INCLUDES \
    -Ivendor/librw -Ivendor/sdl2/include tests/frontend_text_fallback.cpp \
    -Wl,--gc-sections -o /tmp/frontend_text_fallback
  /tmp/frontend_text_fallback
done
```

This does not test actual language fonts or menu rendering on an Android device.

## Modern vehicle camera and reverse transitions

```sh
python3 tests/test_modern_vehicle_camera_contract.py
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=undefined \
  tests/modern_vehicle_camera.cpp -o /tmp/modern_vehicle_camera
/tmp/modern_vehicle_camera
```

The host test covers orbit hold, FOV and pitch smoothing, plus the reverse yaw
controller at 30, 60 and 120 FPS: entry/return duration, speed and acceleration
bounds, angle wrapping, neutral hysteresis, repeated reversals, manual
interruption, independent states, invalid steps and long-frame clamping. The
trajectory timings are tuning hypotheses, not device measurements. The tests do
not exercise in-game collision sweeps, scripted transitions, physical
controller/touch input or rendering; these require device testing.

For comparison builds, define `MODERN_VEHICLE_CAMERA_TRACE` to log per-frame
vehicle speed, dt, phase, actual yaw, legacy candidate yaw, heading, yaw rate,
manual hold and ownership. The candidate is calculated from the current new
trajectory, not a measurement of the old build; record the old build separately
for a real before/after comparison. Check parking spaces, brake taps, immediate
forward driving, repeated direction changes, slopes, narrow tunnels, wall
corners, manual interruption, look-behind and camera/vehicle switching. Check
Classic camera and touch separately. A blocked sweep can constrain the camera;
observe that it recovers smoothly when the obstruction clears.

## Modern third-person muzzle-ray groundwork

```sh
python3 tests/test_modern_aim_contract.py
python3 tests/test_modern_ots_contract.py
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=undefined \
  tests/modern_aim_geometry.cpp -o /tmp/modern_aim_geometry
/tmp/modern_aim_geometry
```

These cover the muzzle-direction math and source-level activation guards, not
engine collision queries or weapon gameplay. The two-stage ray currently applies
only to supported third-person instant-hit and shotgun aim paths. OTS camera,
strafing and aim assist require device validation.


## Modern aim magnetism

```sh
python3 tests/test_modern_magnetism_contract.py
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=undefined \
  tests/modern_aim_magnetism.cpp -o /tmp/modern_aim_magnetism
/tmp/modern_aim_magnetism
c++ -std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter -fsanitize=undefined \
  tests/modern_magnetism_pool.cpp -o /tmp/modern_magnetism_pool
/tmp/modern_magnetism_pool
```

These cover strength ordering, Off identity, bounded and FPS-independent math,
angle wrap, nearest-body height, acquisition scoring/range, override cooldown,
visibility grace and no automatic target hopping. The pool test uses the actual
pool generation handles for deletion and slot reuse. Source contracts check the
camera/muzzle LOS, device and gameplay guards. They do **not** run engine
collision, rendering, actual input arbitration, animated muzzle positions or
weapon gameplay. Test those on-device, including aim release, two enemies,
headshots, friendlies, occlusion and tight cover. The attraction values are
initial tuning, not gameplay-validated.

The feature uses the existing supported Modern OTS camera on PC/Android; the
Switch/handheld configuration lacks that camera path and currently has no
magnetism. Mouse/keyboard are excluded. Heavy/area, sniper, first-person and
vehicle weapons are excluded; the older flamethrower area behavior is not
claimed to have reticle/muzzle parity. The previous Modern Assisted snap, flick
switch and unconditional friction were replaced by this one adjustable system;
Off disables all of those additions. Free Aim applies 65% of the selected
strength; Assisted applies the full strength. Loss of a retained target does not silently
switch to another: release/re-aim or deliberately move the stick strongly to
re-arm acquisition. Touch uses a separate, lighter tuning from a physical pad.

## Modern wheel input

```sh
python3 tests/test_modern_wheels_contract.py
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=undefined \
  tests/modern_wheel_input.cpp -o /tmp/modern_wheel_input
/tmp/modern_wheel_input
```

These tests cover the 180 ms boundary, sector selection, deadzone and clock
wrap, plus source-level ownership and guard wiring. They do not exercise the
actual HUD, controller, touch, audio, script time-scale or gameplay transitions.

## Modern locomotion groundwork (partial Phase 5)

```sh
python3 tests/test_modern_locomotion_contract.py
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=undefined \
  tests/modern_traversal_math.cpp -o /tmp/modern_traversal_math
/tmp/modern_traversal_math
```

The host test covers the reversal timing at 30, 60 and 120 FPS, low-speed
turns and reset on neutral input. The contract checks configuration, script,
player and rollout guards. Neither test exercises real player animations,
controller input, car bailouts or mission behavior. Stock sprint already uses
held input; the hold-to-sprint setting gates the new pivot and rollout recovery,
not a new sprint input mode. The pivot is currently wired to Zelda-style
third-person controls, not the first-person/fighter control paths.

Swimming, shore/dock/boat recovery and mantling remain unimplemented. A height
cap and a generic collision probe are not sufficient to guarantee mission and
island gating, especially because swimming itself could go around a bridge.
Those features require a reliable way to enforce script-controlled access and
playtesting with the original game assets before they can be enabled safely.

## Analog vehicle trigger groundwork (partial fixed-controller milestone)

```sh
python3 tests/test_modern_pedals_contract.py
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=undefined \
  tests/modern_pedal_input.cpp -o /tmp/modern_pedal_input
/tmp/modern_pedal_input
INCLUDES=$(find src -type d -printf ' -I%p')
c++ -w -std=c++11 -ffunction-sections -fdata-sections \
  -DRW_GL3 -DLIBRW_SDL2 -DLIBRW $INCLUDES -Ivendor/librw -Ivendor/sdl2/include \
  tests/modern_pedal_reconcile.cpp src/core/Pad.cpp -Wl,--gc-sections -o /tmp/modern_pedal_reconcile
/tmp/modern_pedal_reconcile
```

These exercise trigger normalization, deadzone, independent pedal pressures,
keyboard/mouse combination and the actual input reconciliation behavior. The
source-contract test checks wiring, not gameplay. Physical SDL controllers with
both mapped triggers, mapped GLFW gamepads once both trigger axes are observed,
and XInput have trigger pedals while driving. Devices without confirmed trigger
axes fall back to the existing controls; Android touch is unchanged. The rest of
the fixed modern, non-remappable layout and its frontend prompts is **not yet
implemented**. These changes have not been tested with physical controllers.

For device testing: in a car and bike, compare light, half and full RT travel,
then use LT to brake and reverse; check holding both triggers, stopping on a hill,
releasing, controller disconnect/reconnect and keyboard/touch controls. Also
check boats, aircraft, drive-bys and camera look controls. The pedal command is
proportional; the resulting vehicle speed is not expected to be linear in pedal
travel. Record the device/controller and backend for any unexpected behavior.
