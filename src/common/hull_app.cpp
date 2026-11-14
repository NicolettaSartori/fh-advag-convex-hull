#include "hull_app.h"

#include <exception>
#include <iostream>
#include <string>

int runHullApp(const HullAlgorithm& algo, int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "";
    try {
        if (mode.empty()) {
            runVisualizer(algo);
            return 0;
        }
        if (mode == "--bench") {
            const int runs = argc > 2 ? std::stoi(argv[2]) : 10;
            if (runs < 1) throw std::invalid_argument("runs must be at least 1");
            runBenchmark(algo, runs);
            return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    std::cout << "Usage:\n"
              << "  " << argv[0] << "                 interactive visualization\n"
              << "  " << argv[0] << " --bench [runs]  time measurements (default 10 runs),\n"
              << "                               results in protocol/" << algo.id << ".csv\n";
    return mode == "--help" ? 0 : 1;
}
