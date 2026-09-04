#include "TetrisScreen.h"

#include <M5Unified.h>
#include <esp_random.h>

#include "../core/HighscoreStore.h"
#include "../core/ScreenId.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr const char* kHighscoreKey = "tetris";

// Die 7 Standard-Tetrominos, jeweils in ihrer Spawn-Ausrichtung als 4x4-
// Raster. Weitere Rotationen werden generisch berechnet (rotateGridCW()).
constexpr bool kPieceI[4][4] = {
    {false, false, false, false},
    {true, true, true, true},
    {false, false, false, false},
    {false, false, false, false},
};
constexpr bool kPieceO[4][4] = {
    {false, false, false, false},
    {false, true, true, false},
    {false, true, true, false},
    {false, false, false, false},
};
constexpr bool kPieceT[4][4] = {
    {false, true, false, false},
    {true, true, true, false},
    {false, false, false, false},
    {false, false, false, false},
};
constexpr bool kPieceS[4][4] = {
    {false, true, true, false},
    {true, true, false, false},
    {false, false, false, false},
    {false, false, false, false},
};
constexpr bool kPieceZ[4][4] = {
    {true, true, false, false},
    {false, true, true, false},
    {false, false, false, false},
    {false, false, false, false},
};
constexpr bool kPieceJ[4][4] = {
    {true, false, false, false},
    {true, true, true, false},
    {false, false, false, false},
    {false, false, false, false},
};
constexpr bool kPieceL[4][4] = {
    {false, false, true, false},
    {true, true, true, false},
    {false, false, false, false},
    {false, false, false, false},
};

const bool (*kPieces[7])[4] = {kPieceI, kPieceO, kPieceT, kPieceS, kPieceZ, kPieceJ, kPieceL};
constexpr uint16_t kPieceColors[7] = {TFT_CYAN, TFT_YELLOW, TFT_PURPLE, TFT_GREEN, TFT_RED, TFT_BLUE, TFT_ORANGE};

void rotateGridCW(bool grid[4][4]) {
    bool rotated[4][4];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            rotated[c][3 - r] = grid[r][c];
        }
    }
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            grid[r][c] = rotated[r][c];
        }
    }
}

} // namespace

TetrisScreen::TetrisScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void TetrisScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    resetGame();
}

void TetrisScreen::resetGame() {
    for (int r = 0; r < kBoardRows; ++r) {
        for (int c = 0; c < kBoardCols; ++c) {
            board_[r][c] = false;
            boardColor_[r][c] = 0;
        }
    }
    score_ = 0;
    linesCleared_ = 0;
    gameOver_ = false;
    stepAccumulatorMs_ = 0;
    touchActive_ = false;
    spawnPiece();
}

void TetrisScreen::spawnPiece() {
    const int index = static_cast<int>(esp_random() % 7);
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            currentShape_[r][c] = kPieces[index][r][c];
        }
    }
    currentColor_ = kPieceColors[index];
    pieceX_ = kBoardCols / 2 - 2;
    pieceY_ = -1;
}

bool TetrisScreen::collides(const bool shape[4][4], int px, int py) const {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (!shape[r][c]) {
                continue;
            }
            const int boardR = py + r;
            const int boardC = px + c;
            if (boardC < 0 || boardC >= kBoardCols || boardR >= kBoardRows) {
                return true;
            }
            if (boardR < 0) {
                continue; // oberhalb des Spielfelds ist beim Spawn erlaubt
            }
            if (board_[boardR][boardC]) {
                return true;
            }
        }
    }
    return false;
}

void TetrisScreen::clearLines() {
    int cleared = 0;
    for (int r = kBoardRows - 1; r >= 0; --r) {
        bool full = true;
        for (int c = 0; c < kBoardCols; ++c) {
            if (!board_[r][c]) {
                full = false;
                break;
            }
        }
        if (!full) {
            continue;
        }
        for (int rr = r; rr > 0; --rr) {
            for (int c = 0; c < kBoardCols; ++c) {
                board_[rr][c] = board_[rr - 1][c];
                boardColor_[rr][c] = boardColor_[rr - 1][c];
            }
        }
        for (int c = 0; c < kBoardCols; ++c) {
            board_[0][c] = false;
        }
        ++cleared;
        ++r; // dieselbe Zeile erneut pruefen, da alles eine Reihe nach unten gerutscht ist
    }
    if (cleared > 0) {
        linesCleared_ += cleared;
        score_ += cleared * 100;
    }
}

void TetrisScreen::endGame() {
    gameOver_ = true;
    highscorestore::saveIfHigher(kHighscoreKey, static_cast<uint32_t>(score_));
}

void TetrisScreen::lockPiece() {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (!currentShape_[r][c]) {
                continue;
            }
            const int boardR = pieceY_ + r;
            const int boardC = pieceX_ + c;
            if (boardR >= 0 && boardR < kBoardRows && boardC >= 0 && boardC < kBoardCols) {
                board_[boardR][boardC] = true;
                boardColor_[boardR][boardC] = currentColor_;
            }
        }
    }
    clearLines();
    spawnPiece();
    if (collides(currentShape_, pieceX_, pieceY_)) {
        endGame();
    }
}

void TetrisScreen::moveLeft() {
    if (!collides(currentShape_, pieceX_ - 1, pieceY_)) {
        --pieceX_;
    }
}

void TetrisScreen::moveRight() {
    if (!collides(currentShape_, pieceX_ + 1, pieceY_)) {
        ++pieceX_;
    }
}

void TetrisScreen::rotate() {
    bool rotated[4][4];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            rotated[r][c] = currentShape_[r][c];
        }
    }
    rotateGridCW(rotated);
    if (!collides(rotated, pieceX_, pieceY_)) {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                currentShape_[r][c] = rotated[r][c];
            }
        }
    }
    // Kollidiert die Drehung (z. B. am Spielfeldrand) - bewusst kein
    // Wandkick, die Drehung wird einfach verworfen (Abschnitt 10:
    // vereinfachte, kindgerechte Steuerung statt SRS-Feinschliff).
}

void TetrisScreen::hardDrop() {
    while (!collides(currentShape_, pieceX_, pieceY_ + 1)) {
        ++pieceY_;
    }
    lockPiece();
}

bool TetrisScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void TetrisScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();

    if (touch.wasPressed()) {
        touchStartX_ = touch.x;
        touchStartY_ = touch.y;
        touchActive_ = true;
        return;
    }

    if (!touch.wasReleased() || !touchActive_) {
        return;
    }
    touchActive_ = false;

    if (touchedHomeIcon(touchStartX_, touchStartY_)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    if (gameOver_) {
        resetGame();
        return;
    }

    const int dx = touch.x - touchStartX_;
    const int dy = touch.y - touchStartY_;

    if (dy > kSwipeDownThreshold && abs(dy) > abs(dx)) {
        hardDrop();
        return;
    }

    if (abs(dx) < kTapMoveTolerance && abs(dy) < kTapMoveTolerance) {
        const int third = M5.Display.width() / 3;
        if (touchStartX_ < third) {
            moveLeft();
        } else if (touchStartX_ < 2 * third) {
            rotate();
        } else {
            moveRight();
        }
    }
}

void TetrisScreen::update(uint32_t deltaMs) {
    handleInput();

    if (gameOver_) {
        draw();
        return;
    }

    if (playtimeTicker_.tick(app_, deltaMs)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    const int computedIntervalMs = 500 - (linesCleared_ / 5) * 30;
    const uint32_t stepIntervalMs = static_cast<uint32_t>(computedIntervalMs > 120 ? computedIntervalMs : 120);
    stepAccumulatorMs_ += deltaMs;
    if (stepAccumulatorMs_ >= stepIntervalMs) {
        stepAccumulatorMs_ -= stepIntervalMs;
        if (!collides(currentShape_, pieceX_, pieceY_ + 1)) {
            ++pieceY_;
        } else {
            lockPiece();
        }
    }

    draw();
}

void TetrisScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, TFT_WHITE);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, TFT_WHITE);
    canvas_.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, TFT_WHITE);
}

void TetrisScreen::drawBoard() {
    canvas_.fillRect(kBoardOffsetX - 2, kBoardOffsetY - 2, kBoardCols * kCellSize + 4, kBoardRows * kCellSize + 4,
                      TFT_WHITE);
    canvas_.fillRect(kBoardOffsetX, kBoardOffsetY, kBoardCols * kCellSize, kBoardRows * kCellSize, TFT_BLACK);

    for (int r = 0; r < kBoardRows; ++r) {
        for (int c = 0; c < kBoardCols; ++c) {
            if (!board_[r][c]) {
                continue;
            }
            const int x = kBoardOffsetX + c * kCellSize;
            const int y = kBoardOffsetY + r * kCellSize;
            canvas_.fillRect(x, y, kCellSize - 1, kCellSize - 1, boardColor_[r][c]);
        }
    }

    if (!gameOver_) {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                if (!currentShape_[r][c]) {
                    continue;
                }
                const int boardR = pieceY_ + r;
                const int boardC = pieceX_ + c;
                if (boardR < 0 || boardR >= kBoardRows || boardC < 0 || boardC >= kBoardCols) {
                    continue;
                }
                const int x = kBoardOffsetX + boardC * kCellSize;
                const int y = kBoardOffsetY + boardR * kCellSize;
                canvas_.fillRect(x, y, kCellSize - 1, kCellSize - 1, currentColor_);
            }
        }
    }
}

void TetrisScreen::draw() {
    canvas_.fillScreen(TFT_BLACK);
    canvas_.fillRect(0, 0, canvas_.width(), kTopBarHeight, TFT_NAVY);

    char buf[24];
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    snprintf(buf, sizeof(buf), "Punkte: %d", score_);
    canvas_.drawString(buf, 4, 4);

    drawBoard();
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
