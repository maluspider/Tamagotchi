#!/usr/bin/env python3
"""Erzeugt die Charakter-Sprite-PNGs unter sdcard/sprites/character/.

Der Charakter ist ein Menschenkind, das ueber die 6 Entwicklungsstufen
sichtbar heranwaechst und trainiert - vom Kleinkind (Stufe "Ei") zum
Karate-/Kampfkuenstler-Kind mit Guertelfarben nach echtem Vorbild
(weiss->gelb->gruen->braun->schwarz), optisch angelehnt an Arcade-
Kampfspiele der 90er (Street Fighter II, Mortal Kombat) - siehe
docs/projektplan.md Abschnitt 4/9.

Reproduzierbare Asset-Pipeline statt handgemalter Dateien: jede Form wird
pixelweise aus Ellipsen/Rechtecken/Linien auf einem 32x32-Raster
zusammengesetzt (kein PIL-Anti-Aliasing, dadurch echter Pixel-Art-Look).

Wichtig - Markerfarben statt fester Farben: Haut/Haar/Kleidung werden NICHT
fest eingefaerbt, sondern mit reservierten, reinen Markerfarben gefuellt
(siehe MARKER_* unten). Das Geraet ersetzt diese beim Zeichnen zur Laufzeit
durch die vom Kind gewaehlten Trait-Farben (Palette-Swap, siehe
src/core/CharacterRenderer.* und src/core/CharacterTraits.h - die dortigen
Markerfarben MUESSEN mit MARKER_* hier uebereinstimmen). Guertel- und
Stirnband-Farben sind bewusst NICHT ueber Marker anpassbar: sie zeigen den
Trainingsfortschritt (Stufe), nicht den frei waehlbaren Look.

Benoetigt Pillow (`pip install pillow`) nur fuer den PNG-Export.
"""

from __future__ import annotations

import os
from dataclasses import dataclass
from typing import Optional

from PIL import Image

GRID = 32
TRANSPARENT = (0, 0, 0, 0)

# Reservierte Markerfarben (reine 0/255-Kanalwerte - siehe CharacterTraits.h
# fuer die Begruendung: verlustfrei durch die 888->565-Quantisierung beim
# PNG-Decodieren auf dem Geraet). Echte Transparenz (Hintergrund) nutzt
# hier ganz normal Alpha=0 (TRANSPARENT) - der reservierte "Transparenz"-
# Markerwert existiert nur auf der Geraeteseite als Fuellfarbe des
# Zeichen-Canvas vor dem Dekodieren, siehe CharacterRenderer.cpp.
MARKER_SKIN = (0, 255, 0, 255)
MARKER_HAIR = (0, 255, 255, 255)
MARKER_CLOTH = (255, 255, 0, 255)

OUTLINE = (18, 8, 31, 255)  # deckt sich mit theme::kOutline (Theme.h)
HEADBAND = (205, 40, 46, 255)
HEADBAND_SHADOW = (150, 25, 32, 255)

# Guertelfarben nach echtem Karate-Vorbild, je Stufe fest (nicht anpassbar -
# zeigt den Trainingsfortschritt).
BELT_WHITE = (232, 230, 224, 255)
BELT_YELLOW = (232, 202, 60, 255)
BELT_GREEN = (68, 163, 92, 255)
BELT_BROWN = (117, 79, 48, 255)
BELT_BLACK = (42, 38, 46, 255)

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "sdcard", "sprites", "character")


def new_grid():
    return [[TRANSPARENT for _ in range(GRID)] for _ in range(GRID)]


def set_px(grid, x, y, color):
    x, y = int(round(x)), int(round(y))
    if 0 <= x < GRID and 0 <= y < GRID:
        grid[y][x] = color


def get_px(grid, x, y):
    if 0 <= x < GRID and 0 <= y < GRID:
        return grid[y][x]
    return TRANSPARENT


def fill_ellipse(grid, cx, cy, rx, ry, color):
    for y in range(GRID):
        for x in range(GRID):
            dx = (x + 0.5 - cx) / rx
            dy = (y + 0.5 - cy) / ry
            if dx * dx + dy * dy <= 1.0:
                grid[y][x] = color


def fill_rect(grid, x0, y0, x1, y1, color):
    x0, x1 = int(round(x0)), int(round(x1))
    y0, y1 = int(round(y0)), int(round(y1))
    for y in range(y0, y1):
        for x in range(x0, x1):
            set_px(grid, x, y, color)


def draw_line(grid, x0, y0, x1, y1, color, thickness=1):
    steps = int(round(max(abs(x1 - x0), abs(y1 - y0), 1)))
    for i in range(steps + 1):
        t = i / steps
        x = x0 + (x1 - x0) * t
        y = y0 + (y1 - y0) * t
        if thickness <= 1:
            set_px(grid, x, y, color)
        else:
            half = thickness / 2.0
            for oy in range(-int(half), int(half) + 1):
                set_px(grid, x, y + oy, color)


def add_outline(grid, color=OUTLINE):
    to_outline = []
    for y in range(GRID):
        for x in range(GRID):
            if grid[y][x][3] != 0:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                if get_px(grid, x + dx, y + dy)[3] != 0:
                    to_outline.append((x, y))
                    break
    for x, y in to_outline:
        grid[y][x] = color


def assert_binary_alpha(grid, name):
    # Marker-Regionen muessen exakt getroffen werden koennen (die Ersetzung
    # auf dem Geraet vergleicht Pixel auf exakte Gleichheit) - deshalb hier
    # nur volle Deckung oder volle Transparenz, nirgends Halbtransparenz/
    # Anti-Aliasing.
    for row in grid:
        for _, _, _, a in row:
            assert a in (0, 255), f"{name}: unerwarteter Alpha-Wert {a}"


def save_png(grid, name):
    assert_binary_alpha(grid, name)
    img = Image.new("RGBA", (GRID, GRID))
    for y in range(GRID):
        for x in range(GRID):
            img.putpixel((x, y), grid[y][x])
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, name)
    img.save(path)
    print("geschrieben:", path)


# --- Augen/Mund -------------------------------------------------------------

def draw_eyes_open(grid, cx, eye_y, spread):
    for sign in (-1, 1):
        ex = cx + sign * spread
        fill_ellipse(grid, ex, eye_y, 1.9, 2.3, (255, 255, 255, 255))
        fill_ellipse(grid, ex + sign * 0.5, eye_y + 0.3, 1.0, 1.1, OUTLINE)


def draw_eyes_blink(grid, cx, eye_y, spread, color=OUTLINE):
    for sign in (-1, 1):
        ex = cx + sign * spread
        draw_line(grid, ex - 1.8, eye_y, ex + 1.8, eye_y, color)


def draw_eyes_sad(grid, cx, eye_y, spread):
    for sign in (-1, 1):
        ex = cx + sign * spread
        fill_ellipse(grid, ex, eye_y + 1, 1.7, 1.9, (255, 255, 255, 255))
        fill_ellipse(grid, ex, eye_y + 1.5, 0.8, 0.9, OUTLINE)
        if sign < 0:
            draw_line(grid, ex - 1.8, eye_y + 0.8, ex + 1.8, eye_y - 1.6, OUTLINE)
        else:
            draw_line(grid, ex - 1.8, eye_y - 1.6, ex + 1.8, eye_y + 0.8, OUTLINE)


def draw_mouth_smile(grid, cx, y):
    draw_line(grid, cx - 1.8, y, cx - 0.8, y + 1, OUTLINE)
    draw_line(grid, cx - 0.8, y + 1, cx + 0.8, y + 1, OUTLINE)
    draw_line(grid, cx + 0.8, y + 1, cx + 1.8, y, OUTLINE)


def draw_mouth_sad(grid, cx, y):
    draw_line(grid, cx - 1.8, y + 1, cx - 0.8, y, OUTLINE)
    draw_line(grid, cx - 0.8, y, cx + 0.8, y, OUTLINE)
    draw_line(grid, cx + 0.8, y, cx + 1.8, y + 1, OUTLINE)


def draw_mouth_determined(grid, cx, y):
    # Schmale, gerade Linie - konzentrierter/entschlossener Ausdruck fuer
    # die Kampf-Stellungen ab Junior (kein Laecheln, kein Traurig-Sein).
    draw_line(grid, cx - 1.6, y, cx + 1.6, y, OUTLINE)


def blush(grid, cx, y, spread):
    # Volldeckend statt halbtransparent, siehe Modulkommentar oben: die
    # Wangen werden NACH der Hautfarbe (Marker) gezeichnet und bleiben nach
    # dem Palette-Swap auf dem Geraet unveraendert rosa, unabhaengig von der
    # gewaehlten Hautfarbe (bewusste Vereinfachung).
    for sign in (-1, 1):
        fill_ellipse(grid, cx + sign * spread, y, 1.4, 0.9, (255, 150, 150, 255))


# --- Koerperbau --------------------------------------------------------------

@dataclass
class StageSpec:
    key: str
    head_cy: float
    head_r: float
    hair_style: str  # "tuft" | "cap" | "spiky"
    torso_top: float
    torso_bottom: float
    torso_half_w: float
    has_legs: bool
    leg_bottom: float
    belt_color: Optional[tuple]
    arm_pose: str  # "none" | "low" | "guard" | "punch" | "fists_up"
    headband: bool
    headband_tails: int


def draw_hair(grid, spec: StageSpec):
    cx, cy, r = 16, spec.head_cy, spec.head_r
    if spec.hair_style == "tuft":
        fill_ellipse(grid, cx, cy - r * 0.75, r * 0.55, r * 0.4, MARKER_HAIR)
        fill_ellipse(grid, cx + 1, cy - r * 1.15, 1.1, 1.3, MARKER_HAIR)
    elif spec.hair_style == "cap":
        fill_ellipse(grid, cx, cy - r * 0.35, r * 0.98, r * 0.75, MARKER_HAIR)
        fill_ellipse(grid, cx, cy + r * 0.15, r * 1.02, r * 0.55, TRANSPARENT)  # Stirn wieder frei
        fill_ellipse(grid, cx, cy - r * 0.05, r * 0.98, r * 0.62, MARKER_HAIR)
        fill_ellipse(grid, cx, cy + r * 0.35, r * 0.9, r * 0.4, TRANSPARENT)
    else:  # "spiky"
        fill_ellipse(grid, cx, cy - r * 0.2, r * 0.98, r * 0.78, MARKER_HAIR)
        fill_ellipse(grid, cx, cy + r * 0.35, r * 0.9, r * 0.4, TRANSPARENT)
        for dx, h in ((-r * 0.55, 1.6), (0, 2.2), (r * 0.55, 1.6)):
            tip_x = cx + dx
            base_y = cy - r * 0.75
            fill_ellipse(grid, tip_x, base_y - h, 1.0, h, MARKER_HAIR)


def draw_headband(grid, spec: StageSpec):
    if not spec.headband:
        return
    cx, cy, r = 16, spec.head_cy, spec.head_r
    band_y = cy - r * 0.15
    fill_rect(grid, cx - r - 0.5, band_y - 1.1, cx + r + 0.5, band_y + 1.1, HEADBAND)
    fill_rect(grid, cx - 1.0, band_y - 1.1, cx + 1.0, band_y + 1.1, HEADBAND_SHADOW)
    for i in range(spec.headband_tails):
        tx = cx + r + 1 + i * 2.2
        ty = band_y + 1
        draw_line(grid, tx, ty, tx + 2.5, ty + 4 + i * 1.5, HEADBAND, thickness=1.6)


def draw_arms(grid, spec: StageSpec):
    # Jeder Arm ist eine durchgehende, dicke Linie von der Schulter zur Hand
    # (statt einzelner, potenziell unverbundener Ellipsen) - garantiert eine
    # sichtbar zusammenhaengende Gliedmasse auch bei ungewoehnlichen
    # Handpositionen (z. B. "fists_up").
    half = spec.torso_half_w
    shoulder_y = spec.torso_top + 1.2

    if spec.arm_pose == "none":
        fill_ellipse(grid, 16 - half - 0.5, shoulder_y + 2, 1.4, 1.8, MARKER_SKIN)
        fill_ellipse(grid, 16 + half + 0.5, shoulder_y + 2, 1.4, 1.8, MARKER_SKIN)
        return

    for sign in (-1, 1):
        shoulder_x = 16 + sign * half * 0.85
        if spec.arm_pose == "low":
            hand_x, hand_y = shoulder_x + sign * 1.2, spec.torso_bottom - 0.5
        elif spec.arm_pose == "guard":
            hand_x, hand_y = 16 + sign * half * 0.4, spec.torso_top - 0.5
        elif spec.arm_pose == "punch":
            if sign > 0:
                hand_x, hand_y = 16 + half + 6.0, shoulder_y  # ausgestreckter Schlagarm
            else:
                hand_x, hand_y = 16 + sign * half * 0.4, spec.torso_top - 0.5  # Deckung
        else:  # "fists_up"
            hand_x, hand_y = 16 + sign * (half + 2.2), spec.head_cy + 2.5

        draw_line(grid, shoulder_x, shoulder_y, hand_x, hand_y, MARKER_CLOTH, thickness=2.8)
        fill_ellipse(grid, hand_x, hand_y, 1.4, 1.4, MARKER_SKIN)


def draw_body(grid, spec: StageSpec):
    # Kopf + Haut.
    fill_ellipse(grid, 16, spec.head_cy, spec.head_r, spec.head_r, MARKER_SKIN)

    # Rumpf (Gi-Oberteil bzw. Strampler bei der Kleinkind-Stufe).
    half = spec.torso_half_w
    fill_rect(grid, 16 - half, spec.torso_top, 16 + half, spec.torso_bottom, MARKER_CLOTH)
    fill_ellipse(grid, 16, spec.torso_top + 1, half, 2.2, MARKER_CLOTH)  # runde Schultern

    # Beine bzw. Fuesschen.
    if spec.has_legs:
        leg_w = half * 0.42
        for sign in (-1, 1):
            lx = 16 + sign * half * 0.5
            fill_rect(grid, lx - leg_w, spec.torso_bottom - 1, lx + leg_w, spec.leg_bottom, MARKER_CLOTH)
            fill_ellipse(grid, lx, spec.leg_bottom - 0.5, leg_w + 0.6, 1.3, MARKER_SKIN)
        draw_line(grid, 16, spec.torso_bottom - 1, 16, spec.leg_bottom, OUTLINE)  # Beintrennung
    else:
        fill_ellipse(grid, 16 - half * 0.45, spec.leg_bottom - 0.5, 1.6, 1.3, MARKER_SKIN)
        fill_ellipse(grid, 16 + half * 0.45, spec.leg_bottom - 0.5, 1.6, 1.3, MARKER_SKIN)

    draw_arms(grid, spec)

    # V-Ausschnitt des Gi (zwei diagonale Linien) - nur ab der Gi-Stufe
    # (Baby+), das Kleinkind traegt noch keinen Gi.
    if spec.belt_color is not None:
        neck_y = spec.torso_top + 0.5
        draw_line(grid, 16, spec.torso_top + half * 0.7, 16 - half * 0.6, neck_y, OUTLINE)
        draw_line(grid, 16, spec.torso_top + half * 0.7, 16 + half * 0.6, neck_y, OUTLINE)
        # Guertel.
        belt_y = spec.torso_bottom - 2.6
        fill_rect(grid, 16 - half - 0.3, belt_y, 16 + half + 0.3, belt_y + 2.2, spec.belt_color)
        fill_rect(grid, 16 - 1.3, belt_y - 0.3, 16 + 1.3, belt_y + 2.5, tuple(max(0, c - 25) for c in spec.belt_color[:3]) + (255,))

    draw_hair(grid, spec)
    draw_headband(grid, spec)


STAGES = {
    "ei": StageSpec(
        key="ei", head_cy=11, head_r=6.4, hair_style="tuft",
        torso_top=16, torso_bottom=25, torso_half_w=6.0,
        has_legs=False, leg_bottom=27,
        belt_color=None, arm_pose="none",
        headband=False, headband_tails=0,
    ),
    "baby": StageSpec(
        key="baby", head_cy=9.5, head_r=6.1, hair_style="tuft",
        torso_top=14.5, torso_bottom=23, torso_half_w=6.3,
        has_legs=True, leg_bottom=28,
        belt_color=BELT_WHITE, arm_pose="low",
        headband=False, headband_tails=0,
    ),
    "kind": StageSpec(
        key="kind", head_cy=8.5, head_r=6.2, hair_style="cap",
        torso_top=13.5, torso_bottom=22, torso_half_w=6.6,
        has_legs=True, leg_bottom=28.5,
        belt_color=BELT_YELLOW, arm_pose="low",
        headband=False, headband_tails=0,
    ),
    "junior": StageSpec(
        key="junior", head_cy=8, head_r=6.3, hair_style="cap",
        torso_top=13, torso_bottom=21.5, torso_half_w=6.8,
        has_legs=True, leg_bottom=29,
        belt_color=BELT_GREEN, arm_pose="guard",
        headband=True, headband_tails=1,
    ),
    "experte": StageSpec(
        key="experte", head_cy=7.5, head_r=6.4, hair_style="spiky",
        torso_top=12.5, torso_bottom=21, torso_half_w=7.0,
        has_legs=True, leg_bottom=29.5,
        belt_color=BELT_BROWN, arm_pose="punch",
        headband=True, headband_tails=2,
    ),
    "meister": StageSpec(
        key="meister", head_cy=7, head_r=6.5, hair_style="spiky",
        torso_top=12, torso_bottom=20.5, torso_half_w=7.2,
        has_legs=True, leg_bottom=30,
        belt_color=BELT_BLACK, arm_pose="fists_up",
        headband=True, headband_tails=2,
    ),
}


def eye_position(spec: StageSpec):
    return 16, spec.head_cy - spec.head_r * 0.05, max(1.8, spec.head_r * 0.42)


def mouth_position(spec: StageSpec):
    return 16, spec.head_cy + spec.head_r * 0.42


def build_variant(spec: StageSpec, mood: str) -> list:
    grid = new_grid()
    draw_body(grid, spec)

    cx, ey, spread = eye_position(spec)
    mx, my = mouth_position(spec)

    if mood == "sad":
        draw_eyes_sad(grid, cx, ey, spread)
        draw_mouth_sad(grid, mx, my)
    elif mood == "blink":
        draw_eyes_blink(grid, cx, ey, spread)
        if spec.arm_pose in ("punch", "fists_up"):
            draw_mouth_determined(grid, mx, my)
        else:
            draw_mouth_smile(grid, mx, my)
    else:
        draw_eyes_open(grid, cx, ey, spread)
        if spec.arm_pose in ("punch", "fists_up"):
            draw_mouth_determined(grid, mx, my)
        else:
            draw_mouth_smile(grid, mx, my)
        blush(grid, cx, my - 0.5, spread + 1.6)

    add_outline(grid)
    return grid


def main():
    for key, spec in STAGES.items():
        for mood, suffix in (("idle", "idle1"), ("blink", "idle2"), ("sad", "sad")):
            grid = build_variant(spec, mood)
            save_png(grid, f"{key}_{suffix}.png")


if __name__ == "__main__":
    main()
