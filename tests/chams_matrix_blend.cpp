// Standalone portable regression for rigid pose blending.
// c++ -std=c++17 tests/chams_matrix_blend.cpp -o chams_matrix_blend
#include "../VeoxDMA/SDK/RigidMatrixBlend.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

static Matrix4x4 Identity() {
    Matrix4x4 m{};
    for (int i=0;i<4;++i) m.matrix[i][i] = 1.f;
    return m;
}
static Matrix4x4 RotateZ90() {
    auto m = Identity();
    m.matrix[0][0]=0.f; m.matrix[0][1]=1.f;
    m.matrix[1][0]=-1.f; m.matrix[1][1]=0.f;
    return m;
}
static bool Near(float a, float b) { return std::abs(a-b) < 1e-4f; }
int main() {
    Matrix4x4 old = Identity();
    Matrix4x4 current = RotateZ90();
    old.matrix[3][0] = 2.f;
    current.matrix[3][0] = 4.f;
    const auto mid = RigidMatrixBlend::Blend(old, current, 0.5f);
    const float s = std::sqrt(0.5f);
    assert(Near(mid.matrix[0][0], s));
    assert(Near(mid.matrix[0][1], s));
    assert(Near(mid.matrix[1][0], -s));
    assert(Near(mid.matrix[1][1], s));
    assert(Near(mid.matrix[3][0], 3.f));
    // The interpolated rotation remains orthonormal, unlike element-wise LERP.
    const float len = mid.matrix[0][0]*mid.matrix[0][0] +
                      mid.matrix[0][1]*mid.matrix[0][1];
    assert(Near(len, 1.f));
    // No extrapolation and an invalid previous pose does not contaminate output.
    const auto atEnd = RigidMatrixBlend::Blend(old, current, 1.f);
    assert(Near(atEnd.matrix[0][0], current.matrix[0][0]));
    const auto atStart = RigidMatrixBlend::Blend(old, current, 0.f);
    assert(Near(atStart.matrix[3][0], old.matrix[3][0]));
    old.matrix[0][0] = std::numeric_limits<float>::quiet_NaN();
    const auto recovered = RigidMatrixBlend::Blend(old, current, 0.5f);
    assert(Near(recovered.matrix[0][0], current.matrix[0][0]));
    std::puts("chams rigid-matrix blend: OK");
}
