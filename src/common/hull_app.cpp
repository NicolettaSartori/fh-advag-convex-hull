#include "hull_app.h"

#include <cctype>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void runBenchmarks(const std::vector<HullAlgorithm>& algos, const std::string& only, int runs) {
    bool found = false;
    for (const HullAlgorithm& algo : algos) {
        if (!only.empty() && algo.id != only) continue;
        found = true;
        if (!algo.implemented()) {
            std::cout << algo.name << ": not implemented yet, skipped.\n\n";
            continue;
        }
        runBenchmark(algo, runs);
        std::cout << '\n';
    }
    if (!found) throw std::invalid_argument("unknown algorithm: " + only);
}

}  // namespace

int runHullApp(const std::vector<HullAlgorithm>& algos, int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "";
    try {
        if (mode.empty()) {
            runVisualizer(algos);
            return 0;
        }
        if (mode == "--bench") {
            std::string only;
            int runs = 10;
            for (int i = 2; i < argc; ++i) {
                const std::string arg = argv[i];
                if (std::isdigit(static_cast<unsigned char>(arg[0])))
                    runs = std::stoi(arg);
                else
                    only = arg;
            }
            if (runs < 1) throw std::invalid_argument("runs must be at least 1");
            runBenchmarks(algos, only, runs);
            return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    std::cout << "Usage:\n"
              << "  " << argv[0] << "                      menu -> visualization\n"
              << "  " << argv[0] << " --bench [runs]       time measurements for all algorithms\n"
              << "  " << argv[0] << " --bench <id> [runs]  only one algorithm, ids:";
    for (const HullAlgorithm& algo : algos) std::cout << ' ' << algo.id;
    std::cout << "\nResults are written to protocol/<id>.csv\n";
    return mode == "--help" ? 0 : 1;
}
