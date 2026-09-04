#pragma once

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Erster Screen nach dem Start (docs/projektplan.md Abschnitt 5).
// Initialisiert die SD-Karte, laedt Profil/Fortschritt und leitet danach zu
// ProfileSetup (falls noch kein Profil existiert) oder Home weiter.
class BootScreen : public Screen {
public:
    BootScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    AppContext& app_;
    StateMachine& stateMachine_;
    bool initDone_ = false;
};
