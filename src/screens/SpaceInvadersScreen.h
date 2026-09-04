#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Space Invaders (docs/projektplan.md Abschnitt 9/10, ab Stufe "Junior").
// Steuerung: linkes/rechtes Bildschirmdrittel = Schiff bewegen (solange
// gehalten), mittleres Drittel antippen = Feuer-Button. Aliens marschieren
// als Block seitlich, weichen an den Raendern aus und ruecken dabei eine
// Reihe nach unten - Wellen mit steigendem Schwierigkeitsgrad (naechste
// Welle startet schneller, sobald alle Aliens einer Welle zerstoert sind).
// Wie das Original-Arcade-Spiel: nur ein Spieler-Geschoss gleichzeitig in
// der Luft. Zeichnet ueber M5Canvas wie SnakeScreen.
class SpaceInvadersScreen : public Screen {
public:
    SpaceInvadersScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    struct Bullet {
        float x = 0;
        float y = 0;
        bool active = false;
    };

    static constexpr int kAlienCols = 5;
    static constexpr int kAlienRows = 3;
    static constexpr int kAlienCount = kAlienCols * kAlienRows;
    static constexpr int kAlienW = 20;
    static constexpr int kAlienH = 14;
    static constexpr int kAlienSpacingX = 30;
    static constexpr int kAlienSpacingY = 22;
    static constexpr int kTopBarHeight = 20;
    static constexpr int kFormationBaseX = 30;
    static constexpr int kFormationBaseY = kTopBarHeight + 12;
    static constexpr int kShipY = 220;
    static constexpr int kShipW = 24;
    static constexpr int kShipH = 12;
    static constexpr int kMaxAlienBullets = 3;
    static constexpr int kStartLives = 3;

    void resetGame();
    void startWave();
    void updateAliens(uint32_t deltaMs);
    void updateBullets(uint32_t deltaMs);
    void fireBullet();
    void handleInput(uint32_t deltaMs);
    void endGame();

    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;

    bool alienAlive_[kAlienCount];
    int aliensAliveCount_ = kAlienCount;
    float formationX_ = 0;
    float formationY_ = 0;
    int alienDirection_ = 1;
    uint32_t alienMoveAccumMs_ = 0;
    uint32_t alienMoveIntervalMs_ = 500;
    uint32_t alienFireAccumMs_ = 0;
    uint32_t alienFireIntervalMs_ = 1400;

    Bullet playerBullet_;
    Bullet alienBullets_[kMaxAlienBullets];

    float shipX_ = 148;
    int lives_ = kStartLives;
    int wave_ = 1;
    int score_ = 0;
    bool gameOver_ = false;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
};
