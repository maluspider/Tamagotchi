#include "HomeScreen.h"

#include <M5Unified.h>

#include "../core/CharacterEngine.h"
#include "../core/RtcClock.h"
#include "config.h"

namespace {

// Platzhalter-Farben/-Groessen pro Entwicklungsstufe (Abschnitt 9) - werden
// ersetzt, sobald echte Sprite-Assets von der SD-Karte geladen werden
// (Abschnitt 4).
uint16_t colorForStage(CharacterStage stage) {
    switch (stage) {
        case CharacterStage::Ei: return TFT_WHITE;
        case CharacterStage::Baby: return TFT_YELLOW;
        case CharacterStage::Kind: return TFT_GREEN;
        case CharacterStage::Junior: return TFT_CYAN;
        case CharacterStage::Experte: return TFT_ORANGE;
        case CharacterStage::Meister: return TFT_PINK;
    }
    return TFT_WHITE;
}

int radiusForStage(CharacterStage stage) {
    return 20 + static_cast<int>(stage) * 6; // waechst sichtbar mit der Stufe
}

constexpr uint32_t kRedrawIntervalMs = 1000; // Uhrzeit-Anzeige reicht sekundengenau

} // namespace

HomeScreen::HomeScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void HomeScreen::onEnter() {
    msSinceLastRedraw_ = kRedrawIntervalMs; // beim Betreten sofort zeichnen
    lowBatterySaveDone_ = false;
    draw();
}

void HomeScreen::update(uint32_t deltaMs) {
    const String today = rtcclock::todayIso();
    app_.playtime.rolloverIfNewDay(today);

    if (!lowBatterySaveDone_ && M5.Power.getBatteryLevel() <= config::kCriticalBatterySavePercent) {
        // Review-Hinweis (Abschnitt 2/15): kleiner Akku - Fortschritt vor
        // moeglichem Abschalten sichern, statt zu riskieren, dass mitten im
        // naechsten Schreibvorgang der Strom ausgeht.
        app_.persistProgress();
        lowBatterySaveDone_ = true;
    }

    msSinceLastRedraw_ += deltaMs;
    if (msSinceLastRedraw_ >= kRedrawIntervalMs) {
        msSinceLastRedraw_ = 0;
        draw();
    }
}

void HomeScreen::drawPlaceholderCharacter() const {
    const int cx = M5.Display.width() / 2;
    const int cy = M5.Display.height() / 2 + 10;
    const CharacterStage stage = app_.character.stage();
    const int r = radiusForStage(stage);
    const uint16_t color = colorForStage(stage);

    M5.Display.fillCircle(cx, cy, r, color);
    M5.Display.drawCircle(cx, cy, r, TFT_BLACK);

    const String today = rtcclock::todayIso();
    if (app_.character.isSad(today)) {
        // Traurige Augen (nach unten geneigte Striche statt Punkte) -
        // einziger Ausloeser ist Inaktivitaet, nie eine falsche Antwort
        // (Abschnitt 7/9, Review).
        M5.Display.drawLine(cx - r / 2, cy - r / 4, cx - r / 4, cy - r / 3, TFT_BLACK);
        M5.Display.drawLine(cx + r / 4, cy - r / 3, cx + r / 2, cy - r / 4, TFT_BLACK);
    } else {
        M5.Display.fillCircle(cx - r / 3, cy - r / 4, 3, TFT_BLACK);
        M5.Display.fillCircle(cx + r / 3, cy - r / 4, 3, TFT_BLACK);
    }
}

void HomeScreen::drawStatusBar() const {
    M5.Display.fillRect(0, 0, M5.Display.width(), 30, TFT_NAVY);

    m5::rtc_time_t time_;
    M5.Rtc.getTime(&time_);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", time_.hours, time_.minutes);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString(buf, 6, 6);

    // Verfuegbare Spielzeit als Zahl + kleines Dreieck-Icon statt Textlabel
    // (Review: Icon statt Wort fuer die juengere Zielgruppe, Abschnitt 5).
    const uint16_t available = app_.playtime.availableMinutes();
    M5.Display.setTextDatum(top_right);
    M5.Display.drawNumber(available, M5.Display.width() - 10, 6);
    const int iconX = M5.Display.width() - 34;
    M5.Display.fillTriangle(iconX, 8, iconX, 22, iconX + 12, 15, TFT_WHITE);

    if (M5.Power.getBatteryLevel() <= config::kLowBatteryWarningPercent) {
        M5.Display.drawRect(M5.Display.width() / 2 - 12, 8, 20, 12, TFT_RED);
        M5.Display.fillRect(M5.Display.width() / 2 + 8, 11, 3, 6, TFT_RED);
    }
}

void HomeScreen::draw() {
    M5.Display.fillScreen(TFT_BLACK);
    drawPlaceholderCharacter();
    drawStatusBar();
}
