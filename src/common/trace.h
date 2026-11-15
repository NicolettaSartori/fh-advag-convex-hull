#pragma once

// Step recording for the visualization.
//
// An algorithm is written as a template over a "Tracer":
//
//     template <class Tracer>
//     std::vector<Point> myHull(std::vector<Point> pts, Tracer& trace) {
//         ...
//         if constexpr (Tracer::enabled) trace.record(step);
//     }
//
// - StepRecorder stores every step for the visualizer.
// - NoTrace is used for the benchmark. Because of `if constexpr`, the tracing
//   code is removed completely at compile time, so it does not affect the
//   measured times.

#include "geometry.h"

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

struct Segment {
    Point a;
    Point b;
};

// Visual roles. The visualizer decides how a role looks (color, shape),
// each algorithm decides what it means (HullAlgorithm::legend).
enum class Role {
    Hull,       // chains: the (partial) hull
    Tested,     // tested: edges / lines currently being looked at
    Candidate,  // candidates: points that are still in play
    Discarded,  // discarded: points that can no longer be on the hull
    Current,    // current: point(s) currently processed (ring)
    Rejected,   // rejected: point(s) just removed (ring)
};

// A snapshot of the algorithm's state that the visualizer can draw.
// Every field is optional, an algorithm only fills what it needs.
struct Step {
    std::string description;                 // shown in the side panel
    std::vector<std::vector<Point>> chains;  // Role::Hull, drawn as polylines
    bool closed = false;                     // true: chains[0] is a closed polygon and gets filled
    std::vector<Segment> tested;             // Role::Tested
    std::vector<Point> candidates;           // Role::Candidate
    std::vector<Point> discarded;            // Role::Discarded
    std::vector<Point> current;              // Role::Current
    std::vector<Point> rejected;             // Role::Rejected
};

// "(x, y)" with one decimal, for step descriptions.
inline std::string str(const Point& p) {
    char buf[48];
    std::snprintf(buf, sizeof buf, "(%.1f, %.1f)", p.x, p.y);
    return buf;
}

struct NoTrace {
    static constexpr bool enabled = false;
    void record(const Step&) {}
};

struct StepRecorder {
    static constexpr bool enabled = true;
    std::vector<Step> steps;
    void record(Step step) { steps.push_back(std::move(step)); }
};
