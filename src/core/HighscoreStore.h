#pragma once

#include <cstdint>

// Lokale Highscores je Spiel (docs/projektplan.md Abschnitt 10: "Highscore
// lokal gespeichert"), persistiert in /highscores.json (atomar, siehe
// JsonStore.h). `gameKey` ist ein kurzer, stabiler Bezeichner pro Spiel
// (z. B. "snake", "tetris") - dient direkt als JSON-Schluessel.
namespace highscorestore {

uint32_t load(const char* gameKey);
void saveIfHigher(const char* gameKey, uint32_t score);

} // namespace highscorestore
