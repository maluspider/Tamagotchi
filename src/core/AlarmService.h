#pragma once

#include "storage/ProfileStore.h"

// Prueft bei jedem Aufruf, ob der eingestellte Wecker (Profile::alarmEnabled/
// alarmHour/alarmMinute, siehe ClockScreen) gerade ausloesen soll -
// unabhaengig vom aktuell aktiven Screen (ein Wecker muss klingeln, auch
// wenn das Kind gerade auf dem Home-Screen oder in einem Spiel ist, siehe
// docs/projektplan.md Abschnitt 11). Wird deshalb direkt aus main.cpp::
// loop() aufgerufen, nicht aus einem Screen heraus. Loest hoechstens einmal
// pro Minute aus.
namespace alarmservice {

void check(const Profile& profile);

} // namespace alarmservice
