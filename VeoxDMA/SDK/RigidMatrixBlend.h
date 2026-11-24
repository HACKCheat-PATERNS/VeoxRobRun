#pragma once
#include <algorithm>
#include <cmath>
#include "Matrix4x4.h"

namespace RigidMatrixBlend {

struct Quat {
    float x = 0.f, y = 0.f, z = 0.f, w = 1.f;
};

inline bool RotationQuat(const Matrix4x4& m, Quat& q)
{
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (!std::isfinite(m.matrix[c][r])) return false;
    if (std::abs(m.matrix[3][3] - 1.f) > 1e-4f) return false;
    const float m00 = m.matrix[0][0], m11 = m.matrix[1][1], m22 = m.matrix[2][2];
    const float trace = m00 + m11 + m22;
    if (trace > 0.f) {
        const float s = 2.f * std::sqrt(trace + 1.f);
        q = {(m.matrix[1][2] - m.matrix[2][1]) / s,
             (m.matrix[2][0] - m.matrix[0][2]) / s,
             (m.matrix[0][1] - m.matrix[1][0]) / s, 0.25f * s};
    } else if (m00 > m11 && m00 > m22) {
        const float s = 2.f * std::sqrt(1.f + m00 - m11 - m22);
        q = {0.25f * s, (m.matrix[1][0] + m.matrix[0][1]) / s,
             (m.matrix[2][0] + m.matrix[0][2]) / s,
             (m.matrix[1][2] - m.matrix[2][1]) / s};
    } else if (m11 > m22) {
        const float s = 2.f * std::sqrt(1.f + m11 - m00 - m22);
        q = {(m.matrix[1][0] + m.matrix[0][1]) / s, 0.25f * s,
             (m.matrix[2][1] + m.matrix[1][2]) / s,
             (m.matrix[2][0] - m.matrix[0][2]) / s};
    } else {
        const float s = 2.f * std::sqrt(1.f + m22 - m00 - m11);
        q = {(m.matrix[2][0] + m.matrix[0][2]) / s,
             (m.matrix[2][1] + m.matrix[1][2]) / s, 0.25f * s,
             (m.matrix[0][1] - m.matrix[1][0]) / s};
    }
    const float norm2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (!std::isfinite(norm2) || norm2 < 0.1f) return false;
    const float norm = 1.f / std::sqrt(norm2);
    q.x *= norm; q.y *= norm; q.z *= norm; q.w *= norm;
    return true;
}

inline void SetRotation(Matrix4x4& m, Quat q)
{
    const float xx=q.x*q.x, yy=q.y*q.y, zz=q.z*q.z;
    const float xy=q.x*q.y, xz=q.x*q.z, yz=q.y*q.z;
    const float wx=q.w*q.x, wy=q.w*q.y, wz=q.w*q.z;
    m.matrix[0][0]=1-2*(yy+zz); m.matrix[0][1]=2*(xy+wz);   m.matrix[0][2]=2*(xz-wy);
    m.matrix[1][0]=2*(xy-wz);   m.matrix[1][1]=1-2*(xx+zz); m.matrix[1][2]=2*(yz+wx);
    m.matrix[2][0]=2*(xz+wy);   m.matrix[2][1]=2*(yz-wx);   m.matrix[2][2]=1-2*(xx+yy);
    m.matrix[0][3] = m.matrix[1][3] = m.matrix[2][3] = 0.f;
    m.matrix[3][3] = 1.f;
}

inline Matrix4x4 Blend(const Matrix4x4& previous,
                       const Matrix4x4& current, float alpha)
{
    Quat oldQ{}, curQ{};
    if (!RotationQuat(previous, oldQ) || !RotationQuat(current, curQ))
        return current;
    alpha = (std::max)(0.f, (std::min)(1.f, alpha));
    const float dot = oldQ.x*curQ.x + oldQ.y*curQ.y +
                      oldQ.z*curQ.z + oldQ.w*curQ.w;
    if (dot < 0.f) {
        curQ.x=-curQ.x; curQ.y=-curQ.y;
        curQ.z=-curQ.z; curQ.w=-curQ.w;
    }
    Quat q{
        oldQ.x*(1-alpha) + curQ.x*alpha,
        oldQ.y*(1-alpha) + curQ.y*alpha,
        oldQ.z*(1-alpha) + curQ.z*alpha,
        oldQ.w*(1-alpha) + curQ.w*alpha
    };
    const float n = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (!std::isfinite(n) || n < 1e-6f) return current;
    const float inv = 1.f / std::sqrt(n);
    q.x *= inv; q.y *= inv; q.z *= inv; q.w *= inv;

    Matrix4x4 out{};
    SetRotation(out, q);
    for (int axis = 0; axis < 3; ++axis)
        out.matrix[3][axis] =
            previous.matrix[3][axis]*(1-alpha) +
            current.matrix[3][axis]*alpha;
    return out;
}

}
