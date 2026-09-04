#include <M5Unified.h>

#include <memory>

#include "core/AppContext.h"
#include "core/ScreenId.h"
#include "core/StateMachine.h"
#include "screens/BootScreen.h"
#include "screens/HomeScreen.h"
#include "screens/ProfileSetupScreen.h"

namespace {

AppContext appContext;
StateMachine stateMachine;
uint32_t lastFrameMs = 0;

} // namespace

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    stateMachine.registerScreen(ScreenId::Boot, [] {
        return std::make_unique<BootScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::ProfileSetup, [] {
        return std::make_unique<ProfileSetupScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Home, [] {
        return std::make_unique<HomeScreen>(appContext, stateMachine);
    });

    stateMachine.switchTo(ScreenId::Boot);
    lastFrameMs = millis();
}

void loop() {
    M5.update();

    const uint32_t now = millis();
    const uint32_t deltaMs = now - lastFrameMs;
    lastFrameMs = now;

    stateMachine.update(deltaMs);
    stateMachine.draw();

    delay(10);
}
