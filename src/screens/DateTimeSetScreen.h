#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Datum/Uhrzeit der RTC einstellen (docs/projektplan.md Abschnitt 11) - von
// echten Nutzern als fehlend gemeldet ("Uhrzeit laesst sich nicht
// einstellen"). Nur ueber SettingsScreen erreichbar (also bereits
// Eltern-PIN-geschuetzt, kein zweiter PIN-Check hier). Fuenf Stepper-Zeilen
// (Jahr/Monat/Tag/Stunde/Minute) plus ein Speichern-Button, der die
// Aenderung erst dann tatsaechlich in M5.Rtc schreibt - Antippen einzelner
// -/+ Felder darf also folgenlos wieder verworfen werden (Home-Icon bricht
// ohne Speichern ab).
class DateTimeSetScreen : public Screen {
public:
    DateTimeSetScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    static constexpr int kRowHeight = 34;
    static constexpr int kRow0Y = 32;  // Jahr
    static constexpr int kRow1Y = 66;  // Monat
    static constexpr int kRow2Y = 100; // Tag
    static constexpr int kRow3Y = 134; // Stunde
    static constexpr int kRow4Y = 168; // Minute
    static constexpr int kSaveY = 206;
    static constexpr int kSaveHeight = 28;
    static constexpr int kMinusX1 = 170;
    static constexpr int kMinusX2 = 210;
    static constexpr int kPlusX1 = 250;
    static constexpr int kPlusX2 = 300;

    void handleInput();
    void clampDay();
    void adjustYear(int delta);
    void adjustMonth(int delta);
    void adjustDay(int delta);
    void adjustHour(int delta);
    void adjustMinute(int delta);
    void save();
    void drawRow(int y, const char* label, int value, bool fourDigits) const;
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;

    int year_ = 2026;
    int month_ = 1;
    int day_ = 1;
    int hour_ = 0;
    int minute_ = 0;
};
