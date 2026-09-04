#include "AlltagMenuScreen.h"

#include <M5Unified.h>

#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr int kCols = 3;
constexpr int kRows = 2;
} // namespace

const AlltagMenuScreen::Entry AlltagMenuScreen::kEntries[AlltagMenuScreen::kEntryCount] = {
    {ScreenId::Clock, "Uhr"},
    {ScreenId::Timer, "Timer"},
    {ScreenId::Checklist, "Liste"},
    {ScreenId::Steckbrief, "Ich"},
    {ScreenId::CharacterCustomize, "Look"},
    {ScreenId::Birthday, "Torte"},
};

AlltagMenuScreen::AlltagMenuScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void AlltagMenuScreen::onEnter() {
    draw();
}

bool AlltagMenuScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void AlltagMenuScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void AlltagMenuScreen::drawEntry(int index, int cx, int cy, int cellSize) const {
    const int half = cellSize / 2 - 10;
    M5.Display.fillRoundRect(cx - half, cy - half, half * 2, half * 2, 10, theme::kPanel);

    const int r = half - 16;
    switch (kEntries[index].screen) {
        case ScreenId::Clock:
            M5.Display.drawCircle(cx, cy - 8, r, theme::kText);
            M5.Display.drawLine(cx, cy - 8, cx, cy - 8 - r + 6, theme::kText);
            M5.Display.drawLine(cx, cy - 8, cx + r / 2, cy - 8, theme::kText);
            break;
        case ScreenId::Timer:
            M5.Display.drawCircle(cx, cy - 4, r, theme::kText);
            M5.Display.fillRect(cx - 4, cy - 8 - r - 6, 8, 6, theme::kText);
            M5.Display.drawLine(cx, cy - 4, cx, cy - 4 - r + 6, theme::kAccentOrange);
            break;
        case ScreenId::Checklist:
            M5.Display.drawRoundRect(cx - r, cy - r - 6, r * 2, r * 2, 4, theme::kText);
            M5.Display.drawLine(cx - r / 2, cy - 8, cx - r / 4, cy - 2, theme::kSuccess);
            M5.Display.drawLine(cx - r / 4, cy - 2, cx + r / 2, cy - r, theme::kSuccess);
            break;
        case ScreenId::Steckbrief:
            M5.Display.drawRoundRect(cx - r, cy - r - 6, r * 2, r * 2, 4, theme::kText);
            M5.Display.fillCircle(cx, cy - r / 2 - 6, r / 3, theme::kAccentOrange);
            M5.Display.drawLine(cx - r / 2, cy + r / 3, cx + r / 2, cy + r / 3, theme::kText);
            break;
        case ScreenId::CharacterCustomize:
            // Kleine Palette mit drei Farbtupfen - steht fuer die
            // anpassbaren Traits (Haut/Haare/Kleidung).
            M5.Display.fillEllipse(cx, cy - 2, r, r * 0.8, theme::kPanelLight);
            M5.Display.fillCircle(cx - r / 2, cy - r / 3, r / 4, theme::kAccentPink);
            M5.Display.fillCircle(cx, cy - r / 2, r / 4, theme::kAccentCyan);
            M5.Display.fillCircle(cx + r / 2, cy - r / 3, r / 4, theme::kAccentGold);
            break;
        case ScreenId::Birthday:
            // Kleine Geburtstagstorte mit Kerze.
            M5.Display.fillRoundRect(cx - r * 0.7, cy, r * 1.4, r * 0.6, 4, theme::kAccentPink);
            M5.Display.fillRect(cx - r * 0.7, cy - 2, r * 1.4, 4, theme::kAccentGold);
            M5.Display.fillRect(cx - 2, cy - r * 0.5, 4, r * 0.5 - 2, theme::kText);
            M5.Display.fillCircle(cx, cy - r * 0.6, 4, theme::kAccentOrange);
            break;
        default:
            break;
    }

    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString(kEntries[index].label, cx, cy + half - 14);
}

void AlltagMenuScreen::draw() {
    M5.Display.fillScreen(theme::kBackground);

    const int cellW = M5.Display.width() / kCols;
    const int cellH = (M5.Display.height() - 10) / kRows;

    for (int i = 0; i < kEntryCount; ++i) {
        const int col = i % kCols;
        const int row = i / kCols;
        const int cx = col * cellW + cellW / 2;
        const int cy = 10 + row * cellH + cellH / 2;
        drawEntry(i, cx, cy, cellW < cellH ? cellW : cellH);
    }

    drawHomeIcon();
}

void AlltagMenuScreen::update(uint32_t) {
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
    if (index < 0 || index >= kEntryCount) {
        return;
    }

    stateMachine_.requestSwitch(kEntries[index].screen);
}
