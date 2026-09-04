#include "ProfileSetupScreen.h"

#include <M5Unified.h>
#include <esp_random.h>

#include "../core/DifficultyTracker.h"
#include "../core/PinCode.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/Subject.h"
#include "../core/Theme.h"
#include "../core/storage/ProfileStore.h"
#include "KidProfiles.h"
#include "config.h"

namespace {
// Wird pro Profil-Zone durchrotiert (falls mehr als 2 Kinder eingetragen
// werden) - siehe include/KidProfiles.h.
constexpr uint16_t kChoiceColors[] = {theme::kAccentOrange, theme::kAccentCyan, theme::kSuccess, theme::kAccentPink};
constexpr size_t kChoiceColorCount = sizeof(kChoiceColors) / sizeof(kChoiceColors[0]);
} // namespace

ProfileSetupScreen::ProfileSetupScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void ProfileSetupScreen::onEnter() {
    draw();
}

void ProfileSetupScreen::drawChoice(int x, int w, size_t profileIndex) const {
    const int h = M5.Display.height();
    const uint16_t color = kChoiceColors[profileIndex % kChoiceColorCount];
    M5.Display.fillRoundRect(x + 10, 20, w - 20, h - 40, 12, color);

    // Kleiner Platzhalter-Kopf ueber dem Namen, damit die Auswahl auch ohne
    // Lesen als "das ist mein Tamagotchi" erkennbar bleibt (Review:
    // icon-first fuer die juengere Zielgruppe, Abschnitt 5).
    const int cx = x + w / 2;
    const int headCy = 20 + (h - 40) / 3;
    M5.Display.fillCircle(cx, headCy, 22, theme::kText);
    M5.Display.drawCircle(cx, headCy, 22, theme::kOutline);
    M5.Display.fillCircle(cx - 8, headCy - 4, 3, theme::kOutline);
    M5.Display.fillCircle(cx + 8, headCy - 4, 3, theme::kOutline);

    M5.Display.setTextColor(theme::kOutline);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(3);
    M5.Display.drawString(kKidProfiles[profileIndex].name, cx, headCy + 55);
}

void ProfileSetupScreen::draw() {
    M5.Display.fillScreen(theme::kBackground);
    const int w = M5.Display.width() / static_cast<int>(kKidProfileCount);
    for (size_t i = 0; i < kKidProfileCount; ++i) {
        drawChoice(static_cast<int>(i) * w, w, i);
    }
}

void ProfileSetupScreen::commitSelection(size_t profileIndex) {
    const KidProfileDefinition& kid = kKidProfiles[profileIndex];

    Profile profile;
    profile.name = kid.name;
    profile.age = kid.age;
    profile.klasse = klasseForAge(kid.age);
    profile.birthdayMonth = kid.birthdayMonth;
    profile.birthdayDay = kid.birthdayDay;
    profile.geraetId = String("geraet-") + String(static_cast<unsigned long>(esp_random()), HEX);
    profile.guard = pincode::hash(config::kDefaultParentalCode);
    profile.isValid = true;

    profilestore::save(profile);
    app_.profile = profile;

    app_.character.load(0, "");
    app_.playtime.load(0, 0, rtcclock::todayIso());
    for (size_t i = 0; i < kSubjectCount; ++i) {
        app_.difficultyBySubject[i] = DifficultyState{};
    }
    app_.persistProgress();

    stateMachine_.requestSwitch(ScreenId::Home);
}

void ProfileSetupScreen::update(uint32_t) {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    const int w = M5.Display.width() / static_cast<int>(kKidProfileCount);
    size_t index = static_cast<size_t>(touch.x / w);
    if (index >= kKidProfileCount) {
        index = kKidProfileCount - 1;
    }
    commitSelection(index);
}
