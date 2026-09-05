#include "BasketballScreen.h"

#include <M5Unified.h>

#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/HighscoreStore.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr const char* kHighscoreKey = "basketball";
} // namespace

BasketballScreen::BasketballScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void BasketballScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    score_ = 0;
    resetBallToLaunch();
}

void BasketballScreen::resetBallToLaunch() {
    ballX_ = canvas_.width() / 2.0f;
    ballY_ = kLaunchY;
    ballVx_ = 0;
    ballVy_ = 0;
    scoredThisShot_ = false;
    state_ = BallState::Idle;
}

void BasketballScreen::finishSession() {
    highscorestore::saveIfHigher(kHighscoreKey, static_cast<uint32_t>(score_));
}

void BasketballScreen::updateFlight(uint32_t deltaMs) {
    if (state_ != BallState::Flying) {
        return;
    }
    const float dt = static_cast<float>(deltaMs);

    prevBallY_ = ballY_;
    ballVy_ += kGravity * dt;
    ballX_ += ballVx_ * dt;
    ballY_ += ballVy_ * dt;

    if (!scoredThisShot_ && ballVy_ > 0 && prevBallY_ < kRimY && ballY_ >= kRimY && ballX_ > kRimX1 &&
        ballX_ < kRimX2) {
        ++score_;
        scoredThisShot_ = true;
        haptics::pulse(80);
    }

    if (ballX_ < -20 || ballX_ > canvas_.width() + 20 || ballY_ > canvas_.height() + 20) {
        resetBallToLaunch();
    }
}

bool BasketballScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void BasketballScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();

    if (touch.wasPressed()) {
        if (touchedHomeIcon(touch.x, touch.y)) {
            finishSession();
            stateMachine_.requestSwitch(ScreenId::Home);
            return;
        }
        if (state_ == BallState::Idle) {
            touchStartX_ = touch.x;
            touchStartY_ = touch.y;
            swiping_ = true;
        }
        return;
    }

    if (touch.wasReleased() && swiping_) {
        swiping_ = false;
        if (state_ == BallState::Idle) {
            const int dx = touch.x - touchStartX_;
            const int dy = touch.y - touchStartY_;
            if (dx * dx + dy * dy > kMinSwipeDist * kMinSwipeDist) {
                ballVx_ = static_cast<float>(dx) * kSwipeScaleX;
                ballVy_ = static_cast<float>(dy) * kSwipeScaleY;
                prevBallY_ = ballY_;
                scoredThisShot_ = false;
                state_ = BallState::Flying;
            }
        }
    }
}

void BasketballScreen::update(uint32_t deltaMs) {
    handleInput();

    if (playtimeTicker_.tick(app_, deltaMs)) {
        finishSession();
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    updateFlight(deltaMs);
    draw();
}

void BasketballScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void BasketballScreen::draw() {
    // Hallenboden-Verlauf (Parkett-Ton) statt flacher Ein-Farb-Flaeche
    // (Nutzerwunsch: "keine rudimentaeren Darstellungen mehr, optimiere
    // Grafik maximal").
    constexpr uint16_t kFloor = theme::rgb565(0x8A, 0x55, 0x22);
    gfxkit::verticalGradient(&canvas_, 0, kTopBarHeight, canvas_.width(), canvas_.height() - kTopBarHeight,
                              gfxkit::darken(theme::kPanel, 0.5f), gfxkit::darken(kFloor, 0.35f));

    // Hallenscheinwerfer (weiche, ueberlappende Lichtflecken) + Parkett-
    // Maserung + Freiwurf-Zone statt einer reinen Verlaufsflaeche
    // (Nutzerwunsch: "hole das Maximum aus der Hardware...maximal
    // hochwertig...mit Backgrounds").
    canvas_.fillEllipse(50, 90, 70, 40, gfxkit::lighten(theme::kPanel, 0.08f));
    canvas_.fillEllipse(canvas_.width() - 50, 110, 70, 42, gfxkit::lighten(theme::kPanel, 0.06f));
    for (int y = 120; y < canvas_.height(); y += 16) {
        canvas_.drawLine(0, y, canvas_.width(), y, gfxkit::darken(kFloor, 0.5f));
    }
    canvas_.drawRect((canvas_.width() - 90) / 2, canvas_.height() - 70, 90, 70, gfxkit::lighten(kFloor, 0.25f));

    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));

    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    char buf[24];
    snprintf(buf, sizeof(buf), "Punkte: %d", score_);
    canvas_.drawString(buf, 4, 4);

    // Brett (gebevelt) + Ring.
    gfxkit::bevelPanel(&canvas_, kRimX1 - 10, 30, (kRimX2 - kRimX1) + 20, 6, 2, TFT_WHITE, true);
    canvas_.drawLine(kRimX1, kRimY, kRimX2, kRimY, theme::kAccentOrange);
    canvas_.drawLine(kRimX1, kRimY, kRimX1, kRimY + 12, theme::kAccentOrange);
    canvas_.drawLine(kRimX2, kRimY, kRimX2, kRimY + 12, theme::kAccentOrange);

    gfxkit::shinyBall(&canvas_, static_cast<int>(ballX_), static_cast<int>(ballY_), static_cast<int>(kBallRadius),
                       theme::kAccentOrange);
    canvas_.drawLine(static_cast<int>(ballX_) - static_cast<int>(kBallRadius), static_cast<int>(ballY_),
                      static_cast<int>(ballX_) + static_cast<int>(kBallRadius), static_cast<int>(ballY_), TFT_BLACK);
    canvas_.drawLine(static_cast<int>(ballX_), static_cast<int>(ballY_) - static_cast<int>(kBallRadius),
                      static_cast<int>(ballX_), static_cast<int>(ballY_) + static_cast<int>(kBallRadius), TFT_BLACK);

    if (state_ == BallState::Idle) {
        canvas_.setTextDatum(bottom_center);
        canvas_.setTextSize(1);
        canvas_.drawString("Ziehen zum Werfen", canvas_.width() / 2, static_cast<int>(kLaunchY) - 16);
    }

    drawHomeIcon();
    canvas_.pushSprite(0, 0);
}
