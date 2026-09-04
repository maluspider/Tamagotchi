#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/SpacedRepetitionStore.h"
#include "../core/StateMachine.h"
#include "../core/TaskEngine.h"

// Aufgaben-Modus (docs/projektplan.md Abschnitt 5): Fach kommt aus
// app_.selectedSubject (von SubjectSelectScreen gesetzt). Aufgabe ->
// Auswertung -> naechste Aufgabe, jederzeit per Home-Icon zurueck zu Home.
//
// Richtige Antwort: EP + Spielzeit (Abschnitt 7/9), Box in der
// Spaced-Repetition-Historie steigt, rollierende Trefferquote steigt
// (Abschnitt 8.3/8.4). Falsche Antwort: kein EP-/Spielzeit-Abzug
// (Abschnitt 7, Review), Box faellt auf 1 zurueck, Trefferquote sinkt -
// kann dadurch die Schwierigkeitsstufe senken (nie aber ueber die vom
// monatlichen Auto-Anstieg gesetzte Obergrenze hinaus erhoehen,
// DifficultyTracker.h).
class TaskScreen : public Screen {
public:
    TaskScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    enum class Phase { AskingQuestion, ShowingFeedback, NoTasksAvailable };

    void loadNextQuestion();
    void drawQuestion() const;
    void drawFeedback() const;
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;
    int answerZoneIndex(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
    TaskEngine engine_;
    SpacedRepetitionStore srs_;

    Task current_;
    Phase phase_ = Phase::AskingQuestion;
    bool lastAnswerCorrect_ = false;
    uint32_t feedbackElapsedMs_ = 0;
};
