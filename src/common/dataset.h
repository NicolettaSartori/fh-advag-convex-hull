#pragma once

// Reading test data.
//
// File format:
//   line 1:        number of points n
//   lines 2..n+1:  x,y

#include "geometry.h"

#include <cstddef>
#include <string>
#include <vector>

// All test data uses coordinates in [0, WORLD_SIZE].
constexpr double WORLD_SIZE = 1000.0;

// Resolves a path relative to the project folder (src/), so the program
// finds data/ and protocol/ no matter which directory it is started from.
std::string projectPath(const std::string& relative);

// Throws std::runtime_error if the file is missing or malformed.
std::vector<Point> loadPoints(const std::string& path);

// n random points, evenly spread with a small margin inside the world square.
std::vector<Point> randomPoints(std::size_t n, unsigned seed);

struct TestCase {
    std::string name;  // e.g. "square_1k"
    std::string path;  // full path to the csv file
};

// All benchmark files: {square, disk, circle} x {1k, 10k, 100k, 1m}.
std::vector<TestCase> benchmarkCases();
