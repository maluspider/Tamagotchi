#include "RtcBackupService.h"

#include <ArduinoJson.h>
#include <M5Unified.h>

#include "config.h"
#include "storage/JsonStore.h"

namespace rtcbackupservice {

void save() {
    m5::rtc_date_t date;
    M5.Rtc.getDate(&date);
    if (date.year < config::kRtcPlausibleMinYear) {
        // RTC selbst schon unplausibel - nichts Sinnvolles zu sichern,
        // sonst wuerde restoreIfImplausible() spaeter genau diese kaputte
        // Zeit "wiederherstellen".
        return;
    }

    m5::rtc_time_t time_;
    M5.Rtc.getTime(&time_);

    JsonDocument doc;
    doc["jahr"] = date.year;
    doc["monat"] = date.month;
    doc["tag"] = date.date;
    doc["stunde"] = time_.hours;
    doc["minute"] = time_.minutes;
    storage::saveJsonAtomic(config::kRtcBackupPath, doc);
}

bool restoreIfImplausible() {
    m5::rtc_date_t currentDate;
    M5.Rtc.getDate(&currentDate);
    if (currentDate.year >= config::kRtcPlausibleMinYear) {
        return false; // RTC hat schon eine plausible Zeit, nichts zu tun.
    }

    JsonDocument doc;
    if (!storage::loadJsonWithFallback(config::kRtcBackupPath, doc)) {
        return false; // Noch keine Sicherung vorhanden (z. B. allererster Start).
    }
    const int year = doc["jahr"] | 0;
    if (year < config::kRtcPlausibleMinYear) {
        return false; // Sicherung selbst unplausibel - nichts Sinnvolles wiederherzustellen.
    }

    m5::rtc_date_t restoredDate;
    restoredDate.year = year;
    restoredDate.month = doc["monat"] | 1;
    restoredDate.date = doc["tag"] | 1;
    M5.Rtc.setDate(&restoredDate);

    m5::rtc_time_t restoredTime;
    restoredTime.hours = doc["stunde"] | 12;
    restoredTime.minutes = doc["minute"] | 0;
    restoredTime.seconds = 0;
    M5.Rtc.setTime(&restoredTime);

    Serial.println(
        "RtcBackupService: RTC-Zeit unplausibel (vermutlich beim Abschalten verloren) - letzte gesicherte "
        "Zeit wiederhergestellt. Tatsaechlich waehrend der Abschaltzeit vergangene Zeit ist unbekannt - "
        "Uhrzeit bei Bedarf ueber Einstellungen > Uhrzeit einstellen korrigieren.");
    return true;
}

} // namespace rtcbackupservice
