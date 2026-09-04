#include "PlaytimeTicker.h"

bool PlaytimeTicker::tick(AppContext& app, uint32_t deltaMs) {
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
