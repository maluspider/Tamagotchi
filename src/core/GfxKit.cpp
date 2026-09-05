#include "GfxKit.h"

#include <cmath>

namespace gfxkit {

namespace {
float clamp01(float v) {
    if (v < 0.0f) {
        return 0.0f;
    }
    if (v > 1.0f) {
        return 1.0f;
    }
    return v;
}
} // namespace

uint16_t lighten(uint16_t color, float amount) {
    amount = clamp01(amount);
    const int r = (color >> 11) & 0x1F;
    const int g = (color >> 5) & 0x3F;
    const int b = color & 0x1F;
    const int nr = r + static_cast<int>((31 - r) * amount);
    const int ng = g + static_cast<int>((63 - g) * amount);
    const int nb = b + static_cast<int>((31 - b) * amount);
    return static_cast<uint16_t>((nr << 11) | (ng << 5) | nb);
}

uint16_t darken(uint16_t color, float amount) {
    amount = clamp01(amount);
    const int r = (color >> 11) & 0x1F;
    const int g = (color >> 5) & 0x3F;
    const int b = color & 0x1F;
    const int nr = static_cast<int>(r * (1.0f - amount));
    const int ng = static_cast<int>(g * (1.0f - amount));
    const int nb = static_cast<int>(b * (1.0f - amount));
    return static_cast<uint16_t>((nr << 11) | (ng << 5) | nb);
}

uint16_t lerp(uint16_t from, uint16_t to, float t) {
    t = clamp01(t);
    const int fr = (from >> 11) & 0x1F, fg = (from >> 5) & 0x3F, fb = from & 0x1F;
    const int tr = (to >> 11) & 0x1F, tg = (to >> 5) & 0x3F, tb = to & 0x1F;
    const int r = fr + static_cast<int>((tr - fr) * t);
    const int g = fg + static_cast<int>((tg - fg) * t);
    const int b = fb + static_cast<int>((tb - fb) * t);
    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

void verticalGradient(LovyanGFX* target, int x, int y, int w, int h, uint16_t topColor, uint16_t bottomColor) {
    if (w <= 0 || h <= 0) {
        return;
    }
    const int denom = (h > 1) ? (h - 1) : 1;
    for (int row = 0; row < h; ++row) {
        const float t = static_cast<float>(row) / static_cast<float>(denom);
        target->drawLine(x, y + row, x + w - 1, y + row, lerp(topColor, bottomColor, t));
    }
}

void bevelPanel(LovyanGFX* target, int x, int y, int w, int h, int r, uint16_t base, bool raised) {
    target->fillRoundRect(x, y, w, h, r, base);
    if (w <= 4 || h <= 4) {
        return;
    }

    const uint16_t hi = raised ? lighten(base, 0.45f) : darken(base, 0.35f);
    const uint16_t lo = raised ? darken(base, 0.35f) : lighten(base, 0.45f);
    const int rr = r > 2 ? r : 2;

    // Helle Kante oben/links, dunkle Kante unten/rechts (bzw. umgekehrt bei
    // raised=false) - kurze, leicht eingerueckte Linienzuege statt eines
    // zweiten vollen Rechtecks, damit sie den abgerundeten Radius nicht
    // eckig durchschneiden.
    target->drawLine(x + rr, y + 1, x + w - rr, y + 1, hi);
    target->drawLine(x + 1, y + rr, x + 1, y + h - rr, hi);
    target->drawLine(x + rr, y + h - 2, x + w - rr, y + h - 2, lo);
    target->drawLine(x + w - 2, y + rr, x + w - 2, y + h - rr, lo);
}

void starfield(LovyanGFX* target, int width, int maxY, int count, uint16_t color, int xOffset, int yOffset) {
    if (width <= 0 || maxY <= 0 || count <= 0) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        // Einfache Hash-Streuung statt einer echten RNG - liefert bei
        // gleichem `i` immer dieselbe Position, damit Screens, die jeden
        // Frame neu zeichnen (z. B. HomeScreen), ein stehendes Sternenfeld
        // statt "Rauschen" zeigen.
        const uint32_t h1 = static_cast<uint32_t>(i) * 2654435761u;
        const uint32_t h2 = static_cast<uint32_t>(i) * 40503u + 12345u;
        const int sx = xOffset + static_cast<int>(h1 % static_cast<uint32_t>(width));
        const int sy = yOffset + static_cast<int>(h2 % static_cast<uint32_t>(maxY));
        const int size = (i % 3 == 0) ? 2 : 1;
        target->fillRect(sx, sy, size, size, color);
    }
}

void hillsSilhouette(LovyanGFX* target, int width, int baseY, int peakHeight, int count, uint16_t color) {
    if (width <= 0 || count <= 0 || peakHeight <= 0) {
        return;
    }
    const float step = static_cast<float>(width) / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        const float cx = step * (static_cast<float>(i) + 0.5f);
        // Feste, leicht unregelmaessige Hoehenformel je Huegel - kein
        // echtes Rauschen noetig, liefert aber eine organisch wirkende
        // Silhouette, die bei jedem Redraw identisch bleibt (siehe
        // starfield() fuer dieselbe Ueberlegung).
        const float h = static_cast<float>(peakHeight) * (0.55f + 0.45f * sinf(static_cast<float>(i) * 2.4f + 1.3f));
        const int px = static_cast<int>(cx);
        const int py = baseY - static_cast<int>(h);
        const int halfBase = static_cast<int>(step * 0.7f) + 1;
        target->fillTriangle(px - halfBase, baseY, px + halfBase, baseY, px, py, color);
    }
}

void shinyBall(LovyanGFX* target, int cx, int cy, int r, uint16_t color) {
    target->fillCircle(cx, cy, r, color);
    target->drawCircle(cx, cy, r, darken(color, 0.4f));
    const int hr = r > 3 ? r / 3 : 1;
    target->fillCircle(cx - r / 3, cy - r / 3, hr, lighten(color, 0.6f));
}

} // namespace gfxkit
