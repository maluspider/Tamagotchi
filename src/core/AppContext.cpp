#include "AppContext.h"

#include "storage/ProgressStore.h"

void AppContext::persistProgress() const {
    ProgressData data;
    data.xp = character.xp();
    data.lastCareDateIso = character.lastCareDateIso();
    data.earnedMinutesToday = playtime.earnedMinutesToday();
    data.spentMinutesToday = playtime.spentMinutesToday();
    data.playtimeDateIso = playtime.dateIso();

    for (size_t i = 0; i < kSubjectCount; ++i) {
        data.difficulty[i] = difficultyBySubject[i];
    }

    progressstore::save(data);
}
