#pragma once

#include <cstddef>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Erststart-Einrichtung: das Kind waehlt sein eigenes, im Code hinterlegtes
// Profil (include/KidProfiles.h: Name + Alter) per Antippen aus. Der
// Tamagotchi-Charakter traegt danach diesen Namen, und die
// Aufgaben-Schwierigkeit richtet sich ueber die Klassenstufe nach dem
// Alter (Abschnitt 8.1). Geraete-Profil ist Laufzeit-Konfiguration (eine
// gemeinsame Firmware fuer beide Geraete, Abschnitt 3) - die Auswahl hier
// legt profile.json einmalig an.
class ProfileSetupScreen : public Screen {
public:
    ProfileSetupScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    void drawChoice(int x, int w, size_t profileIndex) const;
    void commitSelection(size_t profileIndex);

    AppContext& app_;
    StateMachine& stateMachine_;
};
