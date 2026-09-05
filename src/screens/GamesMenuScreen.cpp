#include "GamesMenuScreen.h"

#include <M5Unified.h>

#include "../core/NightModeService.h"
#include "../core/RetroBackdrop.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr int kCols = 3;
constexpr int kRows = 4;
} // namespace

// Freischalt-Reihenfolge exakt nach docs/projektplan.md Abschnitt 9. Das
// Ball-Labyrinth ist bewusst das einfachste Spiel (Nutzerwunsch: "als
// ersten einfachsten Game ein Ball-Labyrinth einbauen") und steht deshalb
// an erster Stelle - freigeschaltet ab derselben Stufe wie Snake ("Baby"),
// da das Spiele-Menue selbst erst ab dieser Stufe ueberhaupt erreichbar
// ist (siehe HomeScreen::handleBottomBarTouch()); eine noch frühere
// Freischaltung waere von Home aus nicht erreichbar.
const GamesMenuScreen::GameEntry GamesMenuScreen::kGames[GamesMenuScreen::kGameCount] = {
    {ScreenId::Labyrinth, CharacterStage::Baby},
    {ScreenId::Snake, CharacterStage::Baby},
    {ScreenId::Tetris, CharacterStage::Kind},
    {ScreenId::Puzzle, CharacterStage::Kind},
    {ScreenId::SpaceInvaders, CharacterStage::Junior},
    {ScreenId::MoorhuhnJagd, CharacterStage::Junior},
    {ScreenId::Pinball, CharacterStage::Experte},
    {ScreenId::Basketball, CharacterStage::Experte},
    {ScreenId::Fussball, CharacterStage::Meister},
    {ScreenId::KampfModus, CharacterStage::Meister},
};

GamesMenuScreen::GamesMenuScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void GamesMenuScreen::onEnter() {
    draw();
}

bool GamesMenuScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void GamesMenuScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void GamesMenuScreen::drawIcon(int index, int cx, int cy, int r, bool unlocked) const {
    M5.Display.fillRoundRect(cx - r, cy - r, r * 2, r * 2, 8, unlocked ? theme::kPanel : theme::kMuted);

    if (!unlocked) {
        // Schloss-Symbol (Buegel + Koerper) statt Spiel-Icon.
        M5.Display.drawRect(cx - 6, cy - 2, 12, 10, theme::kText);
        M5.Display.drawCircle(cx, cy - 4, 5, theme::kText);
        return;
    }

    // Die einzelnen Spiel-Glyphen bleiben bewusst bei ihren "realweltlichen"
    // Wiedererkennungsfarben (Basketball orange, Fussball schwarz/weiss
    // usw.) statt dem Theme zu folgen - Wiedererkennbarkeit schlaegt hier
    // Theme-Treue, wie bei der Frankreich-Flagge in SubjectSelectScreen.
    const int s = r - 8;
    switch (kGames[index].screen) {
        case ScreenId::Labyrinth:
            M5.Display.drawRect(cx - s, cy - s, s * 2, s * 2, TFT_WHITE);
            M5.Display.drawLine(cx - s, cy, cx + s / 2, cy, TFT_WHITE);
            M5.Display.drawLine(cx + s / 2, cy, cx + s / 2, cy + s, TFT_WHITE);
            M5.Display.fillCircle(cx + s / 2, cy - s / 2, 3, TFT_GREEN);
            break;
        case ScreenId::Snake:
            M5.Display.drawLine(cx - s, cy, cx - s / 2, cy - s / 2, TFT_GREEN);
            M5.Display.drawLine(cx - s / 2, cy - s / 2, cx, cy, TFT_GREEN);
            M5.Display.drawLine(cx, cy, cx + s / 2, cy - s / 2, TFT_GREEN);
            M5.Display.drawLine(cx + s / 2, cy - s / 2, cx + s, cy, TFT_GREEN);
            break;
        case ScreenId::Tetris:
            M5.Display.fillRect(cx - s, cy - s / 3, s * 2 / 3, s * 2 / 3, TFT_CYAN);
            M5.Display.fillRect(cx, cy - s / 3, s * 2 / 3, s / 3, TFT_CYAN);
            break;
        case ScreenId::Puzzle:
            M5.Display.fillRect(cx - s, cy - s, s - 2, s - 2, TFT_ORANGE);
            M5.Display.fillRect(cx + 2, cy - s, s - 2, s - 2, TFT_YELLOW);
            M5.Display.fillRect(cx - s, cy + 2, s - 2, s - 2, TFT_GREEN);
            M5.Display.fillRect(cx + 2, cy + 2, s - 2, s - 2, TFT_CYAN);
            break;
        case ScreenId::SpaceInvaders:
            M5.Display.fillRect(cx - s / 2, cy - s / 2, s, s / 2, TFT_GREEN);
            M5.Display.fillRect(cx - s, cy, s / 2, s / 2, TFT_GREEN);
            M5.Display.fillRect(cx + s / 2, cy, s / 2, s / 2, TFT_GREEN);
            break;
        case ScreenId::MoorhuhnJagd:
            M5.Display.drawCircle(cx, cy, s, TFT_RED);
            M5.Display.drawLine(cx - s, cy, cx + s, cy, TFT_RED);
            M5.Display.drawLine(cx, cy - s, cx, cy + s, TFT_RED);
            break;
        case ScreenId::Pinball:
            M5.Display.fillCircle(cx, cy - s / 3, s / 3, TFT_WHITE);
            M5.Display.drawLine(cx - s, cy + s / 2, cx - s / 3, cy + s, TFT_ORANGE);
            M5.Display.drawLine(cx + s, cy + s / 2, cx + s / 3, cy + s, TFT_ORANGE);
            break;
        case ScreenId::Basketball:
            M5.Display.fillCircle(cx, cy, s, TFT_ORANGE);
            M5.Display.drawLine(cx - s, cy, cx + s, cy, TFT_BLACK);
            M5.Display.drawLine(cx, cy - s, cx, cy + s, TFT_BLACK);
            break;
        case ScreenId::Fussball:
            M5.Display.fillCircle(cx, cy, s, TFT_WHITE);
            M5.Display.fillCircle(cx, cy, s / 3, TFT_BLACK);
            break;
        case ScreenId::KampfModus:
            M5.Display.fillTriangle(cx - s, cy - s / 2, cx - s, cy + s / 2, cx, cy, TFT_RED);
            M5.Display.fillTriangle(cx + s, cy - s / 2, cx + s, cy + s / 2, cx, cy, TFT_CYAN);
            break;
        default:
            break;
    }
}

void GamesMenuScreen::draw() {
    M5.Display.fillScreen(theme::kBackground);
    retrobackdrop::drawSynthwaveGrid(&M5.Display, M5.Display.width(), M5.Display.height(), M5.Display.height() - 70);

    const int cellW = M5.Display.width() / kCols;
    const int cellH = (M5.Display.height() - 10) / kRows;

    // Nutzerwunsch: "Figur schlaeft zwischen 20:00 und 07:00, in dieser
    // Zeit kann nichts gespielt werden" - waehrend der Nachtstunden gilt
    // jedes Spiel als gesperrt, unabhaengig von der Charakterstufe.
    const bool isNight = nightmodeservice::isNight(app_.profile);

    for (int i = 0; i < kGameCount; ++i) {
        const int col = i % kCols;
        const int row = i / kCols;
        const int cx = col * cellW + cellW / 2;
        const int cy = 10 + row * cellH + cellH / 2;
        const int r = (cellW < cellH ? cellW : cellH) / 2 - 8;
        const bool unlocked = app_.character.stage() >= kGames[i].requiredStage && !isNight;
        drawIcon(i, cx, cy, r, unlocked);
    }

    drawHomeIcon();
}

void GamesMenuScreen::update(uint32_t) {
    // Beginnt die Nachtstunde, waehrend das Kind schon im Spiele-Menue ist
    // (noch kein Spiel gestartet, siehe PlaytimeTicker fuer den Fall
    // "Spiel laeuft bereits") - sofort zurueck zu Home.
    if (nightmodeservice::isNight(app_.profile)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    const int cellW = M5.Display.width() / kCols;
    const int cellH = (M5.Display.height() - 10) / kRows;
    const int col = touch.x / cellW;
    const int row = (touch.y - 10) / cellH;
    if (row < 0 || row >= kRows || col < 0 || col >= kCols) {
        return;
    }
    const int index = row * kCols + col;
    if (index < 0 || index >= kGameCount) {
        return;
    }

    if (app_.character.stage() >= kGames[index].requiredStage) {
        stateMachine_.requestSwitch(kGames[index].screen);
    }
}
