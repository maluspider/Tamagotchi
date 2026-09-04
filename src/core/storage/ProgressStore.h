#pragma once

#include <Arduino.h>
#include <cstdint>

#include "../DifficultyTracker.h"
#include "../Subject.h"

// /progress.json - haeufig veraenderliche Fortschrittsdaten (Charakter,
// Spielzeitkonto, Schwierigkeitsstufen je Fach). Bewusst getrennt von
// profile.json (das sich selten aendert) und von den Fach-spezifischen
// Spaced-Repetition-Dateien (SpacedRepetitionStore) - siehe
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

    // Schwierigkeitsstufen je Fach (Abschnitt 8.4). Nur `stage`, `ceiling`
    // und `lastCeilingBumpMonthIso` werden persistiert - die rollierende
    // Trefferquote (DifficultyState::recent/...) bleibt bewusst
    // In-Memory-only, siehe DifficultyTracker.h.
    DifficultyState difficulty[kSubjectCount];

    bool isValid = false;
};

namespace progressstore {

ProgressData load();
bool save(const ProgressData& data);

} // namespace progressstore
