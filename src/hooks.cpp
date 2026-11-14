#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <cmath>
#include <vector>
#include "control.hpp"
#include "frame.hpp"
#include "replay.hpp"
#include "state.hpp"

using namespace geode::prelude;

namespace {
    bool g_injecting = false;
    float g_extraDelta = 0.f;
    float g_lastPitch = -1.f;
    bool g_desyncShown = false;

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

    // ---- practice fix ----------------------------------------------------
    // A checkpoint only keeps part of the player. Capture the rest right after
    // the checkpoint is made and put it back after the checkpoint is loaded.
    jbot::fe::PlayerSnap capture(PlayerObject* p) {
        jbot::fe::PlayerSnap s;
        if (!p) return s;
        s.valid = true;
        s.platformer = p->m_isPlatformer;
        s.yVel = p->m_yVelocity;
        s.xVel = p->m_platformerXVelocity;
        s.rotation = p->getRotation();
        s.speed = p->m_playerSpeed;
        return s;
    }

    void restore(PlayerObject* p, jbot::fe::PlayerSnap const& s) {
        if (!p || !s.valid) return;
        p->m_yVelocity = s.yVel;
        if (s.platformer) p->m_platformerXVelocity = s.xVel;
        p->setRotation(s.rotation);
        p->m_playerSpeed = s.speed;
    }

    // ---- input accuracy --------------------------------------------------
    void sendButton(GJBaseGameLayer* layer, bool down, int button, bool player2) {
        g_injecting = true;
        layer->handleButton(down, button, !player2);
        g_injecting = false;
    }

    // Let go of everything the game thinks is held, so nothing sticks after a stop.
    void releaseHeld(GJBaseGameLayer* layer) {
        if (!layer) return;
        for (int p = 0; p < 2; p++) {
            uint8_t mask = jbot::fe::held(p == 1);
            for (int b = 1; b <= 3; b++) {
                if (mask & (1u << b)) sendButton(layer, false, b, p == 1);
            }
        }
    }

    // After a respawn the game has every button up. Make the macro agree with
    // that, so the recorded and the played state never drift apart:
    //  - Play:   re-press what the macro says is held at this frame
    //  - Record: write a release at this frame for what the macro says is held
    void resyncInputs(GJBaseGameLayer* layer, uint32_t frame) {
        jbot::fe::clearHeld();
        auto m = jbot::mode();
        if (m == jbot::Idle || !layer) return;
        for (int p = 0; p < 2; p++) {
            uint8_t want = jbot::fe::heldAt(jbot_rep_data(), jbot_rep_count(), frame, p == 1);
            for (int b = 1; b <= 3; b++) {
                if (!(want & (1u << b))) continue;
                if (m == jbot::Play) sendButton(layer, true, b, p == 1);
                else jbot_rep_record(frame, static_cast<uint32_t>(b), 0u, static_cast<uint32_t>(p));
            }
        }
    }

    void handleReset(GJBaseGameLayer* layer, uint32_t frame) {
        g_extraDelta = 0.f;
        g_desyncShown = false;
        jbot::fe::expectJump();
        auto m = jbot::mode();
        if (m == jbot::Record) {
            jbot_rep_truncate(frame);
            jbot::fe::deathsTruncate(frame);
        }
        else if (m == jbot::Play) jbot_rep_seek(frame);
        resyncInputs(layer, frame);
    }
}

namespace jbot {
    bool startRecording() {
        if (!PlayLayer::get()) return false;
        float tps = selectedTps();
        jbot_rep_mode_set(Idle);
        jbot_rep_clear();
        jbot::fe::deathsClear();
        jbot_rep_tps_set(tps);
        jbot_rep_mode_set(Record);
        jbot::fe::resetStats();
        restartFromStart();
        return true;
    }

    bool startPlayback() {
        if (!PlayLayer::get() || jbot_rep_count() == 0) return false;
        jbot_rep_mode_set(Play);
        jbot::fe::resetStats();
        restartFromStart();
        return true;
    }

    void stopReplay() {
        jbot_rep_mode_set(Idle);
        g_extraDelta = 0.f;
        releaseHeld(PlayLayer::get());
        jbot::fe::clearHeld();
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

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        bool inLevel = typeinfo_cast<PlayLayer*>(this) != nullptr;
        if (inLevel) {
            jbot::fe::beginStep(frameOf(this));
            if (jbot::mode() == jbot::Play) {
                uint32_t frame = jbot::fe::now();
                uint32_t flags = 0;
                while (jbot_rep_poll(frame, &flags)) {
                    sendButton(
                        this,
                        (flags & 1u) != 0,
                        static_cast<int>((flags >> 8) & 0xFFu),
                        (flags & 2u) != 0
                    );
                }
            }
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (typeinfo_cast<PlayLayer*>(this)) {
            auto m = jbot::mode();
            if (m == jbot::Play && !g_injecting) return;
            if (m == jbot::Record) {
                jbot_rep_record(frameOf(this), static_cast<uint32_t>(button), down ? 1u : 0u, isPlayer1 ? 0u : 1u);
            }
            jbot::fe::track(!isPlayer1, button, down);
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};

class $modify(JBPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (jbot::state().noclip && object != m_anticheatSpike) return;

        uint32_t frame = frameOf(this);
        auto m = jbot::mode();
        if (m == jbot::Record) {
            jbot::fe::deathsRecord(frame);
        } else if (m == jbot::Play && !g_desyncShown
                   && !jbot::fe::deathsHas(frame) && frame < jbot_rep_length()) {
            // The macro goes on past this frame, so the run has drifted from the recording.
            g_desyncShown = true;
            Notification::create(fmt::format("JBot: desync, died at frame {}", frame), NotificationIcon::Warning)->show();
        }
        PlayLayer::destroyPlayer(player, object);
    }

    CheckpointObject* createCheckpoint() {
        std::vector<void const*> live;
        if (m_checkpointArray) {
            for (auto obj : CCArrayExt<CCObject*>(m_checkpointArray)) live.push_back(obj);
        }
        jbot::fe::pruneCheckpoints(live);

        auto cp = PlayLayer::createCheckpoint();
        if (cp) {
            jbot::fe::Snap snap;
            snap.frame = frameOf(this);
            snap.p[0] = capture(m_player1);
            snap.p[1] = capture(m_player2);
            jbot::fe::storeCheckpoint(cp, snap);
        }
        return cp;
    }

    void resetLevelFromStart() {
        jbot::fe::clearCheckpoints();
        PlayLayer::resetLevelFromStart();
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        uint32_t frame = 0;
        if (!m_isPracticeMode) {
            jbot::fe::clearCheckpoints();
        } else if (m_checkpointArray && m_checkpointArray->count() > 0) {
            auto cp = static_cast<CheckpointObject*>(m_checkpointArray->lastObject());
            if (auto snap = jbot::fe::findCheckpoint(cp)) {
                frame = snap->frame;
                restore(m_player1, snap->p[0]);
                restore(m_player2, snap->p[1]);
            }
        }
        handleReset(this, frame);
    }

    void onQuit() {
        jbot_rep_mode_set(jbot::Idle);
        releaseHeld(this);
        jbot::fe::clearHeld();
        jbot::fe::clearCheckpoints();
        g_extraDelta = 0.f;
        PlayLayer::onQuit();
    }
};
