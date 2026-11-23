#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

// Select a WHOLE already-sampled pose, rather than taking independent medians
// for every axis and bone. Mixing bone translations from three different poses
// with the newest rotations breaks FK and can pull arms or torso off the mesh.
namespace PoseFrameSelector {

template <typename Segment>
int SelectCoherentFrame(const std::vector<Segment> (&frames)[3], int newest)
{
    if (newest < 0 || newest >= 3 || frames[0].empty() ||
        frames[0].size() != frames[1].size() ||
        frames[0].size() != frames[2].size())
        return newest;

    auto deviation = [](const auto& a, const auto& b) -> double {
        const double dx = static_cast<double>(a.x) - b.x;
        const double dy = static_cast<double>(a.y) - b.y;
        const double dz = static_cast<double>(a.z) - b.z;
        // Clip a single torn joint's contribution so it cannot dominate
        // all the healthy bones. Compare pelvis-relative coordinates.
        return (std::min)(dx*dx + dy*dy + dz*dz, 0.64);
    };

    double distance[3][3]{};
    for (int a = 0; a < 3; ++a) {
        for (int b = a + 1; b < 3; ++b) {
            double d = 0.0;
            for (std::size_t i = 0; i < frames[a].size(); ++i) {
                d += deviation(frames[a][i].a, frames[b][i].a);
                d += deviation(frames[a][i].b, frames[b][i].b);
            }
            if (!std::isfinite(d)) return newest;
            distance[a][b] = distance[b][a] = d;
        }
    }

    // The medoid is an actual sampled frame, including its rotations.
    // Prefer newest on ties to avoid adding latency to a stationary pose.
    int chosen = newest;
    double best = distance[newest][(newest + 1) % 3] +
                  distance[newest][(newest + 2) % 3];
    for (int i = 0; i < 3; ++i) {
        const double score = distance[i][(i + 1) % 3] +
                             distance[i][(i + 2) % 3];
        if (score + 1e-8 < best) {
            best = score;
            chosen = i;
        }
    }
    return chosen;
}

} // namespace PoseFrameSelector
