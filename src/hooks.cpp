#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <cmath>
#include <vector>
#include "control.hpp"
#include "replay.hpp"
#include "state.hpp"

using namespace geode::prelude;

namespace {
    bool g_injecting = false;
    float g_extraDelta = 0.f;
    float g_lastPitch = -1.f;
    std::vector<uint32_t> g_checkpointFrames;

    uint32_t frameOf(GJBaseGameLayer* layer) {
        int progress = layer->m_gameState.m_currentProgress;
        return progress > 0 ? static_cast<uint32_t>(progress) : 0u;
    }

    bool applyPitch(float pitch) {
        auto engine = FMODAudioEngine::sharedEngine();
        if (!engine || !engine->m_system) return false;
        FMOD::ChannelGroup* group = nullptr;
        if (engine->m_system->getMasterChannelGroup(&group) != FMOD_OK || !group) return false;
        return group->setPitch(pitch) == FMOD_OK;
    }

    void restartFromStart() {
        auto pl = PlayLayer::get();
        if (!pl) return;
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (scene) {
            if (auto pause = scene->getChildByType<PauseLayer>(0)) pause->onResume(nullptr);
        }
        pl->resetLevelFromStart();
    }

    void handleReset(uint32_t frame) {
        g_extraDelta = 0.f;
        auto m = jbot::mode();
        if (m == jbot::Record) jbot_rep_truncate(frame);
        else if (m == jbot::Play) jbot_rep_seek(frame);
    }
}

namespace jbot {
    bool startRecording() {
        if (!PlayLayer::get()) return false;
        float tps = selectedTps();
        jbot_rep_mode_set(Idle);
        jbot_rep_clear();
        jbot_rep_tps_set(tps);
        jbot_rep_mode_set(Record);
        restartFromStart();
        return true;
    }

    bool startPlayback() {
        if (!PlayLayer::get() || jbot_rep_count() == 0) return false;
        jbot_rep_mode_set(Play);
        restartFromStart();
        return true;
    }

    void stopReplay() {
        jbot_rep_mode_set(Idle);
        g_extraDelta = 0.f;
    }
}

class $modify(JBScheduler, CCScheduler) {
    void update(float dt) {
        auto& s = jbot::state();
        float pitch = s.speedAudio ? static_cast<float>(s.speed) : 1.f;
        if (pitch != g_lastPitch && applyPitch(pitch)) g_lastPitch = pitch;
        CCScheduler::update(dt * static_cast<float>(s.speed));
    }
};

class $modify(JBGameLayer, GJBaseGameLayer) {
    float getModifiedDelta(float dt) {
        float tps = jbot::effectiveTps();
        if (tps == jbot::kDefaultTps) return GJBaseGameLayer::getModifiedDelta(dt);

        float warp = std::min(m_gameState.m_timeWarp, 1.f);
        if (warp < 0.001f) warp = 0.001f;
        float step = (1.f / tps) * warp;
        float total = dt + g_extraDelta;
        int steps = static_cast<int>(total / step);
        float out = static_cast<float>(steps) * step;
        g_extraDelta = total - out;
        return out;
    }

    void processCommands(float dt) {
        if (jbot::mode() == jbot::Play && typeinfo_cast<PlayLayer*>(this)) {
            uint32_t frame = frameOf(this);
            uint32_t flags = 0;
            while (jbot_rep_poll(frame, &flags)) {
                g_injecting = true;
                this->handleButton(
                    (flags & 1u) != 0,
                    static_cast<int>((flags >> 8) & 0xFFu),
                    (flags & 2u) == 0
                );
                g_injecting = false;
            }
        }
        GJBaseGameLayer::processCommands(dt);
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        auto m = jbot::mode();
        if (m != jbot::Idle && typeinfo_cast<PlayLayer*>(this)) {
            if (m == jbot::Play) {
                if (!g_injecting) return;
            } else {
                jbot_rep_record(frameOf(this), static_cast<uint32_t>(button), down ? 1u : 0u, isPlayer1 ? 0u : 1u);
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};

class $modify(JBPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (jbot::state().noclip && object != m_anticheatSpike) return;
        PlayLayer::destroyPlayer(player, object);
    }

    CheckpointObject* createCheckpoint() {
        auto cp = PlayLayer::createCheckpoint();
        if (cp) g_checkpointFrames.push_back(frameOf(this));
        return cp;
    }

    void removeLastCheckpoint() {
        PlayLayer::removeLastCheckpoint();
        if (!g_checkpointFrames.empty()) g_checkpointFrames.pop_back();
    }

    void resetLevelFromStart() {
        g_checkpointFrames.clear();
        PlayLayer::resetLevelFromStart();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (!m_isPracticeMode) g_checkpointFrames.clear();
        uint32_t frame = g_checkpointFrames.empty() ? 0u : g_checkpointFrames.back();
        handleReset(frame);
    }

    void onQuit() {
        jbot_rep_mode_set(jbot::Idle);
        g_checkpointFrames.clear();
        g_extraDelta = 0.f;
        PlayLayer::onQuit();
    }
};
