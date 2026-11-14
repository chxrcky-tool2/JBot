#ifndef JBOT_ASM_REPLAY

#include "replay.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace {
    struct Entry {
        uint32_t frame;
        uint32_t flags;
    };

    std::vector<Entry> g_entries;
    uint32_t g_cursor = 0;
    uint32_t g_mode = 0;
    float g_tps = 0.f;

    uint32_t lowerBound(uint32_t frame) {
        auto it = std::lower_bound(g_entries.begin(), g_entries.end(), frame,
            [](Entry const& e, uint32_t f) { return e.frame < f; });
        return static_cast<uint32_t>(it - g_entries.begin());
    }
}

extern "C" {
    void jbot_rep_mode_set(uint32_t mode) { g_mode = mode; }
    uint32_t jbot_rep_mode() { return g_mode; }

    void jbot_rep_clear() {
        g_entries.clear();
        g_cursor = 0;
    }

    uint32_t jbot_rep_record(uint32_t frame, uint32_t button, uint32_t down, uint32_t player2) {
        if (g_mode != 1) return 0;
        if (g_entries.size() >= 0x10000000u) return 0;
        uint32_t flags = (down & 1u) | ((player2 & 1u) << 1) | ((button & 0xFFu) << 8);
        g_entries.push_back({frame, flags});
        return 1;
    }

    void jbot_rep_truncate(uint32_t frame) {
        uint32_t idx = lowerBound(frame);
        g_entries.resize(idx);
        g_cursor = std::min(g_cursor, idx);
    }

    void jbot_rep_seek(uint32_t frame) { g_cursor = lowerBound(frame); }

    uint32_t jbot_rep_poll(uint32_t frame, uint32_t* flags) {
        if (g_mode != 2 || g_cursor >= g_entries.size()) return 0;
        Entry const& e = g_entries[g_cursor];
        if (e.frame > frame) return 0;
        *flags = e.flags;
        g_cursor++;
        return 1;
    }

    uint32_t jbot_rep_count() { return static_cast<uint32_t>(g_entries.size()); }
    uint32_t jbot_rep_cursor() { return g_cursor; }
    uint32_t jbot_rep_length() { return g_entries.empty() ? 0u : g_entries.back().frame; }
    const void* jbot_rep_data() { return g_entries.data(); }

    uint32_t jbot_rep_load(const void* src, uint32_t count) {
        if (count > 0x10000000u) return 0;
        g_entries.resize(count);
        if (count) std::memcpy(g_entries.data(), src, static_cast<size_t>(count) * 8);
        g_cursor = 0;
        return 1;
    }

    void jbot_rep_tps_set(float tps) { g_tps = tps; }
    float jbot_rep_tps_get() { return g_tps != 0.f ? g_tps : 240.f; }

    void jbot_rep_free() {
        std::vector<Entry>().swap(g_entries);
        g_cursor = 0;
    }
}

#endif
