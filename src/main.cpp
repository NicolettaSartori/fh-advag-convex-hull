// Convex hull program with all algorithms.
//   ./build/hull                       menu -> visualization
//   ./build/hull --bench [runs]        time measurements for all algorithms
//   ./build/hull --bench <id> [runs]   only one algorithm (andrew, quickhull)

#include "algorithms/algorithms.h"

int main(int argc, char** argv) {
    return runHullApp({makeAndrew(), makeQuickhull()}, argc, argv);
}
