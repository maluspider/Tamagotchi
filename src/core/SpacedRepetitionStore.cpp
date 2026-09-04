#include "SpacedRepetitionStore.h"

#include <ArduinoJson.h>

#include "RtcClock.h"
#include "storage/JsonStore.h"

namespace {

// Intervall bis zur naechsten Wiederholung je Box (docs/projektplan.md
// Abschnitt 8.3). Box 1 (frisch falsch beantwortet oder neu) wird
// absichtlich noch am selben Tag wieder faellig (0 statt 1 Tag) - eine
// falsche Antwort faellt auf Box 1 zurueck, sollte dem Kind aber
// zeitnah/noch in derselben Sitzung erneut begegnen koennen, nicht erst am
// naechsten Kalendertag (Nutzer-Feedback: "falsche Antworten oefters
// wiederholt"). Siehe auch TaskEngine::pickNextTask(), das Box-1-Items
// zusaetzlich hoeher gewichtet.
int intervalDaysForBox(uint8_t box) {
    switch (box) {
        case 1: return 0;
        case 2: return 2;
        case 3: return 4;
        case 4: return 7;
        case 5: return 14;
        default: return 1;
    }
}

String pathForFach(const String& fach) {
    return String("/progress/aufgaben_") + fach + ".json";
}

} // namespace

void SpacedRepetitionStore::load(const char* fach) {
    fach_ = fach;
    items_.clear();

    JsonDocument doc;
    const String path = pathForFach(fach_);
    if (!storage::loadJsonWithFallback(path.c_str(), doc)) {
        return; // noch keine Historie fuer dieses Fach - alles gilt als neu
    }

    for (JsonPair kv : doc.as<JsonObject>()) {
        LeitnerItem item;
        item.id = kv.key().c_str();
        item.box = kv.value()["box"] | static_cast<uint8_t>(1);
        item.naechsteWiederholungIso = kv.value()["naechste_wiederholung"] | "";
        items_.push_back(item);
    }
}

void SpacedRepetitionStore::save() const {
    JsonDocument doc;
    for (const auto& item : items_) {
        doc[item.id]["box"] = item.box;
        doc[item.id]["naechste_wiederholung"] = item.naechsteWiederholungIso;
    }
    const String path = pathForFach(fach_);
    storage::saveJsonAtomic(path.c_str(), doc);
}

LeitnerItem* SpacedRepetitionStore::find(const String& id) {
    for (auto& item : items_) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

bool SpacedRepetitionStore::hasItem(const String& id) const {
    for (const auto& item : items_) {
        if (item.id == id) {
            return true;
        }
    }
    return false;
}

uint8_t SpacedRepetitionStore::boxFor(const String& id) const {
    for (const auto& item : items_) {
        if (item.id == id) {
            return item.box;
        }
    }
    return 1;
}

bool SpacedRepetitionStore::isDue(const String& id, const String& todayIso) const {
    for (const auto& item : items_) {
        if (item.id == id) {
            return rtcclock::epochDayFromIso(todayIso) >= rtcclock::epochDayFromIso(item.naechsteWiederholungIso);
        }
    }
    return false; // unbekanntes Item gilt nicht als "faellig" - siehe hasItem() fuer "neu"
}

void SpacedRepetitionStore::recordAnswer(const String& id, bool correct, const String& todayIso) {
    LeitnerItem* item = find(id);
    if (!item) {
        LeitnerItem newItem;
        newItem.id = id;
        newItem.box = 1;
        newItem.naechsteWiederholungIso = todayIso;
        items_.push_back(newItem);
        item = &items_.back();
    }

    item->box = correct ? static_cast<uint8_t>(item->box < 5 ? item->box + 1 : 5) : static_cast<uint8_t>(1);

    const long dueEpochDay = rtcclock::epochDayFromIso(todayIso) + intervalDaysForBox(item->box);
    item->naechsteWiederholungIso = rtcclock::isoFromEpochDay(dueEpochDay);
}
