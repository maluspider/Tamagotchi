#pragma once

#include <cstddef>
#include <cstdint>

// Hier die eigenen Kinder eintragen (Name + Alter). Beim allerersten Start
// waehlt das Kind sein eigenes Profil per Antippen aus (siehe
// ProfileSetupScreen) - der Tamagotchi-Charakter traegt danach diesen
// Namen, und die Aufgaben-Schwierigkeit richtet sich ueber klasseForAge()
// nach dem Alter (docs/projektplan.md Abschnitt 8.1). Der Charakter waechst
// wie gehabt mit dem Fortschritt (Erfahrungspunkte, Abschnitt 9) -
// unabhaengig vom hier eingetragenen Alter.
struct KidProfileDefinition {
    const char* name;
    uint8_t age;
};

constexpr KidProfileDefinition kKidProfiles[] = {
    {"Kind 1", 7},
    {"Kind 2", 9},
};

constexpr size_t kKidProfileCount = sizeof(kKidProfiles) / sizeof(kKidProfiles[0]);

// Leitet die Schulstufe (1 oder 3 - aktuell die einzigen Stufen mit
// Aufgaben-Content, Abschnitt 8.1) aus dem Alter her. Schwelle ist ein
// grober Richtwert fuer Baselland (Schuleintritt ca. 6-7 -> 1. Klasse, ca.
// 8-9 -> 3. Klasse). Bei abweichendem Schulweg (z. B. vorzeitige
// Einschulung, Repetition) hier anpassen.
constexpr uint8_t kKlasse3AgeThreshold = 8;

inline uint8_t klasseForAge(uint8_t age) {
    return age >= kKlasse3AgeThreshold ? 3 : 1;
}
