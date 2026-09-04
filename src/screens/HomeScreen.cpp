#include "HomeScreen.h"

#include <M5Unified.h>
#include <SD.h>

#include <cmath>

#include "../core/CharacterEngine.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "config.h"

namespace {

// Platzhalter-Farben/-Groessen pro Entwicklungsstufe (Abschnitt 9) - nur
// noch als Fallback aktiv, wenn kein Sprite von der SD-Karte geladen werden
// kann (siehe HomeScreen::drawSpriteCharacter()).
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

// Skalierung der 32x32-Sprite-Vorlage je Stufe - steigt leicht mit der
// Stufe an, damit das sichtbare Wachstum aus Abschnitt 9 erhalten bleibt
// (die Sprites selbst unterscheiden sich vor allem durch Details/Accessoires,
// weniger durch rohe Groesse).
float spriteScaleForStage(CharacterStage stage) {
    switch (stage) {
        case CharacterStage::Ei: return 2.6f;
        case CharacterStage::Baby: return 2.8f;
        case CharacterStage::Kind: return 3.0f;
        case CharacterStage::Junior: return 3.2f;
        case CharacterStage::Experte: return 3.4f;
        case CharacterStage::Meister: return 3.6f;
    }
    return 3.0f;
}

constexpr uint32_t kRedrawIntervalMs = 1000; // Uhrzeit-Anzeige reicht sekundengenau
constexpr int kBottomBarHeight = 40;

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

    const auto touch = M5.Touch.getDetail();
    if (touch.wasPressed() && touch.y >= M5.Display.height() - kBottomBarHeight) {
        handleBottomBarTouch(touch.x, touch.y);
        return;
    }

    msSinceLastRedraw_ += deltaMs;
    if (msSinceLastRedraw_ >= kRedrawIntervalMs) {
        msSinceLastRedraw_ = 0;
        draw();
    }
}

void HomeScreen::handleBottomBarTouch(int x, int /*y*/) {
    const int zoneW = M5.Display.width() / 4;
    const int zone = x / zoneW;

    if (zone == 0) {
        stateMachine_.requestSwitch(ScreenId::SubjectSelect);
        return;
    }
    if (zone == 1) {
        const bool unlocked = app_.character.stage() >= CharacterStage::Baby;
        const bool hasTime = app_.playtime.availableMinutes() > 0;
        if (unlocked && hasTime) {
            stateMachine_.requestSwitch(ScreenId::GamesMenu);
        }
        return;
    }
    if (zone == 2) {
        stateMachine_.requestSwitch(ScreenId::AlltagMenu);
        return;
    }
    // Einstellungen sind Eltern-PIN-geschuetzt (Abschnitt 11).
    app_.pinEntrySetNewMode = false;
    stateMachine_.requestSwitch(ScreenId::PinEntry);
}

bool HomeScreen::drawSpriteCharacter() {
    const CharacterStage stage = app_.character.stage();
    const String today = rtcclock::todayIso();
    const bool sad = app_.character.isSad(today);

    const char* mood;
    if (sad) {
        mood = "sad";
    } else {
        // Wechselt bei jedem Redraw-Tick (Abschnitt 5: 1x/Sekunde) zwischen
        // offenen und geschlossenen Augen - ein einfaches, aber sichtbares
        // Blinzeln ohne eigene Animationsschleife.
        spriteBlinkToggle_ = !spriteBlinkToggle_;
        mood = spriteBlinkToggle_ ? "idle2" : "idle1";
    }

    const String path = String(config::kSpriteCharacterDir) + CharacterEngine::stageAssetKey(stage) + "_" + mood + ".png";
    if (!SD.exists(path)) {
        // Kein Sprite auf der SD-Karte (oder Karte fehlt) - Aufrufer
        // zeichnet stattdessen die Platzhalter-Grafik.
        return false;
    }

    const float scale = spriteScaleForStage(stage);
    const int cx = M5.Display.width() / 2;
    const int cy = M5.Display.height() / 2;
    M5.Display.drawPngFile(SD, path.c_str(), cx, cy, 0, 0, 0, 0, scale, scale, middle_center);

    if (app_.profile.name.length() > 0) {
        const int halfHeight = static_cast<int>(config::kSpriteSourceSizePx * scale / 2.0f);
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.setTextDatum(top_center);
        M5.Display.setTextSize(2);
        M5.Display.drawString(app_.profile.name.c_str(), cx, cy + halfHeight + 6);
    }
    return true;
}

void HomeScreen::drawPlaceholderCharacter() const {
    const int cx = M5.Display.width() / 2;
    const int cy = M5.Display.height() / 2;
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

    // Name des Kindes/Charakters (aus dem beim Erststart gewaehlten Profil,
    // include/KidProfiles.h) unter dem Platzhalter-Charakter.
    if (app_.profile.name.length() > 0) {
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.setTextDatum(top_center);
        M5.Display.setTextSize(2);
        M5.Display.drawString(app_.profile.name.c_str(), cx, cy + r + 8);
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

void HomeScreen::drawBottomBar() const {
    const int y = M5.Display.height() - kBottomBarHeight;
    M5.Display.fillRect(0, y, M5.Display.width(), kBottomBarHeight, TFT_NAVY);

    const int zoneW = M5.Display.width() / 4;

    // Aufgaben: Stift-Symbol (immer verfuegbar, Abschnitt 5).
    {
        const int cx = zoneW / 2;
        M5.Display.drawLine(cx - 8, y + 28, cx + 8, y + 12, TFT_WHITE);
        M5.Display.drawLine(cx - 8, y + 28, cx - 4, y + 24, TFT_WHITE);
        M5.Display.fillTriangle(cx + 6, y + 10, cx + 10, y + 14, cx + 8, y + 16, TFT_YELLOW);
    }

    // Spiele: Play-Dreieck - ausgegraut, solange nicht freigeschaltet
    // (Stufe < Baby) oder kein Spielzeitguthaben vorhanden ist (Abschnitt 7:
    // "Spiele-Menü nur mit vorhandenem Zeitguthaben betretbar").
    {
        const int cx = zoneW + zoneW / 2;
        const bool unlocked = app_.character.stage() >= CharacterStage::Baby;
        const bool hasTime = app_.playtime.availableMinutes() > 0;
        const uint16_t color = (unlocked && hasTime) ? TFT_WHITE : TFT_DARKGREY;
        M5.Display.fillTriangle(cx - 8, y + 10, cx - 8, y + 30, cx + 10, y + 20, color);
    }

    // Alltag: Kreis mit Zeigern (Uhr stellvertretend fuers Alltags-Menue).
    {
        const int cx = 2 * zoneW + zoneW / 2;
        const int cy = y + 20;
        M5.Display.drawCircle(cx, cy, 12, TFT_WHITE);
        M5.Display.drawLine(cx, cy, cx, cy - 8, TFT_WHITE);
        M5.Display.drawLine(cx, cy, cx + 6, cy, TFT_WHITE);
    }

    // Einstellungen: einfaches Zahnrad-Symbol.
    {
        const int cx = 3 * zoneW + zoneW / 2;
        const int cy = y + 20;
        M5.Display.drawCircle(cx, cy, 10, TFT_WHITE);
        M5.Display.drawCircle(cx, cy, 4, TFT_WHITE);
        for (int i = 0; i < 6; ++i) {
            const float angle = static_cast<float>(i) * 3.14159f / 3.0f;
            const int tx = cx + static_cast<int>(13.0f * cosf(angle));
            const int ty = cy + static_cast<int>(13.0f * sinf(angle));
            M5.Display.fillCircle(tx, ty, 2, TFT_WHITE);
        }
    }
}

void HomeScreen::draw() {
    M5.Display.fillScreen(TFT_BLACK);
    if (!drawSpriteCharacter()) {
        drawPlaceholderCharacter();
    }
    drawStatusBar();
    drawBottomBar();
}
