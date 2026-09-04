#pragma once

#include <M5GFX.h>

// Zeichnet Text zentriert in eine maximale Breite/Hoehe, automatisch in
// mehrere Zeilen umgebrochen und noetigenfalls in kleinerer Schrift, damit
// er nie ueber den verfuegbaren Platz hinausragt (Nutzer-Feedback aus dem
// echten Hardware-Test: laengere Quiz-Fragen/Antworten liefen bei fest
// codierter Textgroesse rechts/links aus dem Bildschirm und wurden dadurch
// unlesbar). Nutzt LovyanGFX::textWidth()/fontHeight(), die beide bereits
// die aktuell gesetzte Textgroesse einrechnen.
namespace textfit {

void drawFitted(LovyanGFX* target, const String& text, int cx, int cy, int maxWidth, int maxHeight, int maxTextSize,
                 uint16_t color, int minTextSize = 1);

} // namespace textfit
