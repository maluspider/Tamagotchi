#include "KampfModusScreen.h"

#include <M5Unified.h>

#include <cmath>

#include "../core/CharacterTraits.h"
#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/RetroBackdrop.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;

// Steuerleiste unterhalb der Buehne (siehe Modulkommentar in
// KampfModusScreen.h) - vier gleich grosse, gleich weit auseinander
// liegende Tasten statt der frueheren unsichtbaren Touch-Zonen.
constexpr int kControlBtnY = 216;
constexpr int kControlBtnR = 20;
constexpr int kMoveLeftBtnX = 40;
constexpr int kPunchBtnX = 120;
constexpr int kKickBtnX = 200;
constexpr int kMoveRightBtnX = 280;

// Feste Kampf-Gi-/Haut-/Haarfarben: der Spieler behaelt seine in "Aussehen"
// gewaehlten Haut-/Haartoene (persoenlicher Wiedererkennungswert), traegt
// im Kampf-Modus aber einen festen cyanen Gi statt der frei waehlbaren
// Kleidungsfarbe (bessere Lesbarkeit gegen den festen roten KI-Gi). Die KI
// bekommt einen fest zugewiesenen "Rivalen"-Look.
constexpr uint16_t kPlayerGiColor = theme::kAccentCyan;
constexpr uint16_t kAiGiColor = theme::kDanger;
constexpr uint16_t kAiSkinColor = traits::kSkinTones[2].color565;
constexpr uint16_t kAiHairColor = traits::kHairColors[0].color565;
} // namespace

KampfModusScreen::KampfModusScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void KampfModusScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    resetRound();
    awaitingStart_ = true;
}

void KampfModusScreen::resetRound() {
    playerX_ = 60;
    aiX_ = 260;
    playerHp_ = kStartHp;
    aiHp_ = kStartHp;
    playerCooldownMs_ = 0;
    aiCooldownMs_ = 0;
    playerAttackPoseMs_ = 0;
    aiAttackPoseMs_ = 0;
    playerHurtMs_ = 0;
    aiHurtMs_ = 0;
    playerHitSparkMs_ = 0;
    aiHitSparkMs_ = 0;
    playerMoving_ = false;
    aiMoving_ = false;
    playerWalkAnimMs_ = 0;
    aiWalkAnimMs_ = 0;
    playerWalkToggle_ = false;
    aiWalkToggle_ = false;
    idleAnimMs_ = 0;
    idleToggle_ = false;
    roundOver_ = false;
    swiping_ = false;
}

void KampfModusScreen::updateCooldowns(uint32_t deltaMs) {
    playerCooldownMs_ = (playerCooldownMs_ > deltaMs) ? playerCooldownMs_ - deltaMs : 0;
    aiCooldownMs_ = (aiCooldownMs_ > deltaMs) ? aiCooldownMs_ - deltaMs : 0;
    playerAttackPoseMs_ = (playerAttackPoseMs_ > deltaMs) ? playerAttackPoseMs_ - deltaMs : 0;
    aiAttackPoseMs_ = (aiAttackPoseMs_ > deltaMs) ? aiAttackPoseMs_ - deltaMs : 0;
    playerHurtMs_ = (playerHurtMs_ > deltaMs) ? playerHurtMs_ - deltaMs : 0;
    aiHurtMs_ = (aiHurtMs_ > deltaMs) ? aiHurtMs_ - deltaMs : 0;
    playerHitSparkMs_ = (playerHitSparkMs_ > deltaMs) ? playerHitSparkMs_ - deltaMs : 0;
    aiHitSparkMs_ = (aiHitSparkMs_ > deltaMs) ? aiHitSparkMs_ - deltaMs : 0;
}

void KampfModusScreen::updateAnimations(uint32_t deltaMs) {
    idleAnimMs_ += deltaMs;
    if (idleAnimMs_ >= kIdleFrameMs) {
        idleAnimMs_ = 0;
        idleToggle_ = !idleToggle_;
    }

    if (playerMoving_) {
        playerWalkAnimMs_ += deltaMs;
        if (playerWalkAnimMs_ >= kWalkFrameMs) {
            playerWalkAnimMs_ = 0;
            playerWalkToggle_ = !playerWalkToggle_;
        }
    } else {
        playerWalkAnimMs_ = 0;
    }

    if (aiMoving_) {
        aiWalkAnimMs_ += deltaMs;
        if (aiWalkAnimMs_ >= kWalkFrameMs) {
            aiWalkAnimMs_ = 0;
            aiWalkToggle_ = !aiWalkToggle_;
        }
    } else {
        aiWalkAnimMs_ = 0;
    }
}

void KampfModusScreen::playerAttack(int damage, uint32_t cooldownMs, FighterPose pose, uint32_t poseMs) {
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
    playerAttackPose_ = pose;
    playerAttackPoseMs_ = poseMs;
    // Treffer-Reaktion + Hit-Spark auf dem GEGNER (nicht dem Angreifer) -
    // Standard-Fighting-Game-Semantik, siehe Modulkommentar.
    aiHurtMs_ = kHurtPoseMs;
    aiHitSparkMs_ = kHitSparkMs;
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
        aiMoving_ = true;
    } else {
        aiMoving_ = false;
        if (aiCooldownMs_ == 0) {
            playerHp_ -= 10;
            if (playerHp_ < 0) {
                playerHp_ = 0;
            }
            aiCooldownMs_ = 900;
            aiAttackPoseMs_ = kAiAttackPoseMs;
            playerHurtMs_ = kHurtPoseMs;
            playerHitSparkMs_ = kHitSparkMs;
            haptics::pulse(80);
        }
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
        if (awaitingStart_) {
            awaitingStart_ = false;
            return;
        }
        if (roundOver_) {
            resetRound();
            awaitingStart_ = false;
            return;
        }
        if (touchedButton(touch.x, touch.y, kPunchBtnX, kControlBtnY, kControlBtnR)) {
            playerAttack(8, 400, FighterPose::Punch, kPunchPoseMs);
        } else if (touchedButton(touch.x, touch.y, kKickBtnX, kControlBtnY, kControlBtnR)) {
            playerAttack(14, 700, FighterPose::Kick, kKickPoseMs);
        }
        touchStartX_ = touch.x;
        touchStartY_ = touch.y;
        swiping_ = true;
        return;
    }

    if (awaitingStart_) {
        return;
    }

    playerMoving_ = false;
    if (!roundOver_ && touch.isPressed()) {
        if (touchedButton(touch.x, touch.y, kMoveLeftBtnX, kControlBtnY, kControlBtnR)) {
            playerX_ -= kMoveSpeed * static_cast<float>(deltaMs);
            playerMoving_ = true;
        } else if (touchedButton(touch.x, touch.y, kMoveRightBtnX, kControlBtnY, kControlBtnR)) {
            playerX_ += kMoveSpeed * static_cast<float>(deltaMs);
            playerMoving_ = true;
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
                playerAttack(25, 1500, FighterPose::Punch, kSpecialPoseMs); // Spezialattacke
            }
        }
    }
}

void KampfModusScreen::update(uint32_t deltaMs) {
    handleInput(deltaMs);

    if (awaitingStart_) {
        draw();
        return;
    }

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
    updateAnimations(deltaMs);
    endRoundIfNeeded();
    draw();
}

FighterPose KampfModusScreen::choosePose(bool dead, uint32_t hurtMs, uint32_t attackMs, FighterPose attackPose,
                                          bool moving, bool walkToggle) const {
    if (dead) {
        return FighterPose::Ko;
    }
    if (hurtMs > 0) {
        return FighterPose::Hurt;
    }
    if (attackMs > 0) {
        return attackPose;
    }
    if (moving) {
        return walkToggle ? FighterPose::Walk2 : FighterPose::Walk1;
    }
    return idleToggle_ ? FighterPose::Idle2 : FighterPose::Idle1;
}

void KampfModusScreen::drawHitSpark(int cx, int cy, uint32_t remainingMs, uint32_t totalMs) {
    const float frac = totalMs > 0 ? static_cast<float>(remainingMs) / static_cast<float>(totalMs) : 0.0f;
    const int len = 4 + static_cast<int>(10.0f * frac);
    for (int i = 0; i < 6; ++i) {
        const float angle = (static_cast<float>(i) / 6.0f) * 6.2832f;
        const int ex = cx + static_cast<int>(cosf(angle) * len);
        const int ey = cy + static_cast<int>(sinf(angle) * len);
        canvas_.drawLine(cx, cy, ex, ey, theme::kAccentGold);
    }
}

void KampfModusScreen::drawFighter(float x, bool facingRight, bool isPlayer, uint32_t hurtMs, uint32_t attackMs,
                                    FighterPose attackPose, bool moving, bool walkToggle, bool dead) {
    const int cx = static_cast<int>(x);
    // Weicher Bodenschatten - die Sprite-Vorlagen selbst enthalten keinen
    // (siehe tools/generate_fighter_sprites.py: wuerde Halbtransparenz
    // brauchen, die das Markerfarben-Verfahren nicht zulaesst).
    canvas_.fillEllipse(cx, kGroundY + 2, 16, 4, gfxkit::darken(theme::kBackground, 0.5f));

    const FighterPose pose = choosePose(dead, hurtMs, attackMs, attackPose, moving, walkToggle);
    const uint16_t skin = isPlayer
                               ? traits::kSkinTones[app_.profile.skinToneIndex % traits::kSkinToneCount].color565
                               : kAiSkinColor;
    const uint16_t hair = isPlayer
                               ? traits::kHairColors[app_.profile.hairColorIndex % traits::kHairColorCount].color565
                               : kAiHairColor;
    const uint16_t gi = isPlayer ? kPlayerGiColor : kAiGiColor;

    if (!fighterRenderer_.draw(pose, skin, hair, gi, facingRight, cx, kGroundY, kFighterScale, &canvas_)) {
        // Fallback, falls kein Sprite existiert (SD-Karte fehlt oder Karte
        // nicht neu bespielt) - alte Blob-Darstellung statt eines leeren
        // Bildschirms, analog zu CharacterRenderer/HomeScreen.
        const uint16_t bodyColor = hurtMs > 0 ? TFT_WHITE : gi;
        const int headCy = kGroundY - 34;
        gfxkit::bevelPanel(&canvas_, cx - 8, kGroundY - 28, 16, 28, 3, bodyColor, true);
        canvas_.fillCircle(cx, headCy, 10, bodyColor);
        canvas_.drawCircle(cx, headCy, 10, gfxkit::darken(bodyColor, 0.35f));
        const int eyeDx = facingRight ? 3 : -3;
        canvas_.fillCircle(cx + eyeDx, headCy - 2, 2, TFT_BLACK);
    }

    const uint32_t sparkMs = isPlayer ? playerHitSparkMs_ : aiHitSparkMs_;
    if (sparkMs > 0) {
        drawHitSpark(cx, kGroundY - 46, sparkMs, kHitSparkMs);
    }
}

void KampfModusScreen::drawHealthBars() {
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    canvas_.drawString("Du", 4, 24);
    gfxkit::bevelPanel(&canvas_, 4, 36, 120, 12, 3, theme::kOutline, false);
    const int playerFillW = (116 * playerHp_) / 100;
    if (playerFillW > 0) {
        gfxkit::verticalGradient(&canvas_, 6, 38, playerFillW, 8, gfxkit::lighten(theme::kSuccess, 0.3f),
                                  gfxkit::darken(theme::kSuccess, 0.3f));
    }

    canvas_.setTextDatum(top_right);
    canvas_.drawString("Gegner", canvas_.width() - 4, 24);
    gfxkit::bevelPanel(&canvas_, canvas_.width() - 124, 36, 120, 12, 3, theme::kOutline, false);
    const int aiFillW = (116 * aiHp_) / 100;
    if (aiFillW > 0) {
        gfxkit::verticalGradient(&canvas_, canvas_.width() - 122, 38, aiFillW, 8, gfxkit::lighten(theme::kDanger, 0.3f),
                                  gfxkit::darken(theme::kDanger, 0.3f));
    }
}

void KampfModusScreen::drawButtons() {
    // Steuerleiste unterhalb der Buehne statt unsichtbarer Touch-Zonen
    // (Nutzerwunsch: "nicht mal verstanden wie man es bedient") - vier
    // gleich grosse, deutlich beschriftete/erkennbare Tasten.
    gfxkit::bevelPanel(&canvas_, 0, kGroundY, canvas_.width(), canvas_.height() - kGroundY, 0, theme::kOutline,
                        false);

    gfxkit::shinyBall(&canvas_, kMoveLeftBtnX, kControlBtnY, kControlBtnR, theme::kPanelLight);
    canvas_.fillTriangle(kMoveLeftBtnX + 6, kControlBtnY - 8, kMoveLeftBtnX + 6, kControlBtnY + 8,
                          kMoveLeftBtnX - 7, kControlBtnY, theme::kText);

    gfxkit::shinyBall(&canvas_, kPunchBtnX, kControlBtnY, kControlBtnR, theme::kAccentOrange);
    canvas_.setTextColor(TFT_BLACK);
    canvas_.setTextDatum(middle_center);
    canvas_.setTextSize(2);
    canvas_.drawString("S", kPunchBtnX, kControlBtnY);

    gfxkit::shinyBall(&canvas_, kKickBtnX, kControlBtnY, kControlBtnR, theme::kAccentCyan);
    canvas_.drawString("T", kKickBtnX, kControlBtnY);

    gfxkit::shinyBall(&canvas_, kMoveRightBtnX, kControlBtnY, kControlBtnR, theme::kPanelLight);
    canvas_.fillTriangle(kMoveRightBtnX - 6, kControlBtnY - 8, kMoveRightBtnX - 6, kControlBtnY + 8,
                          kMoveRightBtnX + 7, kControlBtnY, theme::kText);
}

void KampfModusScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.fillRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kAccentGold);
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 6, theme::kOutline);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 5, x + 6, y + 14, x + kHomeIconSize - 6, y + 14, theme::kOutline);
    canvas_.fillRect(x + 9, y + 13, kHomeIconSize - 18, kHomeIconSize - 18, theme::kOutline);
}

void KampfModusScreen::drawInstructionsOverlay() {
    const int w = canvas_.width() - 30;
    const int h = 150;
    const int x = 15;
    const int y = (kGroundY - h) / 2 + 6;
    gfxkit::bevelPanel(&canvas_, x, y, w, h, 10, theme::kPanel, true);

    const int cx = canvas_.width() / 2;
    canvas_.setTextDatum(top_center);
    int ty = y + 14;
    canvas_.setTextColor(theme::kText);
    canvas_.setTextSize(2);
    canvas_.drawString("Wie spielt man?", cx, ty);
    ty += 30;
    canvas_.setTextSize(1);
    canvas_.setTextColor(theme::kTextDim);
    canvas_.drawString("<  >  unten = Bewegen (halten)", cx, ty);
    ty += 18;
    canvas_.drawString("S = Schlag      T = Tritt", cx, ty);
    ty += 18;
    canvas_.drawString("Wischen = Spezial-Angriff", cx, ty);
    ty += 28;
    canvas_.setTextSize(2);
    canvas_.setTextColor(theme::kAccentGold);
    canvas_.drawString("Tippen zum Start", cx, ty);
}

void KampfModusScreen::draw() {
    // Nacht-Buehnen-Hintergrund im SNES-/Strassenkaempfer-Look - Mond+
    // Huegelkette+Zuschauer-Silhouette wirken fuer eine Kampf-Arena
    // stimmiger als eine Sonne. Horizont liegt exakt auf kGroundY, sodass
    // die Kaempfer sichtbar auf der "Buehne" stehen.
    gfxkit::verticalGradient(&canvas_, 0, kTopBarHeight, canvas_.width(), kGroundY - kTopBarHeight,
                              gfxkit::darken(theme::kPanel, 0.5f), theme::kOutline);
    canvas_.fillCircle(canvas_.width() / 2, 66, 16, theme::kTextDim);
    canvas_.fillCircle(canvas_.width() / 2 + 6, 61, 14, theme::kOutline);
    gfxkit::hillsSilhouette(&canvas_, canvas_.width(), kGroundY, 34, 5, gfxkit::darken(theme::kPanelLight, 0.45f));
    gfxkit::starfield(&canvas_, canvas_.width(), 10, 36, theme::kMuted, 0, kGroundY - 14);
    // Bodengitter faechert bis zum Bildschirmrand auf, wird von der neuen
    // Steuerleiste (drawButtons(), deckend) unterhalb von kGroundY aber
    // ohnehin ueberdeckt - height bleibt bewusst die volle Canvas-Hoehe
    // (nicht kGroundY), damit die Perspektive bis zur Deckung durch die
    // Leiste korrekt weiterlaeuft statt an der Horizontlinie zu enden.
    retrobackdrop::drawSynthwaveGrid(&canvas_, canvas_.width(), canvas_.height(), kGroundY, false);
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(top_left);
    canvas_.drawString("Kampf-Modus", 4, 4);

    drawHealthBars();

    const bool playerDead = playerHp_ <= 0;
    const bool aiDead = aiHp_ <= 0;
    drawFighter(playerX_, aiX_ > playerX_, true, playerHurtMs_, playerAttackPoseMs_, playerAttackPose_,
                playerMoving_, playerWalkToggle_, playerDead);
    drawFighter(aiX_, playerX_ > aiX_, false, aiHurtMs_, aiAttackPoseMs_, FighterPose::Punch, aiMoving_,
                aiWalkToggle_, aiDead);

    drawButtons();
    drawHomeIcon();

    if (roundOver_) {
        canvas_.setTextDatum(middle_center);
        canvas_.setTextSize(3);
        canvas_.drawString(playerWon_ ? "Gewonnen!" : "Verloren", canvas_.width() / 2, kGroundY / 2 - 10);
        canvas_.setTextSize(2);
        canvas_.drawString("Tippen fuer neue Runde", canvas_.width() / 2, kGroundY / 2 + 22);
    }

    if (awaitingStart_) {
        drawInstructionsOverlay();
    }

    canvas_.pushSprite(0, 0);
}
