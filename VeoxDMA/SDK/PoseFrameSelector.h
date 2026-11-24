#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace PoseFrameSelector {

template <typename Segment>
int SelectCoherentFrame(const std::vector<Segment> (&frames)[3], int newest)
{
    if (newest < 0 || newest >= 3 || frames[0].empty() ||
        frames[0].size() != frames[1].size() ||
        frames[0].size() != frames[2].size())
        return newest;


    auto valid = [](const auto& s) {
        const auto finite = [](const auto& p) {
            return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
        };
        return finite(s.a) && finite(s.b);
    };
    bool validFrame[3]{true, true, true};
    for (int frame = 0; frame < 3; ++frame)
        for (const auto& segment : frames[frame])
            if (!valid(segment)) {
                validFrame[frame] = false;
                break;
            }
    if (!validFrame[0] || !validFrame[1] || !validFrame[2]) {
        for (int age = 0; age < 3; ++age) {
            const int index = (newest + 3 - age) % 3;
            if (validFrame[index]) return index;
        }
        return newest;
    }

    auto deviation = [](const auto& a, const auto& b) -> double {
        const double dx = static_cast<double>(a.x) - b.x;
        const double dy = static_cast<double>(a.y) - b.y;
        const double dz = static_cast<double>(a.z) - b.z;
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

    int chosen = newest;
    double best = distance[newest][(newest + 1) % 3] +
                  distance[newest][(newest + 2) % 3];
    for (int age = 0; age < 3; ++age) {
        const int i = (newest + 3 - age) % 3;
        const double score = distance[i][(i + 1) % 3] +
                             distance[i][(i + 2) % 3];
        if (score + 1e-8 < best) {
            best = score;
            chosen = i;
        }
    }
    return chosen;
}

template <typename Segment>
bool HasLargeJointDelta(const std::vector<Segment>& previous,
                        const std::vector<Segment>& current,
                        float maxDeltaSq)
{
    if (previous.size() != current.size() ||
        !std::isfinite(maxDeltaSq) || maxDeltaSq < 0.f)
        return true;
    auto large = [maxDeltaSq](const auto& a, const auto& b) {
        const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        const float distanceSq = dx*dx + dy*dy + dz*dz;
        return !std::isfinite(distanceSq) || distanceSq > maxDeltaSq;
    };
    for (std::size_t i = 0; i < current.size(); ++i) {
        if (large(previous[i].a, current[i].a) ||
            large(previous[i].b, current[i].b))
            return true;
    }
    return false;
}

}
