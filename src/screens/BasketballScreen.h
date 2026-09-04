#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Basketball (docs/projektplan.md Abschnitt 9/10, ab Stufe "Experte").
// Steuerung: Swipe-Geste (Antippen -> Ziehen -> Loslassen) auf dem Ball
// wirft ihn mit einer Geschwindigkeit proportional zur Swipe-Richtung und
// -Staerke; danach fliegt er unter Schwerkraft (Wurfwinkel/Distanz
// ergeben sich direkt aus der Swipe-Geste). Trifft er durch den Korb-Ring,
// zaehlt ein Punkt. Kein "Game Over" - gespielt wird, bis das
// Spielzeitguthaben aufgebraucht ist; der beste Punktestand wird als
// Highscore gesichert. Zeichnet ueber M5Canvas wie SnakeScreen.
class BasketballScreen : public Screen {
public:
    BasketballScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    enum class BallState { Idle, Flying };

    static constexpr int kTopBarHeight = 20;
    static constexpr float kBallRadius = 10.0f;
    static constexpr float kLaunchY = 205.0f;
    static constexpr float kGravity = 0.00055f;  // px/ms^2
    static constexpr float kSwipeScaleX = 0.0022f;
    static constexpr float kSwipeScaleY = 0.0040f;
    static constexpr int kMinSwipeDist = 15;
    static constexpr int kRimY = 55;
    static constexpr int kRimX1 = 130;
    static constexpr int kRimX2 = 190;

    void resetBallToLaunch();
    void updateFlight(uint32_t deltaMs);
    void handleInput();
    void finishSession();

    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;

    BallState state_ = BallState::Idle;
    float ballX_ = 0;
    float ballY_ = 0;
    float ballVx_ = 0;
    float ballVy_ = 0;
    float prevBallY_ = 0;
    bool scoredThisShot_ = false;

    bool swiping_ = false;
    int touchStartX_ = 0;
    int touchStartY_ = 0;

    int score_ = 0;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
};
