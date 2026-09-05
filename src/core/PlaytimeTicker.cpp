#include "PlaytimeTicker.h"

#include "NightModeService.h"

bool PlaytimeTicker::tick(AppContext& app, uint32_t deltaMs) {
    // Nutzerwunsch: "Figur schlaeft zwischen 20:00 und 07:00, in dieser
    // Zeit kann nichts gespielt werden" - deckt auch den Fall ab, dass die
    // Nachtstunden waehrend einer bereits laufenden Spielsitzung beginnen
    // (nicht nur den Einstieg von Home/GamesMenuScreen aus, siehe dort).
    if (nightmodeservice::isNight(app.profile)) {
        app.persistProgress();
        return true;
    }

    accumulatorMs_ += deltaMs;
    if (accumulatorMs_ < 60000) {
        return false;
    }
    accumulatorMs_ -= 60000;

    if (!app.playtime.spend(1)) {
        app.persistProgress();
        return true;
    }
    app.persistProgress();
    return false;
}
