#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/CharacterRenderer.h"
#include "../core/CharacterTraits.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// "Aussehen"-Screen (docs/projektplan.md Abschnitt 4/9): live Vorschau des
// Charakter-Sprites plus drei Zeilen (Haut/Haare/Kleidung) mit </>-Pfeilen
// zum Durchblaettern der jeweiligen Farbvoreinstellungen
// (CharacterTraits.h). Aenderungen wirken sofort auf die Vorschau und
// werden sofort persistiert (wie SettingsScreen::adjustDailyLimit()).
// Bewusst NICHT Eltern-PIN-geschuetzt und direkt aus dem Alltagsfunktionen-
// Menue erreichbar - reine Charakter-Optik ist kein Eltern-Kontrollthema.
class CharacterCustomizeScreen : public Screen {
public:
    CharacterCustomizeScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    static constexpr int kPreviewCy = 76;
    static constexpr float kPreviewScale = 2.6f;
    static constexpr int kRowHeight = 36;
    static constexpr int kRow0Y = 126;
    static constexpr int kRow1Y = 164;
    static constexpr int kRow2Y = 202;
    static constexpr int kArrowLeftX1 = 120;
    static constexpr int kArrowLeftX2 = 150;
    static constexpr int kSwatchX1 = 154;
    static constexpr int kSwatchX2 = 182;
    static constexpr int kArrowRightX1 = 186;
    static constexpr int kArrowRightX2 = 216;

    void handleInput();
    void cycleIndex(uint8_t& index, uint8_t count, int x);
    void drawTraitRow(int y, const char* label, const traits::Preset& preset) const;
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
    CharacterRenderer renderer_;
};
