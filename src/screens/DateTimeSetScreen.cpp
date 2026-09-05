#include "DateTimeSetScreen.h"

#include <M5Unified.h>

#include "../core/GfxKit.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr int kMinYear = 2024;
constexpr int kMaxYear = 2099;

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return kDays[month - 1];
}
} // namespace

DateTimeSetScreen::DateTimeSetScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void DateTimeSetScreen::onEnter() {
    m5::rtc_date_t date;
    M5.Rtc.getDate(&date);
    m5::rtc_time_t time_;
    M5.Rtc.getTime(&time_);

    // Steht die RTC noch auf einem unplausiblen Werksdatum (siehe
    // NightModeService::check()), starten wir stattdessen mit einem
    // sinnvollen Vorgabewert statt dem Jahr 2000 - einfacher fuer die
    // Eltern, als erst durch 26 Jahre Jahres-Stepper zu muessen.
    if (date.year < kMinYear) {
        year_ = kMinYear;
        month_ = 1;
        day_ = 1;
        hour_ = 12;
        minute_ = 0;
    } else {
        year_ = date.year;
        month_ = date.month;
        day_ = date.date;
        hour_ = time_.hours;
        minute_ = time_.minutes;
    }

    draw();
}

void DateTimeSetScreen::clampDay() {
    const int maxDay = daysInMonth(year_, month_);
    if (day_ > maxDay) {
        day_ = maxDay;
    }
    if (day_ < 1) {
        day_ = 1;
    }
}

void DateTimeSetScreen::adjustYear(int delta) {
    year_ += delta;
    if (year_ < kMinYear) {
        year_ = kMinYear;
    }
    if (year_ > kMaxYear) {
        year_ = kMaxYear;
    }
    clampDay();
}

void DateTimeSetScreen::adjustMonth(int delta) {
    month_ += delta;
    if (month_ < 1) {
        month_ = 12;
    }
    if (month_ > 12) {
        month_ = 1;
    }
    clampDay();
}

void DateTimeSetScreen::adjustDay(int delta) {
    const int maxDay = daysInMonth(year_, month_);
    day_ += delta;
    if (day_ < 1) {
        day_ = maxDay;
    }
    if (day_ > maxDay) {
        day_ = 1;
    }
}

void DateTimeSetScreen::adjustHour(int delta) {
    hour_ = (hour_ + delta + 24) % 24;
}

void DateTimeSetScreen::adjustMinute(int delta) {
    minute_ = (minute_ + delta + 60) % 60;
}

void DateTimeSetScreen::save() {
    m5::rtc_date_t date;
    date.year = year_;
    date.month = month_;
    date.date = day_;
    M5.Rtc.setDate(&date);

    m5::rtc_time_t time_;
    time_.hours = hour_;
    time_.minutes = minute_;
    time_.seconds = 0;
    M5.Rtc.setTime(&time_);

    stateMachine_.requestSwitch(ScreenId::Settings);
}

bool DateTimeSetScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void DateTimeSetScreen::handleInput() {
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        // Abbrechen ohne zu speichern - siehe Klassenkommentar.
        stateMachine_.requestSwitch(ScreenId::Settings);
        return;
    }

    const int x = touch.x;
    const int y = touch.y;
    const bool minus = x >= kMinusX1 && x < kMinusX2;
    const bool plus = x >= kPlusX1 && x < kPlusX2;

    if (y >= kRow0Y && y < kRow0Y + kRowHeight) {
        if (minus) adjustYear(-1);
        else if (plus) adjustYear(1);
    } else if (y >= kRow1Y && y < kRow1Y + kRowHeight) {
        if (minus) adjustMonth(-1);
        else if (plus) adjustMonth(1);
    } else if (y >= kRow2Y && y < kRow2Y + kRowHeight) {
        if (minus) adjustDay(-1);
        else if (plus) adjustDay(1);
    } else if (y >= kRow3Y && y < kRow3Y + kRowHeight) {
        if (minus) adjustHour(-1);
        else if (plus) adjustHour(1);
    } else if (y >= kRow4Y && y < kRow4Y + kRowHeight) {
        if (minus) adjustMinute(-1);
        else if (plus) adjustMinute(1);
    } else if (y >= kSaveY && y < kSaveY + kSaveHeight) {
        save();
        return;
    }

    draw();
}

void DateTimeSetScreen::update(uint32_t) {
    handleInput();
}

void DateTimeSetScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void DateTimeSetScreen::drawRow(int y, const char* label, int value, bool fourDigits) const {
    gfxkit::bevelPanel(&M5.Display, 4, y, 312, kRowHeight - 4, 8, theme::kPanel, true);
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString(label, 14, y + (kRowHeight - 4) / 2);

    const int rowMidY = y + (kRowHeight - 4) / 2;
    gfxkit::bevelPanel(&M5.Display, kMinusX1, y + 4, kMinusX2 - kMinusX1, kRowHeight - 12, 6, theme::kDanger, true);
    gfxkit::bevelPanel(&M5.Display, kPlusX1, y + 4, kPlusX2 - kPlusX1, kRowHeight - 12, 6, theme::kSuccess, true);
    M5.Display.setTextColor(theme::kOutline);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("-", (kMinusX1 + kMinusX2) / 2, rowMidY);
    M5.Display.drawString("+", (kPlusX1 + kPlusX2) / 2, rowMidY);

    char buf[8];
    snprintf(buf, sizeof(buf), fourDigits ? "%04d" : "%02d", value);
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextSize(1);
    M5.Display.drawString(buf, (kMinusX2 + kPlusX1) / 2, rowMidY);
    M5.Display.setTextSize(2);
}

void DateTimeSetScreen::draw() {
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), M5.Display.height(),
                              gfxkit::darken(theme::kPanel, 0.6f), theme::kBackground);
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), 26, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Uhrzeit einstellen", 6, 3);

    drawRow(kRow0Y, "Jahr", year_, true);
    drawRow(kRow1Y, "Monat", month_, false);
    drawRow(kRow2Y, "Tag", day_, false);
    drawRow(kRow3Y, "Stunde", hour_, false);
    drawRow(kRow4Y, "Minute", minute_, false);

    gfxkit::bevelPanel(&M5.Display, 60, kSaveY, 200, kSaveHeight, 8, theme::kAccentGold, true);
    M5.Display.setTextColor(theme::kOutline);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Speichern", M5.Display.width() / 2, kSaveY + kSaveHeight / 2);

    drawHomeIcon();
}
