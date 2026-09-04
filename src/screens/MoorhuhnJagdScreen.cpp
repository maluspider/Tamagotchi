#include "MoorhuhnJagdScreen.h"

#include <M5Unified.h>
#include <esp_random.h>

#include "../core/HighscoreStore.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr const char* kHighscoreKey = "moorhuhn";
constexpr float kLaneYs[4] = {60.0f, 105.0f, 150.0f, 195.0f};
} // namespace

MoorhuhnJagdScreen::MoorhuhnJagdScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void MoorhuhnJagdScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    resetGame();
}

void MoorhuhnJagdScreen::resetGame() {
    score_ = 0;
    crosshairX_ = M5.Display.width() / 2.0f;
    crosshairY_ = (kTopBarHeight + M5.Display.height()) / 2.0f;
    for (int i = 0; i < kTargetCount; ++i) {
        targets_[i].y = kLaneYs[i % 4];
        targets_[i].x = static_cast<float>((i + 1) * M5.Display.width() / (kTargetCount + 1));
        targets_[i].vx = (i % 2 == 0) ? 0.05f : -0.05f;
    }
}

void MoorhuhnJagdScreen::respawnTarget(Target& target) {
    target.y = kLaneYs[esp_random() % 4];
    target.x = static_cast<float>(esp_random() % static_cast<uint32_t>(M5.Display.width()));
    const int cappedScore = score_ > 30 ? 30 : score_;
    const float speed = 0.05f + static_cast<float>(cappedScore) * 0.003f;
    target.vx = (esp_random() % 2 == 0) ? speed : -speed;
}

void MoorhuhnJagdScreen::finishSession() {
    highscorestore::saveIfHigher(kHighscoreKey, static_cast<uint32_t>(score_));
}

void MoorhuhnJagdScreen::updateTargets(uint32_t deltaMs) {
    const float dt = static_cast<float>(deltaMs);
    for (Target& t : targets_) {
        t.x += t.vx * dt;
        if (t.x < kTargetRadius) {
            t.x = kTargetRadius;
            t.vx = -t.vx;
        }
        if (t.x > canvas_.width() - kTargetRadius) {
            t.x = canvas_.width() - kTargetRadius;
            t.vx = -t.vx;
        }
    }
}

void MoorhuhnJagdScreen::updateCrosshair(uint32_t deltaMs) {
    (void)deltaMs;
    M5.Imu.update();
    const auto imuData = M5.Imu.getImuData();

    const float targetCx = M5.Display.width() / 2.0f + imuData.accel.x * kTiltRangeX;
    const float targetCy = (kTopBarHeight + M5.Display.height()) / 2.0f - imuData.accel.y * kTiltRangeY;

    crosshairX_ += (targetCx - crosshairX_) * kSmoothing;
    crosshairY_ += (targetCy - crosshairY_) * kSmoothing;

    if (crosshairX_ < 4) {
        crosshairX_ = 4;
    }
    if (crosshairX_ > M5.Display.width() - 4) {
        crosshairX_ = M5.Display.width() - 4;
    }
    if (crosshairY_ < kTopBarHeight + 4) {
        crosshairY_ = kTopBarHeight + 4;
    }
    if (crosshairY_ > M5.Display.height() - 4) {
        crosshairY_ = M5.Display.height() - 4;
    }
}

bool MoorhuhnJagdScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void MoorhuhnJagdScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        finishSession();
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    for (Target& t : targets_) {
        const float dx = t.x - crosshairX_;
        const float dy = t.y - crosshairY_;
        if (dx * dx + dy * dy <= kHitRadius * kHitRadius) {
            ++score_;
            respawnTarget(t);
            break; // nur ein Treffer pro Schuss
        }
    }
}

void MoorhuhnJagdScreen::update(uint32_t deltaMs) {
    handleInput();

    if (playtimeTicker_.tick(app_, deltaMs)) {
        finishSession();
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    updateCrosshair(deltaMs);
    updateTargets(deltaMs);
    draw();
}

void MoorhuhnJagdScreen::drawTarget(const Target& target) {
    const int x = static_cast<int>(target.x);
    const int y = static_cast<int>(target.y);
    const int r = static_cast<int>(kTargetRadius);
    canvas_.fillCircle(x, y, r, TFT_ORANGE);
    canvas_.drawCircle(x, y, r, TFT_BLACK);
    const int beakDir = (target.vx >= 0) ? 1 : -1;
    canvas_.fillTriangle(x + beakDir * r, y, x + beakDir * (r + 8), y - 3, x + beakDir * (r + 8), y + 3, TFT_YELLOW);
    canvas_.fillCircle(x + beakDir * (r / 2), y - r / 3, 2, TFT_BLACK);
}

void MoorhuhnJagdScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void MoorhuhnJagdScreen::draw() {
    canvas_.fillScreen(TFT_GREENYELLOW);
    canvas_.fillRect(0, 0, canvas_.width(), kTopBarHeight, theme::kPanel);

    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    char buf[24];
    snprintf(buf, sizeof(buf), "Punkte: %d", score_);
    canvas_.drawString(buf, 4, 4);

    for (const Target& t : targets_) {
        drawTarget(t);
    }

    // Fadenkreuz.
    const int cx = static_cast<int>(crosshairX_);
    const int cy = static_cast<int>(crosshairY_);
    canvas_.drawCircle(cx, cy, 14, TFT_RED);
    canvas_.drawLine(cx - 20, cy, cx + 20, cy, TFT_RED);
    canvas_.drawLine(cx, cy - 20, cx, cy + 20, TFT_RED);

    drawHomeIcon();
    canvas_.pushSprite(0, 0);
}
