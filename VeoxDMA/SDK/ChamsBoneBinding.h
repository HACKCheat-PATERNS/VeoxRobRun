#pragma once
#include <cstdint>

// The packaged 94-bone character mesh contains facial geometry weighted to
// 33 small facial joints. The read-only rig intentionally does not resolve
// those facial joints: mapping them to -1 is not a transient DMA failure.
// Rigidly attach their vertices to the head's own bind-pose transform, rather
// than dropping the face or borrowing an unrelated facial bind pose.
namespace ChamsBoneBinding {

struct Binding {
    int skeletonBone = -1;
    std::uint32_t bindPoseBone = 0;
};

inline Binding Resolve(std::uint32_t meshBone, std::uint32_t meshBoneCount,
                       const int* boneMap)
{
    if (!boneMap || meshBone >= meshBoneCount)
        return {};
    const int mapped = boneMap[meshBone];
    if (mapped >= 0)
        return {mapped, meshBone};

    constexpr std::uint32_t kHeadBone = 4; // M_head_1_jnt
    if (meshBoneCount == 94 && meshBone >= 6 && meshBone <= 38 &&
        boneMap[kHeadBone] >= 0)
        return {boneMap[kHeadBone], kHeadBone};
    return {};
}

} // namespace ChamsBoneBinding
