#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Kampf-Modus (docs/projektplan.md Abschnitt 9/10, ab Stufe "Meister").
// Vereinfachte 1-gegen-1-Variante gegen eine KI (Abschnitt 10, Scope-
// Hinweis: bewusst wenige, klar erkennbare Aktionen statt komplexer
// Combo-Eingaben). Steuerung: Touch-Zonen unten links/rechts (unterhalb
// der Buttons) fuer Bewegung, zwei Buttons "S"(chlag)/"T"(ritt), eine
// Swipe-Geste ueberall auf dem Bildschirm fuer eine Spezialattacke (mehr
// Schaden, laengere Abklingzeit). Die KI-Schwierigkeit ist aktuell fest
// (skaliert nicht mit dem Charakterlevel - siehe Abschnitt 16, offener
// Punkt). Zeichnet ueber M5Canvas wie SnakeScreen.
class KampfModusScreen : public Screen {
public:
    KampfModusScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    static constexpr int kTopBarHeight = 20;
    static constexpr int kGroundY = 190;
    static constexpr int kMoveZoneY = 185; // Bewegungszonen erst unterhalb der Buttons (y=150)
    static constexpr float kMoveSpeed = 0.09f; // px/ms
    static constexpr float kAiSpeed = 0.05f;   // px/ms
    static constexpr float kAttackRange = 55.0f;
    static constexpr float kMinSeparation = 30.0f;
    static constexpr int kStartHp = 100;
    static constexpr int kMinSwipeDist = 30;

    void resetRound();
    void handleInput(uint32_t deltaMs);
    void updateAi(uint32_t deltaMs);
    void updateCooldowns(uint32_t deltaMs);
    void playerAttack(int damage, uint32_t cooldownMs);
    void endRoundIfNeeded();

    void drawFighter(float x, bool facingRight, uint16_t color, bool flashing);
    void drawHealthBars();
    void drawButtons();
    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;
    bool touchedButton(int x, int y, int bx, int by, int br) const;

    float playerX_ = 60;
    float aiX_ = 260;
    int playerHp_ = kStartHp;
    int aiHp_ = kStartHp;

    uint32_t playerCooldownMs_ = 0;
    uint32_t aiCooldownMs_ = 0;
    uint32_t playerFlashMs_ = 0; // kurze visuelle Rueckmeldung nach eigenem Treffer
    uint32_t aiFlashMs_ = 0;

    bool swiping_ = false;
    int touchStartX_ = 0;
    int touchStartY_ = 0;

    bool roundOver_ = false;
    bool playerWon_ = false;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
};
