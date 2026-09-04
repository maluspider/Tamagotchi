#pragma once

#include <Arduino.h>

// Duenner Wrapper um die im Core2 verbaute RTC (BM8563, ueber M5Unified).
//
// Die RTC ist bewusst die primaere Zeitquelle (funktioniert offline
// zuverlaessig, docs/projektplan.md Abschnitt 7) - syncFromNtpIfAvailable()
// ist ein optionaler Zusatz fuer spaetere Phasen mit WLAN (Review-Hinweis:
// RTC-Drift/CEST-CET-Umstellung ueber Monate hinweg korrigieren, sobald
// WLAN verfuegbar ist - siehe Abschnitt 11).
namespace rtcclock {

// "YYYY-MM-DD" gemaess aktueller RTC-Zeit.
String todayIso();

// Tage seit 1970-01-01 fuer ein "YYYY-MM-DD"-Datum. Einzige Stelle im
// Projekt, die Kalenderarithmetik macht (siehe .cpp) - CharacterEngine und
// PlaytimeAccount nutzen das, statt die Rechnung zu duplizieren.
long epochDayFromIso(const String& isoDate);

long todayEpochDay();

// Versucht, die RTC ueber NTP zu synchronisieren, falls WLAN verbunden ist.
// In Phase 0 ist noch kein WLAN konfiguriert, daher aktuell immer ein
// No-op mit Rueckgabe false - Anknuepfungspunkt fuer Phase 5.
bool syncFromNtpIfAvailable();

} // namespace rtcclock
