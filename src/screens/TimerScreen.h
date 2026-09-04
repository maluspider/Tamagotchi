#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Timer/Stoppuhr (docs/projektplan.md Abschnitt 11: "z. B. fuer
// Zaehneputzen (2-Min-Timer mit Countdown-Animation)"). Drei feste
// Presets (2/5/10 Minuten) statt freier Zeiteingabe - kindgerechter und
// deckt den genannten Anwendungsfall direkt ab. Vibriert + piept beim
// Ablauf (wie AlarmService/ClockScreen).
class TimerScreen : public Screen {
public:
    TimerScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    enum class Phase { SelectPreset, Running, Finished };

    void startTimer(uint32_t minutes);
    void handlePresetTap(int x, int y);
    void drawPresetSelect() const;
    void drawRunning() const;
    void drawFinished() const;
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    Phase phase_ = Phase::SelectPreset;
    uint32_t totalMs_ = 0;
    uint32_t remainingMs_ = 0;
    uint32_t lastDisplayedSeconds_ = 0;

    AppContext& app_;
    StateMachine& stateMachine_;
};
