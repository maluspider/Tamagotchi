#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Pinball (docs/projektplan.md Abschnitt 9/10, ab Stufe "Experte").
// Vereinfachte Physik: eine Kugel mit Schwerkraft, die an Waenden und
// zwei statischen Bumpern abprallt; zwei Touch-Zonen unten links/rechts
// simulieren Flipper (geben der Kugel bei Beruehrung einen Impuls nach
// oben/innen, wenn sie sich gerade in Reichweite befindet); Neigen des
// Geraets (IMU) gibt der Kugel einen kleinen seitlichen Schubs ("Nudge",
// wie beim echten Fluepper-Tisch). Faellt die Kugel unten durch, ist ein
// Ball verbraucht (3 Baelle pro Runde). Zeichnet ueber M5Canvas wie
// SnakeScreen.
class PinballScreen : public Screen {
public:
    PinballScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    struct Bumper {
        float x;
        float y;
        float r;
    };

    static constexpr int kTopBarHeight = 20;
    static constexpr float kBallRadius = 6.0f;
    static constexpr int kStartBalls = 3;
    static constexpr int kBumperCount = 3;
    static constexpr float kGravity = 0.00055f;      // px/ms^2
    static constexpr float kFlipperBoostVy = -0.38f;  // px/ms
    static constexpr float kFlipperBoostVx = 0.16f;   // px/ms
    static constexpr float kTiltForce = 0.00035f;     // px/ms^2 pro g Neigung

    void resetGame();
    void launchBall();
    void updatePhysics(uint32_t deltaMs);
    void handleInput(uint32_t deltaMs);
    void loseBall();
    void endGame();

    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;

    static const Bumper kBumpers[kBumperCount];

    float ballX_ = 0;
    float ballY_ = 0;
    float ballVx_ = 0;
    float ballVy_ = 0;

    bool leftFlipperActive_ = false;
    bool rightFlipperActive_ = false;
    bool leftFlipperBoosted_ = false;
    bool rightFlipperBoosted_ = false;

    int score_ = 0;
    int ballsLeft_ = kStartBalls;
    bool gameOver_ = false;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
};
