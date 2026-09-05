#include "PinballScreen.h"

#include <M5Unified.h>

#include <cmath>

#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/HighscoreStore.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr const char* kHighscoreKey = "pinball";
} // namespace

const PinballScreen::Bumper PinballScreen::kBumpers[PinballScreen::kBumperCount] = {
    {100.0f, 70.0f, 14.0f},
    {220.0f, 70.0f, 14.0f},
    {160.0f, 120.0f, 12.0f},
};

PinballScreen::PinballScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void PinballScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    resetGame();
}

void PinballScreen::resetGame() {
    score_ = 0;
    ballsLeft_ = kStartBalls;
    gameOver_ = false;
    launchBall();
}

void PinballScreen::launchBall() {
    ballX_ = canvas_.width() / 2.0f;
    ballY_ = kTopBarHeight + 20.0f;
    ballVx_ = 0;
    ballVy_ = 0;
    leftFlipperBoosted_ = false;
    rightFlipperBoosted_ = false;
}

void PinballScreen::loseBall() {
    --ballsLeft_;
    if (ballsLeft_ <= 0) {
        endGame();
    } else {
        launchBall();
    }
}

void PinballScreen::endGame() {
    gameOver_ = true;
    haptics::pulse(200);
    highscorestore::saveIfHigher(kHighscoreKey, static_cast<uint32_t>(score_));
}

void PinballScreen::updatePhysics(uint32_t deltaMs) {
    const float dt = static_cast<float>(deltaMs);

    ballVy_ += kGravity * dt;

    // Nutzer-Feedback: "schnelle Baelle gehen einfach durch die Wand" - ein
    // einzelner Bewegungsschritt ueber das ganze deltaMs koennte die Kugel
    // (insbesondere kurz nach einem Flipper-Boost mit stark erhoehter
    // Geschwindigkeit) komplett ueber einen Bumper hinweg springen lassen,
    // da nur die Position NACH dem Schritt auf Kollision geprueft wird.
    // Sub-Stepping wie in LabyrinthScreen::updatePhysics() behebt das.
    constexpr float kMaxStepPx = 4.0f;
    constexpr int kMaxSubSteps = 8;
    const float speed = sqrtf(ballVx_ * ballVx_ + ballVy_ * ballVy_);
    int steps = 1;
    if (speed * dt > kMaxStepPx) {
        steps = static_cast<int>(ceilf(speed * dt / kMaxStepPx));
        if (steps > kMaxSubSteps) {
            steps = kMaxSubSteps;
        }
    }
    const float stepDt = dt / static_cast<float>(steps);

    for (int step = 0; step < steps; ++step) {
        ballX_ += ballVx_ * stepDt;
        ballY_ += ballVy_ * stepDt;

        if (ballX_ < kBallRadius + 2) {
            ballX_ = kBallRadius + 2;
            ballVx_ = -ballVx_ * 0.7f;
        }
        if (ballX_ > canvas_.width() - kBallRadius - 2) {
            ballX_ = canvas_.width() - kBallRadius - 2;
            ballVx_ = -ballVx_ * 0.7f;
        }
        if (ballY_ < kTopBarHeight + kBallRadius) {
            ballY_ = kTopBarHeight + kBallRadius;
            ballVy_ = -ballVy_ * 0.7f;
        }

        for (const Bumper& bumper : kBumpers) {
            const float dx = ballX_ - bumper.x;
            const float dy = ballY_ - bumper.y;
            const float dist = sqrtf(dx * dx + dy * dy);
            const float minDist = bumper.r + kBallRadius;
            if (dist > 0.001f && dist < minDist) {
                const float nx = dx / dist;
                const float ny = dy / dist;
                ballX_ = bumper.x + nx * minDist;
                ballY_ = bumper.y + ny * minDist;
                const float dot = ballVx_ * nx + ballVy_ * ny;
                ballVx_ = (ballVx_ - 2.0f * dot * nx) * 1.15f;
                ballVy_ = (ballVy_ - 2.0f * dot * ny) * 1.15f;
                score_ += 10;
                haptics::pulse(30);
            }
        }
    }

    // Flipper: nur wenn gerade aktiv gehalten, die Kugel in Reichweite ist
    // und dieser Flipper seit dem letzten Loslassen noch nicht ausgeloest
    // hat (sonst wuerde er die Kugel jeden Frame erneut beschleunigen).
    if (leftFlipperActive_ && !leftFlipperBoosted_ && ballY_ > 195 && ballY_ < 235 && ballX_ < 130) {
        ballVy_ = kFlipperBoostVy;
        ballVx_ = kFlipperBoostVx;
        leftFlipperBoosted_ = true;
        score_ += 5;
    }
    if (rightFlipperActive_ && !rightFlipperBoosted_ && ballY_ > 195 && ballY_ < 235 &&
        ballX_ > canvas_.width() - 130) {
        ballVy_ = kFlipperBoostVy;
        ballVx_ = -kFlipperBoostVx;
        rightFlipperBoosted_ = true;
        score_ += 5;
    }

    if (ballY_ > canvas_.height() + kBallRadius) {
        loseBall();
    }
}

bool PinballScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void PinballScreen::handleInput(uint32_t deltaMs) {
    const auto touch = M5.Touch.getDetail();

    if (touch.wasPressed() && touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    if (gameOver_) {
        if (touch.wasPressed()) {
            resetGame();
        }
        return;
    }

    const bool pressed = touch.isPressed();
    const int half = M5.Display.width() / 2;
    leftFlipperActive_ = pressed && touch.x < half;
    rightFlipperActive_ = pressed && touch.x >= half;
    if (!leftFlipperActive_) {
        leftFlipperBoosted_ = false;
    }
    if (!rightFlipperActive_) {
        rightFlipperBoosted_ = false;
    }

    // Neigungssensor (IMU) als leichter seitlicher Schubs (Abschnitt 10:
    // "fuer leichte Balleinflussnahme" - bewusst nur ein Nudge, kein
    // primaeres Steuerelement).
    M5.Imu.update();
    const auto imuData = M5.Imu.getImuData();
    // Nutzer-Feedback: "Gyrobewegungen sind invertiert" - Vorzeichen gedreht
    // (siehe auch HomeScreen/MoorhuhnJagdScreen/LabyrinthScreen).
    ballVx_ += -imuData.accel.x * kTiltForce * static_cast<float>(deltaMs);
}

void PinballScreen::update(uint32_t deltaMs) {
    handleInput(deltaMs);

    if (gameOver_) {
        draw();
        return;
    }

    if (playtimeTicker_.tick(app_, deltaMs)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    updatePhysics(deltaMs);
    draw();
}

void PinballScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void PinballScreen::draw() {
    // Dunkler Tisch-Verlauf statt Flat-Hintergrund plus gebevelte
    // Seitenbanden (Nutzerwunsch: "keine rudimentaeren Darstellungen mehr,
    // optimiere Grafik maximal").
    gfxkit::verticalGradient(&canvas_, 0, kTopBarHeight, canvas_.width(), canvas_.height() - kTopBarHeight,
                              gfxkit::darken(theme::kPanel, 0.35f), theme::kOutline);
    gfxkit::bevelPanel(&canvas_, 0, kTopBarHeight, 8, canvas_.height() - kTopBarHeight, 0, theme::kPanelLight, true);
    gfxkit::bevelPanel(&canvas_, canvas_.width() - 8, kTopBarHeight, 8, canvas_.height() - kTopBarHeight, 0,
                        theme::kPanelLight, true);
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));

    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    char buf[32];
    snprintf(buf, sizeof(buf), "Punkte: %d  Baelle: %d", score_, ballsLeft_);
    canvas_.drawString(buf, 4, 4);

    for (const Bumper& bumper : kBumpers) {
        gfxkit::shinyBall(&canvas_, static_cast<int>(bumper.x), static_cast<int>(bumper.y),
                           static_cast<int>(bumper.r), theme::kAccentOrange);
    }

    const int leftLen = leftFlipperActive_ ? 110 : 90;
    const int rightLen = rightFlipperActive_ ? 110 : 90;
    gfxkit::bevelPanel(&canvas_, 10, 210, leftLen, 8, 3, theme::kAccentCyan, true);
    gfxkit::bevelPanel(&canvas_, canvas_.width() - 10 - rightLen, 210, rightLen, 8, 3, theme::kAccentCyan, true);

    gfxkit::shinyBall(&canvas_, static_cast<int>(ballX_), static_cast<int>(ballY_), static_cast<int>(kBallRadius),
                       TFT_WHITE);

    drawHomeIcon();

    if (gameOver_) {
        canvas_.setTextDatum(middle_center);
        canvas_.setTextColor(TFT_WHITE);
        canvas_.setTextSize(3);
        canvas_.drawString("Game Over", canvas_.width() / 2, canvas_.height() / 2 - 25);
        canvas_.setTextSize(2);
        char hsBuf[24];
        snprintf(hsBuf, sizeof(hsBuf), "Highscore: %u", static_cast<unsigned>(highscorestore::load(kHighscoreKey)));
        canvas_.drawString(hsBuf, canvas_.width() / 2, canvas_.height() / 2 + 10);
        canvas_.drawString("Tippen zum Neustart", canvas_.width() / 2, canvas_.height() / 2 + 35);
    }

    canvas_.pushSprite(0, 0);
}
