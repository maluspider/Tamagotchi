#include "AlarmService.h"

#include <M5Unified.h>

namespace alarmservice {

namespace {
int lastTriggeredMinuteOfDay = -1;
} // namespace

void check(const Profile& profile) {
    if (!profile.alarmEnabled) {
        return;
    }

    m5::rtc_time_t time_;
    M5.Rtc.getTime(&time_);

    if (time_.hours != profile.alarmHour || time_.minutes != profile.alarmMinute) {
        return;
    }

    const int minuteOfDay = time_.hours * 60 + time_.minutes;
    if (minuteOfDay == lastTriggeredMinuteOfDay) {
        return; // in dieser Minute schon ausgeloest
    }
    lastTriggeredMinuteOfDay = minuteOfDay;

    M5.Speaker.tone(1500, 300);
    M5.Power.setVibration(200);
    delay(300);
    M5.Power.setVibration(0);
}

} // namespace alarmservice
