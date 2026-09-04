#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Gedaechtnistraining (docs/projektplan.md Abschnitt 8.1): fuer die
// 1. Klasse ein klassisches Karten-Memory ("Bild-Paare merken"), fuer die
// 3. Klasse ein Sequenz-Merkspiel nach Simon-Says-Prinzip ("Sequenzen
// merken"). Beide Varianten sind rein prozedural (Platzhalter-Formen/
// -Farben statt Bild-Assets, Abschnitt 4) und brauchen deshalb keinen SD-
// Karten-Content. Belohnt wird wie im Aufgaben-Modus mit EP + Spielzeit
// (Abschnitt 7/9), aber ohne Spaced-Repetition/Schwierigkeits-Tracking
// (das ist an Fach-Content mit "schwierigkeit"-Feld gebunden, Abschnitt
// 8.2/8.4, hier nicht vorhanden).
//
// Zeichnet wie SnakeScreen ueber ein M5Canvas-Offscreen-Sprite statt
// direkt auf M5.Display, da beide Modi mehrfach pro Sekunde neu zeichnen
// (Aufdeck-Pause bzw. Sequenz-Blinken) und das sonst auf echter Hardware
// flackern wuerde (siehe StateMachine.h).
class GedaechtnisScreen : public Screen {
public:
    GedaechtnisScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    // --- Karten-Memory (1. Klasse) ---
    static constexpr int kCardCols = 4;
    static constexpr int kCardRows = 2;
    static constexpr int kCardCount = kCardCols * kCardRows; // 8 -> 4 Paare
    static constexpr int kTopBarHeight = 24;
    static constexpr uint32_t kMismatchPauseMs = 700;

    void resetMemoryGame();
    void handleMemoryTouch(int x, int y);
    void updateMemory(uint32_t deltaMs);
    void drawMemoryGame();
    int cardIndexAt(int x, int y) const;
    void drawSymbol(int symbol, int cx, int cy, int r);

    uint8_t cardSymbol_[kCardCount] = {};
    bool cardRevealed_[kCardCount] = {};
    bool cardMatched_[kCardCount] = {};
    int firstPick_ = -1;
    int secondPick_ = -1;
    uint32_t pauseRemainingMs_ = 0;

    // --- Sequenz-Merkspiel (3. Klasse) ---
    static constexpr int kMaxSequence = 32;
    static constexpr uint32_t kSequenceStepMs = 600;
    static constexpr uint32_t kSequenceOnMs = 350;

    enum class SequencePhase { Playback, WaitingForInput };

    void resetSequenceGame();
    void handleSequenceTouch(int x, int y);
    void updateSequence(uint32_t deltaMs);
    void drawSequenceGame();
    int sequenceZoneAt(int x, int y) const;

    uint8_t sequence_[kMaxSequence] = {};
    int sequenceLength_ = 0;
    int inputIndex_ = 0;
    uint32_t playbackTimerMs_ = 0;
    int highlightedZone_ = -1;
    SequencePhase sequencePhase_ = SequencePhase::Playback;

    // --- gemeinsam ---
    void awardRoundReward();
    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;
    void handleGameOverTouch();

    bool gameOver_ = false;
    int score_ = 0;
    bool isMemoryMode_ = true;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
};
