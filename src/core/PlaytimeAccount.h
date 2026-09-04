#pragma once

#include <Arduino.h>
#include <cstdint>

#include "config.h"

// Spielzeitkonto (docs/projektplan.md Abschnitt 7). "Verdient" richtet
// sich nach richtig geloesten Aufgaben (Aufgaben-Engine, Phase 1/2) plus
// eventueller Eltern-Bonuszeit (Phase 4, SettingsScreen), "verbraucht"
// nach genutzter Spielzeit in den Mini-Games (Phase 3). Tageslimit +
// taeglicher Reset ueber das RTC-Datum (siehe RtcClock) - funktioniert
// dadurch auch komplett offline zuverlaessig.
class PlaytimeAccount {
public:
    void load(uint16_t earnedMinutesToday, uint16_t spentMinutesToday, const String& dateIso);

    // Vor jedem Zugriff aufrufen (typischerweise einmal pro
    // Screen-update): setzt das Konto zurueck, wenn sich das RTC-Datum seit
    // dem letzten Aufruf geaendert hat.
    void rolloverIfNewDay(const String& todayIso);

    void creditTaskReward();          // +config::kMinutesPerSolvedTask, verfaellt am Tagesende
    void grantBonusMinutes(uint16_t minutes); // Eltern-Bonuszeit (Abschnitt 11, SettingsScreen)
    bool spend(uint16_t minutes);     // false, wenn nicht genug Guthaben verfuegbar

    // Individuelles Tageslimit (Abschnitt 11: "Tageslimit-Anpassung") -
    // ueberschreibt config::kDailyPlaytimeLimitMinutes fuer availableMinutes().
    void setDailyLimitMinutes(uint16_t minutes) { dailyLimitMinutes_ = minutes; }
    uint16_t dailyLimitMinutes() const { return dailyLimitMinutes_; }

    uint16_t availableMinutes() const;
    uint16_t earnedMinutesToday() const { return earnedMinutesToday_; }
    uint16_t spentMinutesToday() const { return spentMinutesToday_; }
    const String& dateIso() const { return dateIso_; }

private:
    uint16_t earnedMinutesToday_ = 0;
    uint16_t spentMinutesToday_ = 0;
    String dateIso_;
    uint16_t dailyLimitMinutes_ = config::kDailyPlaytimeLimitMinutes;
};
