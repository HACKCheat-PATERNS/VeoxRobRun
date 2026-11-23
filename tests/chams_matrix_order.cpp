#include "../VeoxDMA/SDK/Matrix4x4.h"
#include <cmath>
#include <cstdio>

namespace {
struct Point { float x, y, z; };

Matrix4x4 Identity() {
    Matrix4x4 m{};
    for (int i = 0; i < 4; ++i) m.matrix[i][i] = 1.f;
    return m;
}

Matrix4x4 Translation(float x, float y, float z) {
    Matrix4x4 m = Identity();
    m.matrix[3][0] = x;
    m.matrix[3][1] = y;
    m.matrix[3][2] = z;
    return m;
}

Matrix4x4 RotateZ90() {
    Matrix4x4 m = Identity();
    m.matrix[0][0] = 0.f;
    m.matrix[0][1] = 1.f;
    m.matrix[1][0] = -1.f;
    m.matrix[1][1] = 0.f;
    return m;
}

Point Transform(const Matrix4x4& m, Point p) {
    return {
        m.matrix[0][0] * p.x + m.matrix[1][0] * p.y + m.matrix[2][0] * p.z + m.matrix[3][0],
        m.matrix[0][1] * p.x + m.matrix[1][1] * p.y + m.matrix[2][1] * p.z + m.matrix[3][1],
        m.matrix[0][2] * p.x + m.matrix[1][2] * p.y + m.matrix[2][2] * p.z + m.matrix[3][2]
    };
}

bool Near(Point a, Point b) {
    return std::fabs(a.x-b.x) < 1e-5f &&
           std::fabs(a.y-b.y) < 1e-5f &&
           std::fabs(a.z-b.z) < 1e-5f;
}
}

int main() {
    const Matrix4x4 rotate = RotateZ90();
    const Matrix4x4 translate = Translation(10.f, 5.f, 0.f);

    if (!Near(Transform(rotate * translate, {1.f, 0.f, 0.f}),
              {10.f, 6.f, 0.f})) {
        std::fputs("column-major multiplication contract failed\n", stderr);
        return 1;
    }

    const Matrix4x4 inverseBind = Translation(-2.f, 0.f, 0.f);
    const Matrix4x4 boneWorld = rotate * translate;
    const Matrix4x4 skin = inverseBind * boneWorld;
    if (!Near(Transform(skin, {2.f, 0.f, 0.f}),
              {10.f, 5.f, 0.f})) {
        std::fputs("bindPose * boneWorld skinning contract failed\n", stderr);
        return 1;
    }

    if (Near(Transform(boneWorld * inverseBind, {2.f, 0.f, 0.f}),
             {10.f, 5.f, 0.f})) {
        std::fputs("matrix multiplication is unexpectedly commutative\n", stderr);
        return 1;
    }

    std::puts("chams matrix order: OK");
    return 0;
}
