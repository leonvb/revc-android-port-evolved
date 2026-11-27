#!/usr/bin/env python3
"""CPU reference for the planned Dual-Kawase kernels (not a GPU shader test)."""
import math


def extract(c, threshold=.78, knee=.1):
    brightness = max(c)
    knee = max(knee, .00001)
    soft = max(0., min(2*knee, brightness - threshold + knee))
    soft = soft*soft/(4*knee)
    contribution = max(0., max(soft, brightness-threshold) / max(brightness, .00001))
    return tuple(v * contribution for v in c)


def sample(image, x, y, bounds):
    x0, y0, x1, y1 = bounds
    x, y = min(max(x, x0), x1), min(max(y, y0), y1)
    lo_x, lo_y = math.floor(x), math.floor(y)
    hi_x, hi_y = min(lo_x+1, x1), min(lo_y+1, y1)
    fx, fy = x-lo_x, y-lo_y
    return tuple((image[lo_y][lo_x][i]*(1-fx) + image[lo_y][hi_x][i]*fx)*(1-fy) +
                 (image[hi_y][lo_x][i]*(1-fx) + image[hi_y][hi_x][i]*fx)*fy
                 for i in range(3))


def kernel(image, x, y, bounds, kind, radius=1.):
    if kind in ('bright', 'down'):
        taps = [(0, 0, 4), (-1, -1, 1), (1, -1, 1), (-1, 1, 1), (1, 1, 1)]
        norm = 8
    else:
        taps = [(-2, 0, 1), (2, 0, 1), (0, -2, 1), (0, 2, 1),
                (-1, -1, 2), (1, -1, 2), (-1, 1, 2), (1, 1, 2)]
        norm = 12
    values = [(extract(sample(image, x + dx*radius, y + dy*radius, bounds))
               if kind == 'bright' else sample(image, x + dx*radius, y + dy*radius, bounds), w)
              for dx, dy, w in taps]
    return tuple(sum(c[i]*w for c, w in values)/norm for i in range(3))


def resize(image, width, height, kind):
    source_width, source_height = len(image[0]), len(image)
    bounds = (0, 0, source_width-1, source_height-1)
    return [[kernel(image, (x+.5)*source_width/width-.5,
                    (y+.5)*source_height/height-.5, bounds, kind,
                    radius=.5 if kind == 'bright' else 1.)
             for x in range(width)] for y in range(height)]


def near(a, b):
    return all(abs(x-y) < 1.e-10 for x, y in zip(a, b))


assert near(extract((0., 0., 0.)), (0., 0., 0.))
assert near(extract((.1, .2, .3)), (0., 0., 0.))
assert near(extract((1., 0., 0.)), (.22, 0., 0.))
assert near(extract((0., 0., 1.)), (0., 0., .22))
# Normalized blur kernels preserve constant fields at edges and in the middle.
constant = [[(.2, .4, .8)]*7 for _ in range(5)]
for kind in ('down', 'up'):
    for x, y in ((0, 0), (3, 2), (6, 4), (-10, 20)):
        assert near(kernel(constant, x, y, (0, 0, 6, 4), kind), (.2, .4, .8))
# White padding must never bleed into a smaller captured rectangle.
padded = [[(1., 1., 1.)]*8 for _ in range(8)]
for y in range(2, 6):
    for x in range(1, 5):
        padded[y][x] = (0., 0., 0.)
for kind in ('bright', 'down', 'up'):
    assert near(kernel(padded, 1, 2, (1, 2, 4, 5), kind, radius=5), (0., 0., 0.))
# Run all five proposed passes on an asymmetric bright impulse.
scene = [[(0., 0., 0.) for _ in range(32)] for _ in range(16)]
scene[5][10] = (1., .2, .1)
a = resize(scene, 16, 8, 'bright')
b = resize(a, 8, 4, 'down')
c = resize(b, 4, 2, 'down')
b = resize(c, 8, 4, 'up')
a = resize(b, 16, 8, 'up')
assert max(pixel[0] for row in a for pixel in row) > 0
assert all(0 <= pixel[2] <= pixel[1] <= pixel[0] for row in a for pixel in row)
assert sum(pixel[0] > 0 for row in a for pixel in row) > 1
