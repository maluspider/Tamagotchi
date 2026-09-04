#include "CharacterCustomizeScreen.h"

#include <M5Unified.h>

#include "../core/ScreenId.h"
#include "../core/Theme.h"
#include "../core/storage/ProfileStore.h"

namespace {
constexpr int kHomeIconSize = 28;
} // namespace

CharacterCustomizeScreen::CharacterCustomizeScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void CharacterCustomizeScreen::onEnter() {
    draw();
}

void CharacterCustomizeScreen::update(uint32_t) {
    handleInput();
}

bool CharacterCustomizeScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void CharacterCustomizeScreen::cycleIndex(uint8_t& index, uint8_t count, int x) {
    if (x >= kArrowLeftX1 && x < kArrowLeftX2) {
        index = static_cast<uint8_t>((index + count - 1) % count);
    } else if (x >= kArrowRightX1 && x < kArrowRightX2) {
        index = static_cast<uint8_t>((index + 1) % count);
    } else {
        return;
    }
    profilestore::save(app_.profile);
}

void CharacterCustomizeScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::AlltagMenu);
        return;
    }

    const int x = touch.x;
    const int y = touch.y;

    if (y >= kRow0Y && y < kRow0Y + kRowHeight) {
        cycleIndex(app_.profile.skinToneIndex, traits::kSkinToneCount, x);
    } else if (y >= kRow1Y && y < kRow1Y + kRowHeight) {
        cycleIndex(app_.profile.hairColorIndex, traits::kHairColorCount, x);
    } else if (y >= kRow2Y && y < kRow2Y + kRowHeight) {
        cycleIndex(app_.profile.clothingColorIndex, traits::kClothingColorCount, x);
    }

    draw();
}

void CharacterCustomizeScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void CharacterCustomizeScreen::drawTraitRow(int y, const char* label, const traits::Preset& preset) const {
    M5.Display.drawRoundRect(4, y, 312, kRowHeight - 4, 8, theme::kPanelLight);

    const int midY = y + (kRowHeight - 4) / 2;

    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString(label, 14, midY);

    M5.Display.fillTriangle(kArrowLeftX2 - 4, midY, kArrowLeftX1 + 4, midY - 7, kArrowLeftX1 + 4, midY + 7, theme::kAccentCyan);
    M5.Display.fillTriangle(kArrowRightX1 + 4, midY, kArrowRightX2 - 4, midY - 7, kArrowRightX2 - 4, midY + 7, theme::kAccentCyan);

    M5.Display.fillRoundRect(kSwatchX1, y + 5, kSwatchX2 - kSwatchX1, kRowHeight - 14, 4, preset.color565);
    M5.Display.drawRoundRect(kSwatchX1, y + 5, kSwatchX2 - kSwatchX1, kRowHeight - 14, 4, theme::kOutline);

    M5.Display.setTextColor(theme::kTextDim);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    M5.Display.drawString(preset.name, kArrowRightX2 + 6, midY);
}

void CharacterCustomizeScreen::draw() {
    M5.Display.fillScreen(theme::kBackground);
    M5.Display.fillRect(0, 0, M5.Display.width(), 26, theme::kPanel);
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Aussehen", 6, 3);

    const CharacterStage stage = app_.character.stage();
    if (!renderer_.draw(stage, "idle1", app_.profile, M5.Display.width() / 2, kPreviewCy, kPreviewScale)) {
        // Fallback, falls kein Sprite existiert (SD-Karte fehlt) - die
        // Vorschau bleibt so trotzdem nachvollziehbar statt leer.
        M5.Display.fillCircle(M5.Display.width() / 2, kPreviewCy, 30, theme::kMuted);
    }

    drawTraitRow(kRow0Y, "Haut", traits::kSkinTones[app_.profile.skinToneIndex % traits::kSkinToneCount]);
    drawTraitRow(kRow1Y, "Haare", traits::kHairColors[app_.profile.hairColorIndex % traits::kHairColorCount]);
    drawTraitRow(kRow2Y, "Kleidung", traits::kClothingColors[app_.profile.clothingColorIndex % traits::kClothingColorCount]);

    drawHomeIcon();
}
