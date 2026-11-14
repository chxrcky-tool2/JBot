#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cstdint>
#include "replay.hpp"

namespace jbot {
    constexpr float kDefaultTps = 240.f;
    constexpr float kMaxTps = 100000.f;

    struct State {
        bool noclip = false;
        double speed = 1.0;
        bool speedAudio = true;
        bool tpsOn = false;
        float tps = kDefaultTps;
    };

    inline State& state() {
        static State s;
        return s;
    }

    inline void setSpeed(double value) {
        state().speed = std::clamp(value, 0.01, 4.0);
    }

    inline void setTps(float value) {
        state().tps = std::clamp(value, 1.f, kMaxTps);
    }

    inline float selectedTps() {
        auto& s = state();
        return s.tpsOn ? s.tps : kDefaultTps;
    }

    inline float effectiveTps() {
        if (mode() != Idle) return jbot_rep_tps_get();
        return selectedTps();
    }

    inline void loadState() {
        auto mod = geode::Mod::get();
        auto& s = state();
        s.noclip = mod->getSavedValue<bool>("noclip", false);
        s.speedAudio = mod->getSavedValue<bool>("speed-audio", true);
        s.tpsOn = mod->getSavedValue<bool>("tps-on", false);
        setTps(static_cast<float>(mod->getSavedValue<double>("tps", kDefaultTps)));
    }

    inline void saveState() {
        auto mod = geode::Mod::get();
        auto& s = state();
        mod->setSavedValue("noclip", s.noclip);
        mod->setSavedValue("speed-audio", s.speedAudio);
        mod->setSavedValue("tps-on", s.tpsOn);
        mod->setSavedValue("tps", static_cast<double>(s.tps));
    }
}
