#include "BootScreen.h"

#include <M5Unified.h>
#include <SD.h>

#include "../core/RetroBackdrop.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/Subject.h"
#include "../core/Theme.h"
#include "../core/storage/ProfileStore.h"
#include "../core/storage/ProgressStore.h"
#include "config.h"

namespace {
constexpr const char* kLogoText = "Henri & Theo";
// Mindestanzeigedauer des Logos, bevor die (meist sehr schnelle) SD-/Profil-
// Initialisierung startet - ohne das waere das Logo oft nur einen Frame
// lang sichtbar und damit praktisch kein "Startlogo".
constexpr uint32_t kSplashDurationMs = 1800;
} // namespace

BootScreen::BootScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void BootScreen::onEnter() {
    splashElapsedMs_ = 0;
    drawLogo();
}

void BootScreen::drawLogo() {
    M5.Display.fillScreen(theme::kBackground);
    retrobackdrop::drawSynthwaveGrid(&M5.Display, M5.Display.width(), M5.Display.height(),
                                      M5.Display.height() - 80);

    const int cx = M5.Display.width() / 2;
    const int cy = M5.Display.height() / 2 - 10;

    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(4);

    // Neon-Glow: mehrere leicht versetzte, gedimmte Kopien rings um den
    // Schriftzug simulieren ein "Bloom" (M5GFX kennt auf M5.Display keine
    // echte Transparenz/Weichzeichnung), darueber ein Chrom-/Versatz-Effekt
    // (Cyan-Kopie leicht verschoben unter der weissen Hauptschrift) - der
    // klassische 80er-Arcade-Logo-Look.
    constexpr int kGlowOffsets[][2] = {
        {-2, -2}, {2, -2}, {-2, 2}, {2, 2}, {-3, 0}, {3, 0}, {0, -3}, {0, 3},
    };
    M5.Display.setTextColor(theme::kAccentPink);
    for (const auto& offset : kGlowOffsets) {
        M5.Display.drawString(kLogoText, cx + offset[0], cy + offset[1]);
    }
    M5.Display.setTextColor(theme::kAccentCyan);
    M5.Display.drawString(kLogoText, cx + 2, cy + 2);
    M5.Display.setTextColor(theme::kText);
    M5.Display.drawString(kLogoText, cx, cy);

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(theme::kAccentGold);
    M5.Display.drawString("Laedt...", cx, cy + 42);
}

void BootScreen::update(uint32_t deltaMs) {
    splashElapsedMs_ += deltaMs;
    if (splashElapsedMs_ < kSplashDurationMs) {
        return;
    }

    if (initDone_) {
        return;
    }
    initDone_ = true;

    // Core2s SD-Karte haengt am selben SPI-Bus, den M5Unified in M5.begin()
    // bereits mit den Core2-spezifischen Pins konfiguriert hat. Ohne
    // explizite Angabe wuerde SD.begin() den globalen SPI-Bus in seiner
    // (nicht zu Core2 passenden) Default-Belegung verwenden und die Karte
    // faelschlicherweise als nicht vorhanden melden, selbst wenn sie korrekt
    // eingesteckt und formatiert ist.
    SD.begin(config::kSdChipSelectPin, SPI);
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
    if (app_.profile.dailyLimitMinutesOverride > 0) {
        app_.playtime.setDailyLimitMinutes(app_.profile.dailyLimitMinutesOverride);
    }
    app_.playtime.rolloverIfNewDay(today);

    stateMachine_.requestSwitch(ScreenId::Home);
}

void BootScreen::draw() {
    // Statischer Splash-Screen - wird in onEnter() einmalig gezeichnet.
}
