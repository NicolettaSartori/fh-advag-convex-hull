#pragma once

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

// Cross product
//   > 0:counter-clockwise
//   < 0: clockwise
//   = 0: collinear
inline double cross(const Point& o, const Point& a, const Point& b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}
