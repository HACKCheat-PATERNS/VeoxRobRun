#include "../VeoxDMA/SDK/ChamsBoneBinding.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <vector>

namespace {
struct Vertex {
    float x, y, z;
    std::uint8_t bones[4];
    float weights[4];
};
static_assert(sizeof(Vertex) == 32, "Asset vertex layout changed");

template <typename T>
bool Read(std::ifstream& f, T& v) {
    return static_cast<bool>(f.read(reinterpret_cast<char*>(&v), sizeof(T)));
}
}

int main() {
    std::array<int, 94> map{};
    for (int i = 0; i < 94; ++i) map[i] = i;
    for (int i = 6; i <= 38; ++i) map[i] = -1;
    for (int i : {78, 79, 81, 82, 83}) map[i] = -1;

    auto jaw = ChamsBoneBinding::Resolve(32, 94, map.data());
    assert(jaw.skeletonBone == map[4] && jaw.bindPoseBone == 4);
    assert(ChamsBoneBinding::Resolve(6, 94, map.data()).skeletonBone == map[4]);
    assert(ChamsBoneBinding::Resolve(38, 94, map.data()).skeletonBone == map[4]);
    assert(ChamsBoneBinding::Resolve(39, 94, map.data()).skeletonBone == 39);
    assert(ChamsBoneBinding::Resolve(78, 94, map.data()).skeletonBone == -1);
    assert(ChamsBoneBinding::Resolve(6, 93, map.data()).skeletonBone == -1);
    map[4] = -1;
    assert(ChamsBoneBinding::Resolve(32, 94, map.data()).skeletonBone == -1);
    map[4] = 4;

    std::ifstream f("tools/character_mesh.bin", std::ios::binary);
    if (!f) {
        std::fputs("Open tools/character_mesh.bin from repository root\n", stderr);
        return 2;
    }
    char magic[4]{};
    std::uint32_t bones = 0, verts = 0, tris = 0;
    f.read(magic, sizeof(magic));
    if (!Read(f, bones) || !Read(f, verts) || !Read(f, tris) ||
        magic[0] != 'C' || magic[1] != 'H' ||
        magic[2] != 'R' || magic[3] != '1' ||
        bones != 94 || verts == 0 || tris == 0)
        return 3;

    f.seekg(static_cast<std::streamoff>(bones) * (4 + 64), std::ios::cur);
    std::vector<Vertex> vertices(verts);
    std::vector<std::uint16_t> indices(static_cast<std::size_t>(tris) * 3);
    if (!f.read(reinterpret_cast<char*>(vertices.data()), verts * sizeof(Vertex)) ||
        !f.read(reinterpret_cast<char*>(indices.data()),
                indices.size() * sizeof(std::uint16_t)))
        return 4;

    std::vector<bool> oldValid(verts, true), newValid(verts, true);
    for (std::size_t v = 0; v < verts; ++v) {
        for (int influence = 0; influence < 4; ++influence) {
            if (vertices[v].weights[influence] <= 1e-6f) continue;
            const int idx = vertices[v].bones[influence];
            if (idx >= 94) return 5;
            if (map[idx] < 0) oldValid[v] = false;
            if (ChamsBoneBinding::Resolve(idx, bones, map.data()).skeletonBone < 0)
                newValid[v] = false;
        }
    }

    std::uint32_t oldTris = 0, newTris = 0;
    for (std::uint32_t t = 0; t < tris; ++t) {
        const std::size_t p = static_cast<std::size_t>(t) * 3;
        const auto a = indices[p], b = indices[p+1], c = indices[p+2];
        if (a >= verts || b >= verts || c >= verts) return 6;
        if (oldValid[a] && oldValid[b] && oldValid[c]) ++oldTris;
        if (newValid[a] && newValid[b] && newValid[c]) ++newTris;
    }

    std::printf("Asset coverage without/with rigid face: %u/%u vs %u/%u triangles\n",
                oldTris, tris, newTris, tris);
    assert(oldTris * 100 < tris * 93);
    assert(newTris * 100 >= tris * 93);
    std::puts("chams rigid facial binding: OK");
}
