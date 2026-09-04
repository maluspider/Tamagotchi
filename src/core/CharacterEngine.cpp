#include "CharacterEngine.h"

#include "RtcClock.h"
#include "config.h"

namespace {

// XP-Schwellen aus docs/projektplan.md Abschnitt 9 (Startwerte - finale
// Balance-Werte sind laut Abschnitt 16 noch offen).
constexpr uint32_t kStageThresholds[] = {0, 100, 300, 700, 1500, 3000};
constexpr size_t kStageCount = sizeof(kStageThresholds) / sizeof(kStageThresholds[0]);

} // namespace

void CharacterEngine::load(uint32_t xp, const String& lastCareDateIso) {
    xp_ = xp;
    stage_ = stageForXp(xp_);
    lastCareDateIso_ = lastCareDateIso;
}

void CharacterEngine::addXp(uint32_t amount) {
    xp_ += amount;
    const CharacterStage newStage = stageForXp(xp_);
    if (newStage > stage_) {
        stage_ = newStage;
    }
}

void CharacterEngine::markCaredForToday(const String& todayIso) {
    lastCareDateIso_ = todayIso;
}

bool CharacterEngine::isSad(const String& todayIso) const {
    if (lastCareDateIso_.isEmpty()) {
        return false; // frisch angelegtes Profil - noch keine Historie
    }
    const long inactiveDays =
        rtcclock::epochDayFromIso(todayIso) - rtcclock::epochDayFromIso(lastCareDateIso_);
    return inactiveDays >= config::kSadAfterDaysInactive;
}

CharacterStage CharacterEngine::stageForXp(uint32_t xp) {
    CharacterStage stage = CharacterStage::Ei;
    for (size_t i = 0; i < kStageCount; ++i) {
        if (xp >= kStageThresholds[i]) {
            stage = static_cast<CharacterStage>(i);
        }
    }
    return stage;
}

const char* CharacterEngine::stageName(CharacterStage stage) {
    switch (stage) {
        case CharacterStage::Ei: return "Ei";
        case CharacterStage::Baby: return "Baby";
        case CharacterStage::Kind: return "Kind";
        case CharacterStage::Junior: return "Junior";
        case CharacterStage::Experte: return "Experte";
        case CharacterStage::Meister: return "Meister";
    }
    return "?";
}

const char* CharacterEngine::stageAssetKey(CharacterStage stage) {
    switch (stage) {
        case CharacterStage::Ei: return "ei";
        case CharacterStage::Baby: return "baby";
        case CharacterStage::Kind: return "kind";
        case CharacterStage::Junior: return "junior";
        case CharacterStage::Experte: return "experte";
        case CharacterStage::Meister: return "meister";
    }
    return "ei";
}
