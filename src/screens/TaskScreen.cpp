#include "TaskScreen.h"

#include <M5Unified.h>

#include "../core/DifficultyTracker.h"
#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/Subject.h"
#include "../core/TextFit.h"
#include "../core/Theme.h"
#include "config.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr int kQuestionAreaHeight = 90;
constexpr uint32_t kFeedbackDurationMs = 1200;
} // namespace

TaskScreen::TaskScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void TaskScreen::onEnter() {
    srs_.load(subjectSlug(app_.selectedSubject));

    DifficultyState& difficulty = app_.difficultyBySubject[static_cast<size_t>(app_.selectedSubject)];
    const String today = rtcclock::todayIso();
    difficulty::applyMonthlyCeilingBump(difficulty, today.substring(0, 7), config::kMaxDifficultyStage);

    if (!engine_.loadPool(app_.selectedSubject, app_.profile.klasse)) {
        phase_ = Phase::NoTasksAvailable;
        draw();
        return;
    }
    loadNextQuestion();
}

void TaskScreen::loadNextQuestion() {
    const DifficultyState& difficulty = app_.difficultyBySubject[static_cast<size_t>(app_.selectedSubject)];
    if (!engine_.pickNextTask(current_, srs_, difficulty, rtcclock::todayIso())) {
        phase_ = Phase::NoTasksAvailable;
        draw();
        return;
    }
    phase_ = Phase::AskingQuestion;
    draw();
}

bool TaskScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

int TaskScreen::answerZoneIndex(int x, int y) const {
    (void)x;
    if (y < kQuestionAreaHeight || current_.antwortenCount == 0) {
        return -1;
    }
    const int areaHeight = M5.Display.height() - kQuestionAreaHeight;
    const int zoneHeight = areaHeight / current_.antwortenCount;
    const int index = (y - kQuestionAreaHeight) / zoneHeight;
    if (index < 0 || index >= current_.antwortenCount) {
        return -1;
    }
    return index;
}

void TaskScreen::update(uint32_t deltaMs) {
    if (phase_ == Phase::ShowingFeedback) {
        feedbackElapsedMs_ += deltaMs;
        if (feedbackElapsedMs_ >= kFeedbackDurationMs) {
            feedbackElapsedMs_ = 0;
            loadNextQuestion();
        }
        return;
    }

    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }

    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }

    if (phase_ != Phase::AskingQuestion) {
        return;
    }

    const int index = answerZoneIndex(touch.x, touch.y);
    if (index < 0) {
        return;
    }

    lastAnswerCorrect_ = (static_cast<uint8_t>(index) == current_.richtig);
    haptics::pulse(lastAnswerCorrect_ ? 60 : 150);

    const String today = rtcclock::todayIso();
    srs_.recordAnswer(current_.id, lastAnswerCorrect_, today);
    srs_.save();

    DifficultyState& difficulty = app_.difficultyBySubject[static_cast<size_t>(app_.selectedSubject)];
    difficulty::recordAnswer(difficulty, lastAnswerCorrect_);

    if (lastAnswerCorrect_) {
        app_.character.addXp(config::kXpPerCorrectAnswer);
        app_.playtime.creditTaskReward();
        app_.character.markCaredForToday(today);
    }
    app_.persistProgress();

    phase_ = Phase::ShowingFeedback;
    feedbackElapsedMs_ = 0;
    draw();
}

void TaskScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void TaskScreen::drawQuestion() const {
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), kQuestionAreaHeight,
                              gfxkit::lighten(theme::kPanel, 0.15f), gfxkit::darken(theme::kPanel, 0.25f));
    // Laengere Fragen/Antworten (z. B. Rechtschreibung/Quiz) liefen bei
    // fester textSize(3) auf echter Hardware rechts/links aus dem
    // Bildschirm und wurden dadurch unlesbar - textfit::drawFitted() bricht
    // bei Bedarf um und verkleinert die Schrift, statt einfach abzuschneiden.
    textfit::drawFitted(&M5.Display, current_.frage, M5.Display.width() / 2, kQuestionAreaHeight / 2,
                         M5.Display.width() - 20, kQuestionAreaHeight - 10, 3, theme::kText);

    const int areaHeight = M5.Display.height() - kQuestionAreaHeight;
    const int zoneHeight = areaHeight / current_.antwortenCount;
    for (uint8_t i = 0; i < current_.antwortenCount; ++i) {
        const int y = kQuestionAreaHeight + i * zoneHeight;
        gfxkit::bevelPanel(&M5.Display, 4, y + 2, M5.Display.width() - 8, zoneHeight - 4, 6, theme::kPanel, true);
        textfit::drawFitted(&M5.Display, current_.antworten[i], M5.Display.width() / 2, y + zoneHeight / 2,
                             M5.Display.width() - 24, zoneHeight - 10, 3, theme::kText);
    }
}

void TaskScreen::drawFeedback() const {
    const uint16_t color = lastAnswerCorrect_ ? theme::kSuccess : theme::kDanger;
    gfxkit::verticalGradient(&M5.Display, 0, kQuestionAreaHeight, M5.Display.width(),
                              M5.Display.height() - kQuestionAreaHeight, gfxkit::lighten(color, 0.25f),
                              gfxkit::darken(color, 0.25f));
    M5.Display.setTextColor(theme::kOutline);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(4);
    const char* text = lastAnswerCorrect_ ? "Richtig!" : "Naechstes Mal!";
    M5.Display.drawString(text, M5.Display.width() / 2, kQuestionAreaHeight + (M5.Display.height() - kQuestionAreaHeight) / 2);
}

void TaskScreen::draw() {
    gfxkit::verticalGradient(&M5.Display, 0, 0, M5.Display.width(), M5.Display.height(),
                              gfxkit::darken(theme::kPanel, 0.6f), theme::kBackground);

    if (phase_ == Phase::NoTasksAvailable) {
        M5.Display.setTextColor(theme::kText);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(2);
        M5.Display.drawString("Keine Aufgaben gefunden", M5.Display.width() / 2, M5.Display.height() / 2);
        drawHomeIcon();
        return;
    }

    drawQuestion();
    if (phase_ == Phase::ShowingFeedback) {
        drawFeedback();
    }
    drawHomeIcon();
}
