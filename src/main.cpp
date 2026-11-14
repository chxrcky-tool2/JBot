#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "state.hpp"
#include "ticks.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    jbot::loadState();
}

class $modify(ProfGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        uint64_t t0 = jbot::prof::ticks();
        GJBaseGameLayer::update(dt);
        uint64_t spent = jbot::prof::ticks() - t0;

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
        jbot::prof::frame(parent, spent, show, scale);
    }
};
