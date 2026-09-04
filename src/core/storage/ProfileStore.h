#pragma once

#include <Arduino.h>
#include <cstdint>

#include "../PinCode.h"

// /profile.json - Geraete-/Kindprofil, aendert sich selten (siehe
// docs/projektplan.md Abschnitt 6, Review-Anmerkung zur Aufteilung
// zwischen profile.json und progress.json).
struct Profile {
    String name;
    uint8_t klasse = 1; // 1 oder 3
    String geraetId;
    pincode::Digest guard; // Eltern-PIN, gehasht (siehe PinCode.h)

    bool isValid = false; // false = noch kein Profil auf der SD-Karte angelegt
};

namespace profilestore {

Profile load();
bool save(const Profile& profile);

} // namespace profilestore
