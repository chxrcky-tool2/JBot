#pragma once
#include <chrono>
#include <cstdint>

namespace jbot::prof {
    inline uint64_t ticks() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
    constexpr uint64_t kTicksPerSecond = 1000000000ull;

    // exponential moving average, 1/16 weight on the new sample
    inline uint64_t ema(uint64_t avg, uint64_t sample) {
        int64_t d = static_cast<int64_t>(sample) - static_cast<int64_t>(avg);
        return static_cast<uint64_t>(static_cast<int64_t>(avg) + (d >> 4));
    }

    void frame(void* parent, uint64_t spent, bool enabled, float scale);
}
