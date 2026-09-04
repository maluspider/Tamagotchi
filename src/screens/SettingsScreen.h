#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Eltern-Einstellungen (docs/projektplan.md Abschnitt 11), nur ueber
// PinEntryScreen erreichbar. Bewusst auf vier Zeilen reduziert (Tageslimit,
// Bonus-Zeit, Nachtmodus an/aus, PIN aendern) - eine UI zum Anpassen der
// Nachtmodus-Start-/Endzeit ist eine bewusste Vereinfachung nicht
// umgesetzt (siehe Abschnitt 16, offener Punkt); Default 20-7 Uhr bleibt
// fest, bis diese UI ergaenzt wird.
class SettingsScreen : public Screen {
public:
    SettingsScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    static constexpr int kRowHeight = 44;
    static constexpr int kRow0Y = 30;
    static constexpr int kRow1Y = 78;
    static constexpr int kRow2Y = 126;
    static constexpr int kRow3Y = 174;
    static constexpr int kMinusX1 = 170;
    static constexpr int kMinusX2 = 210;
    static constexpr int kPlusX1 = 250;
    static constexpr int kPlusX2 = 300;
    static constexpr uint16_t kMinDailyLimit = 15;
    static constexpr uint16_t kMaxDailyLimit = 180;
    static constexpr uint16_t kBonusMinutes = 10;

    void handleInput();
    void adjustDailyLimit(int deltaMinutes);
    void grantBonus();
    void toggleNightMode();
    void drawRow(int y, const char* label) const;
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
};
