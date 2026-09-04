#include "ProgressStore.h"

#include <ArduinoJson.h>

#include "config.h"
#include "JsonStore.h"

namespace progressstore {

ProgressData load() {
    ProgressData data;

    JsonDocument doc;
    if (!storage::loadJsonWithFallback(config::kProgressPath, doc)) {
        data.isValid = false;
        return data;
    }

    data.xp = doc["charakter"]["erfahrungspunkte"] | static_cast<uint32_t>(0);
    data.lastCareDateIso = doc["charakter"]["letzte_pflege"] | "";

    data.earnedMinutesToday = doc["spielzeitkonto"]["heute_verdient_min"] | static_cast<uint16_t>(0);
    data.spentMinutesToday = doc["spielzeitkonto"]["heute_verbraucht_min"] | static_cast<uint16_t>(0);
    data.playtimeDateIso = doc["spielzeitkonto"]["datum"] | "";

    data.isValid = true;
    return data;
}

bool save(const ProgressData& data) {
    JsonDocument doc;
    doc["charakter"]["erfahrungspunkte"] = data.xp;
    doc["charakter"]["letzte_pflege"] = data.lastCareDateIso;

    doc["spielzeitkonto"]["heute_verdient_min"] = data.earnedMinutesToday;
    doc["spielzeitkonto"]["heute_verbraucht_min"] = data.spentMinutesToday;
    doc["spielzeitkonto"]["datum"] = data.playtimeDateIso;

    return storage::saveJsonAtomic(config::kProgressPath, doc);
}

} // namespace progressstore
