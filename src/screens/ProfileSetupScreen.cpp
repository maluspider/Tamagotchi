#include "ProfileSetupScreen.h"

#include <M5Unified.h>
#include <esp_random.h>

#include "../core/PinCode.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/storage/ProfileStore.h"
#include "config.h"

namespace {
constexpr uint16_t kColorYoung = TFT_ORANGE; // 1. Klasse
constexpr uint16_t kColorOld = TFT_CYAN;     // 3. Klasse
} // namespace

ProfileSetupScreen::ProfileSetupScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void ProfileSetupScreen::onEnter() {
    draw();
}

void ProfileSetupScreen::drawChoice(int x, int w, uint8_t klasse, uint16_t color) const {
    const int h = M5.Display.height();
    M5.Display.fillRoundRect(x + 10, 20, w - 20, h - 40, 12, color);

    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(6);
    M5.Display.drawNumber(klasse, x + w / 2, h / 2);
}

void ProfileSetupScreen::draw() {
    M5.Display.fillScreen(TFT_BLACK);
    const int w = M5.Display.width() / 2;
    drawChoice(0, w, 1, kColorYoung);
    drawChoice(w, w, 3, kColorOld);
}

void ProfileSetupScreen::commitSelection(uint8_t klasse) {
    Profile profile;
    profile.klasse = klasse;
    profile.geraetId = String("geraet-") + String(static_cast<unsigned long>(esp_random()), HEX);
    profile.guard = pincode::hash(config::kDefaultParentalCode);
    profile.isValid = true;

    profilestore::save(profile);
    app_.profile = profile;

    app_.character.load(0, "");
    app_.playtime.load(0, 0, rtcclock::todayIso());
    app_.persistProgress();

    stateMachine_.requestSwitch(ScreenId::Home);
}

void ProfileSetupScreen::update(uint32_t) {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    const int half = M5.Display.width() / 2;
    const uint8_t klasse = (touch.x < half) ? 1 : 3;
    commitSelection(klasse);
}
