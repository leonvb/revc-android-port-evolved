# Standalone GPS path-search tests

The road-search core is independent of RenderWare and can be exercised without game assets:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  tests/gps_path_search.cpp -o /tmp/gps_path_search
/tmp/gps_path_search
```

These tests cover disabled and water nodes, disconnected groups, road direction, height-aware nearest-node selection, bounds and output capacity, circular clipping, and compare the result with Dijkstra on randomly generated directed graphs. They do not replace in-game bridge, overpass, performance, or rendering tests.

## Modern visual policy

```sh
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  tests/modern_visual_policy.cpp -o /tmp/modern_visual_policy
/tmp/modern_visual_policy
python3 tests/test_priority_scan_contract.py
```

This exercises master-off and invalid-value behavior, scene eligibility,
far-clip restoration decisions, priority-distance scaling, alternating
population horizons (including startup batches), distance bounds, and
memory-budget calculations. The source-level priority scan guard checks the
renderer wiring but does not simulate streaming or drawing. Device
playtesting and visual inspection are still necessary.

## Experimental post-processing

```sh
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  tests/modern_postfx_policy.cpp -o /tmp/modern_postfx_policy
/tmp/modern_postfx_policy
python3 tests/test_modern_shaders.py # requires glslangValidator
```

The shader test links the librw GL headers and checks generated shader files.
Neither test exercises live frame capture, GPU rendering, or visual quality.

GL framebuffer color/depth pairing (no GL context required):

```sh
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  tests/gl_framebuffer_mates.cpp -o /tmp/gl_framebuffer_mates
/tmp/gl_framebuffer_mates
```

This checks bookkeeping only; actual GL/GLES attachments and framebuffer
completeness still require a graphics-context/device test.

## Bloom groundwork

```sh
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  tests/modern_bloom_plan.cpp -o /tmp/modern_bloom_plan
/tmp/modern_bloom_plan
python3 tests/modern_bloom_reference.py
python3 tests/test_modern_shaders.py # requires glslangValidator
```

The reference checks the intended filter math on the CPU; it does not execute
shader code or render the bloom effect. The shader test validates GLSL linkage;
Windows CI compiles the HLSL with fxc. Runtime integration is still pending.

## Modern light selection

```sh
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  tests/modern_lighting_policy.cpp -o /tmp/modern_lighting_policy
/tmp/modern_lighting_policy
```

This tests bounded per-object light selection, not rendered lighting.

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
