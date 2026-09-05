#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/CharacterRenderer.h"
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
//
// Zeichnet ueber ein M5Canvas-Offscreen-Sprite wie SnakeScreen (Abschnitt 3)
// statt direkt auf M5.Display: der Screen redraws 1x/Sekunde fuer die
// Uhrzeit, und mehrere aufeinanderfolgende Direkt-Display-Aufrufe
// (Hintergrund loeschen, dann Charakter, dann Status-/Bottom-Bar) waren auf
// echter Hardware als sichtbares Flackern erkennbar - das Offscreen-Canvas
// wird komplett fertig gezeichnet und dann in einem Rutsch angezeigt.
class HomeScreen : public Screen {
public:
    HomeScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    bool drawSpriteCharacter();
    void drawPlaceholderCharacter();
    void drawStatusBar();
    void drawBatteryIndicator(int x, int y);
    void drawBottomBar();
    void drawSleepingIndicator();
    void handleBottomBarTouch(int x, int y);
    void updateCharacterDrift(uint32_t deltaMs);

    AppContext& app_;
    StateMachine& stateMachine_;
    CharacterRenderer characterRenderer_;
    M5Canvas canvas_;
    uint32_t msSinceLastRedraw_ = 0;
    bool lowBatterySaveDone_ = false;
    bool spriteBlinkToggle_ = false; // wechselt idle1/idle2 pro Redraw-Tick
    // Nutzerwunsch: "Figur soll sich durch Bewegen des Geraets langsam
    // smooth bewegen lassen" - per IMU/Neigung sanft Richtung Zielposition
    // gezogener Versatz relativ zur Bildschirmmitte (siehe
    // updateCharacterDrift()).
    float characterOffsetX_ = 0.0f;
    float characterOffsetY_ = 0.0f;
};
