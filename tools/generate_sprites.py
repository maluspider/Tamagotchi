#!/usr/bin/env python3
"""Erzeugt die Charakter-Sprite-PNGs unter sdcard/sprites/character/.

Reproduzierbare Asset-Pipeline statt handgemalter Dateien: jede Form wird
aus einfachen Ellipsen/Rechtecken auf einem 32x32-Pixelraster zusammengesetzt
(docs/projektplan.md Abschnitt 4: "32x32px Standard", chibi/SD-Stil, dunkle
Outlines, begrenzte Palette). Erneut ausfuehrbar mit `python3
tools/generate_sprites.py`, falls Farben/Formen spaeter angepasst werden.

Benoetigt Pillow (`pip install pillow`) nur fuer den PNG-Export; das
eigentliche Zeichnen passiert pixelweise auf einem eigenen Raster, nicht
ueber PIL-Primitiven, damit keine Anti-Aliasing-Kanten entstehen (echter
Pixel-Art-Look).
"""

from __future__ import annotations

import math
import os
from dataclasses import dataclass

from PIL import Image

GRID = 32
TRANSPARENT = (0, 0, 0, 0)
OUTLINE = (35, 26, 20, 255)
OUTLINE_SOFT = (70, 52, 40, 255)  # fuer innere Konturlinien (z.B. Ei-Riss)

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "sdcard", "sprites", "character")


def new_grid():
    return [[TRANSPARENT for _ in range(GRID)] for _ in range(GRID)]


def set_px(grid, x, y, color):
    if 0 <= x < GRID and 0 <= y < GRID:
        grid[y][x] = color


def get_px(grid, x, y):
    if 0 <= x < GRID and 0 <= y < GRID:
        return grid[y][x]
    return TRANSPARENT


def fill_ellipse(grid, cx, cy, rx, ry, color, y_min=None, y_max=None):
    for y in range(GRID):
        if y_min is not None and y < y_min:
            continue
        if y_max is not None and y > y_max:
            continue
        for x in range(GRID):
            dx = (x + 0.5 - cx) / rx
            dy = (y + 0.5 - cy) / ry
            if dx * dx + dy * dy <= 1.0:
                grid[y][x] = color


def fill_rect(grid, x0, y0, x1, y1, color):
    for y in range(y0, y1):
        for x in range(x0, x1):
            set_px(grid, x, y, color)


def draw_line(grid, x0, y0, x1, y1, color):
    steps = int(round(max(abs(x1 - x0), abs(y1 - y0), 1)))
    for i in range(steps + 1):
        t = i / steps
        x = round(x0 + (x1 - x0) * t)
        y = round(y0 + (y1 - y0) * t)
        set_px(grid, x, y, color)


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
    # Das Zielgeraet blendet Alpha gegen den aktuellen Bildschirminhalt, nicht
    # gegen die Koerperfarbe - Halbtransparenz wuerde je nach Hintergrund
    # unterschiedlich (und meist falsch) aussehen. Alle Formen hier nutzen
    # daher bewusst nur volle Deckung oder volle Transparenz.
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


# --- Augen/Mund als wiederverwendbare Bausteine -----------------------------

def draw_eyes_open(grid, cx, eye_y, spread, pupil_color=(30, 24, 20, 255)):
    for sign in (-1, 1):
        ex = cx + sign * spread
        fill_ellipse(grid, ex, eye_y, 2.4, 2.8, (255, 255, 255, 255))
        fill_ellipse(grid, ex + (1 if sign > 0 else -1) * 0.4, eye_y + 0.4, 1.1, 1.3, pupil_color)


def draw_eyes_blink(grid, cx, eye_y, spread, color=(35, 26, 20, 255)):
    for sign in (-1, 1):
        ex = cx + sign * spread
        draw_line(grid, ex - 2, eye_y, ex + 2, eye_y, color)


def draw_eyes_sad(grid, cx, eye_y, spread, pupil_color=(30, 24, 20, 255)):
    # Traurige Augen: kleinere Pupillen, "besorgte" Brauen (innere Ecke zur
    # Nase hin angehoben, aeussere Ecke abgesenkt - klassischer trauriger
    # Ausdruck, nicht zu verwechseln mit zusammengezogenen "wuetenden"
    # Brauen). Ausloeser ist nie eine falsche Antwort, nur Inaktivitaet -
    # siehe CharacterEngine.h.
    for sign in (-1, 1):
        ex = cx + sign * spread
        fill_ellipse(grid, ex, eye_y + 1, 2.0, 2.2, (255, 255, 255, 255))
        fill_ellipse(grid, ex, eye_y + 1.6, 0.9, 1.0, pupil_color)
        if sign < 0:
            draw_line(grid, ex - 2, eye_y + 1, ex + 2, eye_y - 2, (35, 26, 20, 255))
        else:
            draw_line(grid, ex - 2, eye_y - 2, ex + 2, eye_y + 1, (35, 26, 20, 255))


def draw_mouth_smile(grid, cx, y, color=(35, 26, 20, 255)):
    draw_line(grid, cx - 2, y, cx - 1, y + 1, color)
    draw_line(grid, cx - 1, y + 1, cx + 1, y + 1, color)
    draw_line(grid, cx + 1, y + 1, cx + 2, y, color)


def draw_mouth_sad(grid, cx, y, color=(35, 26, 20, 255)):
    draw_line(grid, cx - 2, y + 1, cx - 1, y, color)
    draw_line(grid, cx - 1, y, cx + 1, y, color)
    draw_line(grid, cx + 1, y, cx + 2, y + 1, color)


def blush(grid, cx, y, spread):
    # Volldeckend statt halbtransparent: das Zielgeraet blendet PNG-Alpha
    # gegen den jeweiligen Bildschirmhintergrund, nicht gegen die Koerperfarbe
    # - ein halbtransparenter Rosaton wuerde dort zu dunklen Raendern statt
    # zu roten Waengchen fuehren.
    for sign in (-1, 1):
        fill_ellipse(grid, cx + sign * spread, y, 1.6, 1.0, (255, 150, 150, 255))


@dataclass
class StageSpec:
    key: str
    body_color: tuple
    shade_color: tuple
    has_head_tier: bool
    body_rx: float
    body_ry: float
    body_cy: float
    head_rx: float = 0
    head_ry: float = 0
    head_cy: float = 0
    has_limbs: bool = True
    accessory: str = ""


def draw_body_blob(grid, spec: StageSpec):
    # Untere Koerperform (bei zweistufigen Formen: Rumpf).
    fill_ellipse(grid, 16, spec.body_cy, spec.body_rx, spec.body_ry, spec.body_color)
    # Einfache Schattierung unten rechts als kleine Mondsichel.
    fill_ellipse(
        grid,
        16 + spec.body_rx * 0.35,
        spec.body_cy + spec.body_ry * 0.35,
        spec.body_rx * 0.75,
        spec.body_ry * 0.55,
        spec.shade_color,
        y_min=int(spec.body_cy),
    )
    if spec.has_head_tier:
        fill_ellipse(grid, 16, spec.head_cy, spec.head_rx, spec.head_ry, spec.body_color)
        fill_ellipse(
            grid,
            16 + spec.head_rx * 0.35,
            spec.head_cy + spec.head_ry * 0.3,
            spec.head_rx * 0.7,
            spec.head_ry * 0.5,
            spec.shade_color,
            y_min=int(spec.head_cy),
        )
    if spec.has_limbs:
        foot_y = int(spec.body_cy + spec.body_ry * 0.85)
        fill_ellipse(grid, 16 - spec.body_rx * 0.5, foot_y, 2.4, 1.8, spec.shade_color)
        fill_ellipse(grid, 16 + spec.body_rx * 0.5, foot_y, 2.4, 1.8, spec.shade_color)
        if spec.head_rx:  # ab Kind-Stufe: kleine Arme am Rumpf
            arm_y = int(spec.body_cy - spec.body_ry * 0.1)
            fill_ellipse(grid, 16 - spec.body_rx - 1, arm_y, 1.8, 2.4, spec.body_color)
            fill_ellipse(grid, 16 + spec.body_rx + 1, arm_y, 1.8, 2.4, spec.body_color)


def draw_accessory(grid, spec: StageSpec):
    if spec.accessory == "antenna":
        top = int(spec.head_cy - spec.head_ry)
        draw_line(grid, 16, top, 16, top - 3, (90, 160, 70, 255))
        fill_ellipse(grid, 16, top - 4, 1.6, 1.6, (140, 220, 110, 255))
    elif spec.accessory == "cape":
        neck_y = int(spec.body_cy - spec.body_ry * 0.6)
        cape_color = (90, 110, 220, 255)
        for i, y in enumerate(range(neck_y, neck_y + 5)):
            half_w = 5 - i
            fill_rect(grid, 16 - half_w, y, 16 + half_w, y + 1, cape_color)
        # Kleiner Stern auf der Brust als Fortschritts-Abzeichen.
        star_y = int(spec.body_cy)
        star_color = (255, 224, 120, 255)
        set_px(grid, 16, star_y - 1, star_color)
        set_px(grid, 16, star_y + 1, star_color)
        set_px(grid, 16 - 1, star_y, star_color)
        set_px(grid, 16 + 1, star_y, star_color)
        set_px(grid, 16, star_y, star_color)
    elif spec.accessory == "crown":
        top = int(spec.head_cy - spec.head_ry)
        gold = (255, 215, 0, 255)
        fill_rect(grid, 16 - 5, top - 2, 16 + 5, top + 1, gold)
        for dx in (-5, -1, 3):
            fill_rect(grid, 16 + dx, top - 5, 16 + dx + 3, top - 1, gold)
        fill_ellipse(grid, 16, top - 5, 0.8, 0.8, (255, 90, 90, 255))
        # Kleine Glanzpunkte neben dem Kopf (volldeckend - siehe blush()
        # weiter oben zur Begruendung, warum hier keine Transparenz genutzt
        # wird).
        set_px(grid, int(16 - spec.head_rx - 3), int(spec.head_cy - 2), (255, 255, 255, 255))
        set_px(grid, int(16 + spec.head_rx + 3), int(spec.head_cy + 1), (255, 255, 255, 255))


STAGES = {
    "ei": StageSpec(
        key="ei",
        body_color=(245, 238, 214, 255),
        shade_color=(214, 199, 160, 255),
        has_head_tier=False,
        body_rx=8.5,
        body_ry=10.5,
        body_cy=18,
        has_limbs=False,
    ),
    "baby": StageSpec(
        key="baby",
        body_color=(255, 224, 102, 255),
        shade_color=(224, 181, 62, 255),
        has_head_tier=False,
        body_rx=9.5,
        body_ry=9.5,
        body_cy=18,
        has_limbs=True,
    ),
    "kind": StageSpec(
        key="kind",
        body_color=(129, 216, 89, 255),
        shade_color=(88, 168, 58, 255),
        has_head_tier=True,
        body_rx=8.5,
        body_ry=7.5,
        body_cy=21,
        head_rx=7.0,
        head_ry=7.0,
        head_cy=11,
        has_limbs=True,
    ),
    "junior": StageSpec(
        key="junior",
        body_color=(79, 215, 232, 255),
        shade_color=(38, 165, 186, 255),
        has_head_tier=True,
        body_rx=8.5,
        body_ry=8.0,
        body_cy=21,
        head_rx=7.0,
        head_ry=6.8,
        head_cy=10,
        has_limbs=True,
        accessory="antenna",
    ),
    "experte": StageSpec(
        key="experte",
        body_color=(255, 162, 62, 255),
        shade_color=(214, 118, 30, 255),
        has_head_tier=True,
        body_rx=9.0,
        body_ry=8.3,
        body_cy=21,
        head_rx=7.2,
        head_ry=7.0,
        head_cy=10,
        has_limbs=True,
        accessory="cape",
    ),
    "meister": StageSpec(
        key="meister",
        body_color=(255, 127, 192, 255),
        shade_color=(214, 80, 150, 255),
        has_head_tier=True,
        body_rx=9.5,
        body_ry=8.6,
        body_cy=20,
        head_rx=7.5,
        head_ry=7.2,
        head_cy=9,
        has_limbs=True,
        accessory="crown",
    ),
}


def eye_position(spec: StageSpec):
    if spec.has_head_tier:
        return 16, int(spec.head_cy), max(2.2, spec.head_rx * 0.35)
    return 16, int(spec.body_cy - spec.body_ry * 0.15), spec.body_rx * 0.35


def mouth_position(spec: StageSpec):
    if spec.has_head_tier:
        return 16, int(spec.head_cy + spec.head_ry * 0.45)
    return 16, int(spec.body_cy + spec.body_ry * 0.25)


def build_variant(spec: StageSpec, mood: str) -> list:
    grid = new_grid()
    draw_body_blob(grid, spec)
    draw_accessory(grid, spec)

    cx, ey, spread = eye_position(spec)
    mx, my = mouth_position(spec)

    if spec.key == "ei":
        # Das Ei bekommt nur ein aufgemaltes Gesicht (kein echter Koerper) -
        # ein kleiner Riss deutet das bevorstehende Schluepfen an.
        if mood == "sad":
            draw_eyes_sad(grid, cx, ey, spread * 0.7)
            draw_mouth_sad(grid, mx, my)
        elif mood == "blink":
            draw_eyes_blink(grid, cx, ey, spread * 0.7)
            draw_mouth_smile(grid, mx, my)
        else:
            draw_eyes_open(grid, cx, ey, spread * 0.7)
            draw_mouth_smile(grid, mx, my)
        draw_line(grid, 11, 12, 13, 15, OUTLINE_SOFT)
        draw_line(grid, 13, 15, 12, 18, OUTLINE_SOFT)
    else:
        if mood == "sad":
            draw_eyes_sad(grid, cx, ey, spread)
            draw_mouth_sad(grid, mx, my)
        elif mood == "blink":
            draw_eyes_blink(grid, cx, ey, spread)
            draw_mouth_smile(grid, mx, my)
        else:
            draw_eyes_open(grid, cx, ey, spread)
            draw_mouth_smile(grid, mx, my)
            blush(grid, cx, my - 1, spread + 2)

    add_outline(grid)
    return grid


def main():
    for key, spec in STAGES.items():
        for mood, suffix in (("idle", "idle1"), ("blink", "idle2"), ("sad", "sad")):
            grid = build_variant(spec, mood)
            save_png(grid, f"{key}_{suffix}.png")


if __name__ == "__main__":
    main()
