#pragma once

#include <Arduino.h>
#include <cstdint>

// Spielzeitkonto (docs/projektplan.md Abschnitt 7). "Verdient" richtet
// sich nach richtig geloesten Aufgaben (Aufgaben-Engine kommt in Phase 1),
// "verbraucht" nach genutzter Spielzeit in den Mini-Games (Phase 3).
// Tageslimit + taeglicher Reset ueber das RTC-Datum (siehe RtcClock) -
// funktioniert dadurch auch komplett offline zuverlaessig.
class PlaytimeAccount {
public:
    void load(uint16_t earnedMinutesToday, uint16_t spentMinutesToday, const String& dateIso);

    // Vor jedem Zugriff aufrufen (typischerweise einmal pro
    // Screen-update): setzt das Konto zurueck, wenn sich das RTC-Datum seit
    // dem letzten Aufruf geaendert hat.
    void rolloverIfNewDay(const String& todayIso);

    void creditTaskReward();      // +config::kMinutesPerSolvedTask, verfaellt am Tagesende
    bool spend(uint16_t minutes); // false, wenn nicht genug Guthaben verfuegbar

    uint16_t availableMinutes() const;
    uint16_t earnedMinutesToday() const { return earnedMinutesToday_; }
    uint16_t spentMinutesToday() const { return spentMinutesToday_; }
    const String& dateIso() const { return dateIso_; }

private:
    uint16_t earnedMinutesToday_ = 0;
    uint16_t spentMinutesToday_ = 0;
    String dateIso_;
};
