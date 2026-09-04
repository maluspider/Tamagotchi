#pragma once

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Erster Screen nach dem Start (docs/projektplan.md Abschnitt 5).
// Zeigt zunaechst kurz ein 80er-/Synthwave-Neon-Logo ("Henri & Theo",
// Nutzerwunsch), danach initialisiert er die SD-Karte, laedt Profil/
// Fortschritt und leitet zu ProfileSetup (falls noch kein Profil existiert)
// oder Home weiter.
class BootScreen : public Screen {
public:
    BootScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    void drawLogo();
    void drawSdError();

    AppContext& app_;
    StateMachine& stateMachine_;
    bool initDone_ = false;
    uint32_t splashElapsedMs_ = 0;
    // true, solange SD.begin() zuletzt fehlgeschlagen ist - dann wird statt
    // des normalen Ladevorgangs eine gut lesbare Fehlermeldung gezeigt und
    // SD.begin() periodisch automatisch erneut versucht (z. B. falls die
    // Karte neu eingesteckt wird), ohne dass ein manueller Neustart noetig
    // waere.
    bool sdError_ = false;
    uint32_t sdRetryElapsedMs_ = 0;
};
