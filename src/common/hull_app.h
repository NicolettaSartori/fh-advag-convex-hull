#pragma once

// Framework for convex hull programs: visual mode + benchmark mode.
//
// An algorithm only has to fill in a HullAlgorithm and call runHullApp()
// from its main(). See algorithms/andrew.cpp for an example.

#include "geometry.h"
#include "trace.h"

#include <functional>
#include <string>
#include <vector>

struct HullAlgorithm {
    std::string id;    // short name, used for the protocol file: protocol/<id>.csv
    std::string name;  // display name

    // Fast version without tracing (used for time measurements).
    // Returns the hull points in counter-clockwise order.
    std::function<std::vector<Point>(std::vector<Point>)> compute;

    // Version that records every step (used for the visualization).
    std::function<std::vector<Step>(std::vector<Point>)> computeSteps;
};

// Parses the command line and starts the matching mode:
//   (no arguments)   interactive visualization
//   --bench [runs]   time measurements for all test files, default 10 runs
int runHullApp(const HullAlgorithm& algo, int argc, char** argv);

void runVisualizer(const HullAlgorithm& algo);
void runBenchmark(const HullAlgorithm& algo, int runs);
