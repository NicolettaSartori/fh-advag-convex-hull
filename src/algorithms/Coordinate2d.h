//
// Created by silve on 14/11/2026.
//

#ifndef ADVAG_QUICKHULL_PERSONAL_COORDINATE2D_H
#define ADVAG_QUICKHULL_PERSONAL_COORDINATE2D_H

//My own coordinate struct. Is this faster than using something else? Who knows.
//crossProducts are used a lot, so I optimized them as much as I could.

struct Coordinate2d {
    float x{};
    float y{};
};

inline constexpr float crossProduct(const Coordinate2d& a, const Coordinate2d& b) noexcept {
    return a.x * b.y - a.y * b.x;
}

inline constexpr float crossProduct(const Coordinate2d& a, const Coordinate2d& b, const Coordinate2d& testPoint) noexcept {
    return (b.x - a.x) * (testPoint.y - a.y) - (b.y - a.y) * (testPoint.x - a.x);
}

inline constexpr float crossProductEdge(const Coordinate2d& origin, const float dx, const float dy, const Coordinate2d& testPoint) noexcept {
    return dx * (testPoint.y - origin.y) - dy * (testPoint.x - origin.x);
}

#endif //ADVAG_QUICKHULL_PERSONAL_COORDINATE2D_H
