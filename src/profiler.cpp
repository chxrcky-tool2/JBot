#include <Geode/Geode.hpp>
#include <cstdio>
#include "ticks.hpp"

using namespace geode::prelude;

// In-level update-time panel (plain C++; Assembly is reserved for the replay engine).
namespace jbot::prof {
    namespace {
        constexpr int kTagPanel = 0x5A01;
        constexpr int kTagFirst = 10;   // title, avg, peak, min = tags 10..13

        uint64_t g_ema = 0, g_peak = 0, g_min = 0;
        uint32_t g_peakTimer = 0, g_textTimer = 0;

        CCLayerColor* buildPanel(CCNode* parent) {
            auto panel = CCLayerColor::create({0, 0, 0, 150}, 230.f, 116.f);
            if (!panel) return nullptr;
            parent->addChild(panel, 100, kTagPanel);
            for (int i = 0; i < 4; i++) {
                auto label = CCLabelBMFont::create(i == 0 ? "JBOT" : "", "chatFont.fnt");
                if (!label) continue;
                label->setAnchorPoint({0.f, 1.f});
                label->setPosition({10.f, 108.f - 27.f * i});
                panel->addChild(label, 1, kTagFirst + i);
            }
            return panel;
        }

        void setLine(CCNode* panel, int tag, char const* name, uint64_t ticks) {
            auto us = (ticks * 1000000ull + kTicksPerSecond / 2) / kTicksPerSecond;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%s%llu us", name, static_cast<unsigned long long>(us));
            if (auto label = typeinfo_cast<CCLabelBMFont*>(panel->getChildByTag(tag))) {
                label->setString(buf);
            }
        }
    }

    void frame(void* parentPtr, uint64_t spent, bool enabled, float scale) {
        g_ema = g_ema ? ema(g_ema, spent) : spent;
        if (spent > g_peak) g_peak = spent;
        if (g_min == 0 || spent < g_min) g_min = spent;
        if (++g_peakTimer >= 240) { g_peakTimer = 0; g_peak = g_min = spent; }

        if (++g_textTimer < 20) return;
        g_textTimer = 0;
        if (!parentPtr) return;

        auto parent = static_cast<CCNode*>(parentPtr);
        CCNode* panel = parent->getChildByTag(kTagPanel);
        if (!panel) panel = buildPanel(parent);
        if (!panel) return;

        panel->setVisible(enabled);
        if (!enabled) return;

        panel->setScale(scale);
        auto win = CCDirector::sharedDirector()->getWinSize();
        panel->setPosition({6.f, win.height - 116.f * scale - 6.f});
        setLine(panel, kTagFirst + 1, "avg  ", g_ema);
        setLine(panel, kTagFirst + 2, "peak ", g_peak);
        setLine(panel, kTagFirst + 3, "min  ", g_min);
    }
}
