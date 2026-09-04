#pragma once

#include <cstdint>

#include "Theme.h"

// Definierbare Charakter-Merkmale (Hautfarbe/Haarfarbe/Kleidungsfarbe),
// siehe docs/projektplan.md Abschnitt 4/9 und CharacterCustomizeScreen.
// Die Sprite-Vorlagen (sdcard/sprites/character/, siehe
// tools/generate_sprites.py) enthalten an diesen Stellen reservierte
// Markerfarben statt fester Farben; CharacterRenderer ersetzt sie beim
// Zeichnen durch die hier definierten Trait-Farben (Palette-Swap). Die
// Markerfarben MUESSEN mit MARKER_SKIN/MARKER_HAIR/MARKER_CLOTH in
// tools/generate_sprites.py uebereinstimmen.
namespace traits {

// Reservierte Markerfarben in den Sprite-Vorlagen. Bewusst reine
// 0/255-Kanalwerte, damit die verlustbehaftete 888->565-Quantisierung beim
// PNG-Decodieren keine Rundungsunschaerfe erzeugt (0 und 255 sind unter
// jeder Rundungsregel eindeutig) - der exakte Vergleich in
// CharacterRenderer::applyTraitColors() bleibt so zuverlaessig.
constexpr uint16_t kTransparentKey = theme::rgb565(255, 0, 255);
constexpr uint16_t kSkinMarker = theme::rgb565(0, 255, 0);
constexpr uint16_t kHairMarker = theme::rgb565(0, 255, 255);
constexpr uint16_t kClothMarker = theme::rgb565(255, 255, 0);

struct Preset {
    const char* name;
    uint16_t color565;
};

constexpr Preset kSkinTones[] = {
    {"Hell", theme::rgb565(255, 224, 189)},
    {"Mittel", theme::rgb565(224, 172, 105)},
    {"Oliv", theme::rgb565(198, 134, 66)},
    {"Dunkel", theme::rgb565(108, 62, 38)},
};
constexpr uint8_t kSkinToneCount = sizeof(kSkinTones) / sizeof(kSkinTones[0]);

constexpr Preset kHairColors[] = {
    {"Schwarz", theme::rgb565(40, 32, 28)},
    {"Braun", theme::rgb565(92, 58, 34)},
    {"Blond", theme::rgb565(230, 197, 120)},
    {"Rot", theme::rgb565(176, 70, 40)},
    {"Blau", theme::rgb565(70, 110, 200)},
    {"Pink", theme::rgb565(230, 110, 170)},
};
constexpr uint8_t kHairColorCount = sizeof(kHairColors) / sizeof(kHairColors[0]);

// Farbe des Gi/Strampler-Kleidungsstuecks - bewusst getrennt vom (nicht
// anpassbaren) Guertel, der den Trainingsfortschritt zeigt statt des
// persoenlichen Looks (siehe tools/generate_sprites.py).
constexpr Preset kClothingColors[] = {
    {"Rot", theme::rgb565(220, 70, 70)},
    {"Blau", theme::rgb565(70, 120, 220)},
    {"Gruen", theme::rgb565(90, 190, 90)},
    {"Gelb", theme::rgb565(240, 200, 60)},
    {"Lila", theme::rgb565(150, 90, 200)},
    {"Orange", theme::rgb565(240, 140, 50)},
};
constexpr uint8_t kClothingColorCount = sizeof(kClothingColors) / sizeof(kClothingColors[0]);

} // namespace traits
