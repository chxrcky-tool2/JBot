#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Frame engine: the single place that knows "what physics step are we on",
// which checkpoint was taken on which step, and which buttons are held.
// No Geode dependency, so it can be unit tested on the host (tests/frame_test.cpp).
namespace jbot::fe {
    struct PlayerSnap {
        bool valid = false;
        bool platformer = false;
        double yVel = 0.0;
        double xVel = 0.0;
        float rotation = 0.f;
        float speed = 1.f;
    };

    struct Snap {
        uint32_t frame = 0;
        PlayerSnap p[2];
    };

    // step clock
    void beginStep(uint32_t progress);
    uint32_t now();
    void expectJump();      // next beginStep may rewind/skip (reset, checkpoint load)
    void resetStats();
    uint32_t gaps();        // steps the game skipped between two beginStep calls
    uint32_t rewinds();     // unexpected backwards jumps

    // checkpoint ledger, keyed by the CheckpointObject pointer
    void storeCheckpoint(void const* key, Snap const& snap);
    Snap const* findCheckpoint(void const* key);
    void pruneCheckpoints(std::vector<void const*> const& live);
    void clearCheckpoints();
    size_t checkpointCount();

    // held-button ledger: bit n set = button n is down (1 jump, 2 left, 3 right)
    void track(bool player2, int button, bool down);
    uint8_t held(bool player2);
    void clearHeld();

    // death markers: frames where the recorded attempt died (slc/gdr style "death" input).
    // A macro that ends in a death keeps its marker; markers from attempts that were
    // reset are dropped by truncate, so a clean run carries none.
    void deathsClear();
    void deathsRecord(uint32_t frame);
    void deathsTruncate(uint32_t frame);   // drop markers with frame >= `frame`
    bool deathsHas(uint32_t frame);
    uint32_t deathsCount();
    uint32_t const* deathsData();
    bool deathsLoad(uint32_t const* frames, uint32_t count);  // must be ascending

    // Held mask that a macro implies just before `at`. `entries` is the replay
    // buffer: {u32 frame, u32 flags} pairs, flags bit0 down, bit1 player 2, bits 8-15 button.
    uint8_t heldAt(void const* entries, uint32_t count, uint32_t at, bool player2);
}
