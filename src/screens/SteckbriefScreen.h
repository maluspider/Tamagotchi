#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Charakter-Steckbrief (docs/projektplan.md Abschnitt 11: "Uebersicht
// ueber Stufe, Faehigkeiten, Statistiken ('mein Tamagotchi')"). Rein
// lesende Uebersicht, kein Input ausser dem Home-Icon.
class SteckbriefScreen : public Screen {
public:
    SteckbriefScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
};
