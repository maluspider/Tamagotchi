#pragma once

#include <cstddef>
#include <cstdint>

// Die vier Multiple-Choice-Faecher der Aufgaben-Engine (docs/projektplan.md
// Abschnitt 8.1). Gedaechtnistraining ist kein Multiple-Choice-Fach und hat
// eine eigene, abweichende Spielmechanik (siehe GedaechtnisScreen) - taucht
// hier bewusst nicht auf.
enum class Subject : uint8_t {
    Mathe = 0,
    Rechtschreibung = 1,
    Franzoesisch = 2,
    Quiz = 3,
};

constexpr size_t kSubjectCount = 4;

// Dateiname-/JSON-Schluessel-Fragment, siehe /tasks/<fach>_<klasse>.json
// (Abschnitt 13) und die "schwierigkeit"-Statistik in progress.json
// (Abschnitt 6).
inline const char* subjectSlug(Subject subject) {
    switch (subject) {
        case Subject::Mathe: return "mathe";
        case Subject::Rechtschreibung: return "rechtschreibung";
        case Subject::Franzoesisch: return "franzoesisch";
        case Subject::Quiz: return "quiz";
    }
    return "mathe";
}
