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

// --- XP pro richtig geloester Aufgabe (Abschnitt 9) ---
// Platzhalterwert, wie die Stufenschwellen in CharacterEngine.cpp - siehe
// Abschnitt 16 "offene Punkte" fuer die finale Balance.
constexpr uint32_t kXpPerCorrectAnswer = 15;

// --- Tages-/Routine-Checkliste (Abschnitt 11) ---
// "gibt kleine EP-Belohnung" - kleiner als kXpPerCorrectAnswer, da hier
// nur Abhaken statt tatsaechlichem Lernen stattfindet.
constexpr uint32_t kChecklistRewardXp = 8;

// --- Schwierigkeitsanstieg (Abschnitt 8.4) ---
// Maximale Schwierigkeitsstufe, bis zu der der monatliche Auto-Anstieg die
// Obergrenze je Fach anheben darf. Platzhalterwert wie die uebrigen
// Balance-Zahlen - siehe Abschnitt 16.
constexpr uint8_t kMaxDifficultyStage = 5;

// --- Eltern-PIN (Abschnitt 6, Review: nie im Klartext gespeichert) ---
// Muss ueber die Einstellungen (Phase 4) individuell gesetzt werden, sobald
// diese existieren - siehe docs/projektplan.md Abschnitt 16.
constexpr const char* kDefaultParentalCode = "0000";

// --- Akku-Warnschwellen (Abschnitt 2/15, Review: Low-Battery-Hinweis) ---
constexpr int kLowBatteryWarningPercent = 15;
constexpr int kCriticalBatterySavePercent = 5;

// --- Web-Sync / OTA (Abschnitt 12, Phase 5) ---
// WPA2-Passwort fuer den geraeteeigenen Access Point, der nur waehrend
// WebSyncScreen aktiv ist (Abschnitt 12: "ESP32 im Access-Point-Modus").
// Muss mindestens 8 Zeichen haben (WPA2-Minimum). Vor "produktivem"
// Einsatz anpassen, siehe Abschnitt 16.
constexpr const char* kWebSyncApPassword = "tamagotchi123";
constexpr const char* kOtaHostname = "tamagotchi";

} // namespace config
