// Quickhull algorithm
//
// TODO: implement it like andrew() in andrew.cpp:
//   template <class Tracer>
//   std::vector<Point> quickhull(std::vector<Point> pts, Tracer& trace) { ... }
// and set algo.compute / algo.computeSteps below.
// As long as they are not set, the menu shows a TODO page and the benchmark skips it.

#include "algorithms.h"

HullAlgorithm makeQuickhull() {
    HullAlgorithm algo;
    algo.id = "quickhull";
    algo.name = "Quickhull";
    return algo;
}
