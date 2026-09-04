#pragma once

#include <Arduino.h>
#include <cstdint>
#include <vector>

#include "DifficultyTracker.h"
#include "SpacedRepetitionStore.h"
#include "Subject.h"

// Aufgaben-Engine (docs/projektplan.md Abschnitt 8): laedt den
// klassen-passenden Aufgabenpool eines Fachs von der SD-Karte und waehlt
// die naechste Aufgabe unter Beruecksichtigung von Spaced Repetition
// (Abschnitt 8.3: faellige Items zuerst, dazu 1-2 neue gemischt) und der
// aktuellen Schwierigkeitsstufe des Fachs (Abschnitt 8.4: Filter auf
// `schwierigkeit <= stage`).
struct Task {
    String id;
    String frage;
    String antworten[4];
    uint8_t antwortenCount = 0;
    uint8_t richtig = 0; // Index in antworten[]
    uint8_t schwierigkeit = 1;
};

class TaskEngine {
public:
    // Laedt /tasks/<fach>_<klasse>.json von der SD-Karte (Abschnitt 13).
    // Rueckgabe false, wenn die Datei fehlt, leer oder nicht lesbar ist.
    bool loadPool(Subject subject, uint8_t klasse);

    // Waehlt die naechste Aufgabe: faellige, bereits bekannte Items zuerst,
    // dazu bis zu zwei neue Items gemischt (Abschnitt 8.3), gefiltert auf
    // die aktuelle Schwierigkeitsstufe (Abschnitt 8.4). Faellt auf den
    // gesamten Pool zurueck, falls der gefilterte Kandidatenkreis leer ist.
    bool pickNextTask(Task& out, const SpacedRepetitionStore& srs, const DifficultyState& difficulty,
                       const String& todayIso);

    size_t poolSize() const { return pool_.size(); }

private:
    std::vector<Task> pool_;
    int lastIndex_ = -1;
};
