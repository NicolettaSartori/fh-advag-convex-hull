#pragma once

// Basic 2D geometry shared by all convex hull algorithms.

struct Point {
    double x;
    double y;
};

// Lexicographic order: by x, then by y.
inline bool operator<(const Point& a, const Point& b) {
    return a.x < b.x || (a.x == b.x && a.y < b.y);
}

inline bool operator==(const Point& a, const Point& b) {
    return a.x == b.x && a.y == b.y;
}

// Cross product of (a - o) x (b - o).
//   > 0: o -> a -> b is a left turn (counter-clockwise)
//   < 0: right turn (clockwise)
//   = 0: collinear
inline double cross(const Point& o, const Point& a, const Point& b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}
