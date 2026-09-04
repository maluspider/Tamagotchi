#pragma once

#include <cstdint>

// Basisklasse fuer alle Screens (docs/projektplan.md Abschnitt 5: "Jeder
// Screen als eigene Klasse mit update()/draw()"). Die StateMachine haelt
// jeweils genau einen aktiven Screen und ruft dessen Lifecycle-Methoden im
// Hauptloop auf.
class Screen {
public:
    virtual ~Screen() = default;

    // Wird einmalig aufgerufen, wenn der Screen aktiv wird.
    virtual void onEnter() {}

    // Wird einmalig aufgerufen, bevor ein anderer Screen aktiviert wird.
    virtual void onExit() {}

    virtual void update(uint32_t deltaMs) = 0;
    virtual void draw() = 0;
};
