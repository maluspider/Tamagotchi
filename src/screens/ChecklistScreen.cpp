#include "ChecklistScreen.h"

#include <M5Unified.h>

#include "../core/GfxKit.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"
#include "config.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr const char* kItemLabels[AppContext::kChecklistItemCount] = {
    "Anziehen",
    "Zaehne putzen",
    "Znueni einpacken",
};
constexpr int kRowHeight = 50;
constexpr int kRowTop = 40;
} // namespace

ChecklistScreen::ChecklistScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void ChecklistScreen::onEnter() {
    ensureDailyReset();
    draw();
}

void ChecklistScreen::ensureDailyReset() {
    const String today = rtcclock::todayIso();
    if (app_.checklistDateIso == today) {
        return;
    }
    app_.checklistDateIso = today;
    for (int i = 0; i < AppContext::kChecklistItemCount; ++i) {
        app_.checklistDone[i] = false;
    }
    app_.checklistRewardedToday = false;
}

void ChecklistScreen::toggleItem(int index) {
    if (index < 0 || index >= AppContext::kChecklistItemCount) {
        return;
    }
    app_.checklistDone[index] = !app_.checklistDone[index];
    awardIfComplete();
}

void ChecklistScreen::awardIfComplete() {
    if (app_.checklistRewardedToday) {
        return;
    }
    for (int i = 0; i < AppContext::kChecklistItemCount; ++i) {
        if (!app_.checklistDone[i]) {
            return;
        }
    }
    app_.checklistRewardedToday = true;
    app_.character.addXp(config::kChecklistRewardXp);
    app_.character.markCaredForToday(rtcclock::todayIso());
    app_.persistProgress();
}

bool ChecklistScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void ChecklistScreen::update(uint32_t) {
    ensureDailyReset();

    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::AlltagMenu);
        return;
    }

    if (touch.y >= kRowTop) {
        const int index = (touch.y - kRowTop) / kRowHeight;
        toggleItem(index);
    }
    draw();
}

void ChecklistScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void ChecklistScreen::draw() {
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), M5.Display.height(),
                              gfxkit::darken(theme::kPanel, 0.6f), theme::kBackground);
    // Dezentes Sternenfeld unterhalb der Titelleiste fuer mehr Tiefe
    // (Nutzerwunsch: "grafiken im tools menue ebenfalls maximal
    // verbessern"), analog zu AlltagMenuScreen/HomeScreen.
    gfxkit::starfield(&M5.Display, M5.Display.width(), M5.Display.height() - 30, 18, theme::kTextDim, 0, 30);
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), 30, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Meine Liste", 6, 4);

    for (int i = 0; i < AppContext::kChecklistItemCount; ++i) {
        const int y = kRowTop + i * kRowHeight;
        gfxkit::bevelPanel(&M5.Display, 10, y + 4, M5.Display.width() - 20, kRowHeight - 8, 8, theme::kPanel, true);

        const int boxX = 24;
        const int boxY = y + kRowHeight / 2 - 12;
        gfxkit::bevelPanel(&M5.Display, boxX, boxY, 24, 24, 3, theme::kOutline, false);
        if (app_.checklistDone[i]) {
            M5.Display.fillRect(boxX + 3, boxY + 3, 18, 18, theme::kSuccess);
        }

        M5.Display.setTextColor(theme::kText);
        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextSize(2);
        M5.Display.drawString(kItemLabels[i], boxX + 36, y + kRowHeight / 2);
    }

    if (app_.checklistRewardedToday) {
        M5.Display.setTextColor(theme::kText);
        M5.Display.setTextDatum(bottom_center);
        M5.Display.setTextSize(1);
        M5.Display.drawString("Heute schon belohnt - gut gemacht!", M5.Display.width() / 2, M5.Display.height() - 6);
    }

    drawHomeIcon();
}
