#include "SnakeScreen.h"

#include <M5Unified.h>
#include <esp_random.h>

#include "../core/ScreenId.h"

SnakeScreen::SnakeScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void SnakeScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    resetGame();
}

void SnakeScreen::resetGame() {
    length_ = 3;
    direction_ = Point{1, 0};
    pendingDirection_ = Point{1, 0};

    const int8_t startX = kCols / 2;
    const int8_t startY = kRows / 2;
    for (size_t i = 0; i < length_; ++i) {
        body_[i] = Point{static_cast<int8_t>(startX - static_cast<int>(i)), startY};
    }

    score_ = 0;
    gameOver_ = false;
    stepAccumulatorMs_ = 0;
    minuteAccumulatorMs_ = 0;

    spawnFood();
}

void SnakeScreen::spawnFood() {
    for (;;) {
        const int8_t fx = static_cast<int8_t>(esp_random() % kCols);
        const int8_t fy = static_cast<int8_t>(esp_random() % kRows);

        bool onSnake = false;
        for (size_t i = 0; i < length_; ++i) {
            if (body_[i].x == fx && body_[i].y == fy) {
                onSnake = true;
                break;
            }
        }
        if (!onSnake) {
            food_ = Point{fx, fy};
            return;
        }
    }
}

void SnakeScreen::step() {
    direction_ = pendingDirection_;

    Point newHead = body_[0];
    newHead.x = static_cast<int8_t>(newHead.x + direction_.x);
    newHead.y = static_cast<int8_t>(newHead.y + direction_.y);

    if (newHead.x < 0 || newHead.x >= kCols || newHead.y < 0 || newHead.y >= kRows) {
        gameOver_ = true;
        return;
    }
    for (size_t i = 0; i < length_; ++i) {
        if (body_[i].x == newHead.x && body_[i].y == newHead.y) {
            gameOver_ = true;
            return;
        }
    }

    const bool ateFood = (newHead.x == food_.x && newHead.y == food_.y);
    const Point oldTail = body_[length_ - 1];

    for (size_t i = length_ - 1; i > 0; --i) {
        body_[i] = body_[i - 1];
    }
    body_[0] = newHead;

    if (ateFood) {
        if (length_ < kMaxLength) {
            body_[length_] = oldTail;
            ++length_;
        }
        ++score_;
        spawnFood();
    }
}

void SnakeScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (gameOver_) {
        resetGame();
        return;
    }

    const int centerX = M5.Display.width() / 2;
    const int centerY = kTopBarHeight + (M5.Display.height() - kTopBarHeight) / 2;
    const int dx = touch.x - centerX;
    const int dy = touch.y - centerY;

    Point wanted;
    if (abs(dx) > abs(dy)) {
        wanted = (dx > 0) ? Point{1, 0} : Point{-1, 0};
    } else {
        wanted = (dy > 0) ? Point{0, 1} : Point{0, -1};
    }

    // 180-Grad-Kehrtwende verhindern (klassische Snake-Regel) - sonst
    // beisst sich die Schlange sofort selbst.
    if (wanted.x != -direction_.x || wanted.y != -direction_.y) {
        pendingDirection_ = wanted;
    }
}

void SnakeScreen::update(uint32_t deltaMs) {
    handleInput();

    if (gameOver_) {
        draw();
        return;
    }

    // Spielzeit verbrauchen (Abschnitt 7): jede volle Minute Spielzeit
    // kostet eine Minute Guthaben; ist das Guthaben aufgebraucht, geht es
    // sofort zurueck zu Home ("Spiele-Menü nur mit vorhandenem
    // Zeitguthaben betretbar").
    minuteAccumulatorMs_ += deltaMs;
    if (minuteAccumulatorMs_ >= 60000) {
        minuteAccumulatorMs_ -= 60000;
        if (!app_.playtime.spend(1)) {
            app_.persistProgress();
            stateMachine_.requestSwitch(ScreenId::Home);
            return;
        }
        app_.persistProgress();
    }

    stepAccumulatorMs_ += deltaMs;
    if (stepAccumulatorMs_ >= kStepIntervalMs) {
        stepAccumulatorMs_ -= kStepIntervalMs;
        step();
    }

    draw();
}

void SnakeScreen::drawPlayfield() {
    canvas_.fillRect(0, kTopBarHeight, canvas_.width(), canvas_.height() - kTopBarHeight, TFT_BLACK);

    for (size_t i = 0; i < length_; ++i) {
        const int x = body_[i].x * kCellSize;
        const int y = kTopBarHeight + body_[i].y * kCellSize;
        canvas_.fillRect(x, y, kCellSize - 1, kCellSize - 1, i == 0 ? TFT_YELLOW : TFT_GREEN);
    }

    const int fx = food_.x * kCellSize;
    const int fy = kTopBarHeight + food_.y * kCellSize;
    canvas_.fillRect(fx, fy, kCellSize - 1, kCellSize - 1, TFT_RED);
}

void SnakeScreen::draw() {
    canvas_.fillRect(0, 0, canvas_.width(), kTopBarHeight, TFT_NAVY);

    char buf[24];
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);

    canvas_.setTextDatum(top_left);
    snprintf(buf, sizeof(buf), "Punkte: %d", score_);
    canvas_.drawString(buf, 4, 4);

    canvas_.setTextDatum(top_right);
    snprintf(buf, sizeof(buf), "Zeit: %u", static_cast<unsigned>(app_.playtime.availableMinutes()));
    canvas_.drawString(buf, canvas_.width() - 4, 4);

    drawPlayfield();

    if (gameOver_) {
        canvas_.setTextDatum(middle_center);
        canvas_.setTextColor(TFT_WHITE);
        canvas_.setTextSize(3);
        canvas_.drawString("Game Over", canvas_.width() / 2, canvas_.height() / 2 - 15);
        canvas_.setTextSize(2);
        canvas_.drawString("Tippen zum Neustart", canvas_.width() / 2, canvas_.height() / 2 + 20);
    }

    canvas_.pushSprite(0, 0);
}
