#pragma once

#include <cstdint>

// Zentrale Konfigurationswerte, siehe docs/projektplan.md fuer die
// jeweilige Herleitung/Begruendung der Zahlen.
namespace config {

// --- SD-Karte ---
constexpr int kSdChipSelectPin = 4; // Core2: TFCARD_CS_PIN

// --- Dateipfade auf der SD-Karte ---
// Siehe docs/projektplan.md Abschnitt 6/13 fuer die vollstaendige Struktur
// und die Begruendung der Aufteilung.
constexpr const char* kProfilePath = "/profile.json";
constexpr const char* kProgressPath = "/progress.json";

// --- Spielzeit-Oekonomie (Abschnitt 7) ---
constexpr uint16_t kMinutesPerSolvedTask = 2;
constexpr uint16_t kDailyPlaytimeLimitMinutes = 60;

// --- Inaktivitaets-Schwelle, ab der der Charakter "traurig" wirkt
// (Abschnitt 9, Review: einziger Ausloeser fuer "traurig") ---
constexpr uint8_t kSadAfterDaysInactive = 2;

// --- Eltern-PIN (Abschnitt 6, Review: nie im Klartext gespeichert) ---
// Muss ueber die Einstellungen (Phase 4) individuell gesetzt werden, sobald
// diese existieren - siehe docs/projektplan.md Abschnitt 16.
constexpr const char* kDefaultParentalCode = "0000";

// --- Akku-Warnschwellen (Abschnitt 2/15, Review: Low-Battery-Hinweis) ---
constexpr int kLowBatteryWarningPercent = 15;
constexpr int kCriticalBatterySavePercent = 5;

} // namespace config
