#include "dataset.h"
#include "hull_app.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>

void runBenchmark(const HullAlgorithm& algo, int runs) {
    using Clock = std::chrono::steady_clock;
    const auto msSince = [](Clock::time_point start) {
        return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    };

#ifndef NDEBUG
    std::cout << "WARNING: this is not an optimized build, times will be too slow.\n"
                 "         Configure with: cmake -B build -DCMAKE_BUILD_TYPE=Release\n\n";
#endif

    const std::filesystem::path dir = projectPath("protocol");
    std::filesystem::create_directories(dir);
    const std::filesystem::path file = dir / (algo.id + ".csv");
    std::ofstream out(file);
    if (!out) throw std::runtime_error("Cannot write " + file.string());
    out << "algorithm,dataset,n,hull_points,runs,median_ms,min_ms,max_ms,mean_ms\n";

    std::cout << algo.name << ": " << runs << " runs per test case\n\n";
    std::printf("%-12s %9s %9s %10s %10s %10s %10s\n", "dataset", "n", "hull", "median ms", "min ms",
                "max ms", "load ms");

    for (const TestCase& tc : benchmarkCases()) {
        std::vector<Point> points;
        double loadMs = 0;
        try {
            const auto start = Clock::now();
            points = loadPoints(tc.path);
            loadMs = msSince(start);
        } catch (const std::exception& e) {
            std::printf("%-12s skipped: %s\n", tc.name.c_str(), e.what());
            continue;
        }

        // Warm-up run (caches, page faults), not measured.
        const std::size_t hullSize = algo.compute(points).size();

        std::vector<double> times;
        for (int r = 0; r < runs; ++r) {
            std::vector<Point> input = points;  // fresh copy, not measured
            const auto start = Clock::now();
            const std::vector<Point> hull = algo.compute(std::move(input));
            times.push_back(msSince(start));
            if (hull.size() != hullSize) std::cout << "WARNING: hull size differs between runs\n";
        }

        std::sort(times.begin(), times.end());
        const std::size_t m = times.size() / 2;
        const double median = times.size() % 2 ? times[m] : (times[m - 1] + times[m]) / 2;
        const double mean = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

        std::printf("%-12s %9zu %9zu %10.3f %10.3f %10.3f %10.1f\n", tc.name.c_str(), points.size(),
                    hullSize, median, times.front(), times.back(), loadMs);
        out << algo.id << ',' << tc.name << ',' << points.size() << ',' << hullSize << ',' << runs
            << ',' << median << ',' << times.front() << ',' << times.back() << ',' << mean << '\n';
        out.flush();
    }

    std::cout << "\nResults written to " << file.string() << '\n';
}
