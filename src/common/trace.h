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

#include <string>
#include <utility>
#include <vector>

struct Segment {
    Point a;
    Point b;
};

// A snapshot of the algorithm's state that the visualizer can draw.
struct Step {
    std::string description;                 // shown in the side panel
    std::vector<std::vector<Point>> chains;  // current (partial) hull, drawn as polylines
    std::vector<Segment> tested;             // edges currently being checked
    std::vector<Point> current;              // point(s) currently processed
    std::vector<Point> rejected;             // point(s) just removed / discarded
    bool closed = false;                     // true: chains[0] is the finished hull polygon
};

struct NoTrace {
    static constexpr bool enabled = false;
    void record(const Step&) {}
};

struct StepRecorder {
    static constexpr bool enabled = true;
    std::vector<Step> steps;
    void record(Step step) { steps.push_back(std::move(step)); }
};
