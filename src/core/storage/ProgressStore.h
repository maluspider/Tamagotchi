#pragma once

#include <Arduino.h>
#include <cstdint>

// /progress.json - haeufig veraenderliche Fortschrittsdaten (Charakter,
// Spielzeitkonto). Bewusst getrennt von profile.json (das sich selten
// aendert) und von den Fach-spezifischen Aufgaben-Fortschrittsdateien, die
// mit der Aufgaben-Engine in Phase 1/2 dazukommen - siehe
// docs/projektplan.md Abschnitt 6/13.
//
// "faehigkeiten"/"stufe" aus dem urspruenglichen Datenmodell (Abschnitt 6)
// werden bewusst NICHT persistiert: welche Spiele/Skins frei sind bzw. auf
// welcher Stufe der Charakter steht, ergibt sich deterministisch aus den
// Erfahrungspunkten (Abschnitt 9) und wird bei Bedarf berechnet statt
// dupliziert gespeichert zu werden (vermeidet Drift).
struct ProgressData {
    uint32_t xp = 0;
    String lastCareDateIso;

    uint16_t earnedMinutesToday = 0;
    uint16_t spentMinutesToday = 0;
    String playtimeDateIso;

    bool isValid = false;
};

namespace progressstore {

ProgressData load();
bool save(const ProgressData& data);

} // namespace progressstore
