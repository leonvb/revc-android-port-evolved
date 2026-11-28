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
