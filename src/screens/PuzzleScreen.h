#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/PlaytimeTicker.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Puzzle (docs/projektplan.md Abschnitt 9/10, ab Stufe "Kind"). Direktes
// Touch-Ziehen von Puzzleteilen (Drag&Drop): Kachel antippen, an eine
// andere Position ziehen, loslassen - beide Kacheln tauschen die Plaetze.
// Ziel: die Kacheln 1-9 in aufsteigender Reihenfolge anordnen. Da es noch
// keine echten Sprite-Assets gibt (Abschnitt 4), sind die "Bildmotive"
// aktuell Platzhalter aus Farbe + Zahl statt Charakter-Grafiken - macht
// das Spiel unabhaengig von echten Bildern trotzdem sinnvoll loesbar.
// Zeichnet ueber M5Canvas wie SnakeScreen (staendiges Neuzeichnen waehrend
// des Ziehens).
class PuzzleScreen : public Screen {
public:
    PuzzleScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    static constexpr int kGridSize = 3;
    static constexpr int kTileCount = kGridSize * kGridSize;
    static constexpr int kTileSize = 70;
    static constexpr int kTopBarHeight = 20;
    static constexpr int kGridOffsetX = (320 - kGridSize * kTileSize) / 2;
    static constexpr int kGridOffsetY = kTopBarHeight + (220 - kGridSize * kTileSize) / 2;

    static const uint16_t kTileColors[kTileCount];

    void resetGame();
    bool isSolvedState() const;
    int slotAt(int x, int y) const;
    void handleInput();
    void drawTile(int colorId, int x, int y, int size);
    void drawHomeIcon();
    bool touchedHomeIcon(int x, int y) const;

    int slotColorId_[kTileCount] = {};
    int draggingSlot_ = -1;
    int dragX_ = 0;
    int dragY_ = 0;
    int moveCount_ = 0;
    bool solved_ = false;

    AppContext& app_;
    StateMachine& stateMachine_;
    M5Canvas canvas_;
    PlaytimeTicker playtimeTicker_;
};
