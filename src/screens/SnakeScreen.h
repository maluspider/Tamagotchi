#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Snake (docs/projektplan.md Abschnitt 9/10) - erstes freigeschaltetes
// Spiel ab Stufe "Baby". Steuerung: Antippen relativ zur Bildschirmmitte
// waehlt die naeheste Richtung (oben/unten/links/rechts) - robuster als
// feste Randzonen auf einem kleinen 2"-Screen. Verbraucht Spielzeitguthaben
// minutenweise (Abschnitt 7); ist das Guthaben aufgebraucht, geht es
// automatisch zurueck zu Home.
//
// Zeichnet ueber ein M5Canvas-Offscreen-Sprite (Abschnitt 3: "Sprite-
// basiertes Rendering ... fuer fluessige Pixel-Art-Grafik") statt direkt
// auf M5.Display, damit die haeufigen Redraws nicht flackern.
class SnakeScreen : public Screen {
public:
    SnakeScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    struct Point {
        int8_t x;
        int8_t y;
    };

    static constexpr int kCellSize = 10;
    static constexpr int kTopBarHeight = 20;
    static constexpr int kCols = 32;                                  // 320 / kCellSize
    static constexpr int kRows = (240 - kTopBarHeight) / kCellSize;   // 22
    static constexpr size_t kMaxLength = static_cast<size_t>(kCols) * static_cast<size_t>(kRows);
    static constexpr uint32_t kStepIntervalMs = 180;

    static constexpr int kHomeIconSize = 28;

    void resetGame();
    void spawnFood();
    void step();
    void handleInput();
    void drawPlayfield();
    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;
    void endGame();

    AppContext& app_;
    StateMachine& stateMachine_;
    // Wird im Konstruktor (SnakeScreen.cpp, dort ist M5Unified.h
    // eingebunden) an M5.Display gebunden - siehe Klassenkommentar.
    M5Canvas canvas_;

    Point body_[kMaxLength];
    size_t length_ = 3;
    Point direction_{1, 0};
    Point pendingDirection_{1, 0};
    Point food_{0, 0};

    bool gameOver_ = false;
    int score_ = 0;

    uint32_t stepAccumulatorMs_ = 0;
    PlaytimeTicker playtimeTicker_;
};
