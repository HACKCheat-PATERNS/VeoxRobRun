// Regression tests for whole-pose 3-sample selection (no DMA needed).
// Compile from the repository root:
//   c++ -std=c++17 -Wall -Wextra -pedantic tests/chams_pose_frame_selector.cpp -o pose_frame_test
#include "../VeoxDMA/SDK/PoseFrameSelector.h"
#include <cassert>
#include <cstdio>
#include <vector>

struct Point { float x, y, z; };
struct Segment { Point a, b; };

static Segment Limb(float offset) {
    return {{offset, 1.f, 0.f}, {offset + 0.2f, 0.7f, 0.f}};
}
static void Put(std::vector<Segment> (&frames)[3], int slot, float limb) {
    frames[slot] = {
        {{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}}, // stable torso
        Limb(limb)
    };
}
int main() {
    std::vector<Segment> frames[3];
    Put(frames, 0, 0.f);
    Put(frames, 1, 0.f);
    Put(frames, 2, 0.f);
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 2) == 2);

    // A single torn hand position in the newest sample must not be
    // published together with the newest shoulder rotation.
    Put(frames, 0, 0.f);
    Put(frames, 1, 0.02f);
    Put(frames, 2, 8.f);
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 2) == 1);

    // Real progressive movement selects the actual middle sample
    // rather than fabricating axis-wise bone medians.
    Put(frames, 0, 0.f);
    Put(frames, 1, 0.1f);
    Put(frames, 2, 0.2f);
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 2) == 1);

    // A single torn intermediate sample with healthy newest pose.
    Put(frames, 0, 0.f);
    Put(frames, 1, 5.f);
    Put(frames, 2, 0.f);
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 2) == 2);

    // Ring wrap: on ties prefer the newer of the two sound frames.
    Put(frames, 0, 9.f);   // newest, bad
    Put(frames, 1, 0.f);   // oldest
    Put(frames, 2, 0.02f); // previous, good
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 0) == 2);

    frames[1].clear();
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 0) == 0);
    std::puts("coherent pose selector: OK");
}
