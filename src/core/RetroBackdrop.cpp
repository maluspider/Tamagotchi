#include "RetroBackdrop.h"

#include <cmath>

#include "Theme.h"

namespace retrobackdrop {

namespace {
constexpr int kSunRadius = 34;
constexpr int kGapBandCount = 3;
constexpr int kFanLineCount = 8; // Anzahl Faecher-Linien rechts/links der Mitte
constexpr int kFloorLineCount = 10;

uint16_t lerpColor(uint16_t from, uint16_t to, float t) {
    const int fr = (from >> 11) & 0x1F, fg = (from >> 5) & 0x3F, fb = from & 0x1F;
    const int tr = (to >> 11) & 0x1F, tg = (to >> 5) & 0x3F, tb = to & 0x1F;
    const int r = fr + static_cast<int>((tr - fr) * t);
    const int g = fg + static_cast<int>((tg - fg) * t);
    const int b = fb + static_cast<int>((tb - fb) * t);
    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}
} // namespace

void drawSynthwaveGrid(LovyanGFX* target, int width, int height, int horizonY, bool showSun) {
    const int sunCx = width / 2;
    const int sunCy = horizonY - 20;

    // Sonne: Gold->Pink-Verlauf in duennen waagrechten Streifen (klassischer
    // Retrowave-Sonnenuntergang), plus ein paar Luecken-Baender im unteren
    // Drittel fuer den typischen "zerschnittenen Sonne"-Look.
    if (showSun) {
        for (int dy = -kSunRadius; dy <= kSunRadius; ++dy) {
            const int halfChord = static_cast<int>(sqrtf(static_cast<float>(kSunRadius * kSunRadius - dy * dy)));
            if (halfChord <= 0) {
                continue;
            }
            const float t = static_cast<float>(dy + kSunRadius) / static_cast<float>(kSunRadius * 2);
            const int rowY = sunCy + dy;
            target->drawLine(sunCx - halfChord, rowY, sunCx + halfChord, rowY,
                              lerpColor(theme::kAccentGold, theme::kAccentPink, t));
        }
        for (int i = 1; i <= kGapBandCount; ++i) {
            const int dy = kSunRadius - (i * kSunRadius) / (kGapBandCount + 2);
            const int halfChord = static_cast<int>(sqrtf(static_cast<float>(kSunRadius * kSunRadius - dy * dy)));
            const int rowY = sunCy + dy;
            target->drawLine(sunCx - halfChord, rowY, sunCx + halfChord, rowY, theme::kBackground);
        }
    }

    // Horizontlinie schneidet bewusst durch den unteren Sonnenrand.
    target->drawLine(0, horizonY, width, horizonY, theme::kAccentCyan);

    // Perspektivisches Bodengitter unterhalb des Horizonts: Fluchtpunkt in
    // der Sonnenmitte, Faecher-Linien laufen zum unteren Bildschirmrand
    // auseinander, waagrechte Linien ruecken zum Horizont hin enger
    // zusammen (quadratische Staffelung simuliert Perspektive).
    for (int i = -kFanLineCount; i <= kFanLineCount; ++i) {
        const int xBottom = sunCx + i * (width / kFanLineCount);
        target->drawLine(sunCx, horizonY, xBottom, height, theme::kPanelLight);
    }
    for (int i = 1; i <= kFloorLineCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kFloorLineCount);
        const int y = horizonY + static_cast<int>(static_cast<float>(height - horizonY) * t * t);
        if (y >= height) {
            break;
        }
        target->drawLine(0, y, width, y, theme::kPanelLight);
    }
}

} // namespace retrobackdrop
