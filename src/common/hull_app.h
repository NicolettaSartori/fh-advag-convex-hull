#pragma once

// Framework for convex hull programs: visual mode + benchmark mode.
//
// An algorithm only has to fill in a HullAlgorithm (see algorithms/andrew.cpp).
// main() passes the list of all algorithms to runHullApp().

#include "geometry.h"
#include "trace.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

struct HullAlgorithm {
    std::string id;    // short name, used for the protocol file: protocol/<id>.csv
    std::string name;  // display name

    // Fast version without tracing (used for time measurements).
    // Returns the hull points in counter-clockwise order.
    std::function<std::vector<Point>(std::vector<Point>)> compute;

    // Version that records every step (used for the visualization).
    std::function<std::vector<Step>(std::vector<Point>)> computeSteps;

    // Legend shown in the visualizer: what each role means for this algorithm.
    // Only the roles listed here are explained in the side panel.
    std::vector<std::pair<Role, std::string>> legend;

    // Not implemented yet: the menu shows a TODO page, the benchmark skips it.
    bool implemented() const { return compute && computeSteps; }
};

// Parses the command line and starts the matching mode:
//   (no arguments)        menu to choose an algorithm, then its visualization
//   --bench [runs]        time measurements for all algorithms, default 10 runs
//   --bench <id> [runs]   only for one algorithm
int runHullApp(const std::vector<HullAlgorithm>& algos, int argc, char** argv);

void runVisualizer(const std::vector<HullAlgorithm>& algos);
void runBenchmark(const HullAlgorithm& algo, int runs);
