#pragma once

#include <ArduinoJson.h>

// Generische, stromausfall-sichere JSON-Persistenz auf der SD-Karte.
//
// Hintergrund (siehe docs/projektplan.md Abschnitt 6, Review): Kinder
// trennen das Geraet auch mal mitten im Speichervorgang von der
// Stromversorgung (leerer Akku, Kabel rausgezogen). Ein direktes
// Ueberschreiben der Zieldatei wuerde in diesem Fall eine
// abgeschnittene/kaputte JSON-Datei hinterlassen und damit im schlimmsten
// Fall den gesamten Fortschritt eines Kindes zerstoeren.
//
// saveJsonAtomic() schreibt deshalb nie direkt in die Zieldatei, sondern:
//   1. in eine temporaere ".tmp"-Datei,
//   2. rotiert die bisherige Zieldatei zu ".bak",
//   3. benennt die ".tmp"-Datei atomar auf den Zielnamen um.
// loadJsonWithFallback() liest die Zieldatei; ist sie beschaedigt oder
// fehlt sie, wird automatisch auf die ".bak"-Kopie zurueckgefallen.
namespace storage {

bool saveJsonAtomic(const char* path, const JsonDocument& doc);
bool loadJsonWithFallback(const char* path, JsonDocument& doc);

} // namespace storage
