#include "NightModeService.h"

#include <M5Unified.h>

#include "config.h"

namespace nightmodeservice {

namespace {
constexpr uint8_t kNormalBrightness = 200;
constexpr uint8_t kNightBrightness = 20;
} // namespace

bool isNight(const Profile& profile) {
    m5::rtc_date_t date;
    M5.Rtc.getDate(&date);
    // Frisch ausgelieferte/noch nie gestellte RTCs starten haeufig bei
    // einem Default-Datum (je nach Chip z. B. Jahr 2000 oder 1970) mit
    // Stunde 0 - das faellt in jedes plausible Nachtmodus-Fenster und
    // wuerde den Bildschirm sofort dimmen, noch bevor das Kind/die Eltern
    // die Uhrzeit je gestellt haben (siehe DateTimeSetScreen). Ein Jahr
    // unterhalb von config::kRtcPlausibleMinYear gilt daher als "RTC noch
    // nicht gestellt" - Nachtmodus bleibt in dem Fall inaktiv, unabhaengig
    // von der Uhrzeit. Dieselbe Schwelle nutzt RtcBackupService, um eine
    // beim Abschalten verlorene RTC-Zeit zu erkennen und wiederherzustellen.
    if (date.year < config::kRtcPlausibleMinYear) {
        return false;
    }

    m5::rtc_time_t time_;
    M5.Rtc.getTime(&time_);
    const int hour = time_.hours;

    if (profile.nightStartHour <= profile.nightEndHour) {
        return hour >= profile.nightStartHour && hour < profile.nightEndHour;
    }
    // Zeitraum ueberspannt Mitternacht (z. B. 20 Uhr bis 7 Uhr).
    return hour >= profile.nightStartHour || hour < profile.nightEndHour;
}

void check(const Profile& profile) {
    if (!profile.nightModeEnabled) {
        M5.Display.setBrightness(kNormalBrightness);
        return;
    }
    M5.Display.setBrightness(isNight(profile) ? kNightBrightness : kNormalBrightness);
}

} // namespace nightmodeservice
