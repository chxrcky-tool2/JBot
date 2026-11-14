#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "control.hpp"
#include "macro_io.hpp"
#include "replay.hpp"
#include "state.hpp"

using namespace geode::prelude;

class JBotPanel : public geode::Popup {
protected:
    CCLabelBMFont* m_status = nullptr;
    TextInput* m_nameInput = nullptr;
    TextInput* m_speedInput = nullptr;
    TextInput* m_tpsInput = nullptr;

    CCMenuItemSpriteExtra* addButton(char const* text, float width, SEL_MenuHandler cb, CCPoint pos) {
        auto spr = ButtonSprite::create(text, static_cast<int>(width), true, "bigFont.fnt", "GJ_button_01.png", 28.f, 0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, cb);
        m_buttonMenu->addChildAtPosition(btn, Anchor::Center, pos);
        return btn;
    }

    void addLabel(char const* text, CCPoint pos) {
        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setScale(0.5f);
        label->setAnchorPoint({0.f, 0.5f});
        m_mainLayer->addChildAtPosition(label, Anchor::Center, pos);
    }

    CCMenuItemToggler* addToggler(bool on, SEL_MenuHandler cb, CCPoint pos) {
        auto toggler = CCMenuItemToggler::createWithStandardSprites(this, cb, 0.6f);
        toggler->toggle(on);
        m_buttonMenu->addChildAtPosition(toggler, Anchor::Center, pos);
        return toggler;
    }

    bool init() {
        if (!Popup::init(320.f, 260.f)) return false;
        this->setTitle("JBot");

        auto& s = jbot::state();

        m_status = CCLabelBMFont::create("", "bigFont.fnt");
        m_status->setScale(0.4f);
        m_mainLayer->addChildAtPosition(m_status, Anchor::Center, {0.f, 92.f});

        this->addButton("Record", 80.f, menu_selector(JBotPanel::onRecord), {-100.f, 62.f});
        this->addButton("Play", 80.f, menu_selector(JBotPanel::onPlay), {0.f, 62.f});
        this->addButton("Stop", 80.f, menu_selector(JBotPanel::onStop), {100.f, 62.f});

        m_nameInput = TextInput::create(120.f, "macro name");
        m_nameInput->setMaxCharCount(32);
        m_mainLayer->addChildAtPosition(m_nameInput, Anchor::Center, {-85.f, 28.f});
        this->addButton("Save", 50.f, menu_selector(JBotPanel::onSave), {20.f, 28.f});
        this->addButton("Load", 50.f, menu_selector(JBotPanel::onLoad), {85.f, 28.f});

        this->addLabel("Noclip", {-135.f, -6.f});
        this->addToggler(s.noclip, menu_selector(JBotPanel::onNoclip), {115.f, -6.f});

        this->addLabel("Speedhack", {-135.f, -34.f});
        m_speedInput = TextInput::create(80.f, "1.0");
        m_speedInput->setCommonFilter(CommonFilter::Float);
        m_speedInput->setMaxCharCount(6);
        m_speedInput->setString(fmt::format("{:g}", s.speed), false);
        m_speedInput->setCallback([](std::string const& text) {
            char* end = nullptr;
            double v = std::strtod(text.c_str(), &end);
            if (end == text.c_str() || !(v > 0.0)) return;
            jbot::setSpeed(v);
        });
        m_mainLayer->addChildAtPosition(m_speedInput, Anchor::Center, {60.f, -34.f});

        this->addLabel("Speed audio", {-135.f, -62.f});
        this->addToggler(s.speedAudio, menu_selector(JBotPanel::onSpeedAudio), {115.f, -62.f});

        this->addLabel("TPS bypass", {-135.f, -90.f});
        m_tpsInput = TextInput::create(80.f, "240");
        m_tpsInput->setCommonFilter(CommonFilter::Float);
        m_tpsInput->setMaxCharCount(7);
        m_tpsInput->setString(fmt::format("{:g}", s.tps), false);
        m_tpsInput->setCallback([](std::string const& text) {
            char* end = nullptr;
            double v = std::strtod(text.c_str(), &end);
            if (end == text.c_str() || !(v >= 1.0)) return;
            jbot::setTps(static_cast<float>(v));
            jbot::saveState();
        });
        m_mainLayer->addChildAtPosition(m_tpsInput, Anchor::Center, {60.f, -90.f});
        this->addToggler(s.tpsOn, menu_selector(JBotPanel::onTps), {115.f, -90.f});

        this->refresh(0.f);
        this->schedule(schedule_selector(JBotPanel::refresh), 0.1f);
        return true;
    }

    void refresh(float) {
        char buf[96];
        char const* name = "Idle";
        if (jbot::mode() == jbot::Record) name = "Recording";
        else if (jbot::mode() == jbot::Play) name = "Playing";
        std::snprintf(buf, sizeof(buf), "%s  |  %u inputs  |  %.2fx  |  %g tps",
            name, jbot_rep_count(), jbot::state().speed, jbot::effectiveTps());
        m_status->setString(buf);
    }

    void onRecord(CCObject*) {
        if (jbot::startRecording()) this->onClose(nullptr);
        else Notification::create("Open a level first", NotificationIcon::Error)->show();
    }

    void onPlay(CCObject*) {
        if (jbot::startPlayback()) this->onClose(nullptr);
        else Notification::create("No macro to play", NotificationIcon::Error)->show();
    }

    void onStop(CCObject*) {
        jbot::stopReplay();
    }

    void onSave(CCObject*) {
        auto name = m_nameInput->getString();
        if (jbot::sanitizeName(name).empty()) {
            Notification::create("Enter a macro name", NotificationIcon::Error)->show();
        } else if (jbot_rep_count() == 0) {
            Notification::create("Nothing to save", NotificationIcon::Error)->show();
        } else if (jbot::saveMacro(name)) {
            Notification::create("Macro saved", NotificationIcon::Success)->show();
        } else {
            Notification::create("Could not save macro", NotificationIcon::Error)->show();
        }
    }

    void onLoad(CCObject*) {
        auto name = m_nameInput->getString();
        if (jbot::sanitizeName(name).empty()) {
            Notification::create("Enter a macro name", NotificationIcon::Error)->show();
            return;
        }
        jbot::stopReplay();
        if (jbot::loadMacro(name)) {
            Notification::create("Macro loaded", NotificationIcon::Success)->show();
        } else {
            Notification::create("Could not load macro", NotificationIcon::Error)->show();
        }
    }

    void onNoclip(CCObject* sender) {
        jbot::state().noclip = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
        jbot::saveState();
    }

    void onSpeedAudio(CCObject* sender) {
        jbot::state().speedAudio = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
        jbot::saveState();
    }

    void onTps(CCObject* sender) {
        jbot::state().tpsOn = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
        jbot::saveState();
    }

public:
    static JBotPanel* create() {
        auto ret = new JBotPanel();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class $modify(JBPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto spr = ButtonSprite::create("JBot", 44, true, "bigFont.fnt", "GJ_button_01.png", 26.f, 0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(JBPauseLayer::onJBot));
        btn->setPosition({win.width - 40.f, win.height - 24.f});

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        menu->addChild(btn);

        int priority = -128;
        if (auto ref = this->getChildByType<CCMenu>(0)) priority = ref->getTouchPriority();
        menu->setTouchPriority(priority - 1);

        this->addChild(menu, 100);
    }

    void onJBot(CCObject*) {
        if (auto panel = JBotPanel::create()) panel->show();
    }
};
