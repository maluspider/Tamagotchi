#include "TextFit.h"

namespace textfit {

namespace {
constexpr int kMaxLines = 6;

// Bricht `text` bei Leerzeichen in Zeilen, die bei der aktuell auf `target`
// gesetzten Textgroesse jeweils <= maxWidth breit sind. Ein einzelnes Wort,
// das schon fuer sich allein zu breit ist, wird nicht weiter aufgebrochen
// (bleibt als eigene, ggf. zu breite Zeile stehen) - dem waere ohnehin nur
// mit noch kleinerer Schrift beizukommen, was die aeussere Schleife in
// drawFitted() uebernimmt.
int wrapLines(LovyanGFX* target, const String& text, int maxWidth, String outLines[kMaxLines]) {
    int lineCount = 0;
    String currentLine;
    int wordStart = 0;
    const int len = text.length();

    while (wordStart <= len && lineCount < kMaxLines) {
        int spaceIdx = text.indexOf(' ', wordStart);
        if (spaceIdx < 0) {
            spaceIdx = len;
        }
        const String word = text.substring(wordStart, spaceIdx);
        const String candidate = currentLine.length() ? currentLine + " " + word : word;
        if (currentLine.length() == 0 || target->textWidth(candidate) <= maxWidth) {
            currentLine = candidate;
        } else {
            outLines[lineCount++] = currentLine;
            currentLine = word;
        }
        wordStart = spaceIdx + 1;
        if (spaceIdx >= len) {
            break;
        }
    }
    if (currentLine.length() && lineCount < kMaxLines) {
        outLines[lineCount++] = currentLine;
    }
    return lineCount;
}
} // namespace

void drawFitted(LovyanGFX* target, const String& text, int cx, int cy, int maxWidth, int maxHeight, int maxTextSize,
                 uint16_t color, int minTextSize) {
    String lines[kMaxLines];
    int lineCount = 0;
    int textSize = maxTextSize;

    for (; textSize > minTextSize; --textSize) {
        target->setTextSize(textSize);
        lineCount = wrapLines(target, text, maxWidth, lines);
        if (lineCount * target->fontHeight() <= maxHeight) {
            break;
        }
    }
    if (textSize == minTextSize) {
        target->setTextSize(minTextSize);
        lineCount = wrapLines(target, text, maxWidth, lines);
    }

    target->setTextColor(color);
    target->setTextDatum(middle_center);
    const int lineHeight = target->fontHeight();
    const int totalHeight = lineCount * lineHeight;
    int y = cy - totalHeight / 2 + lineHeight / 2;
    for (int i = 0; i < lineCount; ++i) {
        target->drawString(lines[i], cx, y);
        y += lineHeight;
    }
}

} // namespace textfit
