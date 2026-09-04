#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Tages-/Routine-Checkliste (docs/projektplan.md Abschnitt 11): feste, im
// Code hinterlegte Morgen-Routine zum Abhaken, gibt beim vollstaendigen
// Abhaken eine kleine EP-Belohnung (einmal pro Tag). Zustand ist bewusst
// nur In-Memory (AppContext::checklistDone/..., siehe dortigen Kommentar).
class ChecklistScreen : public Screen {
public:
    ChecklistScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    void ensureDailyReset();
    void toggleItem(int index);
    void awardIfComplete();
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
};
