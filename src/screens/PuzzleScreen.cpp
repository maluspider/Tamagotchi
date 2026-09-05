#include "PuzzleScreen.h"

#include <M5Unified.h>
#include <esp_random.h>

#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
} // namespace

const uint16_t PuzzleScreen::kTileColors[PuzzleScreen::kTileCount] = {
    TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_PURPLE, TFT_PINK, TFT_GREENYELLOW,
};

PuzzleScreen::PuzzleScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void PuzzleScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    resetGame();
}

bool PuzzleScreen::isSolvedState() const {
    for (int i = 0; i < kTileCount; ++i) {
        if (slotColorId_[i] != i) {
            return false;
        }
    }
    return true;
}

void PuzzleScreen::resetGame() {
    for (int i = 0; i < kTileCount; ++i) {
        slotColorId_[i] = i;
    }
    do {
        for (int i = kTileCount - 1; i > 0; --i) {
            const int j = static_cast<int>(esp_random() % static_cast<uint32_t>(i + 1));
            const int tmp = slotColorId_[i];
            slotColorId_[i] = slotColorId_[j];
            slotColorId_[j] = tmp;
        }
    } while (isSolvedState());

    draggingSlot_ = -1;
    moveCount_ = 0;
    solved_ = false;
}

int PuzzleScreen::slotAt(int x, int y) const {
    if (x < kGridOffsetX || y < kGridOffsetY) {
        return -1;
    }
    const int col = (x - kGridOffsetX) / kTileSize;
    const int row = (y - kGridOffsetY) / kTileSize;
    if (col < 0 || col >= kGridSize || row < 0 || row >= kGridSize) {
        return -1;
    }
    return row * kGridSize + col;
}

bool PuzzleScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void PuzzleScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();

    if (touch.wasPressed()) {
        if (touchedHomeIcon(touch.x, touch.y)) {
            stateMachine_.requestSwitch(ScreenId::Home);
            return;
        }
        if (solved_) {
            resetGame();
            return;
        }
        const int slot = slotAt(touch.x, touch.y);
        if (slot >= 0) {
            draggingSlot_ = slot;
            dragX_ = touch.x;
            dragY_ = touch.y;
        }
        return;
    }

    if (touch.isPressed() && draggingSlot_ >= 0) {
        dragX_ = touch.x;
        dragY_ = touch.y;
        return;
    }

    if (touch.wasReleased() && draggingSlot_ >= 0) {
        const int target = slotAt(touch.x, touch.y);
        if (target >= 0 && target != draggingSlot_) {
            const int tmp = slotColorId_[draggingSlot_];
            slotColorId_[draggingSlot_] = slotColorId_[target];
            slotColorId_[target] = tmp;
            ++moveCount_;
        }
        draggingSlot_ = -1;
        solved_ = isSolvedState();
        if (solved_) {
            haptics::pulse(150);
        }
    }
}

void PuzzleScreen::update(uint32_t deltaMs) {
    if (playtimeTicker_.tick(app_, deltaMs)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    handleInput();
    draw();
}

void PuzzleScreen::drawTile(int colorId, int x, int y, int size) {
    // Gebeveltes "Puzzleteil" statt flacher Farbflaeche (Nutzerwunsch:
    // "keine rudimentaeren Darstellungen mehr, optimiere Grafik maximal").
    gfxkit::bevelPanel(&canvas_, x + 2, y + 2, size - 4, size - 4, 6, kTileColors[colorId], true);
    canvas_.setTextColor(TFT_BLACK);
    canvas_.setTextDatum(middle_center);
    canvas_.setTextSize(3);
    canvas_.drawNumber(colorId + 1, x + size / 2, y + size / 2);
}

void PuzzleScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void PuzzleScreen::draw() {
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), canvas_.height(), gfxkit::darken(theme::kPanel, 0.55f),
                              theme::kBackground);
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));

    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    char buf[24];
    snprintf(buf, sizeof(buf), "Zuege: %d", moveCount_);
    canvas_.drawString(buf, 4, 4);

    for (int slot = 0; slot < kTileCount; ++slot) {
        if (slot == draggingSlot_) {
            continue; // wird unten an der Fingerposition gezeichnet
        }
        const int col = slot % kGridSize;
        const int row = slot / kGridSize;
        const int x = kGridOffsetX + col * kTileSize;
        const int y = kGridOffsetY + row * kTileSize;

        if (draggingSlot_ >= 0 && slotAt(dragX_, dragY_) == slot) {
            // Ziel-Slot der aktuellen Ziehbewegung hervorheben.
            canvas_.drawRect(x, y, kTileSize, kTileSize, TFT_YELLOW);
            canvas_.drawRect(x + 1, y + 1, kTileSize - 2, kTileSize - 2, TFT_YELLOW);
        }
        drawTile(slotColorId_[slot], x, y, kTileSize);
    }

    if (draggingSlot_ >= 0) {
        drawTile(slotColorId_[draggingSlot_], dragX_ - kTileSize / 2, dragY_ - kTileSize / 2, kTileSize);
    }

    drawHomeIcon();

    if (solved_) {
        canvas_.setTextDatum(middle_center);
        canvas_.setTextColor(TFT_WHITE);
        canvas_.setTextSize(3);
        canvas_.drawString("Geloest!", canvas_.width() / 2, canvas_.height() / 2 - 15);
        canvas_.setTextSize(2);
        canvas_.drawString("Tippen fuer neue Runde", canvas_.width() / 2, canvas_.height() / 2 + 20);
    }

    canvas_.pushSprite(0, 0);
}
