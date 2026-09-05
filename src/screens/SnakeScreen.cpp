#include "SnakeScreen.h"

#include <M5Unified.h>
#include <esp_random.h>

#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/HighscoreStore.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr const char* kHighscoreKey = "snake";
} // namespace

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
        endGame();
        return;
    }
    for (size_t i = 0; i < length_; ++i) {
        if (body_[i].x == newHead.x && body_[i].y == newHead.y) {
            endGame();
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
        haptics::pulse(40);
        spawnFood();
    }
}

void SnakeScreen::endGame() {
    gameOver_ = true;
    haptics::pulse(200);
    highscorestore::saveIfHigher(kHighscoreKey, static_cast<uint32_t>(score_));
}

bool SnakeScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void SnakeScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Home);
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
    if (playtimeTicker_.tick(app_, deltaMs)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    stepAccumulatorMs_ += deltaMs;
    if (stepAccumulatorMs_ >= kStepIntervalMs) {
        stepAccumulatorMs_ -= kStepIntervalMs;
        step();
    }

    draw();
}

void SnakeScreen::drawPlayfield() {
    // Zweifarbiges Wiesen-Schachbrett statt einer flachen Schwarzflaeche
    // (Nutzerwunsch: "keine rudimentaeren Darstellungen mehr, optimiere
    // Grafik maximal") - klassischer Retro-Rasenboden-Look.
    const uint16_t grassA = gfxkit::darken(theme::kSuccess, 0.82f);
    const uint16_t grassB = gfxkit::darken(theme::kSuccess, 0.88f);
    for (int gy = 0; gy < kRows; ++gy) {
        for (int gx = 0; gx < kCols; ++gx) {
            const int x = gx * kCellSize;
            const int y = kTopBarHeight + gy * kCellSize;
            canvas_.fillRect(x, y, kCellSize, kCellSize, ((gx + gy) % 2 == 0) ? grassA : grassB);
        }
    }

    for (size_t i = 0; i < length_; ++i) {
        const int x = body_[i].x * kCellSize;
        const int y = kTopBarHeight + body_[i].y * kCellSize;
        const uint16_t base = (i == 0) ? theme::kAccentGold : theme::kSuccess;
        // Leicht abgerundete, gebevelte Segmente statt flacher Quadrate.
        gfxkit::bevelPanel(&canvas_, x, y, kCellSize - 1, kCellSize - 1, 3, base, true);
        if (i == 0) {
            // Augen auf dem Kopf-Segment, in Blickrichtung versetzt.
            const int ex = x + (kCellSize - 1) / 2 + direction_.x * 2;
            const int ey = y + (kCellSize - 1) / 2 + direction_.y * 2;
            canvas_.fillCircle(ex, ey, 1, theme::kOutline);
        }
    }

    const int fx = food_.x * kCellSize + kCellSize / 2;
    const int fy = kTopBarHeight + food_.y * kCellSize + kCellSize / 2;
    // Apfel statt flachem Quadrat: Glanzlicht-Kugel plus kleines Blatt.
    gfxkit::shinyBall(&canvas_, fx, fy, (kCellSize - 2) / 2, theme::kDanger);
    canvas_.fillTriangle(fx, fy - (kCellSize - 2) / 2, fx + 3, fy - (kCellSize - 2) / 2 - 4, fx + 1,
                          fy - (kCellSize - 2) / 2, theme::kSuccess);
}

void SnakeScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void SnakeScreen::draw() {
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));

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
        canvas_.drawString("Game Over", canvas_.width() / 2, canvas_.height() / 2 - 25);
        canvas_.setTextSize(2);
        char hsBuf[24];
        snprintf(hsBuf, sizeof(hsBuf), "Highscore: %u", static_cast<unsigned>(highscorestore::load(kHighscoreKey)));
        canvas_.drawString(hsBuf, canvas_.width() / 2, canvas_.height() / 2 + 10);
        canvas_.drawString("Tippen zum Neustart", canvas_.width() / 2, canvas_.height() / 2 + 35);
    }

    drawHomeIcon();
    canvas_.pushSprite(0, 0);
}
