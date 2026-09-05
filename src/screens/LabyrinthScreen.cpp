#include "LabyrinthScreen.h"

#include <M5Unified.h>

#include <cmath>

#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

// "S"-Parcours: obere Wand laesst rechts eine Luecke, untere Wand laesst
// links eine Luecke - die Kugel muss also erst nach rechts, dann nach
// links steuern, um zum unten links liegenden Ziel zu gelangen.
const LabyrinthScreen::Wall LabyrinthScreen::kWalls[LabyrinthScreen::kWallCount] = {
    {0.0f, 85.0f, 225.0f, 10.0f},
    {95.0f, 155.0f, 225.0f, 10.0f},
};

LabyrinthScreen::LabyrinthScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void LabyrinthScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    resetGame();
}

void LabyrinthScreen::resetGame() {
    ballX_ = kStartX;
    ballY_ = kStartY;
    ballVx_ = 0.0f;
    ballVy_ = 0.0f;
    won_ = false;
    elapsedMs_ = 0;
}

void LabyrinthScreen::resolveWallCollision(const Wall& wall) {
    // Klassische Kreis-vs-Rechteck-Kollision: naehesten Punkt auf dem
    // Rechteck zur Kugelmitte finden, bei Ueberlappung die Kugel entlang
    // der Verbindungslinie herausschieben und nur die Geschwindigkeits-
    // komponente in Richtung der Wand daempfen (kein hartes Abprallen wie
    // bei Pinball - ein Labyrinth soll sich "traege" anfuehlen).
    const float closestX = (ballX_ < wall.x) ? wall.x : ((ballX_ > wall.x + wall.w) ? wall.x + wall.w : ballX_);
    const float closestY = (ballY_ < wall.y) ? wall.y : ((ballY_ > wall.y + wall.h) ? wall.y + wall.h : ballY_);
    const float dx = ballX_ - closestX;
    const float dy = ballY_ - closestY;
    const float distSq = dx * dx + dy * dy;
    if (distSq >= kBallRadius * kBallRadius || distSq < 0.0001f) {
        return;
    }
    const float dist = sqrtf(distSq);
    const float overlap = kBallRadius - dist;
    const float nx = dx / dist;
    const float ny = dy / dist;
    ballX_ += nx * overlap;
    ballY_ += ny * overlap;
    const float dot = ballVx_ * nx + ballVy_ * ny;
    if (dot < 0.0f) {
        ballVx_ -= dot * nx;
        ballVy_ -= dot * ny;
        haptics::pulse(20);
    }
}

void LabyrinthScreen::updatePhysics(uint32_t deltaMs) {
    M5.Imu.update();
    const auto imuData = M5.Imu.getImuData();
    const float dt = static_cast<float>(deltaMs);

    // Nutzer-Feedback: "Gyrobewegungen sind invertiert" - Vorzeichen beider
    // Achsen gedreht (siehe auch HomeScreen/MoorhuhnJagdScreen/PinballScreen).
    ballVx_ += -imuData.accel.x * kTiltAccel * dt;
    ballVy_ += imuData.accel.y * kTiltAccel * dt;
    ballVx_ *= kFriction;
    ballVy_ *= kFriction;

    // Nutzer-Feedback: "schnelle Baelle gehen einfach durch die Wand" - bei
    // hoher Geschwindigkeit wuerde ein einzelner Bewegungsschritt ueber
    // ein ganzes deltaMs die Kugel komplett ueber eine duenne Wand hinweg
    // springen lassen, da nur die Position NACH dem Schritt auf Kollision
    // geprueft wird (klassisches Tunneling-Problem bei diskreter Physik).
    // Die Bewegung wird deshalb in mehrere kleine Sub-Schritte von maximal
    // kMaxStepPx Pixeln aufgeteilt, sodass jede Wand mindestens einmal
    // "gesehen" wird.
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

        if (ballX_ < kBallRadius) {
            ballX_ = kBallRadius;
            ballVx_ = 0.0f;
        }
        if (ballX_ > canvas_.width() - kBallRadius) {
            ballX_ = canvas_.width() - kBallRadius;
            ballVx_ = 0.0f;
        }
        if (ballY_ < kTopBarHeight + kBallRadius) {
            ballY_ = kTopBarHeight + kBallRadius;
            ballVy_ = 0.0f;
        }
        if (ballY_ > canvas_.height() - kBallRadius) {
            ballY_ = canvas_.height() - kBallRadius;
            ballVy_ = 0.0f;
        }

        for (const Wall& wall : kWalls) {
            resolveWallCollision(wall);
        }
    }

    const float gdx = ballX_ - kGoalX;
    const float gdy = ballY_ - kGoalY;
    if (gdx * gdx + gdy * gdy <= (kGoalRadius + kBallRadius) * (kGoalRadius + kBallRadius)) {
        won_ = true;
        haptics::pulse(150);
    }
}

bool LabyrinthScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void LabyrinthScreen::update(uint32_t deltaMs) {
    const auto touch = M5.Touch.getDetail();
    if (touch.wasPressed()) {
        if (touchedHomeIcon(touch.x, touch.y)) {
            stateMachine_.requestSwitch(ScreenId::Home);
            return;
        }
        if (won_) {
            resetGame();
            draw();
            return;
        }
    }

    if (won_) {
        draw();
        return;
    }

    if (playtimeTicker_.tick(app_, deltaMs)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    elapsedMs_ += deltaMs;
    updatePhysics(deltaMs);
    draw();
}

void LabyrinthScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void LabyrinthScreen::draw() {
    // Verlaufs-Spielfeldboden + gebevelte Waende statt Flat-Flaechen
    // (Nutzerwunsch: "keine rudimentaeren Darstellungen mehr, optimiere
    // Grafik maximal").
    gfxkit::verticalGradient(&canvas_, 0, kTopBarHeight, canvas_.width(), canvas_.height() - kTopBarHeight,
                              gfxkit::darken(theme::kPanel, 0.5f), theme::kBackground);
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));

    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    char buf[24];
    snprintf(buf, sizeof(buf), "Zeit: %u s", static_cast<unsigned>(elapsedMs_ / 1000));
    canvas_.drawString(buf, 4, 4);

    for (const Wall& wall : kWalls) {
        gfxkit::bevelPanel(&canvas_, static_cast<int>(wall.x), static_cast<int>(wall.y), static_cast<int>(wall.w),
                            static_cast<int>(wall.h), 3, theme::kPanelLight, true);
    }

    // Ziel als Glanzring statt flacher Kreis.
    canvas_.fillCircle(static_cast<int>(kGoalX), static_cast<int>(kGoalY), static_cast<int>(kGoalRadius),
                        gfxkit::darken(theme::kSuccess, 0.3f));
    canvas_.fillCircle(static_cast<int>(kGoalX), static_cast<int>(kGoalY), static_cast<int>(kGoalRadius) - 5,
                        theme::kSuccess);
    canvas_.fillCircle(static_cast<int>(kGoalX), static_cast<int>(kGoalY), static_cast<int>(kGoalRadius) - 9,
                        gfxkit::lighten(theme::kSuccess, 0.35f));

    gfxkit::shinyBall(&canvas_, static_cast<int>(ballX_), static_cast<int>(ballY_), static_cast<int>(kBallRadius),
                       TFT_WHITE);

    drawHomeIcon();

    if (won_) {
        canvas_.setTextDatum(middle_center);
        canvas_.setTextColor(TFT_WHITE);
        canvas_.setTextSize(3);
        canvas_.drawString("Geschafft!", canvas_.width() / 2, canvas_.height() / 2 - 15);
        canvas_.setTextSize(2);
        canvas_.drawString("Tippen fuer neue Runde", canvas_.width() / 2, canvas_.height() / 2 + 20);
    }

    canvas_.pushSprite(0, 0);
}
