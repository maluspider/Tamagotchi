#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"
#include "../core/TaskEngine.h"

// Aufgaben-Modus (docs/projektplan.md Abschnitt 5), Phase-1-Umfang: nur
// Mathe, Aufgabe -> Auswertung -> naechste Aufgabe, jederzeit per
// Home-Icon zurueck zu Home. Richtige Antwort: EP + Spielzeit gemaess
// Abschnitt 7/9 (config::kXpPerCorrectAnswer / config::kMinutesPerSolvedTask
// via PlaytimeAccount::creditTaskReward()). Falsche Antwort: kein Abzug
// (Abschnitt 7, Review) - nur kein Zugewinn.
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

    Task current_;
    Phase phase_ = Phase::AskingQuestion;
    bool lastAnswerCorrect_ = false;
    uint32_t feedbackElapsedMs_ = 0;
};
