#include <M5Unified.h>

#include <memory>

#include "core/AlarmService.h"
#include "core/AppContext.h"
#include "core/NightModeService.h"
#include "core/ScreenId.h"
#include "core/StateMachine.h"
#include "screens/AlltagMenuScreen.h"
#include "screens/BasketballScreen.h"
#include "screens/BootScreen.h"
#include "screens/CharacterCustomizeScreen.h"
#include "screens/ChecklistScreen.h"
#include "screens/ClockScreen.h"
#include "screens/FussballScreen.h"
#include "screens/GamesMenuScreen.h"
#include "screens/GedaechtnisScreen.h"
#include "screens/HomeScreen.h"
#include "screens/KampfModusScreen.h"
#include "screens/MoorhuhnJagdScreen.h"
#include "screens/PinballScreen.h"
#include "screens/PinEntryScreen.h"
#include "screens/ProfileSetupScreen.h"
#include "screens/PuzzleScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/SnakeScreen.h"
#include "screens/SpaceInvadersScreen.h"
#include "screens/SteckbriefScreen.h"
#include "screens/SubjectSelectScreen.h"
#include "screens/TaskScreen.h"
#include "screens/TetrisScreen.h"
#include "screens/TimerScreen.h"
#include "screens/WebSyncScreen.h"

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
    stateMachine.registerScreen(ScreenId::SubjectSelect, [] {
        return std::make_unique<SubjectSelectScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Task, [] {
        return std::make_unique<TaskScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Memory, [] {
        return std::make_unique<GedaechtnisScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Clock, [] {
        return std::make_unique<ClockScreen>(appContext, stateMachine);
    });

    stateMachine.registerScreen(ScreenId::GamesMenu, [] {
        return std::make_unique<GamesMenuScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Snake, [] {
        return std::make_unique<SnakeScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Tetris, [] {
        return std::make_unique<TetrisScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::SpaceInvaders, [] {
        return std::make_unique<SpaceInvadersScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Pinball, [] {
        return std::make_unique<PinballScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Basketball, [] {
        return std::make_unique<BasketballScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Fussball, [] {
        return std::make_unique<FussballScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Puzzle, [] {
        return std::make_unique<PuzzleScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::MoorhuhnJagd, [] {
        return std::make_unique<MoorhuhnJagdScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::KampfModus, [] {
        return std::make_unique<KampfModusScreen>(appContext, stateMachine);
    });

    stateMachine.registerScreen(ScreenId::AlltagMenu, [] {
        return std::make_unique<AlltagMenuScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Timer, [] {
        return std::make_unique<TimerScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Checklist, [] {
        return std::make_unique<ChecklistScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Steckbrief, [] {
        return std::make_unique<SteckbriefScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::CharacterCustomize, [] {
        return std::make_unique<CharacterCustomizeScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::PinEntry, [] {
        return std::make_unique<PinEntryScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::Settings, [] {
        return std::make_unique<SettingsScreen>(appContext, stateMachine);
    });
    stateMachine.registerScreen(ScreenId::WebSync, [] {
        return std::make_unique<WebSyncScreen>(appContext, stateMachine);
    });

    stateMachine.switchTo(ScreenId::Boot);
    lastFrameMs = millis();
}

void loop() {
    M5.update();

    // Screen-unabhaengig, damit der Wecker auch klingelt, waehrend das Kind
    // nicht auf dem Uhr-Screen ist (siehe AlarmService.h).
    alarmservice::check(appContext.profile);
    // Ebenfalls screen-unabhaengig, damit der Nachtmodus greift, egal
    // welcher Screen gerade aktiv ist (siehe NightModeService.h).
    nightmodeservice::check(appContext.profile);

    const uint32_t now = millis();
    const uint32_t deltaMs = now - lastFrameMs;
    lastFrameMs = now;

    stateMachine.update(deltaMs);

    delay(10);
}
