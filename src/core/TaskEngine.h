#pragma once

#include <Arduino.h>
#include <cstdint>
#include <vector>

// Aufgaben-Engine, Phase-1-Umfang: ein Fach (Mathe), ohne Spaced
// Repetition/Schwierigkeitsfilter (kommt in Phase 2, Abschnitt 8.3/8.4) -
// waehlt zufaellig (aber nicht zweimal hintereinander dieselbe) eine
// Aufgabe aus dem klassen-passenden Pool. Siehe docs/projektplan.md
// Abschnitt 8.
struct Task {
    String id;
    String frage;
    String antworten[4];
    uint8_t antwortenCount = 0;
    uint8_t richtig = 0; // Index in antworten[]
};

class TaskEngine {
public:
    // Laedt /tasks/mathe_<klasse>.json von der SD-Karte (Abschnitt 13).
    // Rueckgabe false, wenn die Datei fehlt, leer oder nicht lesbar ist.
    bool loadMathePool(uint8_t klasse);

    bool pickRandomTask(Task& out);

    size_t poolSize() const { return pool_.size(); }

private:
    std::vector<Task> pool_;
    int lastIndex_ = -1;
};
