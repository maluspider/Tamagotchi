#pragma once

#include <cstdint>

#include "../core/AppContext.h"
#include "../core/Screen.h"
#include "../core/StateMachine.h"

// Fach-Auswahl (docs/projektplan.md Abschnitt 5): icon-basiertes Menue
// (Review: icon-first fuer die juengere Zielgruppe, Abschnitt 5) zu den
// vier Multiple-Choice-Faechern (Subject, Abschnitt 8.1) plus
// Gedaechtnistraining (eigene Spielmechanik, siehe GedaechtnisScreen).
// Franzoesisch nur ab Klasse 3 (Fremdsprachenbeginn, Abschnitt 8.1).
class SubjectSelectScreen : public Screen {
public:
    SubjectSelectScreen(AppContext& app, StateMachine& stateMachine);

    void onEnter() override;
    void update(uint32_t deltaMs) override;
    void draw() override;

private:
    enum class EntryKind { Mathe, Rechtschreibung, Franzoesisch, Quiz, Gedaechtnis };
    struct Entry {
        EntryKind kind;
    };
    static constexpr int kMaxEntries = 5;
    static constexpr int kCols = 3;

    int buildEntries(Entry* out, int maxCount) const;
    void drawEntry(const Entry& entry, int cx, int cy, int cellSize) const;
    void activateEntry(const Entry& entry);
    void drawHomeIcon() const;
    bool touchedHomeIcon(int x, int y) const;

    AppContext& app_;
    StateMachine& stateMachine_;
};
