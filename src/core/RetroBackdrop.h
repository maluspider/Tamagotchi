#pragma once

#include <M5GFX.h>

// Prozedural gezeichneter "Synthwave"-Hintergrund (untergehende Sonne +
// perspektivisches Bodengitter unterhalb eines Horizonts) fuer den vom
// Nutzer gewuenschten SNES-/80er-Arcade-Look (Strassenkaempfer-/Outrun-
// Stage-Optik, siehe Theme.h) - ohne zusaetzliche Bild-Assets, rein aus
// Theme-Farben/Zeichenprimitiven, damit keine neuen Dateien auf die
// SD-Karte kopiert werden muessen und bestehende Screens nur einen
// zusaetzlichen Aufruf brauchen.
//
// Zeichnet NICHT den Hintergrund selbst (fillScreen bleibt Sache des
// Aufrufers, vor diesem Aufruf) und liegt bewusst UNTER dem eigentlichen
// Bildschirminhalt - Panels/Icons/Charakter werden vom Aufrufer danach
// darueber gezeichnet, das Gitter scheint nur in den Zwischenraeumen durch.
namespace retrobackdrop {

// horizonY: Position der Horizontlinie (Sonne sitzt knapp darueber, das
// Gitter faechert darunter zum unteren Bildschirmrand auf). showSun=false
// laesst die Sonne weg (z. B. HomeScreen bei Nacht - dort uebernimmt ein
// eigenes Sternenfeld/Mond die Szenerie, siehe HomeScreen::draw()), das
// Bodengitter bleibt in jedem Fall erhalten.
void drawSynthwaveGrid(LovyanGFX* target, int width, int height, int horizonY, bool showSun = true);

} // namespace retrobackdrop
