#pragma once

#include <M5GFX.h>

#include <cstdint>

// Wiederverwendbare Zeichen-Bausteine fuer den vom Nutzer gewuenschten
// "SNES-Niveau"-Look ("vollstaendiges Grafikdesign wie bei Super Nintendo,
// komplexere Grafiken, Hintergruende, keine rudimentaeren Darstellungen
// mehr, optimiere Grafik maximal") - baut ganz ohne neue Bild-Assets rein
// aus Farbverlaeufen, Bevel-Kanten und einem Sternenfeld einen deutlich
// aufwendigeren Eindruck als reine Ein-Farb-Flaechen. Ergaenzt
// RetroBackdrop (Synthwave-Sonne/-Gitter) und wird von HomeScreen sowie
// allen Spiel-Screens verwendet.
namespace gfxkit {

// Faerbt eine RGB565-Farbe Richtung Weiss auf (amount 0..1) - fuer
// Bevel-Hochglanzkanten auf beliebiger Basisfarbe, ohne dass der Aufrufer
// eine passende zweite Theme-Farbe kennen/uebergeben muss.
uint16_t lighten(uint16_t color, float amount);

// Faerbt eine RGB565-Farbe Richtung Schwarz ab (amount 0..1) - siehe
// lighten().
uint16_t darken(uint16_t color, float amount);

// Linearer Farbverlauf zwischen zwei RGB565-Farben, t wird auf [0,1] geklemmt.
uint16_t lerp(uint16_t from, uint16_t to, float t);

// Vertikaler Farbverlauf ueber ein Rechteck (z. B. Himmel-/Wand-/
// Spielfeld-Hintergrund) statt einer flachen fillRect-Einfarbflaeche.
void verticalGradient(LovyanGFX* target, int x, int y, int w, int h, uint16_t topColor, uint16_t bottomColor);

// Abgerundetes Panel mit heller Kante oben/links und dunkler Kante
// unten/rechts (klassischer SNES-UI-"erhabenes Kachel/Button"-Bevel-Look)
// statt einer flachen Ein-Farb-Flaeche. raised=false kehrt den Bevel um
// (eingedrueckt wirkendes Panel, z. B. fuer aktive/gedrueckte Zustaende).
void bevelPanel(LovyanGFX* target, int x, int y, int w, int h, int r, uint16_t base, bool raised = true);

// Deterministisches (nicht bei jedem Redraw "rauschendes") Punktefeld aus
// `count` kleinen Punkten im Bereich [xOffset,xOffset+width) x
// [yOffset,yOffset+maxY) - Positionen ergeben sich aus einer festen
// Hash-Formel je Index statt aus rand(), damit ein Screen, der jeden Frame
// neu zeichnet (z. B. HomeScreen), ein stehendes Sternenfeld statt
// Flimmern zeigt. xOffset/yOffset erlauben die Platzierung in einem
// begrenzten Bereich (z. B. innerhalb eines Tetris-Spielfelds oder als
// Zuschauer-Tribuene ueber einem Spielfeld) statt nur ab (0,0); Aufrufe
// ohne diese beiden Parameter verhalten sich wie zuvor.
void starfield(LovyanGFX* target, int width, int maxY, int count, uint16_t color, int xOffset = 0, int yOffset = 0);

// Deterministische Huegel-/Skyline-Silhouette aus `count` ueberlappenden
// Dreiecken unterschiedlicher Hoehe (feste, nicht zufaellige Formel - siehe
// starfield()) - fuer Landschafts-/Buehnenhintergruende (Moorhuhn-Jagd,
// Kampf-Modus) ohne zusaetzliche Bild-Assets. baseY ist die Grundlinie
// (Huegelfuss), peakHeight die maximale Hoehe der hoechsten Erhebung.
void hillsSilhouette(LovyanGFX* target, int width, int baseY, int peakHeight, int count, uint16_t color);

// Kugel mit Glanzlicht (kleiner heller Kreis oben-links) plus dunklerem
// Rand statt einer reinen Flat-fillCircle - fuer Baelle/Muenzen/Punkte in
// den Spielen.
void shinyBall(LovyanGFX* target, int cx, int cy, int r, uint16_t color);

} // namespace gfxkit
