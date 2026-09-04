#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Uhr/Wecker-Screen (docs/projektplan.md Abschnitt 11). Zeigt die aktuelle
// Uhrzeit gross an und erlaubt das Einstellen eines einzelnen Weckers
// (Stunde/Minute, an/aus) ueber grosse Tap-Flaechen. Das eigentliche
// Ausloesen des Weckers passiert screen-unabhaengig in AlarmService, damit
// er auch klingelt, waehrend das Kind auf einem anderen Screen ist.
class ClockScreen : public Screen {
public:
    ClockScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    void handleTouch(int x, int y);
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
    uint32_t msSinceLastRedraw_ = 0;
};
