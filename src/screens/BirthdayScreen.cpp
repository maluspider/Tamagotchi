#include "BirthdayScreen.h"

#include <M5Unified.h>

#include "../core/GfxKit.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;

constexpr const char* kMonthNames[12] = {
    "Januar", "Februar", "Maerz", "April", "Mai", "Juni",
    "Juli", "August", "September", "Oktober", "November", "Dezember",
};

// Feste (nicht zufaellige) Konfetti-Punkte hinter der Torte statt einer
// leeren Flaeche - Nutzerwunsch "grafiken im tools menue ebenfalls maximal
// verbessern". Relative Positionen (0..1 der Bildschirmbreite) statt
// Pixelwerten, damit sie unabhaengig von der tatsaechlichen Displaybreite
// bleiben; y in festen Pixeln im ruhigen Bereich zwischen Titelleiste und
// Countdown-Text.
struct ConfettiDot {
    float relX;
    int y;
    int size;
};
constexpr ConfettiDot kConfetti[10] = {
    {0.12f, 40, 3}, {0.24f, 95, 4}, {0.85f, 45, 3}, {0.90f, 100, 4}, {0.06f, 70, 3},
    {0.94f, 72, 3}, {0.18f, 120, 4}, {0.80f, 122, 3}, {0.30f, 32, 3}, {0.72f, 34, 4},
};
} // namespace

BirthdayScreen::BirthdayScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void BirthdayScreen::onEnter() {
    draw();
}

void BirthdayScreen::update(uint32_t) {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }
    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::AlltagMenu);
    }
}

bool BirthdayScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void BirthdayScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void BirthdayScreen::drawCake(int cx, int cy) const {
    const int w = 70;
    const int h = 40;
    // Gebevelte Tortenboeden statt Flat-Fill (Nutzerwunsch: "so
    // professionell wie moeglich...90er-Jahre-Videogames").
    gfxkit::bevelPanel(&M5.Display, cx - w / 2, cy, w, h / 2, 6, theme::kAccentPink, true);
    gfxkit::bevelPanel(&M5.Display, cx - w / 3, cy - h / 3, w * 2 / 3, h / 3, 6, theme::kAccentCyan, true);
    // Zuckerguss-Streifen zwischen den Boeden.
    M5.Display.fillRect(cx - w / 2, cy - 2, w, 4, theme::kAccentGold);
    // Kerze + Flamme mit Glanzlicht.
    M5.Display.fillRect(cx - 2, cy - h / 3 - 16, 4, 16, theme::kText);
    gfxkit::shinyBall(&M5.Display, cx, cy - h / 3 - 20, 5, theme::kAccentOrange);
}

long BirthdayScreen::daysUntilBirthday() const {
    // Zaehlt bis zum naechsten Jahrestag von Monat/Tag - kein Geburtsjahr
    // noetig, siehe include/KidProfiles.h. Faellt der Geburtstag auf einen
    // 29. Februar, behandelt die Kalenderarithmetik (RtcClock.cpp) das in
    // Nicht-Schaltjahren als 1. Maerz - eine bewusst hingenommene, seltene
    // Vereinfachung statt einer eigenen Schaltjahr-Sonderbehandlung.
    const String today = rtcclock::todayIso();
    const int year = today.substring(0, 4).toInt();
    const long todayEpoch = rtcclock::todayEpochDay();

    char buf[11];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, app_.profile.birthdayMonth, app_.profile.birthdayDay);
    long candidate = rtcclock::epochDayFromIso(String(buf));
    if (candidate < todayEpoch) {
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year + 1, app_.profile.birthdayMonth, app_.profile.birthdayDay);
        candidate = rtcclock::epochDayFromIso(String(buf));
    }
    return candidate - todayEpoch;
}

void BirthdayScreen::draw() {
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), M5.Display.height(),
                              gfxkit::darken(theme::kPanel, 0.6f), theme::kBackground);
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), 26, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Geburtstag", 6, 3);

    const int cx = M5.Display.width() / 2;

    if (app_.profile.birthdayMonth == 0 || app_.profile.birthdayDay == 0) {
        // Kein Geburtstag hinterlegt (z. B. ein Profil, das vor dieser
        // Funktion angelegt wurde) - Hinweis statt eines falschen
        // Countdowns.
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(theme::kTextDim);
        M5.Display.setTextSize(2);
        M5.Display.drawString("Kein Geburtstag hinterlegt", cx, 100);
        M5.Display.setTextSize(1);
        M5.Display.drawString("Siehe include/KidProfiles.h", cx, 130);
        drawHomeIcon();
        return;
    }

    // Konfetti hinter der Torte zeichnen (siehe kConfetti oben) - feste
    // Akzentfarben im Wechsel statt einer einzigen Farbe, fuer den
    // festlichen Effekt.
    constexpr uint16_t kConfettiColors[4] = {theme::kAccentPink, theme::kAccentCyan, theme::kAccentGold,
                                              theme::kAccentOrange};
    for (int i = 0; i < 10; ++i) {
        const auto& dot = kConfetti[i];
        const int x = static_cast<int>(dot.relX * M5.Display.width());
        M5.Display.fillRect(x, dot.y, dot.size, dot.size, kConfettiColors[i % 4]);
    }

    drawCake(cx, 70);

    const long days = daysUntilBirthday();
    M5.Display.setTextDatum(middle_center);

    if (days == 0) {
        M5.Display.setTextColor(theme::kAccentGold);
        M5.Display.setTextSize(3);
        M5.Display.drawString("Alles Gute,", cx, 150);
        M5.Display.drawString(app_.profile.name, cx, 185);
    } else {
        M5.Display.setTextColor(theme::kText);
        M5.Display.setTextSize(6);
        M5.Display.drawNumber(days, cx, 150);
        M5.Display.setTextSize(2);
        M5.Display.drawString(days == 1 ? "Tag bis" : "Tage bis", cx, 190);
        M5.Display.drawString(String(app_.profile.name) + "s Geburtstag", cx, 214);
    }

    const int monthIndex = (app_.profile.birthdayMonth >= 1 && app_.profile.birthdayMonth <= 12)
                                ? app_.profile.birthdayMonth - 1
                                : 0;
    char dateBuf[8];
    snprintf(dateBuf, sizeof(dateBuf), "%d. ", app_.profile.birthdayDay);
    M5.Display.setTextColor(theme::kTextDim);
    M5.Display.setTextSize(1);
    M5.Display.drawString(String(dateBuf) + kMonthNames[monthIndex], cx, 232);

    drawHomeIcon();
}
