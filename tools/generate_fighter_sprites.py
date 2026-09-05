#!/usr/bin/env python3
"""Erzeugt die Kampf-Modus-Sprite-PNGs unter sdcard/sprites/fighter/.

Nutzer-Feedback ("der streetfighter game ist ein witz...wayto basic...will
etwas was aussieht wie das gameboy streetfighter game...maximal hochwertig
mit echten Animationen und realistischen Grafiken"): KampfModusScreen
zeichnete Kaempfer bisher nur als Rumpf-Rechteck + Kopf-Kreis (ein
"Rumpf-Blob"), keine Gliedmassen, keine Animation. Dieses Skript erzeugt
stattdessen einen echten Seitenansicht-Kampfkuenstler mit Kopf, Rumpf und
JE ZWEI Segmenten pro Arm/Bein (Schulter->Ellbogen->Faust,
Huefte->Knie->Fuss) in 8 Posen (Idle x2 fuers Atmen, Walk x2 fuer den
Schrittwechsel, Schlag, Tritt, Treffer-Reaktion, K.o.) - deutlich naeher am
Look eines waschechten 90er-Kampfspiels (Street Fighter II/Mortal Kombat)
als die alte Blob-Darstellung.

Gleiche Pipeline/Technik wie tools/generate_sprites.py (siehe dort fuer die
ausfuehrliche Begruendung): pixelweise auf einem Raster aus
Ellipsen/Rechtecken/dicken Linien zusammengesetzt (kein Anti-Aliasing -
echter Pixel-Art-Look), Markerfarben statt fester Farben fuer
Haut/Haar/Gi-Farbe (MUESSEN mit traits::kSkinMarker/kHairMarker/
kClothMarker in CharacterTraits.h uebereinstimmen - das Geraet ersetzt sie
zur Laufzeit per Palette-Swap, siehe FighterRenderer.cpp). Der Guertel ist
bewusst NICHT ueber einen Marker anpassbar: Kampf-Modus ist erst ab Stufe
"Meister" spielbar, ein fester schwarzer Guertel passt thematisch zum
bereits erreichten hoechsten Rang (siehe generate_sprites.py, gleiche
Begruendung wie dort fuer Guertel-/Stirnband-Farben).

Der Kaempfer schaut in der Vorlage immer nach RECHTS - FighterRenderer
spiegelt fuer die Gegenrichtung zur Laufzeit ueber pushRotateZoom() mit
negativem zoom_x (kein zweiter Satz Dateien noetig).

Benoetigt Pillow (`pip install pillow`) nur fuer den PNG-Export.

Aufruf ohne Argumente: schreibt die 8 PNGs nach sdcard/sprites/fighter/.
Aufruf mit --preview: schreibt zusaetzlich eine stark hochskalierte
Kontaktabzugs-PNG (Grid aus allen Posen nebeneinander, nearest-neighbor
hochskaliert) zur visuellen Kontrolle waehrend der Entwicklung - wird NICHT
aufs Geraet kopiert, rein ein Hilfsmittel fuer diesen Editier-Durchlauf.
"""

from __future__ import annotations

import os
import sys
from dataclasses import dataclass, field
from typing import Optional

from PIL import Image

GRID_W = 56
GRID_H = 64
TRANSPARENT = (0, 0, 0, 0)

# Muessen exakt mit CharacterTraits.h (traits::kSkinMarker/kHairMarker/
# kClothMarker) uebereinstimmen - siehe Modulkommentar oben.
MARKER_SKIN = (0, 255, 0, 255)
MARKER_HAIR = (0, 255, 255, 255)
MARKER_CLOTH = (255, 255, 0, 255)

OUTLINE = (18, 8, 31, 255)  # deckt sich mit theme::kOutline (Theme.h)
BELT_BLACK = (28, 24, 30, 255)
BELT_BLACK_SHADOW = (12, 10, 14, 255)

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "sdcard", "sprites", "fighter")


def new_grid():
    return [[TRANSPARENT for _ in range(GRID_W)] for _ in range(GRID_H)]


def set_px(grid, x, y, color):
    x, y = int(round(x)), int(round(y))
    if 0 <= x < GRID_W and 0 <= y < GRID_H:
        grid[y][x] = color


def get_px(grid, x, y):
    if 0 <= x < GRID_W and 0 <= y < GRID_H:
        return grid[y][x]
    return TRANSPARENT


def fill_ellipse(grid, cx, cy, rx, ry, color):
    y0 = max(0, int(cy - ry - 1))
    y1 = min(GRID_H, int(cy + ry + 2))
    x0 = max(0, int(cx - rx - 1))
    x1 = min(GRID_W, int(cx + rx + 2))
    for y in range(y0, y1):
        for x in range(x0, x1):
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
    steps = int(round(max(abs(x1 - x0), abs(y1 - y0), 1) * 2))
    for i in range(steps + 1):
        t = i / steps
        x = x0 + (x1 - x0) * t
        y = y0 + (y1 - y0) * t
        if thickness <= 1:
            set_px(grid, x, y, color)
        else:
            half = thickness / 2.0
            for oy in range(-int(half) - 1, int(half) + 2):
                for ox in range(-int(half) - 1, int(half) + 2):
                    if ox * ox + oy * oy <= half * half + 0.5:
                        set_px(grid, x + ox, y + oy, color)


def add_outline(grid, color=OUTLINE):
    to_outline = []
    for y in range(GRID_H):
        for x in range(GRID_W):
            if grid[y][x][3] != 0:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                if get_px(grid, x + dx, y + dy)[3] != 0:
                    to_outline.append((x, y))
                    break
    for x, y in to_outline:
        grid[y][x] = color


def assert_binary_alpha(grid, name):
    for row in grid:
        for _, _, _, a in row:
            assert a in (0, 255), f"{name}: unerwarteter Alpha-Wert {a}"


def save_png(grid, name):
    assert_binary_alpha(grid, name)
    img = Image.new("RGBA", (GRID_W, GRID_H))
    for y in range(GRID_H):
        for x in range(GRID_W):
            img.putpixel((x, y), grid[y][x])
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, name)
    img.save(path)
    print("geschrieben:", path)
    return img


# --- Skelett-Modell ----------------------------------------------------------
# Ein Punkt ist (x, y). Jeder Arm/Bein besteht aus zwei Segmenten
# (Schulter->Ellbogen->Faust bzw. Huefte->Knie->Fuss), damit sichtbar
# angewinkelte Gliedmassen (statt gerader Stelzen) moeglich sind - das ist
# der groesste optische Sprung gegenueber dem alten Rumpf-Blob.


@dataclass
class Pose:
    head_c: tuple = (28, 13)
    head_r: float = 8.5
    torso_cx: float = 28
    torso_top: float = 21
    torso_bottom: float = 40
    torso_half_w: float = 9
    back_leg: tuple = ((25, 40), (22, 50), (20, 58))
    front_leg: tuple = ((31, 40), (35, 50), (38, 58))
    back_arm: tuple = ((24, 23), (19, 30), (18, 36))
    front_arm: tuple = ((32, 23), (38, 27), (40, 21))
    eyes: str = "normal"
    mouth: str = "determined"
    limb_thickness: float = 4.6
    lower_thickness: float = 3.8


def draw_limb(grid, joints, color_upper, color_lower, hand_r, thick_u, thick_l):
    (sx, sy), (ex, ey), (hx, hy) = joints
    draw_line(grid, sx, sy, ex, ey, color_upper, thickness=thick_u)
    draw_line(grid, ex, ey, hx, hy, color_lower, thickness=thick_l)
    fill_ellipse(grid, hx, hy, hand_r, hand_r, MARKER_SKIN)
    # Gelenk-Kappe in der OBEREN Segmentfarbe (nicht color_lower) rundet die
    # Ecke zwischen beiden Liniensegmenten sauber ab, ohne als eigenstaendiger
    # (bei Armen z.B. hautfarbener) Fleck mitten im Aermel aufzufallen.
    fill_ellipse(grid, ex, ey, thick_u * 0.42, thick_u * 0.42, color_upper)


def draw_face(grid, cx, cy, r, style_eyes, style_mouth):
    ex = cx + r * 0.32
    ey = cy - r * 0.05
    spread = r * 0.42
    if style_eyes == "hurt":
        for sign in (-1, 1):
            fx = ex + sign * spread * 0.4
            draw_line(grid, fx - 1.6, ey - 1.6, fx + 1.6, ey + 1.6, OUTLINE, thickness=1.4)
            draw_line(grid, fx - 1.6, ey + 1.6, fx + 1.6, ey - 1.6, OUTLINE, thickness=1.4)
    elif style_eyes == "closed":
        draw_line(grid, ex - spread * 0.6, ey, ex + spread * 0.6, ey - 1, OUTLINE, thickness=1.4)
    else:
        fill_ellipse(grid, ex, ey, 1.8, 2.1, (255, 255, 255, 255))
        fill_ellipse(grid, ex + 0.6, ey + 0.2, 0.95, 1.05, OUTLINE)

    mx, my = cx + r * 0.15, cy + r * 0.5
    if style_mouth == "determined":
        draw_line(grid, mx - 2.2, my, mx + 2.6, my - 0.6, OUTLINE, thickness=1.3)
    elif style_mouth == "open":
        fill_ellipse(grid, mx, my, 1.6, 1.9, OUTLINE)
    elif style_mouth == "grimace":
        draw_line(grid, mx - 2.0, my - 1.2, mx + 2.2, my + 1.0, OUTLINE, thickness=1.3)
        draw_line(grid, mx - 2.0, my + 1.0, mx + 0.4, my - 0.6, OUTLINE, thickness=1.1)


def draw_hair(grid, cx, cy, r):
    # Kurze, nach hinten (links, gegen die Blickrichtung) geblasene Spikes -
    # wirkt bei Kampfhaltung dynamischer als die Tuft/Cap-Stile der
    # Charakter-Sprites. Bewusst auf die OBERE Kopfhaelfte begrenzt (bis
    # knapp ueber die Augenlinie bei cy-r*0.05), damit sie nicht wie ein
    # Augenklappen-Balken ueber das Gesicht faellt.
    fill_ellipse(grid, cx - r * 0.1, cy - r * 0.55, r * 0.95, r * 0.5, MARKER_HAIR)
    fill_ellipse(grid, cx - r * 0.05, cy - r * 0.05, r * 0.9, r * 0.3, TRANSPARENT)
    for dx, dy, h in ((-r * 0.75, -r * 0.65, 3.4), (-r * 0.25, -r * 0.95, 3.0), (r * 0.35, -r * 0.9, 2.2)):
        tip_x = cx + dx - 1.5
        tip_y = cy + dy - h
        fill_ellipse(grid, tip_x, tip_y, 1.3, h * 0.6, MARKER_HAIR)


def draw_pose(pose: Pose) -> list:
    grid = new_grid()

    # Kein gebackener Bodenschatten hier (wuerde Halbtransparenz brauchen,
    # siehe assert_binary_alpha) - FighterRenderer/KampfModusScreen zeichnet
    # den Schatten wie zuvor separat per canvas_.fillEllipse() UNTER dem
    # Sprite.

    # 1) hinteres Bein (teilweise vom Rumpf verdeckt).
    draw_limb(grid, pose.back_leg, MARKER_CLOTH, MARKER_CLOTH, 2.8, pose.limb_thickness, pose.lower_thickness)
    # 2) hinterer Arm (teilweise verdeckt).
    draw_limb(grid, pose.back_arm, MARKER_CLOTH, MARKER_SKIN, 2.5, pose.limb_thickness * 0.85,
              pose.lower_thickness * 0.8)

    # 3) Rumpf (Gi-Oberteil) + Kopf.
    half = pose.torso_half_w
    fill_rect(grid, pose.torso_cx - half, pose.torso_top, pose.torso_cx + half, pose.torso_bottom, MARKER_CLOTH)
    fill_ellipse(grid, pose.torso_cx, pose.torso_top + 1, half, 3.2, MARKER_CLOTH)
    # Guertel (fest schwarz - siehe Modulkommentar).
    belt_y = pose.torso_bottom - 3.2
    fill_rect(grid, pose.torso_cx - half - 0.5, belt_y, pose.torso_cx + half + 0.5, belt_y + 2.6, BELT_BLACK)
    fill_rect(grid, pose.torso_cx - 1.6, belt_y - 0.4, pose.torso_cx + 1.6, belt_y + 3.0, BELT_BLACK_SHADOW)
    # V-Ausschnitt.
    neck_y = pose.torso_top + 0.5
    draw_line(grid, pose.torso_cx, pose.torso_top + half * 0.7, pose.torso_cx - half * 0.6, neck_y, OUTLINE)
    draw_line(grid, pose.torso_cx, pose.torso_top + half * 0.7, pose.torso_cx + half * 0.6, neck_y, OUTLINE)

    hx, hy = pose.head_c
    fill_ellipse(grid, hx, hy, pose.head_r, pose.head_r, MARKER_SKIN)
    draw_hair(grid, hx, hy, pose.head_r)
    draw_face(grid, hx, hy, pose.head_r, pose.eyes, pose.mouth)

    # 4) vorderes Bein (ueber dem Rumpf, voll sichtbar).
    draw_limb(grid, pose.front_leg, MARKER_CLOTH, MARKER_CLOTH, 3.0, pose.limb_thickness, pose.lower_thickness)
    # 5) vorderer Arm (ueber allem - Schlagarm).
    draw_limb(grid, pose.front_arm, MARKER_CLOTH, MARKER_SKIN, 2.8, pose.limb_thickness * 0.9,
              pose.lower_thickness * 0.85)

    add_outline(grid)
    return grid


# --- Posen --------------------------------------------------------------

IDLE1 = Pose()

IDLE2 = Pose(
    head_c=(28, 12),
    torso_top=20, torso_bottom=39,
    back_leg=((25, 39), (22, 49), (20, 57)),
    front_leg=((31, 39), (35, 49), (38, 57)),
    back_arm=((24, 22), (19, 28), (18, 33)),
    front_arm=((32, 22), (38, 25), (41, 18)),
)

WALK1 = Pose(
    # Kampfstellungs-Schlurfschritt statt vollem Schritt (das Original hier
    # war so hoch angewinkelt, dass es beim Bildvergleich wie ein Tritt statt
    # einem Gehschritt aussah) - vorderer Fuss nur leicht vom Boden geloest.
    back_leg=((25, 40), (22, 49), (19, 56)),
    front_leg=((31, 40), (36, 48), (41, 55)),
    back_arm=((24, 23), (20, 28), (21, 34)),
    front_arm=((32, 23), (37, 27), (36, 32)),
)

WALK2 = Pose(
    # Gegenphase zu WALK1: Gewicht auf dem vorderen Bein, hinteres Bein
    # schwingt unter dem Koerper hindurch nach vorn.
    back_leg=((25, 40), (24, 48), (26, 55)),
    front_leg=((31, 40), (33, 50), (34, 58)),
    back_arm=((24, 23), (19, 29), (17, 34)),
    front_arm=((32, 23), (38, 26), (41, 21)),
)

PUNCH = Pose(
    head_c=(30, 13),
    torso_cx=29,
    back_leg=((24, 40), (21, 49), (18, 58)),
    front_leg=((32, 41), (37, 50), (41, 58)),
    back_arm=((23, 25), (17, 31), (14, 35)),
    front_arm=((33, 24), (45, 22), (53, 20)),
    mouth="grimace",
    limb_thickness=4.8,
)

KICK = Pose(
    head_c=(26, 13),
    torso_cx=27,
    back_leg=((24, 41), (22, 50), (20, 58)),
    front_leg=((31, 39), (43, 36), (53, 33)),
    back_arm=((23, 24), (19, 30), (18, 35)),
    front_arm=((30, 24), (34, 29), (36, 33)),
    mouth="grimace",
)

HURT = Pose(
    head_c=(24, 11),
    torso_cx=26,
    torso_top=22, torso_bottom=40,
    back_leg=((23, 41), (19, 49), (15, 56)),
    front_leg=((29, 41), (33, 50), (36, 58)),
    back_arm=((21, 24), (15, 19), (12, 15)),
    front_arm=((29, 23), (35, 17), (39, 13)),
    eyes="hurt",
    mouth="open",
)

# K.o. ist eine eigene, liegende Komposition statt der Stand-Pose - siehe
# Modulkommentar (deutlich andere Silhouette = sofort erkennbar).


def draw_ko() -> list:
    grid = new_grid()

    # Fernes (unteres) Bein, angewinkelt.
    draw_limb(grid, ((38, 54), (46, 48), (50, 53)), MARKER_CLOTH, MARKER_CLOTH, 2.3, 4.4, 3.6)
    # Fernes (unteres) Arm, ausgestreckt liegend.
    draw_limb(grid, ((16, 52), (8, 50), (2, 49)), MARKER_CLOTH, MARKER_SKIN, 2.0, 3.8, 3.2)

    # Liegender Rumpf (breites, niedriges Oval statt hoher Box).
    fill_ellipse(grid, 26, 53, 15, 8, MARKER_CLOTH)
    belt_y = 55
    fill_rect(grid, 14, belt_y, 38, belt_y + 2.4, BELT_BLACK)

    # Kopf, seitlich auf dem Boden.
    fill_ellipse(grid, 45, 55, 7.5, 7.0, MARKER_SKIN)
    draw_hair(grid, 45, 55, 7.5)
    # Bewusstlos: X-Augen statt Gesichtsausdruck.
    for sign in (-1, 1):
        fx = 45 + sign * 2.2
        draw_line(grid, fx - 1.3, 53.5, fx + 1.3, 56.1, OUTLINE, thickness=1.2)
        draw_line(grid, fx - 1.3, 56.1, fx + 1.3, 53.5, OUTLINE, thickness=1.2)
    draw_line(grid, 42, 59, 47, 60, OUTLINE, thickness=1.2)

    # Nahes (oberes) Bein, leicht angewinkelt.
    draw_limb(grid, ((22, 50), (14, 44), (7, 45)), MARKER_CLOTH, MARKER_CLOTH, 2.5, 4.6, 3.8)
    # Nahes (oberes) Arm, quer über die Brust.
    draw_limb(grid, ((30, 48), (22, 45), (16, 49)), MARKER_CLOTH, MARKER_SKIN, 2.2, 4.0, 3.4)

    add_outline(grid)
    return grid


POSES = {
    "idle1": lambda: draw_pose(IDLE1),
    "idle2": lambda: draw_pose(IDLE2),
    "walk1": lambda: draw_pose(WALK1),
    "walk2": lambda: draw_pose(WALK2),
    "punch": lambda: draw_pose(PUNCH),
    "kick": lambda: draw_pose(KICK),
    "hurt": lambda: draw_pose(HURT),
    "ko": draw_ko,
}


def build_preview(images: dict):
    scale = 8
    cols = 4
    rows = 2
    names = list(images.keys())
    sheet = Image.new("RGBA", (GRID_W * scale * cols, GRID_H * scale * rows), (40, 20, 60, 255))
    for i, name in enumerate(names):
        img = images[name].resize((GRID_W * scale, GRID_H * scale), Image.NEAREST)
        col, row = i % cols, i // cols
        sheet.paste(img, (col * GRID_W * scale, row * GRID_H * scale), img)
    path = os.path.join(os.path.dirname(__file__), "fighter_preview.png")
    sheet.save(path)
    print("Vorschau geschrieben:", path)


def main():
    images = {}
    for name, fn in POSES.items():
        grid = fn()
        images[name] = save_png(grid, f"{name}.png")
    if "--preview" in sys.argv:
        build_preview(images)


if __name__ == "__main__":
    main()
