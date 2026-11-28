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

## Modern vehicle camera (partial Phase 3)

```sh
python3 tests/test_modern_vehicle_camera_contract.py
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=undefined \
  tests/modern_vehicle_camera.cpp -o /tmp/modern_vehicle_camera
/tmp/modern_vehicle_camera
```

The host test covers orbit hold timing, FOV smoothing, reverse-view widening,
reverse hysteresis and slope look-ahead at 30, 60 and 120 FPS, plus reset,
invalid steps and long-frame clamping. It does not exercise in-game camera
collision, scripted transitions, real controller/touch input or rendering;
these require device testing. The reverse swing integration also needs in-game
validation.

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
