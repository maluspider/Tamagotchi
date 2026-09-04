#pragma once

#include <Arduino.h>
#include <cstdint>

#include "../PinCode.h"

// /profile.json - Geraete-/Kindprofil, aendert sich selten (siehe
// docs/projektplan.md Abschnitt 6, Review-Anmerkung zur Aufteilung
// zwischen profile.json und progress.json).
struct Profile {
    String name;
    uint8_t age = 0;
    uint8_t klasse = 1; // 1 oder 3, siehe include/KidProfiles.h::klasseForAge()
    String geraetId;
    pincode::Digest guard; // Eltern-PIN, gehasht (siehe PinCode.h)

    // Geburtstag (Abschnitt 11, BirthdayScreen) - aus include/KidProfiles.h
    // beim Erststart uebernommen. 0/0 = kein Geburtstag hinterlegt.
    uint8_t birthdayMonth = 0;
    uint8_t birthdayDay = 0;

    // Wecker (Abschnitt 11) - eine einzelne Alarmzeit pro Geraet, in
    // profile.json statt progress.json (aendert sich selten, siehe
    // Abschnitt 6).
    bool alarmEnabled = false;
    uint8_t alarmHour = 7;
    uint8_t alarmMinute = 0;

    // Nachtmodus (Abschnitt 11, Phase 4): Bildschirm dimmt zwischen
    // nightStartHour und nightEndHour (RTC-basiert, Wraparound ueber
    // Mitternacht wird unterstuetzt). Siehe NightModeService. Standardmaessig
    // AUS - ein frisch angelegtes Profil (und v.a. eine RTC ohne bereits
    // gesetzte Uhrzeit) soll nicht ungefragt/unerwartet dunkel starten;
    // Eltern schalten es bei Bedarf in den Einstellungen ein.
    bool nightModeEnabled = false;
    uint8_t nightStartHour = 20;
    uint8_t nightEndHour = 7;

    // Eltern-Einstellungen (Abschnitt 11, Phase 4): 0 = Standard aus
    // config::kDailyPlaytimeLimitMinutes verwenden, sonst individuelles
    // Tageslimit in Minuten (siehe SettingsScreen).
    uint16_t dailyLimitMinutesOverride = 0;

    // Definierbare Charaktermerkmale (Abschnitt 4/9, CharacterCustomizeScreen):
    // Indizes in traits::kSkinTones/kHairColors/kClothingColors
    // (CharacterTraits.h). 0 = jeweils erste Voreinstellung. Bewusst nicht
    // Eltern-PIN-geschuetzt - reine Optik, kein Sicherheits-/Limit-Thema.
    uint8_t skinToneIndex = 0;
    uint8_t hairColorIndex = 0;
    uint8_t clothingColorIndex = 0;

    bool isValid = false; // false = noch kein Profil auf der SD-Karte angelegt
};

namespace profilestore {

Profile load();
bool save(const Profile& profile);

} // namespace profilestore
