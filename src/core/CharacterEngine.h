#pragma once

#include <Arduino.h>
#include <cstdint>

// Tamagotchi-Charaktersystem (docs/projektplan.md Abschnitt 9).
//
// Review-Entscheidung: Es gibt keinen Ruckschritt mehr durch falsche
// Antworten (kein EP-Verlust, kein Stufen-Downgrade). Laut Review vermeidet
// das sowohl ein demotivierendes Bestrafungssignal fuer Kinder beim Lernen
// als auch ein "Flackern" der Stufe direkt an einer XP-Schwelle. "Traurig"
// wirkt der Charakter ausschliesslich bei mehrtaegiger Inaktivitaet
// (config::kSadAfterDaysInactive), niemals durch Fehler - siehe
// Abschnitt 7/9 (Review).
enum class CharacterStage : uint8_t {
    Ei = 0,
    Baby,
    Kind,
    Junior,
    Experte,
    Meister,
};

class CharacterEngine {
public:
    CharacterEngine() = default;

    void load(uint32_t xp, const String& lastCareDateIso);

    // Erhoeht die Erfahrungspunkte (z. B. nach richtig geloester Aufgabe).
    // Die Stufe steigt ausschliesslich vorwaerts - siehe Klassenkommentar.
    void addXp(uint32_t amount);

    void markCaredForToday(const String& todayIso);

    // true, wenn seit config::kSadAfterDaysInactive Tagen keine Pflege
    // stattfand.
    bool isSad(const String& todayIso) const;

    uint32_t xp() const { return xp_; }
    CharacterStage stage() const { return stage_; }
    const String& lastCareDateIso() const { return lastCareDateIso_; }

    static const char* stageName(CharacterStage stage);

private:
    static CharacterStage stageForXp(uint32_t xp);

    uint32_t xp_ = 0;
    CharacterStage stage_ = CharacterStage::Ei;
    String lastCareDateIso_;
};
