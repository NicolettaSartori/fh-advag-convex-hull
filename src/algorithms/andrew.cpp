// Andrew's Monotone Chain algorithm
// Usage:
//   ./build/hull                  menu -> visualization
//   ./build/hull --bench andrew   time measurements -> protocol/andrew.csv

#include "algorithms.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string str(const Point& p) {
    char buf[48];
    std::snprintf(buf, sizeof buf, "(%.1f, %.1f)", p.x, p.y);
    return buf;
}

Step snapshot(const std::vector<Point>& hull, std::size_t k, std::size_t lowerEnd, std::string text) {
    Step s;
    s.description = std::move(text);
    if (lowerEnd == 0) {
        s.chains.emplace_back(hull.begin(), hull.begin() + k);
    } else {
        s.chains.emplace_back(hull.begin(), hull.begin() + lowerEnd);
        s.chains.emplace_back(hull.begin() + lowerEnd - 1, hull.begin() + k);
    }
    return s;
}

template <class Tracer>
std::vector<Point> andrew(std::vector<Point> pts, Tracer& trace) {
    std::sort(pts.begin(), pts.end());
    pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
    const std::size_t n = pts.size();

    if (n < 3) {
        if constexpr (Tracer::enabled) {
            Step s;
            s.description = "Fewer than 3 distinct points: they are the hull.";
            s.chains.push_back(pts);
            s.closed = true;
            trace.record(std::move(s));
        }
        return pts;
    }

    // stack
    std::vector<Point> hull(2 * n);
    std::size_t k = 0;
    std::size_t lowerEnd = 0;

    if constexpr (Tracer::enabled)
        trace.record(snapshot(hull, k, lowerEnd, "Points are sorted by x. Build the lower hull from left to right."));

    const auto popUntilLeftTurn = [&](const Point& p, std::size_t minK, const char* phase) {
        while (k > minK) {
            const bool left = cross(hull[k - 2], hull[k - 1], p) > 0;
            if constexpr (Tracer::enabled) {
                Step s = snapshot(hull, k, lowerEnd,
                      std::string(phase) + ": " +
                          (left ? "left turn at " + str(hull[k - 1]) + " -> keep it."
                                : "no left turn at " + str(hull[k - 1]) + " -> remove it."));
                s.tested = {{hull[k - 2], hull[k - 1]}, {hull[k - 1], p}};
                s.current = {p};
                if (!left) s.rejected = {hull[k - 1]};
                trace.record(std::move(s));
            }
            if (left) break;
            --k;
        }
    };

    // Lower hull
    for (std::size_t i = 0; i < n; ++i) {
        popUntilLeftTurn(pts[i], 1, "Lower hull");
        hull[k] = pts[i];
        k++;
        if constexpr (Tracer::enabled) {
            Step s = snapshot(hull, k, lowerEnd, "Lower hull: add " + str(pts[i]) + ".");
            s.current = {pts[i]};
            trace.record(std::move(s));
        }
    }

    // Upper hull
    lowerEnd = k;
    if constexpr (Tracer::enabled)
        trace.record(snapshot(hull, k, lowerEnd, "Lower hull done. Build the upper hull from right to left."));

    for (std::size_t i = n - 1; i-- > 0;) {
        popUntilLeftTurn(pts[i], lowerEnd, "Upper hull");
        hull[k] = pts[i];
        k++;
        if constexpr (Tracer::enabled) {
            Step s = snapshot(hull, k, lowerEnd,
                i == 0 ? "Upper hull reached the start point again." : "Upper hull: add " + str(pts[i]) + ".");
            s.current = {pts[i]};
            trace.record(std::move(s));
        }
    }

    hull.resize(k - 1);  // the last point is the start point again

    if constexpr (Tracer::enabled) {
        Step s;
        s.description = "Done: the convex hull has " + std::to_string(hull.size()) + " points.";
        s.chains.push_back(hull);
        s.closed = true;
        trace.record(std::move(s));
    }
    return hull;
}

}  // namespace

HullAlgorithm makeAndrew() {
    HullAlgorithm algo;
    algo.id = "andrew";
    algo.name = "Andrew's Monotone Chain";
    algo.compute = [](std::vector<Point> pts) {
        NoTrace trace;
        return andrew(std::move(pts), trace);
    };
    algo.computeSteps = [](std::vector<Point> pts) {
        StepRecorder recorder;
        andrew(std::move(pts), recorder);
        return std::move(recorder.steps);
    };
    return algo;
}
