#include "HomeScreen.h"

#include <M5Unified.h>

#include <cmath>

#include "../core/CharacterEngine.h"
#include "../core/GfxKit.h"
#include "../core/NightModeService.h"
#include "../core/RetroBackdrop.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"
#include "config.h"

namespace {

// Platzhalter-Farben/-Groessen pro Entwicklungsstufe (Abschnitt 9) - nur
// noch als Fallback aktiv, wenn kein Sprite von der SD-Karte geladen werden
// kann (siehe HomeScreen::drawSpriteCharacter()).
uint16_t colorForStage(CharacterStage stage) {
    switch (stage) {
        case CharacterStage::Ei: return theme::kMuted;
        case CharacterStage::Baby: return theme::kAccentGold;
        case CharacterStage::Kind: return theme::kSuccess;
        case CharacterStage::Junior: return theme::kAccentCyan;
        case CharacterStage::Experte: return theme::kAccentOrange;
        case CharacterStage::Meister: return theme::kAccentPink;
    }
    return theme::kMuted;
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

constexpr uint32_t kRedrawIntervalMs = 1000; // Blinzeln/Uhrzeit-Text reichen sekundengenau
// Nutzerwunsch: "untere Statusbar kann leicht verkleinert werden, um mehr
// vom Homebildschirm zu sehen" - von 40 auf 32px reduziert (Icon-
// Koordinaten in drawBottomBar() proportional mitskaliert).
constexpr int kBottomBarHeight = 32;

// Nutzerwunsch: "Figur soll sich durch Bewegen des Geraets langsam smooth
// bewegen lassen" - Neigungsbereich (Ziel-Versatz in Pixeln bei voller
// Neigung) und Glaettungsfaktor. Bewusst kleiner/langsamer als das
// Fadenkreuz in MoorhuhnJagdScreen (dort soll es direkt reagieren, hier
// nur ein sanftes Mitschwingen).
constexpr float kCharacterTiltRangeX = 34.0f;
constexpr float kCharacterTiltRangeY = 22.0f;
constexpr float kCharacterSmoothing = 0.06f;

} // namespace

HomeScreen::HomeScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void HomeScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
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

    // Fuer ein sichtbar fluessiges Mitschwingen der Figur muss dieser Screen
    // jeden Frame neu zeichnen, nicht mehr nur 1x/Sekunde (das reichte fuer
    // die reine Digitaluhr, aber nicht fuer eine "smooth" Bewegung).
    updateCharacterDrift(deltaMs);

    msSinceLastRedraw_ += deltaMs;
    if (msSinceLastRedraw_ >= kRedrawIntervalMs) {
        msSinceLastRedraw_ = 0;
        // Wechselt bei jedem Blinzel-Tick (weiterhin 1x/Sekunde) zwischen
        // offenen und geschlossenen Augen - siehe drawSpriteCharacter().
        spriteBlinkToggle_ = !spriteBlinkToggle_;
    }
    draw();
}

void HomeScreen::updateCharacterDrift(uint32_t deltaMs) {
    (void)deltaMs;
    M5.Imu.update();
    const auto imuData = M5.Imu.getImuData();

    // Nutzer-Feedback: "Gyrobewegungen sind invertiert" - Vorzeichen beider
    // Achsen gedreht, damit ein Neigen in eine Richtung die Figur auch
    // sichtbar in diese Richtung zieht statt entgegengesetzt.
    const float targetX = -imuData.accel.x * kCharacterTiltRangeX;
    const float targetY = imuData.accel.y * kCharacterTiltRangeY;

    characterOffsetX_ += (targetX - characterOffsetX_) * kCharacterSmoothing;
    characterOffsetY_ += (targetY - characterOffsetY_) * kCharacterSmoothing;
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
        // Nutzerwunsch: "Figur schlaeft zwischen 20:00 und 07:00, in
        // dieser Zeit kann nichts gespielt werden" - siehe auch
        // PlaytimeTicker (deckt eine bereits laufende Sitzung ab) und
        // GamesMenuScreen (deckt den Fall ab, dass die Nachtstunden
        // waehrend des Verweilens im Spiele-Menue beginnen).
        const bool isNight = nightmodeservice::isNight(app_.profile);
        if (unlocked && hasTime && !isNight) {
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

    // Blinzel-Tick (spriteBlinkToggle_) wird zentral in update() 1x/Sekunde
    // umgeschaltet, nicht hier - draw() laeuft inzwischen jeden Frame fuer
    // das sanfte Mitschwingen (siehe updateCharacterDrift()), ein Umschalten
    // an dieser Stelle wuerde daher jeden Frame statt einmal pro Sekunde
    // blinzeln lassen.
    const char* mood = sad ? "sad" : (spriteBlinkToggle_ ? "idle2" : "idle1");

    const float scale = spriteScaleForStage(stage);
    const int cx = M5.Display.width() / 2 + static_cast<int>(characterOffsetX_);
    const int cy = M5.Display.height() / 2 + static_cast<int>(characterOffsetY_);
    const int halfHeightForShadow = static_cast<int>(config::kSpriteSourceSizePx * scale / 2.0f);
    drawGroundShadow(cx, cy + halfHeightForShadow - 4, static_cast<int>(18 * scale / 3.0f));
    if (!characterRenderer_.draw(stage, mood, app_.profile, cx, cy, scale, &canvas_)) {
        // Kein Sprite auf der SD-Karte (oder Karte fehlt) - Aufrufer
        // zeichnet stattdessen die Platzhalter-Grafik.
        return false;
    }

    if (app_.profile.name.length() > 0) {
        const int halfHeight = static_cast<int>(config::kSpriteSourceSizePx * scale / 2.0f);
        canvas_.setTextColor(theme::kText);
        canvas_.setTextDatum(top_center);
        canvas_.setTextSize(2);
        canvas_.drawString(app_.profile.name.c_str(), cx, cy + halfHeight + 6);
    }
    return true;
}

void HomeScreen::drawPlaceholderCharacter() {
    const int cx = M5.Display.width() / 2 + static_cast<int>(characterOffsetX_);
    const int cy = M5.Display.height() / 2 + static_cast<int>(characterOffsetY_);
    const CharacterStage stage = app_.character.stage();
    const int r = radiusForStage(stage);
    const uint16_t color = colorForStage(stage);

    drawGroundShadow(cx, cy + r - 4, r);
    canvas_.fillCircle(cx, cy, r, color);
    canvas_.drawCircle(cx, cy, r, theme::kOutline);

    const String today = rtcclock::todayIso();
    if (app_.character.isSad(today)) {
        // Traurige Augen (nach unten geneigte Striche statt Punkte) -
        // einziger Ausloeser ist Inaktivitaet, nie eine falsche Antwort
        // (Abschnitt 7/9, Review).
        canvas_.drawLine(cx - r / 2, cy - r / 4, cx - r / 4, cy - r / 3, theme::kOutline);
        canvas_.drawLine(cx + r / 4, cy - r / 3, cx + r / 2, cy - r / 4, theme::kOutline);
    } else {
        canvas_.fillCircle(cx - r / 3, cy - r / 4, 3, theme::kOutline);
        canvas_.fillCircle(cx + r / 3, cy - r / 4, 3, theme::kOutline);
    }

    // Name des Kindes/Charakters (aus dem beim Erststart gewaehlten Profil,
    // include/KidProfiles.h) unter dem Platzhalter-Charakter.
    if (app_.profile.name.length() > 0) {
        canvas_.setTextColor(theme::kText);
        canvas_.setTextDatum(top_center);
        canvas_.setTextSize(2);
        canvas_.drawString(app_.profile.name.c_str(), cx, cy + r + 8);
    }
}

void HomeScreen::drawGroundShadow(int cx, int cy, int rx) {
    // Flache Ellipse in gedaempftem Schwarz-Lila unter der Figur - allein
    // dieser eine "weiche" Schatten laesst die Figur auf dem Boden statt
    // frei schwebend wirken (Nutzerwunsch: "keine rudimentaeren
    // Darstellungen mehr, optimiere Grafik maximal").
    if (rx < 6) {
        rx = 6;
    }
    canvas_.fillEllipse(cx, cy, rx, rx / 3, gfxkit::darken(theme::kBackground, 0.5f));
}

void HomeScreen::drawStatusBar() {
    gfxkit::verticalGradient(&canvas_, 0, 0, M5.Display.width(), 30, gfxkit::lighten(theme::kPanel, 0.12f),
                              gfxkit::darken(theme::kPanel, 0.25f));
    canvas_.drawLine(0, 30, M5.Display.width(), 30, gfxkit::darken(theme::kPanel, 0.5f));

    m5::rtc_time_t time_;
    M5.Rtc.getTime(&time_);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", time_.hours, time_.minutes);
    canvas_.setTextColor(theme::kText);
    canvas_.setTextDatum(top_left);
    canvas_.setTextSize(2);
    canvas_.drawString(buf, 6, 6);

    // Akkustand-Anzeige direkt neben der Uhrzeit (Nutzer-Feedback: fehlte
    // bisher komplett - es gab nur eine reine Warnanzeige bei kritischem
    // Akkustand, siehe drawBatteryIndicator()).
    drawBatteryIndicator(70, 9);

    // Verfuegbare Spielzeit als Zahl + kleines Dreieck-Icon statt Textlabel
    // (Review: Icon statt Wort fuer die juengere Zielgruppe, Abschnitt 5).
    // Nutzer-Feedback: das Dreieck sass an einer fest verdrahteten Position
    // und ueberdeckte bei zweistelligen Werten die Zahl halb - jetzt wird
    // die tatsaechliche Textbreite gemessen und das Dreieck links davon
    // platziert, unabhaengig von der Anzahl Ziffern.
    const uint16_t available = app_.playtime.availableMinutes();
    char availableBuf[8];
    snprintf(availableBuf, sizeof(availableBuf), "%u", static_cast<unsigned>(available));
    canvas_.setTextDatum(top_right);
    canvas_.setTextSize(2);
    const int numberRightX = M5.Display.width() - 10;
    canvas_.drawString(availableBuf, numberRightX, 6);
    const int numberWidth = canvas_.textWidth(availableBuf);
    const int iconX = numberRightX - numberWidth - 18;
    canvas_.fillTriangle(iconX, 8, iconX, 22, iconX + 12, 15, theme::kAccentGold);
}

void HomeScreen::drawBatteryIndicator(int x, int y) {
    int level = M5.Power.getBatteryLevel();
    if (level < 0) {
        level = 0;
    }
    if (level > 100) {
        level = 100;
    }

    uint16_t color = theme::kSuccess;
    if (level <= config::kLowBatteryWarningPercent) {
        color = theme::kDanger;
    } else if (level <= 40) {
        color = theme::kAccentGold;
    }

    constexpr int kBodyW = 20;
    constexpr int kBodyH = 12;
    canvas_.drawRect(x, y, kBodyW, kBodyH, theme::kText);
    canvas_.fillRect(x + kBodyW, y + 3, 3, kBodyH - 6, theme::kText); // Batterie-Pluspol
    const int fillW = (kBodyW - 4) * level / 100;
    if (fillW > 0) {
        canvas_.fillRect(x + 2, y + 2, fillW, kBodyH - 4, color);
    }

    canvas_.setTextColor(theme::kText);
    canvas_.setTextDatum(top_left);
    canvas_.setTextSize(1);
    char pctBuf[6];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", level);
    canvas_.drawString(pctBuf, x + kBodyW + 8, y + 3);
}

void HomeScreen::drawBottomBarTile(int zoneIndex, bool enabled) {
    const int y = M5.Display.height() - kBottomBarHeight;
    const int zoneW = M5.Display.width() / 4;
    const int x = zoneIndex * zoneW;
    // Jede Zone bekommt eine eigene erhabene "Konsolen-Taste" statt einer
    // durchgehenden Flat-Leiste (Nutzerwunsch: "keine rudimentaeren
    // Darstellungen mehr, optimiere Grafik maximal") - gesperrte Zonen
    // (Spiele bei Nacht/ohne Zeitguthaben) wirken bewusst "eingedrueckt"
    // statt erhaben, als zusaetzlicher visueller Sperr-Hinweis.
    gfxkit::bevelPanel(&canvas_, x + 2, y + 2, zoneW - 4, kBottomBarHeight - 4, 6, theme::kPanel, enabled);
}

void HomeScreen::drawBottomBar() {
    const int y = M5.Display.height() - kBottomBarHeight;
    canvas_.drawLine(0, y, M5.Display.width(), y, gfxkit::lighten(theme::kPanel, 0.4f));

    const int zoneW = M5.Display.width() / 4;

    const bool gamesUnlocked = app_.character.stage() >= CharacterStage::Baby;
    const bool hasTime = app_.playtime.availableMinutes() > 0;
    const bool isNight = nightmodeservice::isNight(app_.profile);
    const bool gamesEnabled = gamesUnlocked && hasTime && !isNight;

    drawBottomBarTile(0, true);
    drawBottomBarTile(1, gamesEnabled);
    drawBottomBarTile(2, true);
    drawBottomBarTile(3, true);

    // Aufgaben: Stift-Symbol (immer verfuegbar, Abschnitt 5).
    {
        const int cx = zoneW / 2;
        canvas_.drawLine(cx - 6, y + 22, cx + 6, y + 10, theme::kText);
        canvas_.drawLine(cx - 6, y + 22, cx - 3, y + 19, theme::kText);
        canvas_.fillTriangle(cx + 5, y + 8, cx + 8, y + 11, cx + 6, y + 13, theme::kAccentGold);
    }

    // Spiele: Play-Dreieck - ausgegraut, solange nicht freigeschaltet
    // (Stufe < Baby), kein Spielzeitguthaben vorhanden ist (Abschnitt 7:
    // "Spiele-Menü nur mit vorhandenem Zeitguthaben betretbar") oder gerade
    // Nachtstunden sind (Nutzerwunsch: "in dieser Zeit kann nichts gespielt
    // werden").
    {
        const int cx = zoneW + zoneW / 2;
        const uint16_t color = gamesEnabled ? theme::kAccentCyan : theme::kMuted;
        canvas_.fillTriangle(cx - 6, y + 8, cx - 6, y + 24, cx + 8, y + 16, color);
        if (gamesEnabled) {
            canvas_.drawTriangle(cx - 6, y + 8, cx - 6, y + 24, cx + 8, y + 16, gfxkit::lighten(color, 0.5f));
        }
    }

    // Alltag: Kreis mit Zeigern (Uhr stellvertretend fuers Alltags-Menue).
    {
        const int cx = 2 * zoneW + zoneW / 2;
        const int cy = y + 16;
        canvas_.fillCircle(cx, cy, 10, theme::kPanelLight);
        canvas_.drawCircle(cx, cy, 10, theme::kText);
        canvas_.drawLine(cx, cy, cx, cy - 6, theme::kText);
        canvas_.drawLine(cx, cy, cx + 5, cy, theme::kText);
    }

    // Einstellungen: einfaches Zahnrad-Symbol.
    {
        const int cx = 3 * zoneW + zoneW / 2;
        const int cy = y + 16;
        canvas_.drawCircle(cx, cy, 8, theme::kText);
        canvas_.drawCircle(cx, cy, 3, theme::kText);
        for (int i = 0; i < 6; ++i) {
            const float angle = static_cast<float>(i) * 3.14159f / 3.0f;
            const int tx = cx + static_cast<int>(10.0f * cosf(angle));
            const int ty = cy + static_cast<int>(10.0f * sinf(angle));
            canvas_.fillCircle(tx, ty, 2, theme::kText);
        }
    }
}

void HomeScreen::draw() {
    const bool isNight = nightmodeservice::isNight(app_.profile);
    const int horizonY = M5.Display.height() - kBottomBarHeight - 24;

    // Himmel-Farbverlauf statt einer flachen Ein-Farb-Flaeche (Nutzerwunsch:
    // "vollstaendiges Grafikdesign wie bei Super Nintendo...keine
    // rudimentaeren Darstellungen mehr, optimiere Grafik maximal") - bei
    // Nacht dunkler/kuehler mit Sternenfeld statt Sonne, tagsueber waermer
    // Richtung Horizont (passt zum Sonnenuntergang aus RetroBackdrop).
    if (isNight) {
        gfxkit::verticalGradient(&canvas_, 0, 0, M5.Display.width(), horizonY, theme::kOutline, theme::kBackground);
        gfxkit::starfield(&canvas_, M5.Display.width(), horizonY - 10, 40, theme::kTextDim);
    } else {
        gfxkit::verticalGradient(&canvas_, 0, 0, M5.Display.width(), horizonY,
                                  gfxkit::darken(theme::kPanel, 0.5f), theme::kBackground);
    }
    canvas_.fillRect(0, horizonY, M5.Display.width(), M5.Display.height() - horizonY, theme::kBackground);

    // 80er-/SNES-Arcade-Optik (Nutzerwunsch: "Backgrounds Super-Nintendo-
    // Stil, Strassenkaempfer") - Sonne (nur tagsueber)+Bodengitter hinter
    // dem Charakter, Horizont knapp oberhalb der unteren Icon-Leiste.
    retrobackdrop::drawSynthwaveGrid(&canvas_, M5.Display.width(), M5.Display.height(), horizonY, !isNight);
    if (!drawSpriteCharacter()) {
        drawPlaceholderCharacter();
    }
    // Nutzerwunsch: "Figur schlaeft zwischen 20:00 und 07:00, in dieser
    // Zeit kann nichts gespielt werden" - Mond+"Zzz" als sichtbarer Hinweis,
    // warum die Spiele-Zone gerade ausgegraut ist (siehe drawBottomBar()).
    if (isNight) {
        drawSleepingIndicator();
    }
    drawStatusBar();
    drawBottomBar();
    canvas_.pushSprite(0, 0);
}

void HomeScreen::drawSleepingIndicator() {
    const int cx = M5.Display.width() / 2 + static_cast<int>(characterOffsetX_) + 50;
    const int cy = 55;

    // Mond: grosser Kreis minus versetzter kleinerer Kreis in
    // Hintergrundfarbe ergibt eine Sichel, ganz ohne eigene Sprite-Datei.
    canvas_.fillCircle(cx, cy, 14, theme::kAccentGold);
    canvas_.fillCircle(cx + 7, cy - 4, 12, theme::kBackground);

    canvas_.setTextColor(theme::kTextDim);
    canvas_.setTextDatum(top_left);
    canvas_.setTextSize(2);
    canvas_.drawString("Zzz", cx - 10, cy + 16);
}
