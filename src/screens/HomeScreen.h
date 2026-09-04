#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Home-Screen (docs/projektplan.md Abschnitt 5): zeigt den
// Tamagotchi-Charakter (in Phase 0 als Platzhalter-Grafik - finale Sprites
// folgen laut Plan Abschnitt 4 spaeter von der SD-Karte) mitsamt seinem
// Namen (aus dem gewaehlten Kind-Profil, include/KidProfiles.h), Uhrzeit
// und verfuegbares Spielzeitguthaben. Die untere Icon-Leiste fuehrt zu
// Aufgaben-Modus/Spiele/Uhr (Abschnitt 5, Phase-1-Umfang).
class HomeScreen : public Screen {
public:
    HomeScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    void drawPlaceholderCharacter() const;
    void drawStatusBar() const;
    void drawBottomBar() const;
    void handleBottomBarTouch(int x, int y);

    AppContext& app_;
    StateMachine& stateMachine_;
    uint32_t msSinceLastRedraw_ = 0;
    bool lowBatterySaveDone_ = false;
};
