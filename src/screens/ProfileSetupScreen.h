#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Erststart-Einrichtung: Klassenstufe (1./3. Klasse) auswaehlen.
//
// Review-Anforderung (UX-Luecke, docs/projektplan.md Abschnitt 5): Ein
// Erstklaessler kann noch nicht zuverlaessig lesen - deshalb bewusst KEIN
// Text-Menue, sondern zwei grosse Touch-Flaechen mit Ziffern-Icons statt
// Wort-Labels.
//
// Geraete-Profil ist Laufzeit-Konfiguration (eine gemeinsame Firmware fuer
// beide Geraete, Abschnitt 3) - die Auswahl hier legt profile.json einmalig
// an.
class ProfileSetupScreen : public Screen {
public:
    ProfileSetupScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    void drawChoice(int x, int w, uint8_t klasse, uint16_t color) const;
    void commitSelection(uint8_t klasse);

    AppContext& app_;
    StateMachine& stateMachine_;
};
