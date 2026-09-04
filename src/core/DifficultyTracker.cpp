#include "DifficultyTracker.h"

namespace difficulty {

namespace {
constexpr uint8_t kMinSamplesForAdjustment = 5; // zu wenige Antworten -> keine verlaessliche Anpassung
constexpr float kUpThreshold = 0.85f;
constexpr float kDownThreshold = 0.50f;
} // namespace

float rollingHitRate(const DifficultyState& state) {
    if (state.recentCount == 0) {
        return 1.0f;
    }
    uint8_t correctCount = 0;
    for (uint8_t i = 0; i < state.recentCount; ++i) {
        if (state.recent[i]) {
            ++correctCount;
        }
    }
    return static_cast<float>(correctCount) / static_cast<float>(state.recentCount);
}

void recordAnswer(DifficultyState& state, bool correct) {
    state.recent[state.recentIndex] = correct;
    state.recentIndex = static_cast<uint8_t>((state.recentIndex + 1) % 10);
    if (state.recentCount < 10) {
        ++state.recentCount;
    }

    if (state.recentCount < kMinSamplesForAdjustment) {
        return;
    }

    const float hitRate = rollingHitRate(state);
    if (hitRate >= kUpThreshold) {
        if (state.stage < state.ceiling) {
            ++state.stage;
        }
    } else if (hitRate < kDownThreshold) {
        if (state.stage > 1) {
            --state.stage;
        }
    }
}

void applyMonthlyCeilingBump(DifficultyState& state, const String& currentMonthIso, uint8_t maxCeiling) {
    if (state.lastCeilingBumpMonthIso == currentMonthIso) {
        return; // diesen Monat schon erhoeht
    }
    state.lastCeilingBumpMonthIso = currentMonthIso;
    if (state.ceiling < maxCeiling) {
        ++state.ceiling;
    }
}

} // namespace difficulty
