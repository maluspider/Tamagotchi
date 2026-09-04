#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Web-Sync (docs/projektplan.md Abschnitt 12): startet den geraeteeigenen
// Access Point + Web-Server + ArduinoOTA beim Betreten und stoppt beide
// beim Verlassen (onExit()) - laeuft also nur, waehrend dieser Screen
// aktiv ist, um Akku zu sparen (Abschnitt 2, Review: Akku klein). Zeigt
// SSID/Passwort/IP an, damit ein Elternteil sich mit einem Beobachter-
// Geraet verbinden kann.
class WebSyncScreen : public Screen {
public:
    WebSyncScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void onExit() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
};
