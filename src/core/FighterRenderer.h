#pragma once

#include <M5GFX.h>

#include <cstdint>

// Zeichnet den Kampf-Modus-Kaempfer als Sprite von der SD-Karte
// (sdcard/sprites/fighter/, siehe tools/generate_fighter_sprites.py) statt
// des fruehen "Rumpf-Blobs" (Rechteck+Kreis ohne Gliedmassen) - Nutzer-
// Feedback: "der streetfighter game ist ein witz...way to basic...will
// etwas was aussieht wie das gameboy streetfighter game...maximal
// hochwertig mit echten Animationen und realistischen Grafiken". Acht
// Posen (FighterPose) decken Idle-Atmen, Gehen, Schlag, Tritt,
// Treffer-Reaktion und K.o. ab - KampfModusScreen waehlt die Pose ueber
// eine kleine Animations-Zustandsmaschine.
//
// Gleiche Palette-Swap-Technik wie CharacterRenderer (Markerfarben statt
// fester Farben fuer Haut/Haar/Gi in den PNG-Vorlagen, siehe
// CharacterTraits.h fuer die exakten Markerwerte) - hier aber mit frei
// uebergebbaren Zielfarben statt eines Profile-Objekts, damit Spieler
// (eigene Aussehen-Farben + Kampf-Gi-Farbe) und KI (feste Gegnerfarben)
// denselben Renderer mit unterschiedlichen Farben nutzen koennen.
enum class FighterPose : uint8_t {
    Idle1,
    Idle2,
    Walk1,
    Walk2,
    Punch,
    Kick,
    Hurt,
    Ko,
};

class FighterRenderer {
public:
    FighterRenderer();

    // Zeichnet zentriert auf cx, mit dem Fusskontaktpunkt der Vorlage fest
    // auf groundY verankert (posen-unabhaengig, siehe tools/generate_
    // fighter_sprites.py: alle Standposen haben denselben Fuss-Zeilenwert
    // im 56x64-Raster) - dadurch "hoppelt" der Kaempfer beim Posenwechsel
    // nicht vertikal. facingRight=false spiegelt die (immer nach rechts
    // blickende) Vorlage per negativem zoom_x. Gibt false zurueck, wenn
    // keine passende Datei existiert (SD-Karte fehlt) - der Aufrufer soll
    // dann eine Platzhalter-Grafik zeichnen.
    bool draw(FighterPose pose, uint16_t skinColor, uint16_t hairColor, uint16_t giColor, bool facingRight, int cx,
              int groundY, float scale, LovyanGFX* target);

private:
    void ensureSprite();
    void applyColors(uint16_t skinColor, uint16_t hairColor, uint16_t giColor);

    M5Canvas canvas_;
    bool spriteReady_ = false;
};
