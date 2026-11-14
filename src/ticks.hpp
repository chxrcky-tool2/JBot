#pragma once
#include <cstdint>

#ifdef ASMPROF_HAS_ASM

extern "C" {
    uint64_t asmprof_ticks();
    uint64_t asmprof_freq();
    uint64_t asmprof_ema(uint64_t avg, uint64_t sample);

    void asmprof_panel_frame(void* parent, uint64_t spent, uint32_t enabled, float scale);
}

namespace asmprof {
    inline uint64_t ticks() { return asmprof_ticks(); }
    inline uint64_t freq() {
        uint64_t f = asmprof_freq();
        return f ? f : 19200000ull;
    }
    inline uint64_t ema(uint64_t avg, uint64_t sample) { return asmprof_ema(avg, sample); }
    inline void panel_frame(void* parent, uint64_t spent, bool enabled, float scale) {
        asmprof_panel_frame(parent, spent, enabled ? 1u : 0u, scale);
    }
}

#else

#include <chrono>

namespace asmprof {
    inline uint64_t ticks() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
    inline uint64_t freq() { return 1000000000ull; }
    inline uint64_t ema(uint64_t avg, uint64_t sample) {
        int64_t d = static_cast<int64_t>(sample) - static_cast<int64_t>(avg);
        return static_cast<uint64_t>(static_cast<int64_t>(avg) + (d >> 4));
    }

    inline void panel_frame(void*, uint64_t, bool, float) {}
}

#endif
