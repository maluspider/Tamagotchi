#pragma once

#include <Arduino.h>
#include <cstdint>

// Der Eltern-PIN wird nie im Klartext gespeichert (SD-Karten sind
// entnehmbar - ein Kind koennte sie sonst an einem PC auslesen). Stattdessen:
// ein zufaelliger Salt + ein iterierter Streuwert aus salt+pin.
//
// Das ist bewusst KEINE kryptografisch sichere Loesung (fuer einen
// 4-stelligen Eltern-PIN auch nicht das Ziel), sondern genug Obskuritaet,
// dass ein Kind nicht einfach die Profildatei oeffnet und den PIN
// ausliest. Siehe docs/projektplan.md Abschnitt 6 (Review).
namespace pincode {

struct Digest {
    uint32_t salt = 0;
    uint32_t value = 0;
};

Digest hash(const String& code);
bool verify(const String& code, const Digest& stored);

} // namespace pincode
