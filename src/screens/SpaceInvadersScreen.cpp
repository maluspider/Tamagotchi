#include "SpaceInvadersScreen.h"

#include <M5Unified.h>
#include <esp_random.h>

#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/HighscoreStore.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr const char* kHighscoreKey = "space_invaders";
constexpr float kPlayerBulletSpeed = 0.18f; // px/ms
constexpr float kAlienBulletSpeed = 0.09f;  // px/ms
constexpr float kShipSpeed = 0.15f;         // px/ms
constexpr float kAlienStepPx = 6.0f;
} // namespace

SpaceInvadersScreen::SpaceInvadersScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void SpaceInvadersScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    resetGame();
}

void SpaceInvadersScreen::resetGame() {
    lives_ = kStartLives;
    wave_ = 0;
    score_ = 0;
    gameOver_ = false;
    shipX_ = (M5.Display.width() - kShipW) / 2.0f;
    startWave();
}

void SpaceInvadersScreen::startWave() {
    ++wave_;

    for (bool& alive : alienAlive_) {
        alive = true;
    }
    aliensAliveCount_ = kAlienCount;
    formationX_ = 0;
    formationY_ = 0;
    alienDirection_ = 1;
    alienMoveAccumMs_ = 0;
    alienFireAccumMs_ = 0;
    playerBullet_.active = false;
    for (Bullet& b : alienBullets_) {
        b.active = false;
    }

    constexpr uint32_t kBaseIntervalMs = 500;
    constexpr uint32_t kMinIntervalMs = 150;
    const uint32_t reduction = static_cast<uint32_t>((wave_ - 1) * 60);
    alienMoveIntervalMs_ = (kBaseIntervalMs > reduction + kMinIntervalMs) ? (kBaseIntervalMs - reduction) : kMinIntervalMs;
}

void SpaceInvadersScreen::endGame() {
    gameOver_ = true;
    haptics::pulse(200);
    highscorestore::saveIfHigher(kHighscoreKey, static_cast<uint32_t>(score_));
}

void SpaceInvadersScreen::updateAliens(uint32_t deltaMs) {
    alienMoveAccumMs_ += deltaMs;
    if (alienMoveAccumMs_ < alienMoveIntervalMs_) {
        return;
    }
    alienMoveAccumMs_ -= alienMoveIntervalMs_;

    formationX_ += static_cast<float>(alienDirection_) * kAlienStepPx;

    const float leftEdge = kFormationBaseX + formationX_;
    const float rightEdge = kFormationBaseX + (kAlienCols - 1) * kAlienSpacingX + kAlienW + formationX_;
    if (leftEdge < 4 || rightEdge > M5.Display.width() - 4) {
        formationX_ -= static_cast<float>(alienDirection_) * kAlienStepPx; // diesen Schritt rueckgaengig machen
        alienDirection_ = -alienDirection_;
        formationY_ += kAlienH;
    }

    if (kFormationBaseY + formationY_ + (kAlienRows - 1) * kAlienSpacingY + kAlienH >= kShipY) {
        endGame(); // Aliens haben die Verteidigungslinie erreicht
    }
}

void SpaceInvadersScreen::updateBullets(uint32_t deltaMs) {
    if (playerBullet_.active) {
        playerBullet_.y -= kPlayerBulletSpeed * static_cast<float>(deltaMs);
        if (playerBullet_.y < kTopBarHeight) {
            playerBullet_.active = false;
        } else {
            for (int i = 0; i < kAlienCount; ++i) {
                if (!alienAlive_[i]) {
                    continue;
                }
                const int col = i % kAlienCols;
                const int row = i / kAlienCols;
                const float ax = kFormationBaseX + col * kAlienSpacingX + formationX_;
                const float ay = kFormationBaseY + row * kAlienSpacingY + formationY_;
                if (playerBullet_.x >= ax && playerBullet_.x <= ax + kAlienW && playerBullet_.y >= ay &&
                    playerBullet_.y <= ay + kAlienH) {
                    alienAlive_[i] = false;
                    --aliensAliveCount_;
                    score_ += 10;
                    playerBullet_.active = false;
                    haptics::pulse(30);
                    break;
                }
            }
        }
    }

    if (aliensAliveCount_ == 0) {
        startWave();
        return;
    }

    alienFireAccumMs_ += deltaMs;
    if (alienFireAccumMs_ >= alienFireIntervalMs_) {
        alienFireAccumMs_ = 0;

        int freeSlot = -1;
        for (int i = 0; i < kMaxAlienBullets; ++i) {
            if (!alienBullets_[i].active) {
                freeSlot = i;
                break;
            }
        }
        if (freeSlot >= 0) {
            int shooter = -1;
            for (int attempt = 0; attempt < kAlienCount; ++attempt) {
                const int candidate = static_cast<int>(esp_random() % kAlienCount);
                if (alienAlive_[candidate]) {
                    shooter = candidate;
                    break;
                }
            }
            if (shooter >= 0) {
                const int col = shooter % kAlienCols;
                const int row = shooter / kAlienCols;
                alienBullets_[freeSlot].x = kFormationBaseX + col * kAlienSpacingX + formationX_ + kAlienW / 2.0f;
                alienBullets_[freeSlot].y = kFormationBaseY + row * kAlienSpacingY + formationY_ + kAlienH;
                alienBullets_[freeSlot].active = true;
            }
        }
    }

    for (Bullet& b : alienBullets_) {
        if (!b.active) {
            continue;
        }
        b.y += kAlienBulletSpeed * static_cast<float>(deltaMs);
        if (b.y > M5.Display.height()) {
            b.active = false;
            continue;
        }
        if (b.x >= shipX_ && b.x <= shipX_ + kShipW && b.y >= kShipY && b.y <= kShipY + kShipH) {
            b.active = false;
            --lives_;
            haptics::pulse(100);
            if (lives_ <= 0) {
                endGame();
            }
        }
    }
}

void SpaceInvadersScreen::fireBullet() {
    if (playerBullet_.active) {
        return;
    }
    playerBullet_.x = shipX_ + kShipW / 2.0f;
    playerBullet_.y = kShipY;
    playerBullet_.active = true;
}

bool SpaceInvadersScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void SpaceInvadersScreen::handleInput(uint32_t deltaMs) {
    const auto touch = M5.Touch.getDetail();

    if (touch.wasPressed() && touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    if (gameOver_) {
        if (touch.wasPressed()) {
            resetGame();
        }
        return;
    }

    const int third = M5.Display.width() / 3;

    if (touch.isPressed()) {
        if (touch.x < third) {
            shipX_ -= kShipSpeed * static_cast<float>(deltaMs);
        } else if (touch.x >= 2 * third) {
            shipX_ += kShipSpeed * static_cast<float>(deltaMs);
        }
        if (shipX_ < 4) {
            shipX_ = 4;
        }
        if (shipX_ > M5.Display.width() - kShipW - 4) {
            shipX_ = M5.Display.width() - kShipW - 4;
        }
    }

    if (touch.wasPressed() && touch.x >= third && touch.x < 2 * third) {
        fireBullet();
    }
}

void SpaceInvadersScreen::update(uint32_t deltaMs) {
    handleInput(deltaMs);

    if (gameOver_) {
        draw();
        return;
    }

    if (playtimeTicker_.tick(app_, deltaMs)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    updateAliens(deltaMs);
    if (!gameOver_) {
        updateBullets(deltaMs);
    }

    draw();
}

void SpaceInvadersScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void SpaceInvadersScreen::draw() {
    // Sternenfeld-Weltraum-Hintergrund statt flacher Ein-Farb-Flaeche
    // (Nutzerwunsch: "keine rudimentaeren Darstellungen mehr, optimiere
    // Grafik maximal") - passt thematisch besser zu Aliens als der
    // Synthwave-Look anderer Screens.
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), canvas_.height(), theme::kOutline, theme::kBackground);
    gfxkit::starfield(&canvas_, canvas_.width(), canvas_.height(), 50, theme::kTextDim);
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));

    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    char buf[32];
    snprintf(buf, sizeof(buf), "Punkte: %d  Welle: %d", score_, wave_);
    canvas_.drawString(buf, 4, 4);

    canvas_.setTextDatum(top_right);
    snprintf(buf, sizeof(buf), "Leben: %d", lives_);
    canvas_.drawString(buf, canvas_.width() - kHomeIconSize - 12, 4);

    for (int i = 0; i < kAlienCount; ++i) {
        if (!alienAlive_[i]) {
            continue;
        }
        const int col = i % kAlienCols;
        const int row = i / kAlienCols;
        const int x = static_cast<int>(kFormationBaseX + col * kAlienSpacingX + formationX_);
        const int y = static_cast<int>(kFormationBaseY + row * kAlienSpacingY + formationY_);
        // Kleine Alien-Silhouette (Kopf+"Beine") statt flachem Rechteck.
        canvas_.fillRect(x + 3, y, kAlienW - 6, kAlienH - 4, TFT_GREEN);
        canvas_.fillRect(x, y + kAlienH - 6, kAlienW, 4, TFT_GREEN);
        canvas_.fillRect(x + 1, y + kAlienH - 2, 3, 2, TFT_GREEN);
        canvas_.fillRect(x + kAlienW - 4, y + kAlienH - 2, 3, 2, TFT_GREEN);
        canvas_.fillRect(x + 5, y + 3, 2, 2, theme::kOutline);
        canvas_.fillRect(x + kAlienW - 7, y + 3, 2, 2, theme::kOutline);
    }

    if (playerBullet_.active) {
        canvas_.fillRect(static_cast<int>(playerBullet_.x) - 1, static_cast<int>(playerBullet_.y) - 4, 2, 8,
                          theme::kAccentGold);
    }
    for (const Bullet& b : alienBullets_) {
        if (b.active) {
            canvas_.fillRect(static_cast<int>(b.x) - 1, static_cast<int>(b.y) - 4, 2, 8, TFT_RED);
        }
    }

    // Raumschiff: Rumpf + Cockpit-Glanzlicht + Duesen statt Flat-Rechteck.
    {
        const int sx = static_cast<int>(shipX_);
        gfxkit::bevelPanel(&canvas_, sx, kShipY, kShipW, kShipH, 3, TFT_CYAN, true);
        canvas_.fillTriangle(sx + kShipW / 2, kShipY - 5, sx + kShipW / 2 - 5, kShipY + 2, sx + kShipW / 2 + 5,
                              kShipY + 2, TFT_CYAN);
        canvas_.fillRect(sx + 4, kShipY + kShipH, 3, 3, theme::kAccentOrange);
        canvas_.fillRect(sx + kShipW - 7, kShipY + kShipH, 3, 3, theme::kAccentOrange);
    }

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
