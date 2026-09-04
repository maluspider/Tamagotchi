#include "NightModeService.h"

#include <M5Unified.h>

namespace nightmodeservice {

namespace {
constexpr uint8_t kNormalBrightness = 200;
constexpr uint8_t kNightBrightness = 20;
} // namespace

void check(const Profile& profile) {
    if (!profile.nightModeEnabled) {
        M5.Display.setBrightness(kNormalBrightness);
        return;
    }

    m5::rtc_time_t time_;
    M5.Rtc.getTime(&time_);
    const int hour = time_.hours;

    bool isNight;
    if (profile.nightStartHour <= profile.nightEndHour) {
        isNight = hour >= profile.nightStartHour && hour < profile.nightEndHour;
    } else {
        // Zeitraum ueberspannt Mitternacht (z. B. 20 Uhr bis 7 Uhr).
        isNight = hour >= profile.nightStartHour || hour < profile.nightEndHour;
    }

    M5.Display.setBrightness(isNight ? kNightBrightness : kNormalBrightness);
}

} // namespace nightmodeservice
