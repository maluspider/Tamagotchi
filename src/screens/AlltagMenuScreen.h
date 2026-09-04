#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/ScreenId.h"
#include "../core/StateMachine.h"

// Alltagsfunktionen-Menue (docs/projektplan.md Abschnitt 5/11): icon-
// basiertes 3x2-Menue zu Uhr/Wecker, Timer, Checkliste, Charakter-
// Steckbrief, "Aussehen" (CharacterCustomizeScreen, Abschnitt 4/9) und
// Geburtstags-Countdown (BirthdayScreen). Taschenrechner aus dem
// "Zusaetzliche Vorschlaege"-Teil von Abschnitt 11 ist bewusst nicht
// umgesetzt (Zeitbudget, siehe Abschnitt 16, offener Punkt).
class AlltagMenuScreen : public Screen {
public:
    AlltagMenuScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    struct Entry {
        ScreenId screen;
        const char* label;
    };
    static constexpr int kEntryCount = 6;
    static const Entry kEntries[kEntryCount];

    void drawEntry(int index, int cx, int cy, int cellSize) const;
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
};
