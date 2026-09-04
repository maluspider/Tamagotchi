#pragma once

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/ScreenId.h"
#include "../core/StateMachine.h"

// Spiele-Menue (docs/projektplan.md Abschnitt 5): "nur mit vorhandenem
// Zeitguthaben betretbar" - das prueft bereits HomeScreen vor dem Wechsel
// hierher. Zusaetzlich wird hier die Freischalt-Reihenfolge nach
// Charakterstufe durchgesetzt (Abschnitt 9): gesperrte Spiele werden
// ausgegraut mit Schloss-Symbol angezeigt und lassen sich nicht antippen.
class GamesMenuScreen : public Screen {
public:
    GamesMenuScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    struct GameEntry {
        ScreenId screen;
        CharacterStage requiredStage;
    };
    static constexpr int kGameCount = 9;
    static const GameEntry kGames[kGameCount];

    void drawIcon(int index, int cx, int cy, int r, bool unlocked) const;
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
};
