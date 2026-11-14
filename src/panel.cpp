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

class JBotPanel : public CCLayerColor {
protected:
    CCMenu* m_menu = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCLabelBMFont* m_note = nullptr;
    CCTextInputNode* m_nameInput = nullptr;
    CCTextInputNode* m_speedInput = nullptr;
    CCTextInputNode* m_tpsInput = nullptr;
    CCPoint m_center = {0.f, 0.f};
    std::string m_lastSpeed;
    std::string m_lastTps;

    CCPoint at(CCPoint offset) {
        return {m_center.x + offset.x, m_center.y + offset.y};
    }

    void addButton(char const* text, float width, SEL_MenuHandler cb, CCPoint offset) {
        auto spr = ButtonSprite::create(text, static_cast<int>(width), true, "bigFont.fnt", "GJ_button_01.png", 28.f, 0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, cb);
        btn->setPosition(this->at(offset));
        m_menu->addChild(btn);
    }

    void addLabel(char const* text, CCPoint offset) {
        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setScale(0.5f);
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition(this->at(offset));
        this->addChild(label, 2);
    }

    void addToggler(bool on, SEL_MenuHandler cb, CCPoint offset) {
        auto off = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
        auto onSpr = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        auto toggler = CCMenuItemToggler::create(off, onSpr, this, cb);
        toggler->setScale(0.7f);
        toggler->toggle(on);
        toggler->setPosition(this->at(offset));
        m_menu->addChild(toggler);
    }

    CCTextInputNode* addInput(float width, char const* placeholder, char const* allowed, int maxLen, CCPoint offset) {
        auto bg = cocos2d::extension::CCScale9Sprite::create("square02_small.png");
        bg->setContentSize({width, 30.f});
        bg->setOpacity(90);
        bg->setPosition(this->at(offset));
        this->addChild(bg, 1);

        auto input = CCTextInputNode::create(width - 10.f, 30.f, placeholder, "bigFont.fnt");
        input->setAllowedChars(allowed);
        input->setMaxLabelLength(maxLen);
        input->setTouchPriority(-502);
        input->setPosition(this->at(offset));
        this->addChild(input, 2);
        return input;
    }

    void note(char const* text) {
        m_note->setString(text);
    }

    void close() {
        this->setKeypadEnabled(false);
        this->removeFromParentAndCleanup(true);
    }

    void registerWithTouchDispatcher() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
    }

    bool ccTouchBegan(CCTouch*, CCEvent*) override {
        return true;
    }

    void keyBackClicked() override {
        this->close();
    }

    bool init() override {
        if (!CCLayerColor::initWithColor({0, 0, 0, 120})) return false;
        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        auto win = CCDirector::sharedDirector()->getWinSize();
        m_center = CCPoint(win.width / 2.f, win.height / 2.f);

        auto bg = cocos2d::extension::CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({320.f, 260.f});
        bg->setPosition(m_center);
        this->addChild(bg, 0);

        m_menu = CCMenu::create();
        m_menu->setPosition({0.f, 0.f});
        m_menu->setTouchPriority(-501);
        this->addChild(m_menu, 3);

        auto title = CCLabelBMFont::create("JBot", "goldFont.fnt");
        title->setScale(0.8f);
        title->setPosition(this->at({0.f, 112.f}));
        this->addChild(title, 2);

        auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(JBotPanel::onCloseBtn));
        closeBtn->setPosition(this->at({-152.f, 122.f}));
        m_menu->addChild(closeBtn);

        auto& s = jbot::state();

        m_status = CCLabelBMFont::create("", "bigFont.fnt");
        m_status->setScale(0.4f);
        m_status->setPosition(this->at({0.f, 92.f}));
        this->addChild(m_status, 2);

        this->addButton("Record", 80.f, menu_selector(JBotPanel::onRecord), {-100.f, 62.f});
        this->addButton("Play", 80.f, menu_selector(JBotPanel::onPlay), {0.f, 62.f});
        this->addButton("Stop", 80.f, menu_selector(JBotPanel::onStop), {100.f, 62.f});

        m_nameInput = this->addInput(120.f, "macro name", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_- ", 32, {-85.f, 28.f});
        this->addButton("Save", 50.f, menu_selector(JBotPanel::onSave), {20.f, 28.f});
        this->addButton("Load", 50.f, menu_selector(JBotPanel::onLoad), {85.f, 28.f});

        this->addLabel("Noclip", {-135.f, -6.f});
        this->addToggler(s.noclip, menu_selector(JBotPanel::onNoclip), {115.f, -6.f});

        char buf[32];

        this->addLabel("Speedhack", {-135.f, -34.f});
        m_speedInput = this->addInput(80.f, "1.0", "0123456789.", 6, {60.f, -34.f});
        std::snprintf(buf, sizeof(buf), "%g", s.speed);
        m_lastSpeed = buf;
        m_speedInput->setString(m_lastSpeed);

        this->addLabel("Speed audio", {-135.f, -62.f});
        this->addToggler(s.speedAudio, menu_selector(JBotPanel::onSpeedAudio), {115.f, -62.f});

        this->addLabel("TPS bypass", {-135.f, -90.f});
        m_tpsInput = this->addInput(80.f, "240", "0123456789.", 7, {60.f, -90.f});
        std::snprintf(buf, sizeof(buf), "%g", s.tps);
        m_lastTps = buf;
        m_tpsInput->setString(m_lastTps);
        this->addToggler(s.tpsOn, menu_selector(JBotPanel::onTps), {115.f, -90.f});

        m_note = CCLabelBMFont::create("", "bigFont.fnt");
        m_note->setScale(0.35f);
        m_note->setPosition(this->at({0.f, -118.f}));
        this->addChild(m_note, 2);

        this->refresh(0.f);
        this->schedule(schedule_selector(JBotPanel::refresh), 0.1f);
        return true;
    }

    void applyInputs() {
        std::string speed = m_speedInput->getString();
        if (speed != m_lastSpeed) {
            m_lastSpeed = speed;
            char* end = nullptr;
            double v = std::strtod(speed.c_str(), &end);
            if (end != speed.c_str() && v > 0.0) jbot::setSpeed(v);
        }

        std::string tps = m_tpsInput->getString();
        if (tps != m_lastTps) {
            m_lastTps = tps;
            char* end = nullptr;
            double v = std::strtod(tps.c_str(), &end);
            if (end != tps.c_str() && v >= 1.0) {
                jbot::setTps(static_cast<float>(v));
                jbot::saveState();
            }
        }
    }

    void refresh(float) {
        this->applyInputs();
        char buf[96];
        char const* name = "Idle";
        if (jbot::mode() == jbot::Record) name = "Recording";
        else if (jbot::mode() == jbot::Play) name = "Playing";
        std::snprintf(buf, sizeof(buf), "%s  |  %u inputs  |  %.2fx  |  %g tps",
            name, jbot_rep_count(), jbot::state().speed, jbot::effectiveTps());
        m_status->setString(buf);
    }

    void onCloseBtn(CCObject*) {
        this->close();
    }

    void onRecord(CCObject*) {
        if (jbot::startRecording()) this->close();
        else this->note("Open a level first");
    }

    void onPlay(CCObject*) {
        if (jbot::startPlayback()) this->close();
        else this->note("No macro to play");
    }

    void onStop(CCObject*) {
        jbot::stopReplay();
        this->note("Stopped");
    }

    void onSave(CCObject*) {
        std::string name = m_nameInput->getString();
        if (jbot::sanitizeName(name).empty()) this->note("Enter a macro name");
        else if (jbot_rep_count() == 0) this->note("Nothing to save");
        else if (jbot::saveMacro(name)) this->note("Macro saved");
        else this->note("Could not save macro");
    }

    void onLoad(CCObject*) {
        std::string name = m_nameInput->getString();
        if (jbot::sanitizeName(name).empty()) {
            this->note("Enter a macro name");
            return;
        }
        jbot::stopReplay();
        if (jbot::loadMacro(name)) this->note("Macro loaded");
        else this->note("Could not load macro");
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

    void show() {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (scene) scene->addChild(this, 1000);
    }
};

class $modify(JBPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto win = CCDirector::sharedDirector()->getWinSize();
        // The "ՋBot" name is Armenian, which GD's bitmap fonts cannot draw, so the
        // button is a prebuilt image (resources/jbot-button.png). If that image
        // is missing, fall back to a plain "JBot" text button so it still works.
        CCNode* spr = CCSprite::create("jbot-button.png"_spr);
        if (spr) {
            spr->setScale(0.42f);
        } else {
            spr = ButtonSprite::create("JBot", 44, true, "bigFont.fnt", "GJ_button_01.png", 26.f, 0.6f);
        }
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(JBPauseLayer::onJBot));
        btn->setID("jbot-button"_spr);
        btn->setPosition({win.width - 46.f, win.height - 26.f});

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
