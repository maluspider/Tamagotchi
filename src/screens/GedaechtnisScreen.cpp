#include "GedaechtnisScreen.h"

#include <M5Unified.h>
#include <esp_random.h>

#include "../core/GfxKit.h"
#include "../core/Haptics.h"
#include "../core/RtcClock.h"
#include "../core/ScreenId.h"
#include "../core/Theme.h"
#include "config.h"

namespace {
constexpr int kHomeIconSize = 28;
} // namespace

GedaechtnisScreen::GedaechtnisScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine), canvas_(&M5.Display) {}

void GedaechtnisScreen::onEnter() {
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    // 1. Klasse: "Bild-Paare merken" -> Karten-Memory. 3. Klasse:
    // "Sequenzen merken" -> Simon-Says (Abschnitt 8.1).
    isMemoryMode_ = (app_.profile.klasse == 1);
    if (isMemoryMode_) {
        resetMemoryGame();
    } else {
        resetSequenceGame();
    }
    draw();
}

void GedaechtnisScreen::update(uint32_t deltaMs) {
    if (isMemoryMode_) {
        updateMemory(deltaMs);
    } else {
        updateSequence(deltaMs);
    }
}

void GedaechtnisScreen::draw() {
    if (isMemoryMode_) {
        drawMemoryGame();
    } else {
        drawSequenceGame();
    }
}

void GedaechtnisScreen::awardRoundReward() {
    ++score_;
    app_.character.addXp(config::kXpPerCorrectAnswer);
    app_.playtime.creditTaskReward();
    app_.character.markCaredForToday(rtcclock::todayIso());
    app_.persistProgress();
}

bool GedaechtnisScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void GedaechtnisScreen::drawHomeIcon() {
    const int x = canvas_.width() - kHomeIconSize - 6;
    const int y = 6;
    canvas_.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    canvas_.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    canvas_.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

// ---------------------------------------------------------------------
// Karten-Memory (1. Klasse)
// ---------------------------------------------------------------------

void GedaechtnisScreen::resetMemoryGame() {
    uint8_t deck[kCardCount];
    for (int i = 0; i < kCardCount; ++i) {
        deck[i] = static_cast<uint8_t>(i / 2); // kCardCount/2 Symbole, je 2x
    }
    // Fisher-Yates-Shuffle.
    for (int i = kCardCount - 1; i > 0; --i) {
        const int j = static_cast<int>(esp_random() % static_cast<uint32_t>(i + 1));
        const uint8_t tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }

    for (int i = 0; i < kCardCount; ++i) {
        cardSymbol_[i] = deck[i];
        cardRevealed_[i] = false;
        cardMatched_[i] = false;
    }
    firstPick_ = -1;
    secondPick_ = -1;
    pauseRemainingMs_ = 0;
    score_ = 0;
    gameOver_ = false;
}

int GedaechtnisScreen::cardIndexAt(int x, int y) const {
    if (y < kTopBarHeight) {
        return -1;
    }
    const int cellW = M5.Display.width() / kCardCols;
    const int cellH = (M5.Display.height() - kTopBarHeight) / kCardRows;
    const int col = x / cellW;
    const int row = (y - kTopBarHeight) / cellH;
    if (col < 0 || col >= kCardCols || row < 0 || row >= kCardRows) {
        return -1;
    }
    return row * kCardCols + col;
}

void GedaechtnisScreen::handleMemoryTouch(int x, int y) {
    const int index = cardIndexAt(x, y);
    if (index < 0 || cardMatched_[index] || cardRevealed_[index]) {
        return;
    }

    if (firstPick_ == -1) {
        firstPick_ = index;
        cardRevealed_[index] = true;
        return;
    }
    if (secondPick_ != -1) {
        return; // Auswertungspause laeuft noch
    }

    secondPick_ = index;
    cardRevealed_[index] = true;

    if (cardSymbol_[firstPick_] == cardSymbol_[secondPick_]) {
        cardMatched_[firstPick_] = true;
        cardMatched_[secondPick_] = true;
        firstPick_ = -1;
        secondPick_ = -1;
        haptics::pulse(60);
        awardRoundReward();

        bool allMatched = true;
        for (int i = 0; i < kCardCount; ++i) {
            if (!cardMatched_[i]) {
                allMatched = false;
                break;
            }
        }
        if (allMatched) {
            gameOver_ = true;
        }
    } else {
        haptics::pulse(150);
        pauseRemainingMs_ = kMismatchPauseMs;
    }
}

void GedaechtnisScreen::updateMemory(uint32_t deltaMs) {
    if (pauseRemainingMs_ > 0) {
        pauseRemainingMs_ = (pauseRemainingMs_ > deltaMs) ? pauseRemainingMs_ - deltaMs : 0;
        if (pauseRemainingMs_ == 0) {
            cardRevealed_[firstPick_] = false;
            cardRevealed_[secondPick_] = false;
            firstPick_ = -1;
            secondPick_ = -1;
            draw();
        }
        return; // waehrend der Pause keine Touches auswerten
    }

    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }
    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }
    if (gameOver_) {
        resetMemoryGame();
        draw();
        return;
    }
    handleMemoryTouch(touch.x, touch.y);
    draw();
}

void GedaechtnisScreen::drawSymbol(int symbol, int cx, int cy, int r) {
    switch (symbol % 4) {
        case 0:
            canvas_.fillCircle(cx, cy, r, TFT_RED);
            break;
        case 1:
            canvas_.fillRect(cx - r, cy - r, r * 2, r * 2, TFT_GREEN);
            break;
        case 2:
            canvas_.fillTriangle(cx, cy - r, cx - r, cy + r, cx + r, cy + r, TFT_CYAN);
            break;
        default:
            canvas_.fillRoundRect(cx - r, cy - r, r * 2, r * 2, r, TFT_YELLOW);
            break;
    }
}

void GedaechtnisScreen::drawMemoryGame() {
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), canvas_.height(), gfxkit::darken(theme::kPanel, 0.55f),
                              theme::kBackground);
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));
    canvas_.setTextColor(theme::kText);
    canvas_.setTextDatum(top_left);
    canvas_.setTextSize(1);
    char buf[16];
    snprintf(buf, sizeof(buf), "Paare: %d/%d", score_, kCardCount / 2);
    canvas_.drawString(buf, 4, 4);

    const int cellW = canvas_.width() / kCardCols;
    const int cellH = (canvas_.height() - kTopBarHeight) / kCardRows;

    for (int i = 0; i < kCardCount; ++i) {
        const int col = i % kCardCols;
        const int row = i / kCardCols;
        const int cx = col * cellW + cellW / 2;
        const int cy = kTopBarHeight + row * cellH + cellH / 2;
        const int half = (cellW < cellH ? cellW : cellH) / 2 - 8;

        if (cardMatched_[i] || cardRevealed_[i]) {
            gfxkit::bevelPanel(&canvas_, cx - half, cy - half, half * 2, half * 2, 8, TFT_WHITE, true);
            drawSymbol(cardSymbol_[i], cx, cy, half - 6);
        } else {
            gfxkit::bevelPanel(&canvas_, cx - half, cy - half, half * 2, half * 2, 8, theme::kMuted, true);
        }
    }

    drawHomeIcon();

    if (gameOver_) {
        canvas_.setTextDatum(middle_center);
        canvas_.setTextColor(TFT_WHITE);
        canvas_.setTextSize(3);
        canvas_.drawString("Geschafft!", canvas_.width() / 2, canvas_.height() / 2);
        canvas_.setTextSize(2);
        canvas_.drawString("Tippen fuer neue Runde", canvas_.width() / 2, canvas_.height() / 2 + 30);
    }

    canvas_.pushSprite(0, 0);
}

// ---------------------------------------------------------------------
// Sequenz-Merkspiel (3. Klasse)
// ---------------------------------------------------------------------

void GedaechtnisScreen::resetSequenceGame() {
    sequenceLength_ = 1;
    sequence_[0] = static_cast<uint8_t>(esp_random() % 4);
    inputIndex_ = 0;
    playbackTimerMs_ = 0;
    highlightedZone_ = -1;
    sequencePhase_ = SequencePhase::Playback;
    score_ = 0;
    gameOver_ = false;
}

int GedaechtnisScreen::sequenceZoneAt(int x, int y) const {
    if (y < kTopBarHeight) {
        return -1;
    }
    const int midX = M5.Display.width() / 2;
    const int midY = kTopBarHeight + (M5.Display.height() - kTopBarHeight) / 2;
    const int col = (x < midX) ? 0 : 1;
    const int row = (y < midY) ? 0 : 1;
    return row * 2 + col; // 0=oben-links,1=oben-rechts,2=unten-links,3=unten-rechts
}

void GedaechtnisScreen::handleSequenceTouch(int x, int y) {
    const int zone = sequenceZoneAt(x, y);
    if (zone < 0) {
        return;
    }

    if (zone != sequence_[inputIndex_]) {
        haptics::pulse(150);
        gameOver_ = true;
        return;
    }
    haptics::pulse(40);

    ++inputIndex_;
    if (inputIndex_ >= sequenceLength_) {
        awardRoundReward();
        if (sequenceLength_ < kMaxSequence) {
            sequence_[sequenceLength_] = static_cast<uint8_t>(esp_random() % 4);
            ++sequenceLength_;
        }
        sequencePhase_ = SequencePhase::Playback;
        playbackTimerMs_ = 0;
        inputIndex_ = 0;
    }
}

void GedaechtnisScreen::updateSequence(uint32_t deltaMs) {
    if (gameOver_) {
        const auto touch = M5.Touch.getDetail();
        if (touch.wasPressed()) {
            if (touchedHomeIcon(touch.x, touch.y)) {
                stateMachine_.requestSwitch(ScreenId::Home);
                return;
            }
            resetSequenceGame();
        }
        draw();
        return;
    }

    if (sequencePhase_ == SequencePhase::Playback) {
        playbackTimerMs_ += deltaMs;
        const uint32_t within = playbackTimerMs_ % kSequenceStepMs;
        const int stepIndex = static_cast<int>(playbackTimerMs_ / kSequenceStepMs);

        if (stepIndex >= sequenceLength_) {
            highlightedZone_ = -1;
            sequencePhase_ = SequencePhase::WaitingForInput;
            inputIndex_ = 0;
            playbackTimerMs_ = 0;
        } else {
            highlightedZone_ = (within < kSequenceOnMs) ? sequence_[stepIndex] : -1;
        }
        draw();
        return;
    }

    // WaitingForInput
    const auto touch = M5.Touch.getDetail();
    if (!touch.wasPressed()) {
        return;
    }
    if (touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Home);
        return;
    }
    handleSequenceTouch(touch.x, touch.y);
    draw();
}

void GedaechtnisScreen::drawSequenceGame() {
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), canvas_.height(), gfxkit::darken(theme::kPanel, 0.55f),
                              theme::kBackground);
    gfxkit::verticalGradient(&canvas_, 0, 0, canvas_.width(), kTopBarHeight, gfxkit::lighten(theme::kPanel, 0.15f),
                              gfxkit::darken(theme::kPanel, 0.25f));
    canvas_.setTextColor(theme::kText);
    canvas_.setTextDatum(top_left);
    canvas_.setTextSize(1);
    char buf[16];
    snprintf(buf, sizeof(buf), "Runde: %d", sequenceLength_);
    canvas_.drawString(buf, 4, 4);

    static constexpr uint16_t kZoneColors[4] = {TFT_RED, TFT_GREEN, TFT_YELLOW, TFT_CYAN};
    const int zoneW = canvas_.width() / 2;
    const int zoneH = (canvas_.height() - kTopBarHeight) / 2;

    for (int zone = 0; zone < 4; ++zone) {
        const int col = zone % 2;
        const int row = zone / 2;
        const int x = col * zoneW;
        const int y = kTopBarHeight + row * zoneH;
        gfxkit::bevelPanel(&canvas_, x + 4, y + 4, zoneW - 8, zoneH - 8, 6, kZoneColors[zone], true);
        // Nutzer-Feedback: ein weisses Aufblinken der ganzen Zone war
        // gegen die helleren Zonenfarben (Gelb/Cyan) schlecht sichtbar und
        // verdeckte zudem den Farb-Landmark der Zone. Ein grosser
        // schwarzer Punkt in der Mitte ist gegen alle vier Zonenfarben
        // gleichermassen gut erkennbar.
        if (highlightedZone_ == zone) {
            const int cx = x + zoneW / 2;
            const int cy = y + zoneH / 2;
            const int r = (zoneW < zoneH ? zoneW : zoneH) / 2 - 20;
            canvas_.fillCircle(cx, cy, r, TFT_BLACK);
        }
    }

    drawHomeIcon();

    if (gameOver_) {
        canvas_.setTextDatum(middle_center);
        canvas_.setTextColor(TFT_WHITE);
        canvas_.setTextSize(3);
        canvas_.drawString("Game Over", canvas_.width() / 2, canvas_.height() / 2 - 15);
        canvas_.setTextSize(2);
        char scoreBuf[24];
        snprintf(scoreBuf, sizeof(scoreBuf), "Laengste Reihe: %d", sequenceLength_ - 1);
        canvas_.drawString(scoreBuf, canvas_.width() / 2, canvas_.height() / 2 + 20);
    }

    canvas_.pushSprite(0, 0);
}
