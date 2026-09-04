#include "JsonStore.h"

#include <SD.h>

namespace storage {

namespace {

bool tryLoadJsonFile(const String& path, JsonDocument& doc) {
    if (!SD.exists(path)) {
        return false;
    }
    File file = SD.open(path, FILE_READ);
    if (!file) {
        return false;
    }
    const DeserializationError err = deserializeJson(doc, file);
    file.close();
    return err == DeserializationError::Ok;
}

} // namespace

bool saveJsonAtomic(const char* path, const JsonDocument& doc) {
    const String target(path);
    const String tmpPath = target + ".tmp";
    const String bakPath = target + ".bak";

    // Rest aus einem evtl. vorherigen Absturz waehrend des letzten
    // Schreibvorgangs entfernen, bevor neu geschrieben wird.
    if (SD.exists(tmpPath)) {
        SD.remove(tmpPath);
    }

    File tmpFile = SD.open(tmpPath, FILE_WRITE);
    if (!tmpFile) {
        return false;
    }

    const size_t written = serializeJson(doc, tmpFile);
    tmpFile.flush();
    tmpFile.close();

    if (written == 0) {
        SD.remove(tmpPath);
        return false;
    }

    // Bisherige gueltige Version als Backup aufheben, bevor sie ersetzt
    // wird - damit loadJsonWithFallback() im Fehlerfall darauf
    // zurueckfallen kann.
    if (SD.exists(target)) {
        if (SD.exists(bakPath)) {
            SD.remove(bakPath);
        }
        SD.rename(target, bakPath);
    }

    return SD.rename(tmpPath, target);
}

bool loadJsonWithFallback(const char* path, JsonDocument& doc) {
    const String target(path);
    const String bakPath = target + ".bak";

    if (tryLoadJsonFile(target, doc)) {
        return true;
    }

    // Zieldatei fehlt oder ist beschaedigt (z. B. Stromausfall waehrend
    // des Schreibens) - auf die letzte bekanntermassen gueltige Sicherung
    // zurueckfallen.
    doc.clear();
    return tryLoadJsonFile(bakPath, doc);
}

} // namespace storage
