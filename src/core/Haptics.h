#pragma once

#include <cstdint>

// Kurze, nicht blockierende Vibrations-Rueckmeldung (Nutzerwunsch: "bei
// Spielen und Quiz etc Vibrationsfeedback einbauen") fuer Tap-/Treffer-/
// Erfolgs-/Fehler-Ereignisse. pulse() setzt lediglich einen Timer; das
// tatsaechliche Ein-/Abschalten von M5.Power.setVibration() passiert in
// update(), das screen-unabhaengig aus main.cpp::loop() aufgerufen wird
// (wie AlarmService/NightModeService) - ein Screen muss selbst nichts
// ticken, und ein blockierendes delay() (wie es AlarmService fuer den
// Wecker nutzt) wuerde bei haeufigen Spielereignissen spuerbar ruckeln.
namespace haptics {

// Startet eine Vibration fuer mindestens `ms` Millisekunden. Mehrfache
// Aufrufe waehrend eine Vibration noch laeuft verlaengern sie hoechstens
// (nie verkuerzen), damit sich schnell aufeinanderfolgende Ereignisse
// (z. B. mehrere Treffer kurz hintereinander) nicht gegenseitig abschneiden.
void pulse(uint32_t ms = 60);

void update(uint32_t deltaMs);

} // namespace haptics
