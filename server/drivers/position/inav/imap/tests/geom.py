
# Desc: Useful geometric routines
# Date: 6 Sep 2002
# CVS: $Id$


from math import *


def normalize(a):
    """Normalize an angle in domain -pi tp pi."""
    return atan2(sin(a), cos(a))


def ltog(l, o):
    """Compute the absolute pose of l, which is defined relative to o
    (cartesian)."""

    return (o[0] + l[0] * cos(o[2]) - l[1] * sin(o[2]),
            o[1] + l[0] * sin(o[2]) + l[1] * cos(o[2]),
            normalize(o[2] + l[2]))

 
def gtol(g, o):
    """Compute the pose of g relative to o (cartesian)."""

    return (+(g[0] - o[0]) * cos(o[2]) + (g[1] - o[1]) * sin(o[2]),
            -(g[0] - o[0]) * sin(o[2]) + (g[1] - o[1]) * cos(o[2]),
            normalize(g[2] - o[2]))


def sub(b, a):
    """Compute c = b - a"""

    c = (b[0] - a[0], b[1] - a[1], normalize(b[2] - a[2]))
    return c


def ptoc(p):
    """Compute the cartesian pose of the polar point p."""

    return (p[0] * cos(p[1]),
            p[0] * sin(p[1]),
            normalize(p[2]))


def ctop(c):
    """Compute the polar pose of the cartesian point p."""

    return (sqrt(c[0] * c[0] + c[1] * c[1]),
            atan2(c[1], c[0]),
            c[2])


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


def magnitude(v, m):
    """Compute the magnitude of a vector using the given metric."""

    assert(len(v) == len(m))

    d = 0
    for i in range(len(v)):
        d += v[i]**2 * m[i]

    return sqrt(d)

