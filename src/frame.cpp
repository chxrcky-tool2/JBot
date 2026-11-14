#include "frame.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace jbot::fe {
    namespace {
        uint32_t g_now = 0;
        uint32_t g_gaps = 0;
        uint32_t g_rewinds = 0;
        bool g_started = false;
        uint8_t g_held[2] = {0, 0};
        std::unordered_map<void const*, Snap> g_checkpoints;

        std::vector<uint32_t> g_deaths;

        bool validButton(int b) { return b >= 1 && b <= 3; }
    }

    void beginStep(uint32_t progress) {
        if (g_started) {
            if (progress < g_now) g_rewinds++;
            else if (progress > g_now + 1) g_gaps += progress - g_now - 1;
        }
        g_now = progress;
        g_started = true;
    }

    uint32_t now() { return g_now; }
    void expectJump() { g_started = false; }
    void resetStats() { g_gaps = 0; g_rewinds = 0; g_started = false; }
    uint32_t gaps() { return g_gaps; }
    uint32_t rewinds() { return g_rewinds; }

    void storeCheckpoint(void const* key, Snap const& snap) { g_checkpoints[key] = snap; }

    Snap const* findCheckpoint(void const* key) {
        auto it = g_checkpoints.find(key);
        return it == g_checkpoints.end() ? nullptr : &it->second;
    }

    void pruneCheckpoints(std::vector<void const*> const& live) {
        std::unordered_set<void const*> keep(live.begin(), live.end());
        for (auto it = g_checkpoints.begin(); it != g_checkpoints.end();) {
            if (keep.count(it->first)) ++it;
            else it = g_checkpoints.erase(it);
        }
    }

    void clearCheckpoints() { g_checkpoints.clear(); }
    size_t checkpointCount() { return g_checkpoints.size(); }

    void track(bool player2, int button, bool down) {
        if (!validButton(button)) return;
        uint8_t bit = static_cast<uint8_t>(1u << button);
        uint8_t& mask = g_held[player2 ? 1 : 0];
        if (down) mask |= bit;
        else mask &= static_cast<uint8_t>(~bit);
    }

    uint8_t held(bool player2) { return g_held[player2 ? 1 : 0]; }
    void clearHeld() { g_held[0] = g_held[1] = 0; }

    void deathsClear() { g_deaths.clear(); }

    void deathsRecord(uint32_t frame) {
        if (!g_deaths.empty() && g_deaths.back() >= frame) return;
        g_deaths.push_back(frame);
    }

    void deathsTruncate(uint32_t frame) {
        auto it = std::lower_bound(g_deaths.begin(), g_deaths.end(), frame);
        g_deaths.erase(it, g_deaths.end());
    }

    bool deathsHas(uint32_t frame) {
        return std::binary_search(g_deaths.begin(), g_deaths.end(), frame);
    }

    uint32_t deathsCount() { return static_cast<uint32_t>(g_deaths.size()); }
    uint32_t const* deathsData() { return g_deaths.data(); }

    bool deathsLoad(uint32_t const* frames, uint32_t count) {
        if (count > 0x1000000u) return false;
        for (uint32_t i = 1; i < count; i++) {
            if (frames[i] <= frames[i - 1]) return false;
        }
        g_deaths.assign(frames, frames + count);
        return true;
    }

    uint8_t heldAt(void const* entries, uint32_t count, uint32_t at, bool player2) {
        auto const* e = static_cast<uint32_t const*>(entries);
        uint8_t mask = 0;
        if (!e) return 0;
        for (uint32_t i = 0; i < count; i++) {
            uint32_t frame = e[i * 2];
            uint32_t flags = e[i * 2 + 1];
            if (frame >= at) break;
            if (((flags & 2u) != 0) != player2) continue;
            int button = static_cast<int>((flags >> 8) & 0xFFu);
            if (!validButton(button)) continue;
            uint8_t bit = static_cast<uint8_t>(1u << button);
            if (flags & 1u) mask |= bit;
            else mask &= static_cast<uint8_t>(~bit);
        }
        return mask;
    }
}
