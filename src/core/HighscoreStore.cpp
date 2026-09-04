#include "HighscoreStore.h"

#include <ArduinoJson.h>

#include "storage/JsonStore.h"

namespace {
constexpr const char* kPath = "/highscores.json";
} // namespace

namespace highscorestore {

uint32_t load(const char* gameKey) {
    JsonDocument doc;
    if (!storage::loadJsonWithFallback(kPath, doc)) {
        return 0;
    }
    return doc[gameKey] | static_cast<uint32_t>(0);
}

void saveIfHigher(const char* gameKey, uint32_t score) {
    JsonDocument doc;
    storage::loadJsonWithFallback(kPath, doc); // ok, wenn die Datei (noch) nicht existiert

    const uint32_t current = doc[gameKey] | static_cast<uint32_t>(0);
    if (score > current) {
        doc[gameKey] = score;
        storage::saveJsonAtomic(kPath, doc);
    }
}

} // namespace highscorestore
