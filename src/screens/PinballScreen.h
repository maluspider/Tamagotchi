#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Pinball (docs/projektplan.md Abschnitt 9/10, ab Stufe "Experte").
// Ueberarbeitete Physik: eine Kugel mit Schwerkraft, die an Waenden,
// mehreren Bumpern/Kickern UND zwei echten, um einen Drehpunkt schwenkenden
// Flippern abprallt (Nutzerwunsch: "pinball Ball geht durch die Balken
// durch und prallt nicht ab...pinball komplett ueberarbeiten") - die
// Flipper sind jetzt echte Kollisionsobjekte (Kreis-vs-Liniensegment,
// siehe resolveFlipperCollision()) statt einer reinen "Trefferzone", die
// nur bei aktiv gehaltenem Flipper ausloeste und die Kugel sonst
// ungebremst durchliess. Ist ein Flipper gerade aktiv gehalten, bekommt
// die Kugel beim ersten Kontakt zusaetzlich einen kraeftigen Schub
// ("Flick"), danach wirkt der Flipper bis zum Loslassen nur noch als
// passive Prallflaeche (kein wiederholtes Nachbeschleunigen pro Frame).
// Neigen des Geraets (IMU) gibt der Kugel einen kleinen seitlichen Schubs
// ("Nudge", wie beim echten Fluepper-Tisch). Faellt die Kugel unten durch,
// ist ein Ball verbraucht (3 Baelle pro Runde). Zeichnet ueber M5Canvas wie
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
        int points;
        uint16_t color;
    };

    static constexpr int kTopBarHeight = 20;
    static constexpr float kBallRadius = 6.0f;
    static constexpr int kStartBalls = 3;
    // 3 klassische Rundbumper + 2 kleine Slingshot-Kicker neben den
    // Flippern + 1 hoeher bepunktetes Bonus-Ziel oben mittig (Nutzerwunsch:
    // "pinball komplett ueberarbeiten und verbessern" - mehr Tisch-Elemente
    // statt nur dreier gleichwertiger Bumper).
    static constexpr int kBumperCount = 6;
    static constexpr float kGravity = 0.00055f;      // px/ms^2
    static constexpr float kFlipperBoostVy = -0.38f;  // px/ms
    static constexpr float kFlipperBoostVx = 0.16f;   // px/ms
    static constexpr float kTiltForce = 0.00035f;     // px/ms^2 pro g Neigung

    // Flipper-Geometrie: Drehpunkt + zwei feste Endpositionen (Ruhe-/
    // Aktiv-Winkel, kein weiches Zwischen-Animieren noetig fuer den
    // vereinfachten Kollisions-/Steuerungs-Umfang dieses Spiels). Rechter
    // Flipper ist der linke, an der Tischmitte gespiegelt.
    static constexpr float kFlipperHalfThickness = 3.5f;
    static constexpr float kFlipperPivotLeftX = 60.0f;
    static constexpr float kFlipperPivotY = 205.0f;
    static constexpr float kFlipperRestOffsetX = 48.0f;
    static constexpr float kFlipperRestOffsetY = 8.0f;
    static constexpr float kFlipperActiveOffsetX = 44.0f;
    static constexpr float kFlipperActiveOffsetY = -30.0f;

    void resetGame();
    void launchBall();
    void updatePhysics(uint32_t deltaMs);
    void handleInput(uint32_t deltaMs);
    void loseBall();
    void endGame();
    void resolveFlipperCollision(float pivotX, float pivotY, float tipX, float tipY, bool active, bool& boostedFlag);
    void leftFlipperTip(bool active, float& tipX, float& tipY) const;
    void rightFlipperTip(bool active, float& tipX, float& tipY) const;

    void drawFlipper(float pivotX, float pivotY, float tipX, float tipY);
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
