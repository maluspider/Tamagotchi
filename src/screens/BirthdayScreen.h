#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Geburtstags-Countdown (docs/projektplan.md Abschnitt 11): zeigt die Tage
// bis zum naechsten Geburtstag des aktiven Profils, aus dem in
// include/KidProfiles.h hinterlegten Geburtstag (Monat/Tag, kein Jahr
// noetig - der Countdown zaehlt nur bis zum naechsten Jahrestag). Ist am
// Profil kein Geburtstag hinterlegt (0/0, z. B. bei einem vor dieser
// Funktion angelegten Profil), zeigt der Screen stattdessen einen Hinweis
// statt eines falschen Countdowns.
class BirthdayScreen : public Screen {
public:
    BirthdayScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    bool touchedHomeIcon(int x, int y) const;
    void drawHomeIcon() const;
    void drawCake(int cx, int cy) const;
    long daysUntilBirthday() const;

    AppContext& app_;
    StateMachine& stateMachine_;
};
