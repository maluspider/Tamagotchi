#pragma once

#include <Arduino.h>
#include <cstdint>

// Schwierigkeitsanstieg nach Trefferquote UND Zeit (docs/projektplan.md
// Abschnitt 8.4).
//
// Review-Praezedenz: Der monatliche Auto-Anstieg hebt ausschliesslich die
// Obergrenze `ceiling`. Die tatsaechlich genutzte Stufe `stage` (die beim
// Aufgaben-Filtern verwendet wird) folgt ausschliesslich der rollierenden
// Trefferquote der letzten 10 Antworten und kann dadurch nie ueber eine
// wegen schwacher Leistung gedrueckte Stufe "hinweggehievt" werden - sie
// kann hoechstens bis zur (jetzt hoeheren) Obergrenze steigen, wenn die
// Trefferquote das hergibt.
//
// `recent`/`recentCount`/`recentIndex` (die rollierende Trefferquote) sind
// bewusst NICHT Teil des persistierten Zustands (siehe ProgressStore) -
// nach einem Neustart beginnt das Fenster wieder leer. Das ist eine
// bewusste Vereinfachung: fuer ein Kindergeraet, das selten mitten am Tag
// neu startet, ist das unkritisch und spart eine unhandliche
// Array-Serialisierung.
struct DifficultyState {
    uint8_t stage = 1;
    uint8_t ceiling = 1;
    bool recent[10] = {};
    uint8_t recentCount = 0;
    uint8_t recentIndex = 0;
    String lastCeilingBumpMonthIso; // "YYYY-MM"
};

namespace difficulty {

float rollingHitRate(const DifficultyState& state);

// Verbucht eine Antwort in der rollierenden Trefferquote und passt `stage`
// bei Bedarf an (>= 85% -> +1 bis zur Obergrenze, < 50% -> -1 bis min. 1).
void recordAnswer(DifficultyState& state, bool correct);

// Hebt die Obergrenze um 1 an, aber hoechstens einmal pro Kalendermonat und
// nie ueber maxCeiling hinaus.
void applyMonthlyCeilingBump(DifficultyState& state, const String& currentMonthIso, uint8_t maxCeiling);

} // namespace difficulty
