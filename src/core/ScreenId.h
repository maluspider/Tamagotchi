#pragma once

// Siehe docs/projektplan.md Abschnitt 5 fuer die vollstaendige, geplante
// Screen-Hierarchie. Alltagsfunktionen-Menue/Einstellungen kommen in
// Phase 4 dazu.
enum class ScreenId {
    Boot,
    ProfileSetup,
    Home,
    SubjectSelect,
    Task,
    Memory,
    Clock,

    GamesMenu,
    Snake,
    Tetris,
    SpaceInvaders,
    Pinball,
    Basketball,
    Fussball,
    Puzzle,
    MoorhuhnJagd,
    KampfModus,
};
