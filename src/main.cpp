#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <fmt/format.h>
#include "ticks.hpp"

using namespace geode::prelude;

class $modify(ProfGameLayer, GJBaseGameLayer) {
    struct Fields {
        uint64_t freq = asmprof::freq();
        uint64_t ema = 0;
        uint64_t peak = 0;
        uint32_t textTimer = 0;
        uint32_t peakTimer = 0;
        CCLabelBMFont* label = nullptr;
    };

    void ensureLabel() {
        auto f = m_fields.self();
        if (f->label) return;

        auto pl = typeinfo_cast<PlayLayer*>(this);
        if (!pl || !pl->m_uiLayer) return;

        auto win = CCDirector::get()->getWinSize();
        auto label = CCLabelBMFont::create("", "chatFont.fnt");
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({6.f, win.height - 6.f});
        label->setOpacity(170);
        label->setZOrder(100);
        pl->m_uiLayer->addChild(label);
        f->label = label;
    }

    void update(float dt) {
        uint64_t t0 = asmprof::ticks();
        GJBaseGameLayer::update(dt);
        uint64_t spent = asmprof::ticks() - t0;

        auto f = m_fields.self();
        f->ema = f->ema ? asmprof::ema(f->ema, spent) : spent;
        if (spent > f->peak) f->peak = spent;

        // peak resets about once per second at 240 updates/s
        if (++f->peakTimer >= 240) {
            f->peakTimer = 0;
            f->peak = spent;
        }

        // refresh the text 12 times per second at most
        if (++f->textTimer < 20) return;
        f->textTimer = 0;

        this->ensureLabel();
        if (!f->label) return;

        auto mod = Mod::get();
        bool show = mod->getSettingValue<bool>("show-hud");
        f->label->setVisible(show);
        if (!show) return;

        f->label->setScale(static_cast<float>(mod->getSettingValue<double>("hud-scale")));
        double toUs = 1e6 / static_cast<double>(f->freq);
        f->label->setString(
            fmt::format("upd {:.0f} us | peak {:.0f} us",
                static_cast<double>(f->ema) * toUs,
                static_cast<double>(f->peak) * toUs).c_str()
        );
    }
};
