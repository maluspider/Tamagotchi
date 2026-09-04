#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Fussball (docs/projektplan.md Abschnitt 9/10, ab Stufe "Meister").
// Steuerung: Swipe zum Schiessen aufs Tor (oben). Ein einfacher
// Torwart-Gegner bewegt sich entlang der Torlinie zum Ball und haelt den
// Schuss, wenn er rechtzeitig davor steht - seine maximale
// Reaktionsgeschwindigkeit steigt mit jedem erzielten Tor (Abschnitt 10:
// "steigende Reaktionsgeschwindigkeit"). Anders als Basketball fliegt der
// Ball hier geradlinig (kein Bogen) - einfacher zu lesen fuer ein
// Torschuss-Timing-Spiel. Kein "Game Over", gespielt wird bis das
// Spielzeitguthaben aufgebraucht ist. Zeichnet ueber M5Canvas wie
// SnakeScreen.
class FussballScreen : public Screen {
public:
    FussballScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    enum class BallState { Idle, Flying };

    static constexpr int kTopBarHeight = 20;
    static constexpr float kBallRadius = 8.0f;
    static constexpr float kLaunchY = 205.0f;
    static constexpr float kGoalLineY = 40.0f;
    static constexpr float kSwipeScale = 0.0035f;
    static constexpr int kMinSwipeDist = 15;
    static constexpr float kGoalX1 = 60.0f;
    static constexpr float kGoalX2 = 260.0f;
    static constexpr float kKeeperHalfWidth = 22.0f;
    static constexpr float kKeeperBaseSpeed = 0.06f;      // px/ms
    static constexpr float kKeeperSpeedPerGoal = 0.006f;  // px/ms Zuwachs je Tor

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
    float keeperX_ = 0;

    bool swiping_ = false;
    int touchStartX_ = 0;
    int touchStartY_ = 0;

    int goals_ = 0;
    int saves_ = 0;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
};
