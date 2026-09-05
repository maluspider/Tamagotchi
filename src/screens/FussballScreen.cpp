#include "FussballScreen.h"

#include <M5Unified.h>

#include <cmath>

#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/HighscoreStore.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr const char* kHighscoreKey = "fussball";
} // namespace

FussballScreen::FussballScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void FussballScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    goals_ = 0;
    saves_ = 0;
    keeperX_ = (kGoalX1 + kGoalX2) / 2.0f;
    resetBallToLaunch();
}

void FussballScreen::resetBallToLaunch() {
    ballX_ = canvas_.width() / 2.0f;
    ballY_ = kLaunchY;
    ballVx_ = 0;
    ballVy_ = 0;
    state_ = BallState::Idle;
}

void FussballScreen::finishSession() {
    highscorestore::saveIfHigher(kHighscoreKey, static_cast<uint32_t>(goals_));
}

void FussballScreen::updateFlight(uint32_t deltaMs) {
    const float dt = static_cast<float>(deltaMs);

    // Torwart-KI: bewegt sich Richtung Ball, Geschwindigkeit begrenzt und
    // steigt mit jedem erzielten Tor (Abschnitt 10).
    const float keeperSpeed = kKeeperBaseSpeed + static_cast<float>(goals_) * kKeeperSpeedPerGoal;
    const float targetX = (state_ == BallState::Flying) ? ballX_ : (kGoalX1 + kGoalX2) / 2.0f;
    if (keeperX_ < targetX) {
        keeperX_ += keeperSpeed * dt;
        if (keeperX_ > targetX) {
            keeperX_ = targetX;
        }
    } else if (keeperX_ > targetX) {
        keeperX_ -= keeperSpeed * dt;
        if (keeperX_ < targetX) {
            keeperX_ = targetX;
        }
    }
    if (keeperX_ < kGoalX1 + kKeeperHalfWidth) {
        keeperX_ = kGoalX1 + kKeeperHalfWidth;
    }
    if (keeperX_ > kGoalX2 - kKeeperHalfWidth) {
        keeperX_ = kGoalX2 - kKeeperHalfWidth;
    }

    if (state_ != BallState::Flying) {
        return;
    }

    ballX_ += ballVx_ * dt;
    ballY_ += ballVy_ * dt;

    if (ballX_ < 0 || ballX_ > canvas_.width()) {
        resetBallToLaunch();
        return;
    }

    if (ballY_ <= kGoalLineY) {
        if (ballX_ > kGoalX1 && ballX_ < kGoalX2 && fabsf(ballX_ - keeperX_) < kKeeperHalfWidth + kBallRadius) {
            ++saves_;
            haptics::pulse(60);
        } else if (ballX_ > kGoalX1 && ballX_ < kGoalX2) {
            ++goals_;
            haptics::pulse(80);
        }
        resetBallToLaunch();
    }
}

bool FussballScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void FussballScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();

    if (touch.wasPressed()) {
        if (touchedHomeIcon(touch.x, touch.y)) {
            finishSession();
            stateMachine_.requestSwitch(ScreenId::Home);
            return;
        }
        if (state_ == BallState::Idle) {
            touchStartX_ = touch.x;
            touchStartY_ = touch.y;
            swiping_ = true;
        }
        return;
    }

    if (touch.wasReleased() && swiping_) {
        swiping_ = false;
        if (state_ == BallState::Idle) {
            const int dx = touch.x - touchStartX_;
            const int dy = touch.y - touchStartY_;
            if (dx * dx + dy * dy > kMinSwipeDist * kMinSwipeDist && dy < 0) {
                ballVx_ = static_cast<float>(dx) * kSwipeScale;
                ballVy_ = static_cast<float>(dy) * kSwipeScale;
                state_ = BallState::Flying;
            }
        }
    }
}

void FussballScreen::update(uint32_t deltaMs) {
    handleInput();

    if (playtimeTicker_.tick(app_, deltaMs)) {
        finishSession();
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    updateFlight(deltaMs);
    draw();
}

void FussballScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void FussballScreen::draw() {
    // Rasen-Streifen-Muster statt Flat-Hintergrund (Nutzerwunsch: "keine
    // rudimentaeren Darstellungen mehr, optimiere Grafik maximal").
    constexpr uint16_t kGrassA = theme::rgb565(0x2E, 0x8B, 0x2A);
    constexpr uint16_t kGrassB = theme::rgb565(0x27, 0x7A, 0x24);
    constexpr int kStripeH = 20;
    for (int y = kTopBarHeight; y < canvas_.height(); y += kStripeH) {
        canvas_.fillRect(0, y, canvas_.width(), kStripeH, ((y / kStripeH) % 2 == 0) ? kGrassA : kGrassB);
    }
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));

    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    char buf[32];
    snprintf(buf, sizeof(buf), "Tore: %d  Paraden: %d", goals_, saves_);
    canvas_.drawString(buf, 4, 4);

    // Tor mit Netzraster statt leerem Rahmen.
    const int goalX1i = static_cast<int>(kGoalX1);
    const int goalX2i = static_cast<int>(kGoalX2);
    const int goalTopY = 20;
    const int goalBotY = static_cast<int>(kGoalLineY);

    // Stadion-Werbebande links/rechts vom Tor statt reinem Rasen bis zum
    // Bildrand - bunte kurze Blockstreifen wie LED-Banden hinter dem Tor
    // (Nutzerwunsch: "hole das Maximum aus der Hardware...maximal
    // hochwertig...mit Backgrounds").
    static constexpr uint16_t kHoardingColors[3] = {TFT_WHITE, theme::kAccentCyan, theme::kAccentGold};
    for (int hx = 0; hx < goalX1i; hx += 18) {
        canvas_.fillRect(hx, goalTopY + 2, 14, 6, kHoardingColors[(hx / 18) % 3]);
    }
    for (int hx = goalX2i; hx < canvas_.width(); hx += 18) {
        canvas_.fillRect(hx, goalTopY + 2, 14, 6, kHoardingColors[(hx / 18) % 3]);
    }

    canvas_.drawRect(goalX1i, goalTopY, goalX2i - goalX1i, goalBotY - goalTopY, TFT_WHITE);
    for (int gx = goalX1i + 10; gx < goalX2i; gx += 10) {
        canvas_.drawLine(gx, goalTopY, gx, goalBotY, gfxkit::darken(TFT_WHITE, 0.5f));
    }
    for (int gy = goalTopY + 8; gy < goalBotY; gy += 8) {
        canvas_.drawLine(goalX1i, gy, goalX2i, gy, gfxkit::darken(TFT_WHITE, 0.5f));
    }

    // Torwart als gebeveltes Trikot-Panel statt Flat-Rechteck.
    gfxkit::bevelPanel(&canvas_, static_cast<int>(keeperX_ - kKeeperHalfWidth), static_cast<int>(kGoalLineY) - 6,
                        static_cast<int>(kKeeperHalfWidth * 2), 10, 2, theme::kDanger, true);

    // Ball mit klassischem Fuenfeck-Pattern-Andeutung (Kreis + Innenkreis +
    // kleine Punkte) statt reiner Zwei-Farb-Kugel.
    gfxkit::shinyBall(&canvas_, static_cast<int>(ballX_), static_cast<int>(ballY_), static_cast<int>(kBallRadius),
                       TFT_WHITE);
    canvas_.fillCircle(static_cast<int>(ballX_), static_cast<int>(ballY_), static_cast<int>(kBallRadius) / 3,
                        TFT_BLACK);

    if (state_ == BallState::Idle) {
        canvas_.setTextDatum(bottom_center);
        canvas_.setTextSize(1);
        canvas_.drawString("Nach oben ziehen zum Schiessen", canvas_.width() / 2, static_cast<int>(kLaunchY) - 16);
    }

    drawHomeIcon();
    canvas_.pushSprite(0, 0);
}
