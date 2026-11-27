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
```

This exercises master-off and invalid-value behavior, scene eligibility,
far-clip restoration decisions, alternating population horizons (including
startup batches), distance bounds, and memory-budget calculations. It tests
policy, not the actual camera, streaming, or population systems. Device
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
