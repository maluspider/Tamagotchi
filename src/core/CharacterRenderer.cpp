#include "CharacterRenderer.h"

#include <M5Unified.h>
#include <SD.h>

#include <memory>

#include "CharacterTraits.h"
#include "config.h"

namespace {
constexpr int kSourceSize = 32; // deckt sich mit config::kSpriteSourceSizePx
} // namespace

CharacterRenderer::CharacterRenderer() : canvas_(&M5.Display) {}

void CharacterRenderer::ensureSprite() {
    if (spriteReady_) {
        return;
    }
    canvas_.setColorDepth(16);
    canvas_.createSprite(kSourceSize, kSourceSize);
    spriteReady_ = true;
}

void CharacterRenderer::applyTraitColors(const Profile& profile) {
    const uint16_t skinColor = traits::kSkinTones[profile.skinToneIndex % traits::kSkinToneCount].color565;
    const uint16_t hairColor = traits::kHairColors[profile.hairColorIndex % traits::kHairColorCount].color565;
    const uint16_t clothColor = traits::kClothingColors[profile.clothingColorIndex % traits::kClothingColorCount].color565;

    // Direkter Zugriff auf den Sprite-Puffer statt readPixel()/writePixel():
    // bei setColorDepth(16) ist der Puffer garantiert ein dicht gepacktes
    // uint16_t-Array (RGB565, keine Zeilen-Auffuellung bei dieser
    // Groesse) - so bleibt der Farbvergleich exakt und unabhaengig von
    // eventuellen Farbraum-Konvertierungen einer Lese-/Schreib-Hilfsfunktion.
    uint16_t* buf = static_cast<uint16_t*>(canvas_.getBuffer());
    const int total = kSourceSize * kSourceSize;
    for (int i = 0; i < total; ++i) {
        const uint16_t px = buf[i];
        if (px == traits::kSkinMarker) {
            buf[i] = skinColor;
        } else if (px == traits::kHairMarker) {
            buf[i] = hairColor;
        } else if (px == traits::kClothMarker) {
            buf[i] = clothColor;
        }
    }
}

bool CharacterRenderer::draw(CharacterStage stage, const char* mood, const Profile& profile, int cx, int cy, float scale) {
    const String path = String(config::kSpriteCharacterDir) + CharacterEngine::stageAssetKey(stage) + "_" + mood + ".png";
    if (!SD.exists(path)) {
        // Kein Sprite auf der SD-Karte (oder Karte fehlt) - Aufrufer
        // zeichnet stattdessen eine Platzhalter-Grafik.
        return false;
    }

    // Datei selbst einlesen statt canvas_.drawPngFile(SD, path, ...) zu
    // nutzen: M5GFX's dateisystem-generische Ueberladung braucht dafuer
    // eine DataWrapperT<T>-Spezialisierung fuer den exakten Typ von SD
    // (fs::SDFS) - die existiert in dieser M5GFX-Version nicht (nur fuer
    // die SdFat-Bibliothek), das fuehrt zu einem "abstract type"-
    // Kompilierfehler. Der Byte-Puffer-Pfad (drawPng()) braucht dagegen
    // keine Dateisystem-Spezialisierung und funktioniert immer. Die
    // Sprite-Dateien sind winzig (<1 KB, 32x32 mit begrenzter Palette),
    // komplett in den RAM lesen ist unproblematisch.
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
    // Vor dem Dekodieren mit der Transparenz-Markerfarbe fuellen: der
    // 16bpp-Canvas-Puffer hat keinen Alpha-Kanal, daher blendet drawPng
    // transparente Quellpixel gegen das, was bereits im Puffer steht. Diese
    // Fuellfarbe dient anschliessend auch pushRotateZoom() als Farbschluessel
    // fuer den transparenten Ausschnitt beim Zeichnen auf den Bildschirm.
    canvas_.fillSprite(traits::kTransparentKey);
    canvas_.drawPng(buffer.get(), fileSize, 0, 0);
    applyTraitColors(profile);
    canvas_.pushRotateZoom(cx, cy, 0.0f, scale, scale, traits::kTransparentKey);
    return true;
}
