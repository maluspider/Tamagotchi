#pragma once

#include <cstdint>

// Einheitliches Farbschema fuer die gesamte UI (docs/projektplan.md
// Abschnitt 4): Neonfarben vor dunklem Indigo/Lila, angelehnt an 80er-
// Jahre-Synthwave-Optik und Arcade-Kampfspiele der SNES-/Arcade-Aera
// (Street Fighter II, Mortal Kombat) - siehe auch die Sprite-Ueberarbeitung
// in tools/generate_sprites.py, die denselben Grundton fuer Hintergrund/
// Outline verwendet. Screens nutzen diese Konstanten statt roher TFT_*-
// Werte, damit sich das Farbschema an einer Stelle aendern laesst.
//
// Bewusste Ausnahme: reiner Text/Icon-Linien bleiben ueberwiegend
// TFT_WHITE/theme::kText (maximaler Kontrast) statt eines Neontons - fuer
// die junge Zielgruppe (Abschnitt 1) darf die Lesbarkeit nicht unter der
// Optik leiden.
namespace theme {

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Finalisierte Palette (Nutzerwunsch: "Verbessere Grafik und Farbthemes
// SNES/80er-Jahre-Arcade-Games...knallige Neon-Retro-Farben finalisiere") -
// Hintergrund/Panel etwas dunkler fuer mehr Kontrast, Akzentfarben naeher an
// ihren maximal gesaettigten Grundtoenen (weniger Pastell-/Mischanteil) fuer
// den knalligeren Arcade-Look.
constexpr uint16_t kBackground   = rgb565(0x14, 0x06, 0x26); // fast schwarzes Indigo/Lila - Screen-Hintergrund
constexpr uint16_t kPanel        = rgb565(0x39, 0x16, 0x5C); // helleres Lila - Kopf-/Fussleisten, Kacheln
constexpr uint16_t kPanelLight   = rgb565(0x63, 0x30, 0x8C); // Rahmen/Trennlinien auf Panels
constexpr uint16_t kAccentPink   = rgb565(0xFF, 0x0A, 0x8C); // Neonpink/-magenta - Hauptakzent/aktiv
constexpr uint16_t kAccentCyan   = rgb565(0x00, 0xF0, 0xFF); // Neoncyan - Sekundaerakzent
constexpr uint16_t kAccentOrange = rgb565(0xFF, 0x55, 0x00); // knalliges Arcade-Orange
constexpr uint16_t kAccentGold   = rgb565(0xFF, 0xE0, 0x00); // Neongelb/Gold - Hervorhebungen, EP/Muenzen
constexpr uint16_t kSuccess      = rgb565(0x00, 0xFF, 0x6A); // Neongruen - richtig/freigeschaltet
constexpr uint16_t kDanger       = rgb565(0xFF, 0x14, 0x44); // Neonrot - falsch/gesperrt/Batteriewarnung
constexpr uint16_t kMuted        = rgb565(0x6B, 0x5B, 0x7A); // gedaempftes Lila-Grau - deaktiviert
constexpr uint16_t kOutline      = rgb565(0x0E, 0x05, 0x1A); // Naeherungsschwarz mit Lila-Stich
constexpr uint16_t kText         = 0xFFFF;                    // Weiss - Text/Icon-Linien (Kontrast/Lesbarkeit)
constexpr uint16_t kTextDim      = rgb565(0xC9, 0xB8, 0xE0); // gedaempfter Fliedertext - Sekundaerinfo

} // namespace theme
