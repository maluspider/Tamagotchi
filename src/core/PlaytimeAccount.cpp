#include "PlaytimeAccount.h"

#include "config.h"

void PlaytimeAccount::load(uint16_t earnedMinutesToday, uint16_t spentMinutesToday, const String& dateIso) {
    earnedMinutesToday_ = earnedMinutesToday;
    spentMinutesToday_ = spentMinutesToday;
    dateIso_ = dateIso;
}

void PlaytimeAccount::rolloverIfNewDay(const String& todayIso) {
    if (dateIso_ == todayIso) {
        return;
    }
    // Neuer Tag (RTC-Mitternachtswechsel) - Restguthaben verfaellt bewusst,
    // kein Uebertrag (Abschnitt 7: haelt es fair zwischen den Kindern).
    earnedMinutesToday_ = 0;
    spentMinutesToday_ = 0;
    dateIso_ = todayIso;
}

void PlaytimeAccount::creditTaskReward() {
    earnedMinutesToday_ = static_cast<uint16_t>(earnedMinutesToday_ + config::kMinutesPerSolvedTask);
}

bool PlaytimeAccount::spend(uint16_t minutes) {
    if (minutes > availableMinutes()) {
        return false;
    }
    spentMinutesToday_ = static_cast<uint16_t>(spentMinutesToday_ + minutes);
    return true;
}

uint16_t PlaytimeAccount::availableMinutes() const {
    const uint16_t cappedEarned = earnedMinutesToday_ < config::kDailyPlaytimeLimitMinutes
                                       ? earnedMinutesToday_
                                       : config::kDailyPlaytimeLimitMinutes;
    if (spentMinutesToday_ >= cappedEarned) {
        return 0;
    }
    return static_cast<uint16_t>(cappedEarned - spentMinutesToday_);
}
