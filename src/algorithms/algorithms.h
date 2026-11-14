#pragma once

// All convex hull algorithms of this project.
// To add one: write algorithms/<name>.cpp with a make<Name>() function,
// declare it here, add it to the list in src/main.cpp and to CMakeLists.txt.

#include "hull_app.h"

HullAlgorithm makeAndrew();
HullAlgorithm makeQuickhull();
