#pragma once
#include <cstdint>

extern "C" {
    void jbot_rep_mode_set(uint32_t mode);
    uint32_t jbot_rep_mode();
    void jbot_rep_clear();
    uint32_t jbot_rep_record(uint32_t frame, uint32_t button, uint32_t down, uint32_t player2);
    void jbot_rep_truncate(uint32_t frame);
    void jbot_rep_seek(uint32_t frame);
    uint32_t jbot_rep_poll(uint32_t frame, uint32_t* flags);
    uint32_t jbot_rep_count();
    uint32_t jbot_rep_cursor();
    uint32_t jbot_rep_length();
    const void* jbot_rep_data();
    uint32_t jbot_rep_load(const void* src, uint32_t count);
    void jbot_rep_tps_set(float tps);
    float jbot_rep_tps_get();
    void jbot_rep_free();
}

namespace jbot {
    enum Mode : uint32_t { Idle = 0, Record = 1, Play = 2 };

    inline Mode mode() { return static_cast<Mode>(jbot_rep_mode()); }
}
