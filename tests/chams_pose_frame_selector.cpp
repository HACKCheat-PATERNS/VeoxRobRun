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
        {{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}},
        Limb(limb)
    };
}
int main() {
    std::vector<Segment> frames[3];
    Put(frames, 0, 0.f);
    Put(frames, 1, 0.f);
    Put(frames, 2, 0.f);
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 2) == 2);

    Put(frames, 0, 0.f);
    Put(frames, 1, 0.02f);
    Put(frames, 2, 8.f);
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 2) == 1);

    Put(frames, 0, 0.f);
    Put(frames, 1, 0.1f);
    Put(frames, 2, 0.2f);
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 2) == 1);

    Put(frames, 0, 0.f);
    Put(frames, 1, 5.f);
    Put(frames, 2, 0.f);
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 2) == 2);

    Put(frames, 0, 9.f);
    Put(frames, 1, 0.f);
    Put(frames, 2, 0.02f);
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 0) == 2);

    frames[1].clear();
    assert(PoseFrameSelector::SelectCoherentFrame(frames, 0) == 0);

    std::vector<Segment> previous{Limb(0.f), Limb(0.f)};
    std::vector<Segment> current{Limb(0.f), Limb(0.6f)};
    assert(PoseFrameSelector::HasLargeJointDelta(previous, current, 0.25f));
    current[1] = Limb(0.1f);
    assert(!PoseFrameSelector::HasLargeJointDelta(previous, current, 0.25f));
    current.pop_back();
    assert(PoseFrameSelector::HasLargeJointDelta(previous, current, 0.25f));

    std::puts("coherent pose selector and atomic snap: OK");
}
