#include "SteckbriefScreen.h"

#include <M5Unified.h>

#include "../core/CharacterEngine.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr int kTotalGameCount = 10;

// Deckt sich mit der Freischalt-Tabelle in GamesMenuScreen.cpp
// (Abschnitt 9) - hier nur zur Anzeige einer Gesamtzahl dupliziert, um
// GamesMenuScreen keine unnoetige public-API dafuer zu geben. Nutzer-
// Feedback ("Tabelle, wie viele Punkte fuer weitere Spiele noetig sind?")
// zeigte, dass diese Zahl beim Hinzufuegen des 10. Spiels (Labyrinth,
// schaltet wie Snake bei "Baby" frei) nicht mitaktualisiert worden war -
// seitdem hier zusaetzlich pro Stufe geprueft gegen GamesMenuScreen::kGames.
int unlockedGameCount(CharacterStage stage) {
    switch (stage) {
        case CharacterStage::Ei: return 0;
        case CharacterStage::Baby: return 2;
        case CharacterStage::Kind: return 4;
        case CharacterStage::Junior: return 6;
        case CharacterStage::Experte: return 8;
        case CharacterStage::Meister: return 10;
    }
    return 0;
}

// XP-Schwellen aus CharacterEngine.cpp (dort nicht public, daher hier
// dupliziert - gleiche Begruendung wie bei unlockedGameCount()).
uint32_t xpThresholdFor(CharacterStage stage) {
    switch (stage) {
        case CharacterStage::Ei: return 0;
        case CharacterStage::Baby: return 100;
        case CharacterStage::Kind: return 300;
        case CharacterStage::Junior: return 700;
        case CharacterStage::Experte: return 1500;
        case CharacterStage::Meister: return 3000;
    }
    return 0;
}
} // namespace

SteckbriefScreen::SteckbriefScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void SteckbriefScreen::onEnter() {
    draw();
}

bool SteckbriefScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void SteckbriefScreen::update(uint32_t) {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }
    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::AlltagMenu);
    }
}

void SteckbriefScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void SteckbriefScreen::draw() {
    M5.Display.fillScreen(theme::kBackground);
    M5.Display.fillRect(0, 0, M5.Display.width(), 30, theme::kPanel);
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Mein Steckbrief", 6, 4);

    int y = 44;
    const int lineHeight = 28;
    auto drawLine = [&](const String& text) {
        M5.Display.setTextColor(theme::kText);
        M5.Display.setTextDatum(top_left);
        M5.Display.setTextSize(2);
        M5.Display.drawString(text, 14, y);
        y += lineHeight;
    };

    const CharacterStage stage = app_.character.stage();

    drawLine(String("Name: ") + (app_.profile.name.length() ? app_.profile.name : String("-")));
    drawLine(String("Stufe: ") + CharacterEngine::stageName(stage));
    drawLine(String("Erfahrungspunkte: ") + String(app_.character.xp()));
    drawLine(String("Klasse: ") + String(app_.profile.klasse));
    drawLine(String("Spiele frei: ") + String(unlockedGameCount(stage)) + " / " + String(kTotalGameCount));

    // Nutzerwunsch: "Tabelle, wie viele Punkte fuer weitere Spiele noetig
    // sind?" - es gibt keine eigene Tabellen-Ansicht, aber diese Zeile
    // beantwortet dieselbe Frage direkt am aktuellen Punktestand: naechste
    // Stufe, dafuer noetige EP, zusaetzlich freigeschaltete Spiele. Bei
    // textSize(1) passt die laengste Variante (~44 Zeichen) sicher in die
    // Bildschirmbreite.
    if (stage == CharacterStage::Meister) {
        M5.Display.setTextColor(theme::kAccentGold);
        M5.Display.setTextDatum(top_left);
        M5.Display.setTextSize(1);
        M5.Display.drawString("Hoechste Stufe erreicht!", 14, y);
        y += 20;
    } else {
        const CharacterStage nextStage = static_cast<CharacterStage>(static_cast<uint8_t>(stage) + 1);
        const int newGames = unlockedGameCount(nextStage) - unlockedGameCount(stage);
        String nextLine = String("Naechste Stufe: ") + CharacterEngine::stageName(nextStage) + " (" +
                           String(xpThresholdFor(nextStage)) + " EP, +" + String(newGames) + " Spiele)";
        M5.Display.setTextColor(theme::kTextDim);
        M5.Display.setTextDatum(top_left);
        M5.Display.setTextSize(1);
        M5.Display.drawString(nextLine, 14, y);
        y += 20;
    }

    if (!app_.character.lastCareDateIso().isEmpty()) {
        const String today = rtcclock::todayIso();
        const long days =
            rtcclock::epochDayFromIso(today) - rtcclock::epochDayFromIso(app_.character.lastCareDateIso());
        drawLine(String("Zuletzt gespielt: vor ") + String(days) + (days == 1 ? " Tag" : " Tagen"));
    }

    drawHomeIcon();
}
