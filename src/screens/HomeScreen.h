#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Home-Screen (docs/projektplan.md Abschnitt 5): zeigt den
// Tamagotchi-Charakter (in Phase 0 als Platzhalter-Grafik - finale Sprites
// folgen laut Plan Abschnitt 4 spaeter von der SD-Karte), Uhrzeit und
// verfuegbares Spielzeitguthaben.
class HomeScreen : public Screen {
public:
    HomeScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    void drawPlaceholderCharacter() const;
    void drawStatusBar() const;

    AppContext& app_;
    StateMachine& stateMachine_;
    uint32_t msSinceLastRedraw_ = 0;
    bool lowBatterySaveDone_ = false;
};
