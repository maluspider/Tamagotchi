#pragma once

#include "storage/ProfileStore.h"

// Nachtmodus (docs/projektplan.md Abschnitt 11): dimmt den Bildschirm
// zwischen Profile::nightStartHour und nightEndHour (RTC-basiert, auch
// ohne WLAN zuverlaessig, Wraparound ueber Mitternacht wird unterstuetzt).
// Screen-unabhaengig aus main.cpp::loop() aufgerufen, damit der
// Nachtmodus greift, egal welcher Screen gerade aktiv ist - analog zu
// AlarmService. Bewusste Vereinfachung: kein "kurz antippen zum
// Aufhellen" (siehe Abschnitt 16, offener Punkt) - der Bildschirm bleibt
// waehrend der Nachtstunden durchgehend gedimmt, aber lesbar.
namespace nightmodeservice {

void check(const Profile& profile);

} // namespace nightmodeservice
