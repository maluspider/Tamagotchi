#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Kugel-Labyrinth (Nutzerwunsch: "als ersten einfachsten Game ein
// Ball-Labyrinth einbauen, wo der Ball durch Tilten des Geraets ans Ziel
// gebracht werden muss"). Bewusst das einfachste der Spiele - deshalb schon
// ab Stufe "Ei" freigeschaltet (vor Snake): Neigen des Geraets (IMU)
// beschleunigt die Kugel in Neigungsrichtung, ein simpler, fest codierter
// Wandparcours (zwei Waende mit versetzten Luecken, "S"-Verlauf) fuehrt
// zum Ziel unten links. Kein Highscore - wie Puzzle/Kampf-Modus gibt es
// hier keinen sinnvollen Zahlenwert dafuer (siehe deren Klassenkommentare),
// nur "geschafft" ja/nein. Zeichnet ueber M5Canvas wie die anderen Spiele.
class LabyrinthScreen : public Screen {
public:
    LabyrinthScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    struct Wall {
        float x;
        float y;
        float w;
        float h;
    };

    static constexpr int kTopBarHeight = 20;
    static constexpr float kBallRadius = 6.0f;
    static constexpr float kGoalRadius = 14.0f;
    static constexpr float kTiltAccel = 0.0012f; // px/ms^2 pro Neigungs-"g" (Hauptsteuerung, siehe updatePhysics())
    static constexpr float kFriction = 0.985f;   // pro Frame, bremst ohne Neigung sanft ab
    static constexpr float kStartX = 24.0f;
    static constexpr float kStartY = 40.0f;
    static constexpr float kGoalX = 40.0f;
    static constexpr float kGoalY = 205.0f;
    static constexpr int kWallCount = 2;
    static const Wall kWalls[kWallCount];
    static constexpr int kHomeIconSize = 28;

    void resetGame();
    void resolveWallCollision(const Wall& wall);
    void updatePhysics(uint32_t deltaMs);
    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;

    float ballX_ = kStartX;
    float ballY_ = kStartY;
    float ballVx_ = 0.0f;
    float ballVy_ = 0.0f;
    bool won_ = false;
    uint32_t elapsedMs_ = 0;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
};
