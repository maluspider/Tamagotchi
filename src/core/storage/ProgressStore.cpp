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

    for (size_t i = 0; i < kSubjectCount; ++i) {
        const char* slug = subjectSlug(static_cast<Subject>(i));
        data.difficulty[i].stage = doc["statistik"]["schwierigkeit"][slug]["stufe"] | static_cast<uint8_t>(1);
        data.difficulty[i].ceiling = doc["statistik"]["schwierigkeit"][slug]["obergrenze"] | static_cast<uint8_t>(1);
        data.difficulty[i].lastCeilingBumpMonthIso =
            doc["statistik"]["schwierigkeit"][slug]["letzter_monatsanstieg"] | "";
    }

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

    for (size_t i = 0; i < kSubjectCount; ++i) {
        const char* slug = subjectSlug(static_cast<Subject>(i));
        doc["statistik"]["schwierigkeit"][slug]["stufe"] = data.difficulty[i].stage;
        doc["statistik"]["schwierigkeit"][slug]["obergrenze"] = data.difficulty[i].ceiling;
        doc["statistik"]["schwierigkeit"][slug]["letzter_monatsanstieg"] = data.difficulty[i].lastCeilingBumpMonthIso;
    }

    return storage::saveJsonAtomic(config::kProgressPath, doc);
}

} // namespace progressstore
