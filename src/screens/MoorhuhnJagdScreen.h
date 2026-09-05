#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Moorhuhn-Jagd / Shooting Gallery (docs/projektplan.md Abschnitt 9/10, ab
// Stufe "Junior"). Steuerung: Neigungssensor (IMU) bewegt ein Fadenkreuz
// (absolute Zuordnung Neigungswinkel -> Position, sanft geglaettet gegen
// Zittern - kein driftanfaelliges Aufintegrieren von Geschwindigkeit),
// Antippen des Bildschirms loest den Schuss aus. Ziele bewegen sich
// horizontal in festen Bahnen ("Skript-Pfaden") und werden nach einem
// Treffer an neuer Position/mit hoeherer Geschwindigkeit neu platziert.
//
// Nutzerwunsch "bei Moorhuhn 60 Sekunden Timer einbauen...wieviele
// Abschuesse in dieser Zeit inkl. Highscore": statt eines unbegrenzten
// Score-Zaehlers (nur durch das Spielzeitguthaben begrenzt) laeuft jetzt
// eine feste 60-Sekunden-Runde - score_ zaehlt die Treffer ("Abschuesse")
// innerhalb dieser Runde, am Ende ("Zeit abgelaufen") wird der
// Rundenendstand als Highscore gespeichert (HighscoreStore, wie bei den
// anderen Spielen) und per Antippen eine neue Runde gestartet. Zeichnet
// ueber M5Canvas wie SnakeScreen.
class MoorhuhnJagdScreen : public Screen {
public:
    MoorhuhnJagdScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    struct Target {
        float x = 0;
        float y = 0;
        float vx = 0;
    };

    static constexpr int kTopBarHeight = 20;
    static constexpr int kTargetCount = 4;
    static constexpr float kTargetRadius = 13.0f;
    static constexpr float kHitRadius = 24.0f;
    static constexpr float kTiltRangeX = 180.0f;
    static constexpr float kTiltRangeY = 130.0f;
    static constexpr float kSmoothing = 0.18f;
    static constexpr uint32_t kRoundDurationMs = 60000;

    void resetGame();
    void respawnTarget(Target& target);
    void updateTargets(uint32_t deltaMs);
    void updateCrosshair(uint32_t deltaMs);
    void handleInput();
    void finishSession();
    void endRound();

    void drawTarget(const Target& target);
    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;

    Target targets_[kTargetCount];
    float crosshairX_ = 0;
    float crosshairY_ = 0;

    int score_ = 0;
    uint32_t roundElapsedMs_ = 0;
    bool roundOver_ = false;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
};
