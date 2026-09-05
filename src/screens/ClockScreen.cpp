#include "ClockScreen.h"

#include <M5Unified.h>

#include "../core/GfxKit.h"
#include "../core/NightModeService.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"
#include "../core/storage/ProfileStore.h"

namespace {
constexpr uint32_t kRedrawIntervalMs = 1000;
constexpr int kHomeIconSize = 28;

// Tap-Flaechen fuer die Wecker-Einstellung (grosszuegig bemessen,
// kindgerecht).
constexpr int kBellCenterY = 150;
constexpr int kBellRadius = 22;
constexpr int kBellTouchHalfWidth = 40;
constexpr int kBellTouchTop = kBellCenterY - kBellTouchHalfWidth;
constexpr int kBellTouchBottom = kBellCenterY + kBellTouchHalfWidth;

constexpr int kAlarmLabelY = 195;
constexpr int kStepperTop = 214;
constexpr int kStepperHeight = 24;
} // namespace

ClockScreen::ClockScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void ClockScreen::onEnter() {
    msSinceLastRedraw_ = kRedrawIntervalMs;
    draw();
}

bool ClockScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void ClockScreen::handleTouch(int x, int y) {
    const int centerX = M5.Display.width() / 2;

    if (y >= kBellTouchTop && y <= kBellTouchBottom && x >= centerX - kBellTouchHalfWidth &&
        x <= centerX + kBellTouchHalfWidth) {
        app_.profile.alarmEnabled = !app_.profile.alarmEnabled;
        profilestore::save(app_.profile);
        return;
    }

    if (y >= kStepperTop && y <= kStepperTop + kStepperHeight) {
        const int zoneW = M5.Display.width() / 4;
        const int zone = x / zoneW;
        switch (zone) {
            case 0: app_.profile.alarmHour = static_cast<uint8_t>((app_.profile.alarmHour + 23) % 24); break;
            case 1: app_.profile.alarmHour = static_cast<uint8_t>((app_.profile.alarmHour + 1) % 24); break;
            case 2: app_.profile.alarmMinute = static_cast<uint8_t>((app_.profile.alarmMinute + 59) % 60); break;
            case 3: app_.profile.alarmMinute = static_cast<uint8_t>((app_.profile.alarmMinute + 1) % 60); break;
            default: return;
        }
        profilestore::save(app_.profile);
    }
}

void ClockScreen::update(uint32_t deltaMs) {
    const auto touch = M5.Touch.getDetail();
    if (touch.wasPressed()) {
        if (touchedHomeIcon(touch.x, touch.y)) {
            stateMachine_.requestSwitch(ScreenId::AlltagMenu);
            return;
        }
        handleTouch(touch.x, touch.y);
        draw();
        return;
    }

    msSinceLastRedraw_ += deltaMs;
    if (msSinceLastRedraw_ >= kRedrawIntervalMs) {
        msSinceLastRedraw_ = 0;
        draw();
    }
}

void ClockScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void ClockScreen::draw() {
    // Himmel-Hintergrund analog zu HomeScreen statt einer flachen
    // Ein-Farb-Flaeche (Nutzerwunsch: "grafiken im tools menue ebenfalls
    // maximal verbessern") - Sternenfeld bei Nacht, warmer Verlauf tagsueber,
    // beides passend zur tatsaechlichen Uhrzeit (nightmodeservice::isNight),
    // damit die Uhr optisch zeigt, was sie anzeigt.
    const bool isNight = nightmodeservice::isNight(app_.profile);
    if (isNight) {
        gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), M5.Display.height(), theme::kOutline,
                                  theme::kBackground);
        gfxkit::starfield(&M5.Display, M5.Display.width(), M5.Display.height() - 30, 36, theme::kTextDim);
    } else {
        gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), M5.Display.height(),
                                  gfxkit::darken(theme::kPanel, 0.6f), theme::kBackground);
    }

    m5::rtc_time_t time_;
    M5.Rtc.getTime(&time_);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", time_.hours, time_.minutes);

    const int centerX = M5.Display.width() / 2;

    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(7);
    M5.Display.drawString(buf, centerX, 70);

    // Glocke: gefuellt = Wecker aktiv, nur Umriss = inaktiv. Glanzlicht-
    // Kugel statt Flat-Kreis (Nutzerwunsch: "so professionell wie moeglich
    // ... 90er-Jahre-Videogames").
    const uint16_t bellColor = app_.profile.alarmEnabled ? theme::kAccentGold : theme::kMuted;
    gfxkit::shinyBall(&M5.Display, centerX, kBellCenterY, kBellRadius, bellColor);

    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextSize(3);
    char alarmBuf[6];
    snprintf(alarmBuf, sizeof(alarmBuf), "%02d:%02d", app_.profile.alarmHour, app_.profile.alarmMinute);
    M5.Display.drawString(alarmBuf, centerX, kAlarmLabelY);

    const int zoneW = M5.Display.width() / 4;
    for (int i = 0; i < 4; ++i) {
        gfxkit::bevelPanel(&M5.Display, i * zoneW + 4, kStepperTop, zoneW - 8, kStepperHeight, 4, theme::kPanel,
                            true);
    }
    M5.Display.setTextSize(3);
    M5.Display.drawString("-", zoneW * 0 + zoneW / 2, kStepperTop + kStepperHeight / 2);
    M5.Display.drawString("+", zoneW * 1 + zoneW / 2, kStepperTop + kStepperHeight / 2);
    M5.Display.drawString("-", zoneW * 2 + zoneW / 2, kStepperTop + kStepperHeight / 2);
    M5.Display.drawString("+", zoneW * 3 + zoneW / 2, kStepperTop + kStepperHeight / 2);

    drawHomeIcon();
}
