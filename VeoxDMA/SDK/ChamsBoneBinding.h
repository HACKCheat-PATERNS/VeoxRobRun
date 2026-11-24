#pragma once
#include <cstdint>

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

    constexpr std::uint32_t kHeadBone = 4;
    if (meshBoneCount == 94 && meshBone >= 6 && meshBone <= 38 &&
        boneMap[kHeadBone] >= 0)
        return {boneMap[kHeadBone], kHeadBone};
    return {};
}

}
