#include "RtcClock.h"

#include <M5Unified.h>
#include <WiFi.h>
#include <time.h>

namespace rtcclock {

namespace {

// Howard Hinnant's "days_from_civil" - liefert Tage seit 1970-01-01 fuer
// ein gregorianisches Datum, ohne floating point und ohne <ctime>-
// Zeitzonen-Ueberraschungen.
long toEpochDay(int y, int m, int d) {
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long>(doe) - 719468;
}

// Howard Hinnant's "civil_from_days" - Kehrfunktion zu toEpochDay().
void civilFromEpochDay(long z, int& y, int& m, int& d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long yr = static_cast<long>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    m = static_cast<int>(mp + (mp < 10 ? 3 : -9));
    y = static_cast<int>(yr + (m <= 2 ? 1 : 0));
}

} // namespace

String todayIso() {
    m5::rtc_date_t date;
    M5.Rtc.getDate(&date);
    char buf[11];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", date.year, date.month, date.date);
    return String(buf);
}

long epochDayFromIso(const String& isoDate) {
    if (isoDate.length() < 10) {
        return 0;
    }
    const int y = isoDate.substring(0, 4).toInt();
    const int m = isoDate.substring(5, 7).toInt();
    const int d = isoDate.substring(8, 10).toInt();
    return toEpochDay(y, m, d);
}

String isoFromEpochDay(long epochDay) {
    int y, m, d;
    civilFromEpochDay(epochDay, y, m, d);
    char buf[11];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
    return String(buf);
}

long todayEpochDay() {
    m5::rtc_date_t date;
    M5.Rtc.getDate(&date);
    return toEpochDay(date.year, date.month, date.date);
}

bool syncFromNtpIfAvailable() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5000)) {
        return false;
    }

    m5::rtc_date_t date;
    date.year = timeinfo.tm_year + 1900;
    date.month = timeinfo.tm_mon + 1;
    date.date = timeinfo.tm_mday;

    m5::rtc_time_t time_;
    time_.hours = timeinfo.tm_hour;
    time_.minutes = timeinfo.tm_min;
    time_.seconds = timeinfo.tm_sec;

    M5.Rtc.setDate(&date);
    M5.Rtc.setTime(&time_);
    return true;
}

} // namespace rtcclock
