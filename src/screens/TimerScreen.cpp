#include "TimerScreen.h"

#include <M5Unified.h>

#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr int kPresetMinutes[3] = {2, 5, 10};
} // namespace

TimerScreen::TimerScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void TimerScreen::onEnter() {
    phase_ = Phase::SelectPreset;
    draw();
}

void TimerScreen::startTimer(uint32_t minutes) {
    totalMs_ = minutes * 60000;
    remainingMs_ = totalMs_;
    lastDisplayedSeconds_ = remainingMs_ / 1000;
    phase_ = Phase::Running;
}

void TimerScreen::handlePresetTap(int x, int y) {
    if (y < 60 || y > 200) {
        return;
    }
    const int w = M5.Display.width() / 3;
    const int index = x / w;
    if (index >= 0 && index < 3) {
        startTimer(static_cast<uint32_t>(kPresetMinutes[index]));
    }
}

bool TimerScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void TimerScreen::update(uint32_t deltaMs) {
    bool needsRedraw = false;

    if (phase_ == Phase::Running) {
        if (remainingMs_ > deltaMs) {
            remainingMs_ -= deltaMs;
        } else {
            remainingMs_ = 0;
            phase_ = Phase::Finished;
            M5.Speaker.tone(1800, 250);
            M5.Power.setVibration(200);
            delay(250);
            M5.Power.setVibration(0);
            needsRedraw = true;
        }
        const uint32_t seconds = remainingMs_ / 1000;
        if (seconds != lastDisplayedSeconds_) {
            lastDisplayedSeconds_ = seconds;
            needsRedraw = true;
        }
    }

    const auto touch = M5.Touch.getDetail();
    if (touch.wasPressed()) {
        if (touchedHomeIcon(touch.x, touch.y)) {
            stateMachine_.requestSwitch(ScreenId::AlltagMenu);
            return;
        }
        if (phase_ == Phase::SelectPreset) {
            handlePresetTap(touch.x, touch.y);
        } else {
            // Waehrend Running/Finished: Antippen bricht ab bzw. startet neu.
            phase_ = Phase::SelectPreset;
        }
        needsRedraw = true;
    }

    if (needsRedraw) {
        draw();
    }
}

void TimerScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void TimerScreen::drawPresetSelect() const {
    const int w = M5.Display.width() / 3;
    for (int i = 0; i < 3; ++i) {
        const int cx = i * w + w / 2;
        M5.Display.fillRoundRect(i * w + 10, 60, w - 20, 140, 10, theme::kPanel);
        M5.Display.setTextColor(theme::kText);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(4);
        M5.Display.drawNumber(kPresetMinutes[i], cx, 120);
        M5.Display.setTextSize(2);
        M5.Display.drawString("Min", cx, 160);
    }
}

void TimerScreen::drawRunning() const {
    const uint32_t totalSeconds = remainingMs_ / 1000;
    const uint32_t mm = totalSeconds / 60;
    const uint32_t ss = totalSeconds % 60;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02u:%02u", static_cast<unsigned>(mm), static_cast<unsigned>(ss));
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(7);
    M5.Display.drawString(buf, M5.Display.width() / 2, 100);

    const int barW = 280;
    const int barX = (M5.Display.width() - barW) / 2;
    const int barY = 160;
    M5.Display.drawRect(barX, barY, barW, 20, theme::kPanelLight);
    const float ratio = totalMs_ > 0 ? static_cast<float>(remainingMs_) / static_cast<float>(totalMs_) : 0.0f;
    M5.Display.fillRect(barX + 2, barY + 2, static_cast<int>(static_cast<float>(barW - 4) * ratio), 16, theme::kAccentCyan);

    M5.Display.setTextSize(1);
    M5.Display.drawString("Antippen zum Abbrechen", M5.Display.width() / 2, 200);
}

void TimerScreen::drawFinished() const {
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(4);
    M5.Display.drawString("Fertig!", M5.Display.width() / 2, 110);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Tippen fuer neuen Timer", M5.Display.width() / 2, 160);
}

void TimerScreen::draw() {
    M5.Display.fillScreen(theme::kBackground);
    M5.Display.fillRect(0, 0, M5.Display.width(), 30, theme::kPanel);
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Timer", 6, 4);

    switch (phase_) {
        case Phase::SelectPreset:
            drawPresetSelect();
            break;
        case Phase::Running:
            drawRunning();
            break;
        case Phase::Finished:
            drawFinished();
            break;
    }

    drawHomeIcon();
}
