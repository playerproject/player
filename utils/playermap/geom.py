
# Desc: Geometry routines
# Author: Andrew Howard
# Date: 29 Mar 2003
# CVS: $Id$

from math import *


def normalize(a):
    """Normalize an angle in domain -pi tp pi."""
    return atan2(sin(a), cos(a))


def coord_add(l, o):
    """Compute the absolute pose of l, which is defined relative to o
    (cartesian)."""

    if len(l) == 2:
        return (o[0] + l[0] * cos(o[2]) - l[1] * sin(o[2]),
                o[1] + l[0] * sin(o[2]) + l[1] * cos(o[2]))
    elif len(l) == 3:
        return (o[0] + l[0] * cos(o[2]) - l[1] * sin(o[2]),
                o[1] + l[0] * sin(o[2]) + l[1] * cos(o[2]),
                normalize(o[2] + l[2]))
    assert(0)
    return

 
def coord_sub(g, o):
    """Compute the pose of g relative to o (cartesian)."""

    if len(g) == 2:
        return (+(g[0] - o[0]) * cos(o[2]) + (g[1] - o[1]) * sin(o[2]),
                -(g[0] - o[0]) * sin(o[2]) + (g[1] - o[1]) * cos(o[2]))

    elif len(g) == 3:
        return (+(g[0] - o[0]) * cos(o[2]) + (g[1] - o[1]) * sin(o[2]),
                -(g[0] - o[0]) * sin(o[2]) + (g[1] - o[1]) * cos(o[2]),
                normalize(g[2] - o[2]))
    assert(0)
    return


def dist_metric(a, b, m):
    """Compute the distance between two poses using the given metric."""

    d = 0
    for i in range(3):
        d += (a[i] - b[i]) ** 2

    return sqrt(d)


def nearest_line_point(pa, pb, p):
    """Compute minimum distance between a line and a point."""

    a = pb[0] - pa[0]
    b = pb[1] - pa[1]
    s = (a * (p[0] - pa[0]) + b * (p[1] - pa[1])) / (a * a + b * b)

    n = (pa[0] + a * s, pa[1] + b * s)
    d = (n[0] - p[0], n[1] - p[1])
    dist = sqrt(d[0] * d[0] + d[1] * d[1])

    return (dist, n)


def nearest_segment_point(pa, pb, p):
    """Compute minimum distance between a line segment and a point."""

    a = pb[0] - pa[0]
    b = pb[1] - pa[1]
    s = (a * (p[0] - pa[0]) + b * (p[1] - pa[1])) / (a * a + b * b)

    if s < 0:
        s = 0
    if s > 1:
        s = 1

    n = (pa[0] + a * s, pa[1] + b * s)
    d = (n[0] - p[0], n[1] - p[1])
    dist = sqrt(d[0] * d[0] + d[1] * d[1])

    return (dist, n)
