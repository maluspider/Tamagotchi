#include "ProfileStore.h"

#include <ArduinoJson.h>

#include "config.h"
#include "JsonStore.h"

namespace profilestore {

Profile load() {
    Profile profile;

    JsonDocument doc;
    if (!storage::loadJsonWithFallback(config::kProfilePath, doc)) {
        profile.isValid = false;
        return profile;
    }

    profile.name = doc["profil"]["name"] | "";
    profile.age = doc["profil"]["alter"] | static_cast<uint8_t>(0);
    profile.klasse = doc["profil"]["klasse"] | static_cast<uint8_t>(1);
    profile.geraetId = doc["profil"]["geraet_id"] | "";
    profile.birthdayMonth = doc["profil"]["geburtstag_monat"] | static_cast<uint8_t>(0);
    profile.birthdayDay = doc["profil"]["geburtstag_tag"] | static_cast<uint8_t>(0);
    profile.guard.salt = doc["guard"]["salt"] | static_cast<uint32_t>(0);
    profile.guard.value = doc["guard"]["value"] | static_cast<uint32_t>(0);
    profile.alarmEnabled = doc["wecker"]["aktiv"] | false;
    profile.alarmHour = doc["wecker"]["stunde"] | static_cast<uint8_t>(7);
    profile.alarmMinute = doc["wecker"]["minute"] | static_cast<uint8_t>(0);
    profile.nightModeEnabled = doc["nachtmodus"]["aktiv"] | false;
    profile.nightStartHour = doc["nachtmodus"]["start_stunde"] | static_cast<uint8_t>(20);
    profile.nightEndHour = doc["nachtmodus"]["ende_stunde"] | static_cast<uint8_t>(7);
    profile.dailyLimitMinutesOverride = doc["einstellungen"]["tageslimit_min"] | static_cast<uint16_t>(0);
    profile.skinToneIndex = doc["aussehen"]["hautfarbe"] | static_cast<uint8_t>(0);
    profile.hairColorIndex = doc["aussehen"]["haarfarbe"] | static_cast<uint8_t>(0);
    profile.clothingColorIndex = doc["aussehen"]["kleidung"] | static_cast<uint8_t>(0);
    profile.isValid = profile.geraetId.length() > 0;

    return profile;
}

bool save(const Profile& profile) {
    JsonDocument doc;
    doc["profil"]["name"] = profile.name;
    doc["profil"]["alter"] = profile.age;
    doc["profil"]["klasse"] = profile.klasse;
    doc["profil"]["geraet_id"] = profile.geraetId;
    doc["profil"]["geburtstag_monat"] = profile.birthdayMonth;
    doc["profil"]["geburtstag_tag"] = profile.birthdayDay;
    doc["guard"]["salt"] = profile.guard.salt;
    doc["guard"]["value"] = profile.guard.value;
    doc["wecker"]["aktiv"] = profile.alarmEnabled;
    doc["wecker"]["stunde"] = profile.alarmHour;
    doc["wecker"]["minute"] = profile.alarmMinute;
    doc["nachtmodus"]["aktiv"] = profile.nightModeEnabled;
    doc["nachtmodus"]["start_stunde"] = profile.nightStartHour;
    doc["nachtmodus"]["ende_stunde"] = profile.nightEndHour;
    doc["einstellungen"]["tageslimit_min"] = profile.dailyLimitMinutesOverride;
    doc["aussehen"]["hautfarbe"] = profile.skinToneIndex;
    doc["aussehen"]["haarfarbe"] = profile.hairColorIndex;
    doc["aussehen"]["kleidung"] = profile.clothingColorIndex;

    return storage::saveJsonAtomic(config::kProfilePath, doc);
}

} // namespace profilestore
