#include "KampfModusScreen.h"

#include <M5Unified.h>

#include <cmath>

#include "../core/Haptics.h"
#include "../core/RetroBackdrop.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr int kPunchBtnX = 95;
constexpr int kPunchBtnY = 150;
constexpr int kPunchBtnR = 24;
constexpr int kKickBtnX = 225;
constexpr int kKickBtnY = 150;
constexpr int kKickBtnR = 24;
} // namespace

KampfModusScreen::KampfModusScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void KampfModusScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    resetRound();
}

void KampfModusScreen::resetRound() {
    playerX_ = 60;
    aiX_ = 260;
    playerHp_ = kStartHp;
    aiHp_ = kStartHp;
    playerCooldownMs_ = 0;
    aiCooldownMs_ = 0;
    playerFlashMs_ = 0;
    aiFlashMs_ = 0;
    roundOver_ = false;
    swiping_ = false;
}

void KampfModusScreen::updateCooldowns(uint32_t deltaMs) {
    playerCooldownMs_ = (playerCooldownMs_ > deltaMs) ? playerCooldownMs_ - deltaMs : 0;
    aiCooldownMs_ = (aiCooldownMs_ > deltaMs) ? aiCooldownMs_ - deltaMs : 0;
    playerFlashMs_ = (playerFlashMs_ > deltaMs) ? playerFlashMs_ - deltaMs : 0;
    aiFlashMs_ = (aiFlashMs_ > deltaMs) ? aiFlashMs_ - deltaMs : 0;
}

void KampfModusScreen::playerAttack(int damage, uint32_t cooldownMs) {
    if (playerCooldownMs_ > 0) {
        return;
    }
    if (fabsf(aiX_ - playerX_) > kAttackRange) {
        return;
    }
    aiHp_ -= damage;
    if (aiHp_ < 0) {
        aiHp_ = 0;
    }
    playerCooldownMs_ = cooldownMs;
    playerFlashMs_ = 150;
    haptics::pulse(40);
}

void KampfModusScreen::updateAi(uint32_t deltaMs) {
    const float dist = aiX_ - playerX_; // positiv: KI rechts vom Spieler
    const float absDist = fabsf(dist);

    if (absDist > kAttackRange) {
        aiX_ += (dist > 0 ? -1.0f : 1.0f) * kAiSpeed * static_cast<float>(deltaMs);
        if (aiX_ < 10) {
            aiX_ = 10;
        }
        if (aiX_ > M5.Display.width() - 10) {
            aiX_ = M5.Display.width() - 10;
        }
    } else if (aiCooldownMs_ == 0) {
        playerHp_ -= 10;
        if (playerHp_ < 0) {
            playerHp_ = 0;
        }
        aiCooldownMs_ = 900;
        aiFlashMs_ = 150;
        haptics::pulse(80);
    }
}

void KampfModusScreen::endRoundIfNeeded() {
    if (playerHp_ <= 0) {
        roundOver_ = true;
        playerWon_ = false;
        haptics::pulse(200);
    } else if (aiHp_ <= 0) {
        roundOver_ = true;
        playerWon_ = true;
        haptics::pulse(150);
    }
}

bool KampfModusScreen::touchedButton(int x, int y, int bx, int by, int br) const {
    const int dx = x - bx;
    const int dy = y - by;
    return dx * dx + dy * dy <= br * br;
}

bool KampfModusScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void KampfModusScreen::handleInput(uint32_t deltaMs) {
    const auto touch = M5.Touch.getDetail();

    if (touch.wasPressed()) {
        if (touchedHomeIcon(touch.x, touch.y)) {
            stateMachine_.requestSwitch(ScreenId::Home);
            return;
        }
        if (roundOver_) {
            resetRound();
            return;
        }
        if (touchedButton(touch.x, touch.y, kPunchBtnX, kPunchBtnY, kPunchBtnR)) {
            playerAttack(8, 400);
        } else if (touchedButton(touch.x, touch.y, kKickBtnX, kKickBtnY, kKickBtnR)) {
            playerAttack(14, 700);
        }
        touchStartX_ = touch.x;
        touchStartY_ = touch.y;
        swiping_ = true;
        return;
    }

    if (!roundOver_ && touch.isPressed() && touch.y >= kMoveZoneY) {
        const int third = M5.Display.width() / 3;
        if (touch.x < third) {
            playerX_ -= kMoveSpeed * static_cast<float>(deltaMs);
        } else if (touch.x >= 2 * third) {
            playerX_ += kMoveSpeed * static_cast<float>(deltaMs);
        }
        if (playerX_ < 10) {
            playerX_ = 10;
        }
        if (playerX_ > M5.Display.width() - 10) {
            playerX_ = M5.Display.width() - 10;
        }
        if (fabsf(playerX_ - aiX_) < kMinSeparation) {
            playerX_ = (playerX_ < aiX_) ? aiX_ - kMinSeparation : aiX_ + kMinSeparation;
        }
    }

    if (touch.wasReleased() && swiping_) {
        swiping_ = false;
        if (!roundOver_) {
            const int dx = touch.x - touchStartX_;
            const int dy = touch.y - touchStartY_;
            if (dx * dx + dy * dy > kMinSwipeDist * kMinSwipeDist) {
                playerAttack(25, 1500); // Spezialattacke
            }
        }
    }
}

void KampfModusScreen::update(uint32_t deltaMs) {
    handleInput(deltaMs);

    if (roundOver_) {
        draw();
        return;
    }

    if (playtimeTicker_.tick(app_, deltaMs)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    updateCooldowns(deltaMs);
    updateAi(deltaMs);
    endRoundIfNeeded();
    draw();
}

void KampfModusScreen::drawFighter(float x, bool facingRight, uint16_t color, bool flashing) {
    const int cx = static_cast<int>(x);
    const int headCy = kGroundY - 34;
    const uint16_t bodyColor = flashing ? TFT_WHITE : color;
    canvas_.fillRect(cx - 8, kGroundY - 28, 16, 28, bodyColor);
    canvas_.fillCircle(cx, headCy, 10, bodyColor);
    const int eyeDx = facingRight ? 3 : -3;
    canvas_.fillCircle(cx + eyeDx, headCy - 2, 2, TFT_BLACK);
}

void KampfModusScreen::drawHealthBars() {
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    canvas_.drawString("Du", 4, 24);
    canvas_.drawRect(4, 36, 120, 12, TFT_WHITE);
    canvas_.fillRect(5, 37, (118 * playerHp_) / 100, 10, TFT_GREEN);

    canvas_.setTextDatum(top_right);
    canvas_.drawString("Gegner", canvas_.width() - 4, 24);
    canvas_.drawRect(canvas_.width() - 124, 36, 120, 12, TFT_WHITE);
    canvas_.fillRect(canvas_.width() - 123, 37, (118 * aiHp_) / 100, 10, TFT_RED);
}

void KampfModusScreen::drawButtons() {
    canvas_.fillCircle(kPunchBtnX, kPunchBtnY, kPunchBtnR, TFT_ORANGE);
    canvas_.drawCircle(kPunchBtnX, kPunchBtnY, kPunchBtnR, TFT_WHITE);
    canvas_.setTextColor(TFT_BLACK);
    canvas_.setTextDatum(middle_center);
    canvas_.setTextSize(2);
    canvas_.drawString("S", kPunchBtnX, kPunchBtnY);

    canvas_.fillCircle(kKickBtnX, kKickBtnY, kKickBtnR, TFT_CYAN);
    canvas_.drawCircle(kKickBtnX, kKickBtnY, kKickBtnR, TFT_WHITE);
    canvas_.drawString("T", kKickBtnX, kKickBtnY);
}

void KampfModusScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void KampfModusScreen::draw() {
    canvas_.fillScreen(theme::kBackground);
    // Arena-Hintergrund im SNES-/Strassenkaempfer-Look (Nutzerwunsch:
    // "Backgrounds Super-Nintendo-Stil") statt der schlichten grauen
    // Boden-Trennlinie - Horizont liegt exakt auf kGroundY, sodass die
    // Kaempfer sichtbar auf der "Buehne" stehen.
    retrobackdrop::drawSynthwaveGrid(&canvas_, canvas_.width(), canvas_.height(), kGroundY);
    canvas_.fillRect(0, 0, canvas_.width(), kTopBarHeight, theme::kPanel);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    canvas_.drawString("Kampf-Modus", 4, 4);

    drawHealthBars();
    drawFighter(playerX_, aiX_ > playerX_, TFT_CYAN, playerFlashMs_ > 0);
    drawFighter(aiX_, playerX_ > aiX_, TFT_RED, aiFlashMs_ > 0);

    drawButtons();
    drawHomeIcon();

    if (roundOver_) {
        canvas_.setTextDatum(middle_center);
        canvas_.setTextSize(3);
        canvas_.drawString(playerWon_ ? "Gewonnen!" : "Verloren", canvas_.width() / 2, canvas_.height() / 2 - 15);
        canvas_.setTextSize(2);
        canvas_.drawString("Tippen fuer neue Runde", canvas_.width() / 2, canvas_.height() / 2 + 20);
    }

    canvas_.pushSprite(0, 0);
}
