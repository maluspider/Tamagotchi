#include "TaskScreen.h"

#include <M5Unified.h>

#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "config.h"

namespace {
constexpr int kHomeIconSize = 28;
constexpr int kQuestionAreaHeight = 90;
constexpr uint32_t kFeedbackDurationMs = 1200;
} // namespace

TaskScreen::TaskScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void TaskScreen::onEnter() {
    if (!engine_.loadMathePool(app_.profile.klasse)) {
        phase_ = Phase::NoTasksAvailable;
        draw();
        return;
    }
    loadNextQuestion();
}

void TaskScreen::loadNextQuestion() {
    if (!engine_.pickRandomTask(current_)) {
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
    if (lastAnswerCorrect_) {
        app_.character.addXp(config::kXpPerCorrectAnswer);
        app_.playtime.creditTaskReward();
        app_.character.markCaredForToday(rtcclock::todayIso());
        app_.persistProgress();
    }

    phase_ = Phase::ShowingFeedback;
    feedbackElapsedMs_ = 0;
    draw();
}

void TaskScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, TFT_WHITE);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, TFT_WHITE);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, TFT_WHITE);
}

void TaskScreen::drawQuestion() const {
    M5.Display.fillRect(0, 0, M5.Display.width(), kQuestionAreaHeight, TFT_NAVY);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(3);
    M5.Display.drawString(current_.frage.c_str(), M5.Display.width() / 2, kQuestionAreaHeight / 2);

    const int areaHeight = M5.Display.height() - kQuestionAreaHeight;
    const int zoneHeight = areaHeight / current_.antwortenCount;
    for (uint8_t i = 0; i < current_.antwortenCount; ++i) {
        const int y = kQuestionAreaHeight + i * zoneHeight;
        M5.Display.drawRect(4, y + 2, M5.Display.width() - 8, zoneHeight - 4, TFT_WHITE);
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.setTextSize(3);
        M5.Display.drawString(current_.antworten[i].c_str(), M5.Display.width() / 2, y + zoneHeight / 2);
    }
}

void TaskScreen::drawFeedback() const {
    const uint16_t color = lastAnswerCorrect_ ? TFT_GREEN : TFT_RED;
    M5.Display.fillRect(0, kQuestionAreaHeight, M5.Display.width(), M5.Display.height() - kQuestionAreaHeight, color);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(4);
    const char* text = lastAnswerCorrect_ ? "Richtig!" : "Naechstes Mal!";
    M5.Display.drawString(text, M5.Display.width() / 2, kQuestionAreaHeight + (M5.Display.height() - kQuestionAreaHeight) / 2);
}

void TaskScreen::draw() {
    M5.Display.fillScreen(TFT_BLACK);

    if (phase_ == Phase::NoTasksAvailable) {
        M5.Display.setTextColor(TFT_WHITE);
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
