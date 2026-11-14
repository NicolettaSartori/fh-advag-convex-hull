#include "dataset.h"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>

std::string projectPath(const std::string& relative) {
#ifdef HULL_PROJECT_DIR
    return std::string(HULL_PROJECT_DIR) + "/" + relative;
#else
    return relative;
#endif
}

std::vector<Point> loadPoints(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open " + path);

    // Read the whole file at once and parse it with strtod: much faster than
    // getline + stringstream for a million lines.
    const std::string buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const char* p = buffer.c_str();
    char* end = nullptr;

    const long n = std::strtol(p, &end, 10);
    if (end == p || n < 0) throw std::runtime_error(path + ": first line must be the number of points");
    p = end;

    std::vector<Point> points;
    points.reserve(static_cast<std::size_t>(n));
    for (long i = 0; i < n; ++i) {
        const auto fail = [&] {
            throw std::runtime_error(path + ": invalid point in line " + std::to_string(i + 2));
        };
        const double x = std::strtod(p, &end);
        if (end == p) fail();
        p = end;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p != ',') fail();
        ++p;
        const double y = std::strtod(p, &end);
        if (end == p) fail();
        p = end;
        points.push_back({x, y});
    }
    return points;
}

std::vector<Point> randomPoints(std::size_t n, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(0.05 * WORLD_SIZE, 0.95 * WORLD_SIZE);
    std::vector<Point> points(n);
    for (auto& p : points) p = {dist(rng), dist(rng)};
    return points;
}

std::vector<Point> loadTestCase(const TestCase& tc) {
    return tc.path.empty() ? randomPoints(tc.randomCount, tc.seed) : loadPoints(tc.path);
}

std::vector<TestCase> benchmarkCases() {
    const std::pair<const char*, std::size_t> sizes[] = {
        {"1k", 1'000}, {"10k", 10'000}, {"100k", 100'000}, {"1m", 1'000'000}};

    std::vector<TestCase> cases;
    for (const char* dist : {"square", "disk", "circle"}) {
        for (const auto& [label, count] : sizes) {
            const std::string name = std::string(dist) + "_" + label;
            cases.push_back({name, projectPath("data/" + name + ".csv")});
        }
    }
    for (const auto& [label, count] : sizes) {
        constexpr unsigned SEED = 12345;
        cases.push_back({std::string("random_") + label, "", count, SEED});
    }
    return cases;
}
