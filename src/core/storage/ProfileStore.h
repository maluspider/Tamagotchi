#pragma once

#include <Arduino.h>
#include <cstdint>

#include "../PinCode.h"

// /profile.json - Geraete-/Kindprofil, aendert sich selten (siehe
// docs/projektplan.md Abschnitt 6, Review-Anmerkung zur Aufteilung
// zwischen profile.json und progress.json).
struct Profile {
    String name;
    uint8_t age = 0;
    uint8_t klasse = 1; // 1 oder 3, siehe include/KidProfiles.h::klasseForAge()
    String geraetId;
    pincode::Digest guard; // Eltern-PIN, gehasht (siehe PinCode.h)

    // Wecker (Abschnitt 11) - eine einzelne Alarmzeit pro Geraet, in
    // profile.json statt progress.json (aendert sich selten, siehe
    // Abschnitt 6).
    bool alarmEnabled = false;
    uint8_t alarmHour = 7;
    uint8_t alarmMinute = 0;

    bool isValid = false; // false = noch kein Profil auf der SD-Karte angelegt
};

namespace profilestore {

Profile load();
bool save(const Profile& profile);

} // namespace profilestore
