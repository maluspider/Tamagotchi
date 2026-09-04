#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// PIN-Eingabe (docs/projektplan.md Abschnitt 11: "Einstellungen
// (Eltern-PIN-geschützt)"). Zwei Modi ueber app_.pinEntrySetNewMode
// gesteuert: normal = gegen den gespeicherten PIN pruefen (Erfolg ->
// Settings), true = die eingegebenen 4 Ziffern werden der neue PIN
// (SettingsScreen::"PIN aendern"). Numerischer Ziffernblock statt
// Texteingabe, damit es auch fuer das juengere Kind bedienbar bliebe,
// falls es das Menue versehentlich erreicht (der PIN selbst bleibt
// natuerlich den Eltern vorbehalten).
class PinEntryScreen : public Screen {
public:
    PinEntryScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    static constexpr int kPinLength = 4;
    static constexpr int kCols = 3;
    static constexpr int kRows = 4;
    static constexpr int kTopBarHeight = 60;

    void handleKeyTap(int x, int y);
    void submitIfComplete();
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    String entered_;
    bool showError_ = false;
    uint32_t errorTimerMs_ = 0;

    AppContext& app_;
    StateMachine& stateMachine_;
};
