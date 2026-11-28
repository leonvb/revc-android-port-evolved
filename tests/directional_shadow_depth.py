#!/usr/bin/env python3
"""RGB8 depth packing/reference comparison for the GL shadow shader."""
import math
import random


def encode(depth):
    depth = min(depth, 0.999999)
    a = [(depth * x) % 1.0 for x in (1.0, 255.0, 65025.0)]
    a = [a[0] - a[1] / 255.0, a[1] - a[2] / 255.0, a[2]]
    # Model an undithered, nearest-filtered UNORM8 color attachment.
    return [round(max(0.0, min(1.0, c)) * 255.0) / 255.0 for c in a]


def decode(rgb):
    return rgb[0] + rgb[1] / 255.0 + rgb[2] / 65025.0


random.seed(1603)
for depth in [0.0, 1.0, 0.5, 1/255, 1/65025] + [random.random() for _ in range(100000)]:
    actual = decode(encode(depth))
    assert abs(actual - min(depth, 0.999999)) < 0.5 / 16581375 + 1e-12
# A cleared white map must compare as beyond the far plane.
assert decode([1.0, 1.0, 1.0]) > 1.0
assert decode(encode(0.25)) < 0.75 - 0.0015  # receiver behind occluder
assert decode(encode(0.75)) > 0.25 - 0.0015  # receiver in front
assert math.isclose(decode(encode(0.5)), 0.5, abs_tol=0.00001)
