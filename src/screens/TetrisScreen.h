#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Tetris (docs/projektplan.md Abschnitt 9/10, ab Stufe "Kind"). Steuerung:
// drei Touch-Zonen (links bewegen | tippen = drehen | rechts bewegen),
// zusaetzlich Swipe nach unten ueberall auf dem Spielfeld = Hard-Drop.
// Standard-7-Steine-Set, Fallgeschwindigkeit steigt mit den geraeumten
// Reihen. Rotation ist bewusst vereinfacht: eine generische 90°-Drehung
// des 4x4-Rasters (rotateGridCW()) statt hartcodierter Rotations-
// Zustandstabellen pro Stein - weniger fehleranfaellig und ohne
// Wandkick-Sonderfaelle (SRS). Zeichnet ueber M5Canvas wie SnakeScreen.
class TetrisScreen : public Screen {
public:
    TetrisScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    static constexpr int kBoardCols = 10;
    static constexpr int kBoardRows = 16;
    static constexpr int kCellSize = 12;
    static constexpr int kTopBarHeight = 20;
    static constexpr int kBoardOffsetX = (320 - kBoardCols * kCellSize) / 2;
    static constexpr int kBoardOffsetY = kTopBarHeight + 4;
    static constexpr int kSwipeDownThreshold = 40;
    static constexpr int kTapMoveTolerance = 16;

    void resetGame();
    void spawnPiece();
    bool collides(const bool shape[4][4], int px, int py) const;
    void lockPiece();
    void clearLines();
    void moveLeft();
    void moveRight();
    void rotate();
    void hardDrop();
    void endGame();
    void handleInput();

    void drawBoard();
    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;

    bool board_[kBoardRows][kBoardCols] = {};
    uint16_t boardColor_[kBoardRows][kBoardCols] = {};

    bool currentShape_[4][4] = {};
    uint16_t currentColor_ = 0;
    int pieceX_ = 0;
    int pieceY_ = 0;

    int score_ = 0;
    int linesCleared_ = 0;
    bool gameOver_ = false;

    uint32_t stepAccumulatorMs_ = 0;

    int touchStartX_ = 0;
    int touchStartY_ = 0;
    bool touchActive_ = false;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
};
