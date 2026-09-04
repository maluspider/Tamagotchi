#pragma once

#include <Arduino.h>
#include <cstdint>
#include <vector>

// Spaced-Repetition-Zustand (vereinfachtes 5-Box-Leitner-System,
// docs/projektplan.md Abschnitt 8.3) fuer genau ein Fach, persistiert unter
// /progress/aufgaben_<fach>.json (Abschnitt 6/13). Getrennt von
// progress.json, weil diese Datei potenziell viele Items enthaelt und
// unabhaengig je Fach waechst - siehe Abschnitt 6 fuer die Begruendung der
// Datei-Aufteilung.
struct LeitnerItem {
    String id;
    uint8_t box = 1;
    String naechsteWiederholungIso;
};

class SpacedRepetitionStore {
public:
    void load(const char* fach);
    void save() const;

    bool hasItem(const String& id) const;
    bool isDue(const String& id, const String& todayIso) const;
    // Aktuelle Box eines bekannten Items (1, falls unbekannt) - fuer
    // TaskEngine::pickNextTask(), um kuerzlich falsch beantwortete Items
    // (Box 1) unter den faelligen Kandidaten staerker zu gewichten.
    uint8_t boxFor(const String& id) const;

    // Richtig: eine Box aufsteigen (max. 5). Falsch: zurueck auf Box 1.
    // Aktualisiert gleichzeitig die naechste Wiederholung gemaess der
    // Box-Intervalltabelle (Abschnitt 8.3). Legt das Item an, falls es noch
    // nicht bekannt war.
    void recordAnswer(const String& id, bool correct, const String& todayIso);

private:
    LeitnerItem* find(const String& id);

    String fach_;
    std::vector<LeitnerItem> items_;
};
