#pragma once
#include <cstdint>

namespace RigRecoveryPolicy {
constexpr std::uint64_t RetryIntervalMs(int attempts) noexcept
{
    return attempts <= 0 ? 500ULL :
           attempts == 1 ? 1500ULL :
           attempts < 4 ? 4000ULL : 10000ULL;
}

constexpr int IncrementAttempt(int attempts) noexcept
{
    return attempts < 4 ? attempts + 1 : 4;
}
}
