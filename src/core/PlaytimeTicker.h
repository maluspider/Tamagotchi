#pragma once

#include <cstdint>

#include "AppContext.h"

// Gemeinsame Spielzeit-Verbrauchslogik fuer alle Mini-Games
// (docs/projektplan.md Abschnitt 7): jede volle Minute Spielzeit kostet
// eine Minute Guthaben. Ein Objekt pro Spiel-Screen, einmal pro update()
// aufgerufen.
//
// Spiele generieren nie EP/Spielzeit (das ist der Aufgaben-Engine
// vorbehalten, Abschnitt 7/9) - sie sind ausschliesslich die
// "Verbrauchsseite" der Spielzeit-Oekonomie.
class PlaytimeTicker {
public:
    // Rueckgabe true, sobald das Spielzeitguthaben in diesem Aufruf
    // aufgebraucht wurde - der Screen muss dann zu Home wechseln.
    // persistProgress() wurde in diesem Fall bereits aufgerufen.
    bool tick(AppContext& app, uint32_t deltaMs);

private:
    uint32_t accumulatorMs_ = 0;
};
