#include "algorithms.h"

#include "Coordinate2d.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

// ---- visualization helpers (only used when tracing) ------------------------

Point toPoint(const Coordinate2d& c) {
    return {c.x, c.y};
}

std::vector<Point> toPoints(const std::vector<Coordinate2d>& cs) {
    std::vector<Point> result;
    result.reserve(cs.size());
    for (const Coordinate2d& c : cs) result.push_back(toPoint(c));
    return result;
}

std::string str(const Coordinate2d& c) {
    return ::str(toPoint(c));
}

// Wraps StepRecorder and keeps the extra state the step view needs:
// hull points found so far (in order) and the points that were dropped.
template <class Tracer>
struct QuickhullTrace {
    static constexpr bool enabled = Tracer::enabled;

    Tracer& out;
    const std::vector<Coordinate2d>* points = nullptr;
    std::vector<Point> known;      // hull points found so far, in hull order
    std::vector<Point> discarded;  // points that can no longer be on the hull

    void discard(int first, int last) {
        for (int i = first; i <= last; ++i) discarded.push_back(toPoint((*points)[static_cast<std::size_t>(i)]));
    }

    // A new hull point c always lies between a and b: insert it right after a.
    void addHullPoint(const Coordinate2d& a, const Coordinate2d& c) {
        const auto it = std::find(known.begin(), known.end(), toPoint(a));
        known.insert(it == known.end() ? known.end() : it + 1, toPoint(c));
    }

    void record(std::string text, std::vector<Segment> tested = {}, std::vector<Point> current = {}) {
        Step s;
        s.description = std::move(text);
        s.chains.push_back(known);
        s.closed = true;
        s.tested = std::move(tested);
        s.current = std::move(current);
        s.discarded = discarded;

        // Still undecided: neither a known hull point nor dropped.
        std::vector<Point> decided = discarded;
        decided.insert(decided.end(), known.begin(), known.end());
        std::sort(decided.begin(), decided.end());
        for (const Coordinate2d& c : *points)
            if (!std::binary_search(decided.begin(), decided.end(), toPoint(c))) s.candidates.push_back(toPoint(c));
        out.record(std::move(s));
    }

    void recordFinal(const std::vector<Coordinate2d>& hull, std::string text) {
        Step s;
        s.description = std::move(text);
        s.chains.push_back(toPoints(hull));
        s.closed = true;
        s.discarded = discarded;
        out.record(std::move(s));
    }
};

// ---- Georg's algorithm -------------------------------------------------------

struct partitionResult {
    int middle; // last index left of ab
    int end;    // last index that isn't collinear
};

template<typename A>
partitionResult partition(
    A& points,
    int first,
    int last,
    const Coordinate2d a,
    const Coordinate2d b
) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    int end = last; // last element that isn't collinear with AB
    while (first <= last) {
        if (const float result = crossProductEdge(a, dx, dy, points[static_cast<std::size_t>(first)]); result > 0) {
            // left of AB (top) -> leave in place
            first++;
        }
        else if (result < 0) {
            // right of AB (bottom) -> swap to right
            std::swap(points[static_cast<std::size_t>(first)], points[static_cast<std::size_t>(last)]);
            --last;
        }
        else {
            // collinear on AB -> swap to back
            Coordinate2d temp = points[static_cast<std::size_t>(first)];
            points[static_cast<std::size_t>(first)] = points[static_cast<std::size_t>(last)];
            points[static_cast<std::size_t>(last)] = points[static_cast<std::size_t>(end)];
            points[static_cast<std::size_t>(end)] = temp;
            --last;
            --end;
        }
    }

    return partitionResult{.middle = first, .end = end};
}

template<typename A, typename T>
void findHullPoints(
    A& points,
    int first,
    int last,
    Coordinate2d a,
    Coordinate2d b,
    std::vector<Coordinate2d>& hullPoints,
    T& trace
) {
    if (first > last) {
        if constexpr (T::enabled)
            trace.record("No points left of " + str(a) + " -> " + str(b) + ": this is a hull edge.",
                         {{toPoint(a), toPoint(b)}});
        return;
    }
    if (first == last) {
        if constexpr (T::enabled) {
            const Coordinate2d c = points[static_cast<std::size_t>(last)];
            trace.addHullPoint(a, c);
            trace.record("Only one point left of " + str(a) + " -> " + str(b) + ": " + str(c) + " is a hull point.",
                         {{toPoint(a), toPoint(c)}, {toPoint(c), toPoint(b)}}, {toPoint(c)});
        }
        hullPoints.push_back(points[static_cast<std::size_t>(last)]);
        return;
    }

    // find outermost point
    int cIndex = first;
    float maxCP = 0.0f;
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;

    for (int i = first; i <= last; ++i) {
        if (const float cp = crossProductEdge(a, dx, dy, points[static_cast<std::size_t>(i)]); cp > maxCP) {
            cIndex = i;
            maxCP = cp;
        }
    }

    if (maxCP <= 0.0f) {
        return;
    }

    // swap c to last index
    std::swap(points[static_cast<std::size_t>(last)], points[static_cast<std::size_t>(cIndex)]);
    cIndex = last;
    last--;
    Coordinate2d c = points[static_cast<std::size_t>(cIndex)];

    if constexpr (T::enabled) {
        trace.addHullPoint(a, c);
        trace.record(std::to_string(last - first + 2) + " points left of " + str(a) + " -> " + str(b) +
                         ". The farthest one, " + str(c) + ", is on the hull.",
                     {{toPoint(a), toPoint(b)}, {toPoint(a), toPoint(c)}, {toPoint(c), toPoint(b)}}, {toPoint(c)});
    }

    // partition again
    partitionResult result = partition(points, first, last, a, c);

    if constexpr (T::enabled) {
        trace.record(std::to_string(result.middle - first) + " points left of " + str(a) + " -> " + str(c) +
                         ": solve this side first.",
                     {{toPoint(a), toPoint(c)}}, {toPoint(c)});
    }

    findHullPoints(points, first, result.middle - 1, a, c, hullPoints, trace);

    hullPoints.push_back(c);

    first = result.middle;

    result = partition(points, first, result.end, c, b);

    if constexpr (T::enabled) {
        trace.discard(result.middle, last);  // inside the triangle A-C-B or on its border
        trace.record(std::to_string(result.middle - first) + " points left of " + str(c) + " -> " + str(b) +
                         ", the rest is inside the triangle and dropped.",
                     {{toPoint(c), toPoint(b)}}, {toPoint(c)});
    }

    findHullPoints(points, first, result.middle - 1, c, b, hullPoints, trace);
}

template<typename A, typename T>
std::vector<Coordinate2d> calculateQuickhullPoints(
    A& points,
    T& trace
) {
    if constexpr (T::enabled) trace.points = &points;

    if (points.size() < 2) {
        if constexpr (T::enabled) trace.recordFinal({}, "Fewer than 2 points: no hull.");
        return {};
    }

    int smallestXIndex = 0;
    int biggestXIndex = static_cast<int>(points.size()) - 1;

    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto idx = static_cast<int>(i);
        const auto& p = points[i];
        const auto& smallest = points[static_cast<std::size_t>(smallestXIndex)];
        const auto& biggest = points[static_cast<std::size_t>(biggestXIndex)];

        if (p.x < smallest.x || (p.x == smallest.x && p.y < smallest.y)) {
            smallestXIndex = idx;
        }
        if (p.x > biggest.x || (p.x == biggest.x && p.y > biggest.y)) {
            biggestXIndex = idx;
        }
    }

    const Coordinate2d& minP = points[static_cast<std::size_t>(smallestXIndex)];
    const Coordinate2d& maxP = points[static_cast<std::size_t>(biggestXIndex)];
    if (minP.x == maxP.x && minP.y == maxP.y) {
        std::cerr << "All points are identical!" << std::endl;
        if constexpr (T::enabled) trace.recordFinal({minP}, "All points are identical: the hull is one point.");
        return {minP};
    }

    std::swap(points[0], points[static_cast<std::size_t>(smallestXIndex)]);
    if (biggestXIndex == 0) {
        biggestXIndex = smallestXIndex;
    }
    smallestXIndex = 0;
    const int lastIndex = static_cast<int>(points.size()) - 1;
    std::swap(points[static_cast<std::size_t>(lastIndex)], points[static_cast<std::size_t>(biggestXIndex)]);
    biggestXIndex = lastIndex;

    Coordinate2d a = points[static_cast<std::size_t>(smallestXIndex)];
    Coordinate2d b = points[static_cast<std::size_t>(biggestXIndex)];

    if constexpr (T::enabled) {
        trace.known = {toPoint(a), toPoint(b)};
        trace.record("Leftmost point A " + str(a) + " and rightmost point B " + str(b) + " are on the hull.", {},
                     {toPoint(a), toPoint(b)});
    }

    partitionResult result = partition(points, 1, lastIndex - 1, a, b);

    if constexpr (T::enabled) {
        trace.discard(result.end + 1, lastIndex - 1);  // on the line A-B
        trace.record("The line A-B splits the points: " + std::to_string(result.middle - 1) +
                         " above (left of A -> B), " + std::to_string(result.end - result.middle + 1) +
                         " below. Both sides are solved separately.",
                     {{toPoint(a), toPoint(b)}});
    }

    std::vector<Coordinate2d> hullPoints;
    hullPoints.reserve(std::min<std::size_t>(points.size(), 4096));
    hullPoints.push_back(a);

    findHullPoints(points, 1, result.middle - 1, a, b, hullPoints, trace);

    hullPoints.push_back(b);
    findHullPoints(points, result.middle, result.end, b, a, hullPoints, trace);

    if constexpr (T::enabled)
        trace.recordFinal(hullPoints, "Done: the convex hull has " + std::to_string(hullPoints.size()) + " points.");
    return hullPoints;
}

// ---- adapter: Point (double) -> Coordinate2d (float) -------------------------

std::vector<Coordinate2d> toCoordinates(const std::vector<Point>& pts) {
    std::vector<Coordinate2d> result(pts.size());
    for (std::size_t i = 0; i < pts.size(); ++i)
        result[i] = {static_cast<float>(pts[i].x), static_cast<float>(pts[i].y)};
    return result;
}

}  // namespace

HullAlgorithm makeQuickhull() {
    HullAlgorithm algo;
    algo.id = "quickhull";
    algo.name = "Quickhull";
    algo.legend = {
        {Role::Hull, "Hull points found so far"},
        {Role::Tested, "Edge / triangle being checked"},
        {Role::Current, "Farthest point (new hull point)"},
        {Role::Candidate, "Points not decided yet"},
        {Role::Discarded, "Dropped points (inside)"},
    };
    algo.compute = [](std::vector<Point> pts) {
        std::vector<Coordinate2d> points = toCoordinates(pts);
        NoTrace trace;
        return toPoints(calculateQuickhullPoints(points, trace));
    };
    algo.computeSteps = [](std::vector<Point> pts) {
        std::vector<Coordinate2d> points = toCoordinates(pts);
        StepRecorder recorder;
        QuickhullTrace<StepRecorder> trace{recorder, nullptr, {}, {}};
        calculateQuickhullPoints(points, trace);
        return std::move(recorder.steps);
    };
    return algo;
}
