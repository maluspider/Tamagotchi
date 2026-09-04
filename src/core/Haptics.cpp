#include "Haptics.h"

#include <M5Unified.h>

namespace haptics {

namespace {
constexpr uint8_t kVibrationStrength = 200; // gleiche Staerke wie AlarmService
uint32_t remainingMs_ = 0;
} // namespace

void pulse(uint32_t ms) {
    if (remainingMs_ == 0) {
        M5.Power.setVibration(kVibrationStrength);
    }
    if (ms > remainingMs_) {
        remainingMs_ = ms;
    }
}

void update(uint32_t deltaMs) {
    if (remainingMs_ == 0) {
        return;
    }
    remainingMs_ = (remainingMs_ > deltaMs) ? remainingMs_ - deltaMs : 0;
    if (remainingMs_ == 0) {
        M5.Power.setVibration(0);
    }
}

} // namespace haptics
