#include "SettingsScreen.h"

#include <M5Unified.h>

#include "../core/GfxKit.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"
#include "../core/storage/ProfileStore.h"
#include "config.h"

namespace {
constexpr int kHomeIconSize = 28;
} // namespace

SettingsScreen::SettingsScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void SettingsScreen::onEnter() {
    draw();
}

void SettingsScreen::adjustDailyLimit(int deltaMinutes) {
    int current = app_.profile.dailyLimitMinutesOverride > 0 ? app_.profile.dailyLimitMinutesOverride
                                                               : config::kDailyPlaytimeLimitMinutes;
    current += deltaMinutes;
    if (current < kMinDailyLimit) {
        current = kMinDailyLimit;
    }
    if (current > kMaxDailyLimit) {
        current = kMaxDailyLimit;
    }
    app_.profile.dailyLimitMinutesOverride = static_cast<uint16_t>(current);
    app_.playtime.setDailyLimitMinutes(static_cast<uint16_t>(current));
    profilestore::save(app_.profile);
}

void SettingsScreen::grantBonus() {
    app_.playtime.grantBonusMinutes(kBonusMinutes);
    app_.persistProgress();
}

void SettingsScreen::toggleNightMode() {
    app_.profile.nightModeEnabled = !app_.profile.nightModeEnabled;
    profilestore::save(app_.profile);
}

bool SettingsScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void SettingsScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    const int x = touch.x;
    const int y = touch.y;

    if (y >= kRow0Y && y < kRow0Y + kRowHeight) {
        if (x >= kMinusX1 && x < kMinusX2) {
            adjustDailyLimit(-5);
        } else if (x >= kPlusX1 && x < kPlusX2) {
            adjustDailyLimit(5);
        }
    } else if (y >= kRow1Y && y < kRow1Y + kRowHeight) {
        grantBonus();
    } else if (y >= kRow2Y && y < kRow2Y + kRowHeight) {
        toggleNightMode();
    } else if (y >= kRow3Y && y < kRow3Y + kRowHeight) {
        // Kein erneuter PIN-Check noetig - Settings selbst ist bereits
        // Eltern-PIN-geschuetzt (siehe Klassenkommentar).
        stateMachine_.requestSwitch(ScreenId::DateTimeSet);
        return;
    } else if (y >= kRow4Y && y < kRow4Y + kRowHeight) {
        app_.pinEntrySetNewMode = true;
        stateMachine_.requestSwitch(ScreenId::PinEntry);
        return;
    } else if (y >= kRow5Y && y < kRow5Y + kRowHeight) {
        stateMachine_.requestSwitch(ScreenId::WebSync);
        return;
    }

    draw();
}

void SettingsScreen::update(uint32_t) {
    handleInput();
}

void SettingsScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void SettingsScreen::drawRow(int y, const char* label) const {
    gfxkit::bevelPanel(&M5.Display, 4, y, 312, kRowHeight - 4, 8, theme::kPanel, true);
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString(label, 14, y + (kRowHeight - 4) / 2);
}

void SettingsScreen::draw() {
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), M5.Display.height(),
                              gfxkit::darken(theme::kPanel, 0.6f), theme::kBackground);
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), 26, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Einstellungen", 6, 3);

    // Zeile 0: Tageslimit -/+.
    drawRow(kRow0Y, "Limit");
    const int rowMidY0 = kRow0Y + (kRowHeight - 4) / 2;
    gfxkit::bevelPanel(&M5.Display, kMinusX1, kRow0Y + 6, kMinusX2 - kMinusX1, kRowHeight - 16, 6, theme::kDanger,
                        true);
    gfxkit::bevelPanel(&M5.Display, kPlusX1, kRow0Y + 6, kPlusX2 - kPlusX1, kRowHeight - 16, 6, theme::kSuccess,
                        true);
    M5.Display.setTextColor(theme::kOutline);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("-", (kMinusX1 + kMinusX2) / 2, rowMidY0);
    M5.Display.drawString("+", (kPlusX1 + kPlusX2) / 2, rowMidY0);
    const uint16_t currentLimit = app_.profile.dailyLimitMinutesOverride > 0 ? app_.profile.dailyLimitMinutesOverride
                                                                              : config::kDailyPlaytimeLimitMinutes;
    char limitBuf[16];
    snprintf(limitBuf, sizeof(limitBuf), "%u Min", static_cast<unsigned>(currentLimit));
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextSize(1);
    M5.Display.drawString(limitBuf, (kMinusX2 + kPlusX1) / 2, rowMidY0);
    M5.Display.setTextSize(2);

    // Zeile 1: Bonus-Zeit.
    drawRow(kRow1Y, "Bonus");
    char bonusBuf[24];
    snprintf(bonusBuf, sizeof(bonusBuf), "+%u Min antippen", static_cast<unsigned>(kBonusMinutes));
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_right);
    M5.Display.setTextSize(1);
    M5.Display.drawString(bonusBuf, 306, kRow1Y + (kRowHeight - 4) / 2);

    // Zeile 2: Nachtmodus an/aus.
    drawRow(kRow2Y, "Nachtmodus");
    M5.Display.setTextDatum(middle_right);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(app_.profile.nightModeEnabled ? theme::kSuccess : theme::kDanger);
    M5.Display.drawString(app_.profile.nightModeEnabled ? "AN" : "AUS", 306, kRow2Y + (kRowHeight - 4) / 2);

    // Zeile 3: Uhrzeit einstellen.
    drawRow(kRow3Y, "Uhrzeit");
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_right);
    M5.Display.setTextSize(1);
    M5.Display.drawString("antippen", 306, kRow3Y + (kRowHeight - 4) / 2);

    // Zeile 4: PIN aendern.
    drawRow(kRow4Y, "PIN aendern");
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_right);
    M5.Display.setTextSize(1);
    M5.Display.drawString("antippen", 306, kRow4Y + (kRowHeight - 4) / 2);

    // Zeile 5: Web-Sync starten.
    drawRow(kRow5Y, "Web-Sync");
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_right);
    M5.Display.setTextSize(1);
    M5.Display.drawString("antippen", 306, kRow5Y + (kRowHeight - 4) / 2);

    drawHomeIcon();
}
