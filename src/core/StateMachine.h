#pragma once

#include <functional>
#include <map>
#include <memory>

#include "Screen.h"
#include "ScreenId.h"

// Zentrale State-Machine (docs/projektplan.md Abschnitt 5). Screens
// registrieren sich als Factory-Funktion, damit sie erst beim
// tatsaechlichen Wechsel instanziiert werden.
class StateMachine {
public:
    using ScreenFactory = std::function<std::unique_ptr<Screen>()>;

    void registerScreen(ScreenId id, ScreenFactory factory);

    // Sofortiger Wechsel. Nur ausserhalb eines laufenden Screen-Callbacks
    // aufrufen (z. B. einmalig in main.cpp::setup()) - siehe requestSwitch().
    void switchTo(ScreenId id);

    // Sicherer Wechsel aus einem Screen heraus (z. B. am Ende von
    // update()): wird erst zu Beginn des naechsten update()-Aufrufs
    // angewendet, damit der aufrufende Screen nicht waehrend seiner
    // eigenen Ausfuehrung zerstoert wird (Review: State-Machine-Sicherheit,
    // Abschnitt 5).
    void requestSwitch(ScreenId id);

    // Ruft nur update() auf dem aktiven Screen auf. Es gibt bewusst KEIN
    // begleitendes draw() hier: jeder Screen entscheidet in seinem eigenen
    // update() (bzw. onEnter()), wann tatsaechlich neu gezeichnet werden
    // muss (z. B. HomeScreen/ClockScreen hoechstens 1x/Sekunde,
    // SnakeScreen jeden Frame). Ein zusaetzlicher, bedingungsloser
    // draw()-Aufruf hier wuerde diese Drosselung wirkungslos machen und
    // bei den nicht Sprite-gepufferten Screens (alle ausser SnakeScreen)
    // auf echter Hardware sichtbar flackern.
    void update(uint32_t deltaMs);

    ScreenId currentId() const { return currentId_; }

private:
    void applySwitch(ScreenId id);

    std::map<ScreenId, ScreenFactory> factories_;
    std::unique_ptr<Screen> current_;
    ScreenId currentId_ = ScreenId::Boot;

    bool hasPendingSwitch_ = false;
    ScreenId pendingId_ = ScreenId::Boot;
};
