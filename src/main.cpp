#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "ticks.hpp"

using namespace geode::prelude;

// panel_arm64.S calls CCDirector::getWinSize() by hand and relies on CCSize being
// returned through memory (sret, address in x8). That is only true while CCSize is
// not trivially copyable. If this ever fires, the assembly must read s0/s1 instead.
static_assert(!std::is_trivially_copy_constructible_v<cocos2d::CCSize>,
              "CCSize is returned in s0/s1 here; panel_arm64.S assumes sret via x8");

// Thin shim: Geode's $modify hook has to be C++. Everything else
// (stats, cocos2d panel creation, layout, text) lives in src/asm/panel_arm64.S.
class $modify(ProfGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        uint64_t t0 = asmprof::ticks();
        GJBaseGameLayer::update(dt);
        uint64_t spent = asmprof::ticks() - t0;

        // settings are re-read every 20 updates, not on every one
        static uint32_t counter = 0;
        static bool show = true;
        static float scale = 0.4f;
        if (counter++ % 20 == 0) {
            auto mod = Mod::get();
            show = mod->getSettingValue<bool>("show-hud");
            scale = static_cast<float>(mod->getSettingValue<double>("hud-scale"));
        }

        auto pl = typeinfo_cast<PlayLayer*>(this);
        void* parent = (pl && pl->m_uiLayer) ? static_cast<CCNode*>(pl->m_uiLayer) : nullptr;
        asmprof::panel_frame(parent, spent, show, scale);
    }
};
