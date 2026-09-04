#pragma once

#include "CharacterEngine.h"
#include "DifficultyTracker.h"
#include "PlaytimeAccount.h"
#include "Subject.h"
#include "storage/ProfileStore.h"

// Gemeinsamer Zustand, auf den mehrere Screens zugreifen (siehe
// docs/projektplan.md Abschnitt 5: "Charakter-Engine, Aufgaben-Engine und
// Spielzeitkonto sind eigenstaendige Module, die von mehreren Screens
// genutzt werden, nicht an einen Screen gebunden"). Wird von main.cpp
// gehalten und an jede Screen-Factory weitergereicht.
struct AppContext {
    Profile profile;
    CharacterEngine character;
    PlaytimeAccount playtime;
    DifficultyState difficultyBySubject[kSubjectCount];

    // Vom SubjectSelectScreen gesetzt, bevor zu TaskScreen gewechselt wird
    // (Abschnitt 5: "Fach waehlen -> Aufgabe").
    Subject selectedSubject = Subject::Mathe;

    // Vom SettingsScreen gesetzt, bevor zu PinEntryScreen gewechselt wird,
    // um zwischen "PIN pruefen" (Standard) und "neuen PIN festlegen" zu
    // unterscheiden (Abschnitt 11, Phase 4).
    bool pinEntrySetNewMode = false;

    // Tages-/Routine-Checkliste (Abschnitt 11, Phase 4). Bewusst nur
    // In-Memory (kein Persistieren noetig - Faelle "Geraet startet mitten
    // in der Morgenroutine neu" sind selten, verglichen mit dem Aufwand
    // fuer eine eigene Persistenzschicht dafuer).
    static constexpr int kChecklistItemCount = 3;
    bool checklistDone[kChecklistItemCount] = {};
    String checklistDateIso;
    bool checklistRewardedToday = false;

    // Schreibt charakter/spielzeitkonto/schwierigkeit atomar nach
    // /progress.json (siehe JsonStore.h). Wird u. a. bei kritischem
    // Akkustand aufgerufen, damit nicht mitten in einem spaeteren
    // Schreibvorgang der Strom ausgeht.
    void persistProgress() const;
};
