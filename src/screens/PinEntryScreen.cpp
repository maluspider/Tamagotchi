#include "PinEntryScreen.h"

#include <M5Unified.h>

#include "../core/PinCode.h"
#include "../core/ScreenId.h"
#include "../core/storage/ProfileStore.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr const char* kKeyLabels[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "", "0", "C"};
} // namespace

PinEntryScreen::PinEntryScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void PinEntryScreen::onEnter() {
    entered_ = "";
    showError_ = false;
    draw();
}

bool PinEntryScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void PinEntryScreen::submitIfComplete() {
    if (entered_.length() != kPinLength) {
        return;
    }

    if (app_.pinEntrySetNewMode) {
        app_.profile.guard = pincode::hash(entered_);
        profilestore::save(app_.profile);
        app_.pinEntrySetNewMode = false;
        entered_ = "";
        stateMachine_.requestSwitch(ScreenId::Settings);
        return;
    }

    if (pincode::verify(entered_, app_.profile.guard)) {
        entered_ = "";
        stateMachine_.requestSwitch(ScreenId::Settings);
    } else {
        showError_ = true;
        errorTimerMs_ = 900;
        entered_ = "";
    }
}

void PinEntryScreen::handleKeyTap(int x, int y) {
    if (y < kTopBarHeight) {
        return;
    }
    const int cellW = M5.Display.width() / kCols;
    const int cellH = (M5.Display.height() - kTopBarHeight) / kRows;
    const int col = x / cellW;
    const int row = (y - kTopBarHeight) / cellH;
    if (col < 0 || col >= kCols || row < 0 || row >= kRows) {
        return;
    }
    const int index = row * kCols + col;
    const char* label = kKeyLabels[index];
    if (label[0] == '\0') {
        return;
    }
    if (label[0] == 'C') {
        entered_ = "";
        return;
    }
    if (entered_.length() < kPinLength) {
        entered_ += label;
        submitIfComplete();
    }
}

void PinEntryScreen::update(uint32_t deltaMs) {
    if (showError_) {
        errorTimerMs_ = (errorTimerMs_ > deltaMs) ? errorTimerMs_ - deltaMs : 0;
        if (errorTimerMs_ == 0) {
            showError_ = false;
            draw();
        }
    }

    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        app_.pinEntrySetNewMode = false;
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    handleKeyTap(touch.x, touch.y);
    draw();
}

void PinEntryScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, TFT_WHITE);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, TFT_WHITE);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, TFT_WHITE);
}

void PinEntryScreen::draw() {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.fillRect(0, 0, M5.Display.width(), kTopBarHeight, showError_ ? TFT_RED : TFT_NAVY);

    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextDatum(top_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString(app_.pinEntrySetNewMode ? "Neuen PIN festlegen" : "Eltern-PIN eingeben",
                           M5.Display.width() / 2, 6);

    for (int i = 0; i < kPinLength; ++i) {
        const int cx = M5.Display.width() / 2 - 45 + i * 30;
        const int cy = 40;
        if (i < static_cast<int>(entered_.length())) {
            M5.Display.fillCircle(cx, cy, 6, TFT_WHITE);
        } else {
            M5.Display.drawCircle(cx, cy, 6, TFT_WHITE);
        }
    }

    const int cellW = M5.Display.width() / kCols;
    const int cellH = (M5.Display.height() - kTopBarHeight) / kRows;
    for (int index = 0; index < 12; ++index) {
        if (kKeyLabels[index][0] == '\0') {
            continue;
        }
        const int col = index % kCols;
        const int row = index / kCols;
        const int cx = col * cellW + cellW / 2;
        const int cy = kTopBarHeight + row * cellH + cellH / 2;
        const uint16_t color = (kKeyLabels[index][0] == 'C') ? TFT_ORANGE : TFT_DARKGREY;
        M5.Display.fillRoundRect(cx - cellW / 2 + 6, cy - cellH / 2 + 6, cellW - 12, cellH - 12, 6, color);
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(3);
        M5.Display.drawString(kKeyLabels[index], cx, cy);
    }

    drawHomeIcon();
}
