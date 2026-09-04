#include "SubjectSelectScreen.h"

#include <M5Unified.h>

#include "../core/ScreenId.h"
#include "../core/Subject.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
} // namespace

SubjectSelectScreen::SubjectSelectScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void SubjectSelectScreen::onEnter() {
    draw();
}

int SubjectSelectScreen::buildEntries(Entry* out, int maxCount) const {
    int n = 0;
    if (n < maxCount) out[n++] = Entry{EntryKind::Mathe};
    if (n < maxCount) out[n++] = Entry{EntryKind::Rechtschreibung};
    if (app_.profile.klasse >= 3 && n < maxCount) out[n++] = Entry{EntryKind::Franzoesisch};
    if (n < maxCount) out[n++] = Entry{EntryKind::Quiz};
    if (n < maxCount) out[n++] = Entry{EntryKind::Gedaechtnis};
    return n;
}

bool SubjectSelectScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void SubjectSelectScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void SubjectSelectScreen::drawEntry(const Entry& entry, int cx, int cy, int cellSize) const {
    const int half = cellSize / 2 - 6;
    M5.Display.fillRoundRect(cx - half, cy - half, half * 2, half * 2, 10, theme::kPanel);
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_center);

    const int r = half - 10;
    switch (entry.kind) {
        case EntryKind::Mathe:
            M5.Display.setTextSize(4);
            M5.Display.drawString("+", cx, cy);
            break;
        case EntryKind::Rechtschreibung:
            M5.Display.setTextSize(3);
            M5.Display.drawString("Aa", cx, cy);
            break;
        case EntryKind::Franzoesisch: {
            // Franzoesische Flagge - bewusst nicht Teil des Farbschemas
            // (Wiedererkennungswert als Fach-Icon wichtiger als Theme-Treue).
            const int stripeW = (2 * r) / 3;
            M5.Display.fillRect(cx - stripeW, cy - r / 2, stripeW, r, TFT_BLUE);
            M5.Display.fillRect(cx - stripeW / 3, cy - r / 2, stripeW, r, TFT_WHITE);
            M5.Display.fillRect(cx + stripeW / 3, cy - r / 2, stripeW, r, TFT_RED);
            break;
        }
        case EntryKind::Quiz:
            M5.Display.setTextSize(4);
            M5.Display.drawString("?", cx, cy);
            break;
        case EntryKind::Gedaechtnis:
            M5.Display.fillRoundRect(cx - r, cy - r / 2, r - 2, r, 4, theme::kAccentOrange);
            M5.Display.fillRoundRect(cx + 2, cy - r / 2, r - 2, r, 4, theme::kAccentCyan);
            break;
    }
}

void SubjectSelectScreen::draw() {
    M5.Display.fillScreen(theme::kBackground);

    Entry entries[kMaxEntries];
    const int count = buildEntries(entries, kMaxEntries);
    const int rows = (count + kCols - 1) / kCols;
    const int cellW = M5.Display.width() / kCols;
    const int cellH = (M5.Display.height() - 10) / (rows > 0 ? rows : 1);

    for (int i = 0; i < count; ++i) {
        const int col = i % kCols;
        const int row = i / kCols;
        const int cx = col * cellW + cellW / 2;
        const int cy = 10 + row * cellH + cellH / 2;
        drawEntry(entries[i], cx, cy, cellW < cellH ? cellW : cellH);
    }

    drawHomeIcon();
}

void SubjectSelectScreen::activateEntry(const Entry& entry) {
    switch (entry.kind) {
        case EntryKind::Mathe:
            app_.selectedSubject = Subject::Mathe;
            stateMachine_.requestSwitch(ScreenId::Task);
            return;
        case EntryKind::Rechtschreibung:
            app_.selectedSubject = Subject::Rechtschreibung;
            stateMachine_.requestSwitch(ScreenId::Task);
            return;
        case EntryKind::Franzoesisch:
            app_.selectedSubject = Subject::Franzoesisch;
            stateMachine_.requestSwitch(ScreenId::Task);
            return;
        case EntryKind::Quiz:
            app_.selectedSubject = Subject::Quiz;
            stateMachine_.requestSwitch(ScreenId::Task);
            return;
        case EntryKind::Gedaechtnis:
            stateMachine_.requestSwitch(ScreenId::Memory);
            return;
    }
}

void SubjectSelectScreen::update(uint32_t) {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    Entry entries[kMaxEntries];
    const int count = buildEntries(entries, kMaxEntries);
    const int rows = (count + kCols - 1) / kCols;
    const int cellW = M5.Display.width() / kCols;
    const int cellH = (M5.Display.height() - 10) / (rows > 0 ? rows : 1);

    const int col = touch.x / cellW;
    const int row = (touch.y - 10) / cellH;
    if (row < 0 || col < 0 || col >= kCols) {
        return;
    }
    const int index = row * kCols + col;
    if (index < 0 || index >= count) {
        return;
    }
    activateEntry(entries[index]);
}
