#include "FighterRenderer.h"

#include <M5Unified.h>
#include <SD.h>

#include <memory>

#include "CharacterTraits.h"
#include "config.h"

namespace {

const char* poseFileName(FighterPose pose) {
    switch (pose) {
        case FighterPose::Idle1: return "idle1.png";
        case FighterPose::Idle2: return "idle2.png";
        case FighterPose::Walk1: return "walk1.png";
        case FighterPose::Walk2: return "walk2.png";
        case FighterPose::Punch: return "punch.png";
        case FighterPose::Kick: return "kick.png";
        case FighterPose::Hurt: return "hurt.png";
        case FighterPose::Ko: return "ko.png";
    }
    return "idle1.png";
}

// Fuss-Kontaktzeile im 56x64-Raster (siehe tools/generate_fighter_sprites.py,
// Pose-Standardwerte back_leg/front_leg) - fuer JEDE Standpose identisch,
// damit draw() den Kaempfer unabhaengig von der aktuellen Pose auf
// derselben Boden-Hoehe verankern kann.
constexpr int kFootRow = 58;

} // namespace

FighterRenderer::FighterRenderer() : canvas_(&M5.Display) {}

void FighterRenderer::ensureSprite() {
    if (spriteReady_) {
        return;
    }
    canvas_.setColorDepth(16);
    canvas_.createSprite(config::kFighterSourceWidthPx, config::kFighterSourceHeightPx);
    // Explizit setzen statt sich auf den Default zu verlassen: bei einem
    // NICHT quadratischen Sprite (56x64, anders als CharacterRenderers
    // 32x32) soll pushRotateZoom() unmissverstaendlich um den geometrischen
    // Mittelpunkt spiegeln/skalieren - das haelt draw()s footOffset-Rechnung
    // (Fusszeile relativ zur Mitte) unabhaengig von der LovyanGFX-Version
    // korrekt.
    canvas_.setPivot(config::kFighterSourceWidthPx / 2.0f, config::kFighterSourceHeightPx / 2.0f);
    spriteReady_ = true;
}

void FighterRenderer::applyColors(uint16_t skinColor, uint16_t hairColor, uint16_t giColor) {
    // Direkter Pufferzugriff statt readPixel()/writePixel() - siehe
    // CharacterRenderer::applyTraitColors() fuer die Begruendung (bei
    // setColorDepth(16) ein dicht gepacktes RGB565-Array).
    uint16_t* buf = static_cast<uint16_t*>(canvas_.getBuffer());
    const int total = config::kFighterSourceWidthPx * config::kFighterSourceHeightPx;
    for (int i = 0; i < total; ++i) {
        const uint16_t px = buf[i];
        if (px == traits::kSkinMarker) {
            buf[i] = skinColor;
        } else if (px == traits::kHairMarker) {
            buf[i] = hairColor;
        } else if (px == traits::kClothMarker) {
            buf[i] = giColor;
        }
    }
}

bool FighterRenderer::draw(FighterPose pose, uint16_t skinColor, uint16_t hairColor, uint16_t giColor,
                            bool facingRight, int cx, int groundY, float scale, LovyanGFX* target) {
    const String path = String(config::kSpriteFighterDir) + poseFileName(pose);
    if (!SD.exists(path)) {
        return false;
    }

    // Byte-Puffer-Pfad statt canvas_.drawPngFile(SD, ...) - siehe
    // CharacterRenderer::draw() fuer die ausfuehrliche Begruendung (fehlende
    // DataWrapperT<fs::SDFS>-Spezialisierung in dieser M5GFX-Version).
    File file = SD.open(path, FILE_READ);
    if (!file) {
        return false;
    }
    const size_t fileSize = file.size();
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[fileSize]);
    const int bytesRead = file.read(buffer.get(), fileSize);
    file.close();
    if (bytesRead < 0 || static_cast<size_t>(bytesRead) != fileSize) {
        return false;
    }

    ensureSprite();
    canvas_.fillSprite(traits::kTransparentKey);
    canvas_.drawPng(buffer.get(), fileSize, 0, 0);
    applyColors(skinColor, hairColor, giColor);

    // Pivot bleibt bewusst auf dem Sprite-Mittelpunkt (M5Canvas-Default) -
    // der feste Offset von der Mitte zur Fusszeile (kFootRow) haelt den
    // Kaempfer posen-unabhaengig auf derselben Bodenhoehe verankert.
    const float footOffset = static_cast<float>(kFootRow - config::kFighterSourceHeightPx / 2);
    const int cyTarget = groundY - static_cast<int>(footOffset * scale);
    canvas_.pushRotateZoom(target, cx, cyTarget, 0.0f, facingRight ? scale : -scale, scale, traits::kTransparentKey);
    return true;
}
