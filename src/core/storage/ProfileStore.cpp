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
    profile.klasse = doc["profil"]["klasse"] | static_cast<uint8_t>(1);
    profile.geraetId = doc["profil"]["geraet_id"] | "";
    profile.guard.salt = doc["guard"]["salt"] | static_cast<uint32_t>(0);
    profile.guard.value = doc["guard"]["value"] | static_cast<uint32_t>(0);
    profile.isValid = profile.geraetId.length() > 0;

    return profile;
}

bool save(const Profile& profile) {
    JsonDocument doc;
    doc["profil"]["name"] = profile.name;
    doc["profil"]["klasse"] = profile.klasse;
    doc["profil"]["geraet_id"] = profile.geraetId;
    doc["guard"]["salt"] = profile.guard.salt;
    doc["guard"]["value"] = profile.guard.value;

    return storage::saveJsonAtomic(config::kProfilePath, doc);
}

} // namespace profilestore
