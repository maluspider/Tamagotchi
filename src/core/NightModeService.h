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

// true, wenn die aktuelle RTC-Uhrzeit innerhalb des in
// Profile::nightStartHour/nightEndHour konfigurierten Nachtfensters liegt
// (RTC-Jahr muss dafuer plausibel sein, siehe check()). Bewusst
// UNABHAENGIG vom nightModeEnabled-Schalter, der nur die Bildschirm-
// Dimmung steuert - Nutzerwunsch "Figur schlaeft zwischen 20 und 7 Uhr, in
// dieser Zeit kann nichts gespielt werden" soll unabhaengig davon gelten,
// ob die (rein kosmetische) Dimmung gerade an- oder ausgeschaltet ist.
// Genutzt von PlaytimeTicker/HomeScreen/GamesMenuScreen, um Spiele
// waehrend der Nachtstunden zu sperren.
bool isNight(const Profile& profile);

void check(const Profile& profile);

} // namespace nightmodeservice
