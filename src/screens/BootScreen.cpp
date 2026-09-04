#include "BootScreen.h"

#include <M5Unified.h>
#include <SD.h>

#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/Subject.h"
#include "../core/storage/ProfileStore.h"
#include "../core/storage/ProgressStore.h"
#include "config.h"

BootScreen::BootScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void BootScreen::onEnter() {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Laedt...", M5.Display.width() / 2, M5.Display.height() / 2);
}

void BootScreen::update(uint32_t) {
    if (initDone_) {
        return;
    }
    initDone_ = true;

    SD.begin(config::kSdChipSelectPin);
    if (!SD.exists("/progress")) {
        // Wird von SpacedRepetitionStore fuer /progress/aufgaben_<fach>.json
        // benoetigt (Abschnitt 6/8.3) - das SD-Filesystem legt Verzeichnisse
        // nicht implizit beim Schreiben an.
        SD.mkdir("/progress");
    }

    app_.profile = profilestore::load();

    if (!app_.profile.isValid) {
        stateMachine_.requestSwitch(ScreenId::ProfileSetup);
        return;
    }

    const ProgressData progress = progressstore::load();
    app_.character.load(progress.xp, progress.lastCareDateIso);
    for (size_t i = 0; i < kSubjectCount; ++i) {
        app_.difficultyBySubject[i] = progress.difficulty[i];
    }

    const String today = rtcclock::todayIso();
    const String playtimeDate = progress.playtimeDateIso.length() ? progress.playtimeDateIso : today;
    app_.playtime.load(progress.earnedMinutesToday, progress.spentMinutesToday, playtimeDate);
    app_.playtime.rolloverIfNewDay(today);

    stateMachine_.requestSwitch(ScreenId::Home);
}

void BootScreen::draw() {
    // Statischer Splash-Screen - wird in onEnter() einmalig gezeichnet.
}
