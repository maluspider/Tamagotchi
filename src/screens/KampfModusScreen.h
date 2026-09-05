#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/FighterRenderer.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Kampf-Modus (docs/projektplan.md Abschnitt 9/10, ab Stufe "Meister").
// Vereinfachte 1-gegen-1-Variante gegen eine KI (Abschnitt 10, Scope-
// Hinweis: bewusst wenige, klar erkennbare Aktionen statt komplexer
// Combo-Eingaben).
//
// Ueberarbeitet nach Nutzer-Feedback ("der streetfighter game ist ein
// witz...way to basic...will etwas was aussieht wie das gameboy
// streetfighter game...maximal hochwertig mit echten Animationen und
// realistischen Grafiken...nicht mal verstanden wie man es bedient"):
// - Echte Sprite-Kaempfer (FighterRenderer, sdcard/sprites/fighter/, siehe
//   tools/generate_fighter_sprites.py) statt eines Rumpf-Rechtecks + Kreis
//   ohne Gliedmassen. Eine kleine Animations-Zustandsmaschine waehlt pro
//   Kaempfer/Frame eine von 8 Posen (Idle-Atmen im Wechsel, Gehen im
//   Wechsel waehrend Bewegung, Schlag/Tritt-Pose fuer kurze Zeit nach einem
//   Angriff, Treffer-Reaktion, K.o.-Liegepose).
// - Steuerung nicht mehr ueber unsichtbare Touch-Zonen, sondern eine
//   deutlich sichtbare Steuerleiste unterhalb der Buehne: ◀/▶-Tasten zum
//   Bewegen (Halten), "S"/"T"-Tasten fuer Schlag/Tritt (Antippen), dazu
//   weiterhin eine Wisch-Geste ueberall auf dem Bildschirm fuer eine
//   Spezialattacke (mehr Schaden, laengere Abklingzeit). Beim Betreten des
//   Screens erklaert ein kurzes "Wie spielt man?"-Overlay (einmal pro
//   Aufruf, durch Antippen weg) alle Eingaben, bevor die Runde startet.
//
// Die KI-Schwierigkeit ist aktuell fest (skaliert nicht mit dem
// Charakterlevel - siehe Abschnitt 16, offener Punkt). Zeichnet ueber
// M5Canvas wie SnakeScreen.
class KampfModusScreen : public Screen {
public:
    KampfModusScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    static constexpr int kTopBarHeight = 20;
    static constexpr int kGroundY = 190;
    static constexpr float kMoveSpeed = 0.09f; // px/ms
    static constexpr float kAiSpeed = 0.05f;   // px/ms
    static constexpr float kAttackRange = 55.0f;
    static constexpr float kMinSeparation = 34.0f;
    static constexpr int kStartHp = 100;
    static constexpr int kMinSwipeDist = 30;
    static constexpr float kFighterScale = 1.7f;

    // Animations-Timings (siehe Modulkommentar). Anschlag-/Tritt-Posen sind
    // bewusst kuerzer als die jeweilige Abklingzeit (playerAttack()s
    // cooldownMs), damit der Kaempfer sichtbar in die Ausgangsposition
    // zurueckkehrt statt in der Angriffspose einzufrieren.
    static constexpr uint32_t kIdleFrameMs = 500;
    static constexpr uint32_t kWalkFrameMs = 160;
    static constexpr uint32_t kPunchPoseMs = 220;
    static constexpr uint32_t kKickPoseMs = 320;
    static constexpr uint32_t kSpecialPoseMs = 380;
    static constexpr uint32_t kHurtPoseMs = 260;
    static constexpr uint32_t kHitSparkMs = 140;
    static constexpr uint32_t kAiAttackPoseMs = 260;

    void resetRound();
    void handleInput(uint32_t deltaMs);
    void updateAi(uint32_t deltaMs);
    void updateCooldowns(uint32_t deltaMs);
    void updateAnimations(uint32_t deltaMs);
    void playerAttack(int damage, uint32_t cooldownMs, FighterPose pose, uint32_t poseMs);
    void endRoundIfNeeded();

    FighterPose choosePose(bool dead, uint32_t hurtMs, uint32_t attackMs, FighterPose attackPose, bool moving,
                            bool walkToggle) const;
    void drawFighter(float x, bool facingRight, bool isPlayer, uint32_t hurtMs, uint32_t attackMs,
                      FighterPose attackPose, bool moving, bool walkToggle, bool dead);
    void drawHitSpark(int cx, int cy, uint32_t remainingMs, uint32_t totalMs);
    void drawHealthBars();
    void drawButtons();
    void drawHomeIcon();
    void drawInstructionsOverlay();
    bool touchedHomeIcon(int x, int y) const;
    bool touchedButton(int x, int y, int bx, int by, int br) const;

    float playerX_ = 60;
    float aiX_ = 260;
    int playerHp_ = kStartHp;
    int aiHp_ = kStartHp;

    uint32_t playerCooldownMs_ = 0;
    uint32_t aiCooldownMs_ = 0;

    // Angriffs-Pose (Spieler schlaegt/tritt gerade) getrennt von der
    // Treffer-Reaktion (Gegner hat gerade Schaden genommen) - beides kann
    // gleichzeitig fuer unterschiedliche Kaempfer aktiv sein.
    FighterPose playerAttackPose_ = FighterPose::Punch;
    uint32_t playerAttackPoseMs_ = 0;
    uint32_t aiAttackPoseMs_ = 0; // KI zeigt immer Punch-Pose beim eigenen Treffer

    uint32_t playerHurtMs_ = 0;
    uint32_t aiHurtMs_ = 0;
    uint32_t playerHitSparkMs_ = 0;
    uint32_t aiHitSparkMs_ = 0;

    bool playerMoving_ = false;
    uint32_t playerWalkAnimMs_ = 0;
    bool playerWalkToggle_ = false;

    bool aiMoving_ = false;
    uint32_t aiWalkAnimMs_ = 0;
    bool aiWalkToggle_ = false;

    uint32_t idleAnimMs_ = 0;
    bool idleToggle_ = false;

    bool swiping_ = false;
    int touchStartX_ = 0;
    int touchStartY_ = 0;

    bool roundOver_ = false;
    bool playerWon_ = false;

    // Einmal pro Screen-Aufruf gezeigtes Erklaer-Overlay (Nutzerwunsch: "nicht
    // mal verstanden wie man es bedient") - haelt die Runde an, bis
    // angetippt wird.
    bool awaitingStart_ = true;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
    FighterRenderer fighterRenderer_;
};
