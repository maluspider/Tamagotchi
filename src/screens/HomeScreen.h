#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Home-Screen (docs/projektplan.md Abschnitt 5): zeigt den
// Tamagotchi-Charakter als echtes Pixel-Art-Sprite von der SD-Karte
// (sdcard/sprites/character/, siehe tools/generate_sprites.py und
// Abschnitt 4) mitsamt seinem Namen (aus dem gewaehlten Kind-Profil,
// include/KidProfiles.h), Uhrzeit und verfuegbares Spielzeitguthaben.
// Fehlt die SD-Karte oder eine Sprite-Datei, zeichnet
// drawPlaceholderCharacter() automatisch die alte Platzhalter-Grafik als
// Fallback - das Geraet bleibt so auch ohne (vollstaendige) Sprite-Assets
// benutzbar. Die untere Icon-Leiste fuehrt zu Aufgaben-Modus/Spiele/Uhr
// (Abschnitt 5, Phase-1-Umfang).
class HomeScreen : public Screen {
public:
    HomeScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    bool drawSpriteCharacter();
    void drawPlaceholderCharacter() const;
    void drawStatusBar() const;
    void drawBottomBar() const;
    void handleBottomBarTouch(int x, int y);

    AppContext& app_;
    StateMachine& stateMachine_;
    uint32_t msSinceLastRedraw_ = 0;
    bool lowBatterySaveDone_ = false;
    bool spriteBlinkToggle_ = false; // wechselt idle1/idle2 pro Redraw-Tick
};
