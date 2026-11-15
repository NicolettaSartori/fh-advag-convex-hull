#include "algorithms.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace {

using Index = std::ptrdiff_t;

// > 0: p is outside of the directed hull edge a -> b (strictly right of it).
// The edge direction is computed once per loop instead of once per point.
struct Edge {
    Point a;
    double dx;
    double dy;

    Edge(const Point& from, const Point& to) : a(from), dx(to.x - from.x), dy(to.y - from.y) {}

    double outside(const Point& p) const { return dy * (p.x - a.x) - dx * (p.y - a.y); }

    // How far p lies in the direction a -> b (only used to break ties).
    double along(const Point& p) const { return dx * (p.x - a.x) + dy * (p.y - a.y); }
};

struct Partition {
    Index middle;  // first index that is not outside
    Index end;     // last index that is not on the line
};

// Reorders pts[first..last] into three groups (3-way partitioning):
//   [first, middle)   outside of a -> b
//   [middle, end]     strictly inside of a -> b
//   (end, last]       on the line through a and b
Partition partition(std::vector<Point>& pts, Index first, Index last, const Point& a, const Point& b) {
    const Edge edge(a, b);
    Index end = last;
    while (first <= last) {
        const double side = edge.outside(pts[first]);
        if (side > 0) {
            // outside -> stays at the front
            ++first;
        } else if (side < 0) {
            // inside -> swap to the back
            std::swap(pts[first], pts[last]);
            --last;
        } else {
            // on the line -> swap behind the inside points
            const Point temp = pts[first];
            pts[first] = pts[last];
            pts[last] = pts[end];
            pts[end] = temp;
            --last;
            --end;
        }
    }
    return {first, end};
}

template <class Tracer>
class Quickhull {
public:
    Quickhull(std::vector<Point>& pts, Tracer& trace) : pts_(pts), trace_(trace) {}

    std::vector<Point> run() {
        const Index n = static_cast<Index>(pts_.size());
        if (n == 0) return {};

        // Leftmost (smallest x, then smallest y) and rightmost (largest x, then largest y) point.
        Index minI = 0;
        Index maxI = 0;
        for (Index i = 1; i < n; ++i) {
            if (pts_[i] < pts_[minI]) minI = i;
            if (pts_[maxI] < pts_[i]) maxI = i;
        }

        if (pts_[minI] == pts_[maxI]) {
            // All points are identical.
            hull_.push_back(pts_[minI]);
            if constexpr (Tracer::enabled) record(finalStep("All points are identical: the hull is one point."));
            return hull_;
        }

        // Move A to the front and B to the back, the rest is in between.
        std::swap(pts_[0], pts_[minI]);
        if (maxI == 0) maxI = minI;
        std::swap(pts_[n - 1], pts_[maxI]);
        const Point a = pts_[0];
        const Point b = pts_[n - 1];

        if constexpr (Tracer::enabled) {
            known_ = {a, b};
            Step s = snapshot("Leftmost point A " + str(a) + " and rightmost point B " + str(b) +
                              " are on the hull.");
            s.current = {a, b};
            record(std::move(s));
        }

        // Below the line A-B (outside of A -> B) | above the line | on the line
        const Partition p = partition(pts_, 1, n - 2, a, b);

        if constexpr (Tracer::enabled) {
            discard(p.end + 1, n - 2);
            Step s = snapshot("The line A-B splits the points: " + std::to_string(p.middle - 1) + " below, " +
                              std::to_string(p.end - p.middle + 1) + " above. Both sides are solved separately.");
            s.tested = {{a, b}};
            record(std::move(s));
        }

        hull_.reserve(64);
        hull_.push_back(a);
        findHull(1, p.middle - 1, a, b);  // lower hull, from A to B
        hull_.push_back(b);
        findHull(p.middle, p.end, b, a);  // upper hull, from B back to A

        if constexpr (Tracer::enabled)
            record(finalStep("Done: the convex hull has " + std::to_string(hull_.size()) + " points."));
        return std::move(hull_);
    }

private:
    // pts_[first..last] are exactly the points outside of the edge a -> b.
    // Appends the hull points between a and b (in this order) to hull_.
    void findHull(Index first, Index last, const Point& a, const Point& b) {
        if (first > last) {
            if constexpr (Tracer::enabled) {
                Step s = snapshot("No points outside of " + str(a) + " -> " + str(b) + ": this is a hull edge.");
                s.tested = {{a, b}};
                record(std::move(s));
            }
            return;
        }

        if (first == last) {
            // Only one point outside: it must be on the hull.
            const Point c = pts_[first];
            if constexpr (Tracer::enabled) {
                insertKnown(a, c);
                Step s = snapshot("Only one point outside of " + str(a) + " -> " + str(b) + ": " + str(c) +
                                  " is a hull point.");
                s.tested = {{a, c}, {c, b}};
                s.current = {c};
                record(std::move(s));
            }
            hull_.push_back(c);
            return;
        }

        // Farthest point from the edge (largest cross product = largest distance).
        // Tie: several points on a line parallel to the edge. Only the two ends of
        // that line are hull points, so take the one farthest in direction a -> b
        // (otherwise a point in the middle of a hull edge could be chosen).
        const Edge edge(a, b);
        Index cIndex = first;
        double maxSide = 0;
        for (Index i = first; i <= last; ++i) {
            const double side = edge.outside(pts_[i]);
            if (side > maxSide || (side == maxSide && edge.along(pts_[i]) > edge.along(pts_[cIndex]))) {
                maxSide = side;
                cIndex = i;
            }
        }
        if (maxSide <= 0) return;  // cannot happen: all points in the range are outside

        if constexpr (Tracer::enabled) {
            Step s = snapshot(std::to_string(last - first + 1) + " points outside of " + str(a) + " -> " + str(b) +
                              ". The farthest one, " + str(pts_[cIndex]) + ", is on the hull.");
            s.tested = {{a, b}};
            s.current = {pts_[cIndex]};
            record(std::move(s));
        }

        // Move C to the end of the range, it is not part of the subproblems.
        std::swap(pts_[last], pts_[cIndex]);
        const Point c = pts_[last];
        --last;

        // [first, left.middle)          outside of A -> C
        // [left.middle, right.middle)   outside of C -> B
        // [right.middle, last]          inside the triangle A-C-B (or on its border): dropped
        const Partition left = partition(pts_, first, last, a, c);
        const Partition right = partition(pts_, left.middle, left.end, c, b);

        if constexpr (Tracer::enabled) {
            insertKnown(a, c);
            discard(right.middle, last);
            Step s = snapshot(std::to_string(last - right.middle + 1) + " points inside the triangle are dropped. " +
                              "Remaining: " + std::to_string(left.middle - first) + " outside of " + str(a) +
                              " -> " + str(c) + ", " + std::to_string(right.middle - left.middle) +
                              " outside of " + str(c) + " -> " + str(b) + ".");
            s.tested = {{a, c}, {c, b}, {a, b}};
            s.current = {c};
            record(std::move(s));
        }

        findHull(first, left.middle - 1, a, c);
        hull_.push_back(c);
        findHull(left.middle, right.middle - 1, c, b);
    }

    // ---- visualization only (never called with NoTrace) --------------------

    void record(Step s) { trace_.record(std::move(s)); }

    void discard(Index first, Index last) {
        for (Index i = first; i <= last; ++i) discarded_.push_back(pts_[i]);
    }

    // known_ holds the hull points found so far in counter-clockwise order.
    // A new hull point c always lies between the two ends of the edge a -> b.
    void insertKnown(const Point& a, const Point& c) {
        for (std::size_t i = 0; i < known_.size(); ++i) {
            if (known_[i] == a) {
                known_.insert(known_.begin() + static_cast<Index>(i) + 1, c);
                return;
            }
        }
    }

    // Current state: hull found so far, dropped points, and all points that are
    // still undecided (neither on the known hull nor dropped).
    Step snapshot(std::string text) const {
        Step s;
        s.description = std::move(text);
        s.chains.push_back(known_);
        s.closed = true;
        s.discarded = discarded_;

        std::vector<Point> decided = discarded_;
        decided.insert(decided.end(), known_.begin(), known_.end());
        std::sort(decided.begin(), decided.end());
        for (const Point& p : pts_)
            if (!std::binary_search(decided.begin(), decided.end(), p)) s.candidates.push_back(p);
        return s;
    }

    Step finalStep(std::string text) const {
        Step s;
        s.description = std::move(text);
        s.chains.push_back(hull_);
        s.closed = true;
        s.discarded = discarded_;
        return s;
    }

    std::vector<Point>& pts_;
    Tracer& trace_;
    std::vector<Point> hull_;
    std::vector<Point> known_;      // visualization only
    std::vector<Point> discarded_;  // visualization only
};

template <class Tracer>
std::vector<Point> quickhull(std::vector<Point> pts, Tracer& trace) {
    return Quickhull<Tracer>(pts, trace).run();
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
        {Role::Candidate, "Points still outside"},
        {Role::Discarded, "Dropped points (inside)"},
    };
    algo.compute = [](std::vector<Point> pts) {
        NoTrace trace;
        return quickhull(std::move(pts), trace);
    };
    algo.computeSteps = [](std::vector<Point> pts) {
        StepRecorder recorder;
        quickhull(std::move(pts), recorder);
        return std::move(recorder.steps);
    };
    return algo;
}
