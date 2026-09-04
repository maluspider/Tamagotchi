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

constexpr uint16_t kBackground   = rgb565(0x1A, 0x0B, 0x2E); // tiefes Indigo/Lila - Screen-Hintergrund
constexpr uint16_t kPanel        = rgb565(0x3D, 0x1B, 0x5E); // helleres Lila - Kopf-/Fussleisten, Kacheln
constexpr uint16_t kPanelLight   = rgb565(0x5A, 0x2E, 0x7E); // Rahmen/Trennlinien auf Panels
constexpr uint16_t kAccentPink   = rgb565(0xFF, 0x2E, 0x88); // Neonpink - Hauptakzent/aktiv
constexpr uint16_t kAccentCyan   = rgb565(0x00, 0xE5, 0xFF); // Neoncyan - Sekundaerakzent
constexpr uint16_t kAccentOrange = rgb565(0xFF, 0x6B, 0x35); // Sonnenuntergangs-Orange
constexpr uint16_t kAccentGold   = rgb565(0xFF, 0xD2, 0x3F); // Neongelb/Gold - Hervorhebungen, EP/Muenzen
constexpr uint16_t kSuccess      = rgb565(0x39, 0xFF, 0x8F); // Neongruen/-tuerkis - richtig/freigeschaltet
constexpr uint16_t kDanger       = rgb565(0xFF, 0x38, 0x60); // Neonrot - falsch/gesperrt/Batteriewarnung
constexpr uint16_t kMuted        = rgb565(0x6B, 0x5B, 0x7A); // gedaempftes Lila-Grau - deaktiviert
constexpr uint16_t kOutline      = rgb565(0x12, 0x08, 0x1F); // Naeherungsschwarz mit Lila-Stich
constexpr uint16_t kText         = 0xFFFF;                    // Weiss - Text/Icon-Linien (Kontrast/Lesbarkeit)
constexpr uint16_t kTextDim      = rgb565(0xC9, 0xB8, 0xE0); // gedaempfter Fliedertext - Sekundaerinfo

} // namespace theme
