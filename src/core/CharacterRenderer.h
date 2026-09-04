#pragma once

#include <M5GFX.h>

#include <cstdint>

#include "CharacterEngine.h"
#include "storage/ProfileStore.h"

// Zeichnet den Tamagotchi-Charakter als Sprite von der SD-Karte
// (sdcard/sprites/character/, Abschnitt 4) mit zur Laufzeit definierbaren
// Traits (Hautfarbe/Haarfarbe/Kleidungsfarbe, siehe CharacterTraits.h und
// CharacterCustomizeScreen). Die Sprite-Dateien enthalten reservierte
// Markerfarben an den anpassbaren Stellen statt fester Farben; draw() laedt
// die Datei in ein internes M5Canvas, ersetzt die Markerfarben durch die im
// Profil gespeicherten Trait-Farben und zeichnet das Ergebnis skaliert und
// zentriert auf die Zielflaeche - ein klassisches Palette-Swap-Verfahren,
// damit nicht fuer jede Farbkombination eine eigene Bilddatei noetig ist.
//
// Wird sowohl von HomeScreen als auch von CharacterCustomizeScreen benutzt
// (deshalb ein eigenstaendiges Modul statt Screen-lokaler Code). Das
// interne Sprite wird beim ersten draw()-Aufruf angelegt (nicht im
// Konstruktor/einer eigenen onEnter()-aehnlichen Methode), damit beide
// Aufrufer es ohne Extra-Lifecycle-Methode einfach benutzen koennen.
class CharacterRenderer {
public:
    CharacterRenderer();

    // Zeichnet den Charakter zentriert auf (cx, cy) der uebergebenen
    // Zielflaeche (M5.Display oder ein Screen-eigenes M5Canvas - Screens mit
    // periodischem Redraw sollen in ein eigenes Offscreen-Canvas zeichnen und
    // es erst am Ende in einem Rutsch anzeigen, sonst flackert es auf
    // echter Hardware sichtbar, siehe HomeScreen). Gibt false zurueck, wenn
    // keine passende Sprite-Datei existiert (SD-Karte fehlt oder Datei
    // fehlt) - der Aufrufer soll dann eine eigene Platzhalter-Grafik
    // zeichnen.
    bool draw(CharacterStage stage, const char* mood, const Profile& profile, int cx, int cy, float scale,
              LovyanGFX* target);

private:
    void ensureSprite();
    void applyTraitColors(const Profile& profile);

    M5Canvas canvas_;
    bool spriteReady_ = false;
};
