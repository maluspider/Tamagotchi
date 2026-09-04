#include "TaskEngine.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <esp_random.h>

// Aufgabenpools sind vom Content-Team/Eltern gepflegter, statischer Inhalt
// (Abschnitt 8.2/13) - im Unterschied zu profile.json/progress.json wird
// hier bewusst nicht ueber JsonStore mit .tmp/.bak-Rotation geschrieben,
// denn diese Dateien werden von der Firmware nur gelesen, nie geschrieben.

bool TaskEngine::loadPool(Subject subject, uint8_t klasse) {
    pool_.clear();
    lastIndex_ = -1;

    const String path = String("/tasks/") + subjectSlug(subject) + "_" + String(klasse) + ".json";
    if (!SD.exists(path)) {
        return false;
    }
    File file = SD.open(path, FILE_READ);
    if (!file) {
        return false;
    }

    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err != DeserializationError::Ok) {
        return false;
    }

    for (JsonObject item : doc.as<JsonArray>()) {
        Task task;
        task.id = item["id"] | "";
        task.frage = item["frage"] | "";
        task.richtig = item["richtig"] | static_cast<uint8_t>(0);
        task.schwierigkeit = item["schwierigkeit"] | static_cast<uint8_t>(1);

        uint8_t count = 0;
        for (JsonVariant answer : item["antworten"].as<JsonArray>()) {
            if (count >= 4) {
                break;
            }
            task.antworten[count] = answer.as<String>();
            ++count;
        }
        task.antwortenCount = count;

        if (task.frage.length() > 0 && task.antwortenCount > 0) {
            pool_.push_back(task);
        }
    }

    return !pool_.empty();
}

bool TaskEngine::pickNextTask(Task& out, const SpacedRepetitionStore& srs, const DifficultyState& difficulty,
                               const String& todayIso) {
    if (pool_.empty()) {
        return false;
    }

    std::vector<size_t> due;
    std::vector<size_t> fresh;
    for (size_t i = 0; i < pool_.size(); ++i) {
        if (pool_[i].schwierigkeit > difficulty.stage) {
            continue; // Schwierigkeitsfilter (Abschnitt 8.4)
        }
        if (srs.hasItem(pool_[i].id)) {
            if (srs.isDue(pool_[i].id, todayIso)) {
                due.push_back(i);
            }
        } else {
            fresh.push_back(i);
        }
    }

    // Kuerzlich falsch beantwortete Items (Box 1) zusaetzlich hoeher
    // gewichten, indem sie mehrfach in den Kandidatenkreis aufgenommen
    // werden - sie sollen unter mehreren gleichzeitig faelligen Items eher
    // an die Reihe kommen als laengst gefestigte (Nutzer-Feedback: "falsche
    // Antworten oefters wiederholt").
    std::vector<size_t> candidates = due;
    for (size_t idx : due) {
        if (srs.boxFor(pool_[idx].id) <= 1) {
            candidates.push_back(idx);
        }
    }
    for (int n = 0; n < 2 && !fresh.empty(); ++n) {
        const size_t pick = esp_random() % fresh.size();
        candidates.push_back(fresh[pick]);
        fresh.erase(fresh.begin() + static_cast<long>(pick));
    }

    if (candidates.empty()) {
        // Nichts Faelliges/Neues auf der aktuellen Schwierigkeitsstufe -
        // Fallback: irgendein Item auf dieser Stufe.
        for (size_t i = 0; i < pool_.size(); ++i) {
            if (pool_[i].schwierigkeit <= difficulty.stage) {
                candidates.push_back(i);
            }
        }
    }
    if (candidates.empty()) {
        // Immer noch nichts (z. B. sehr kleiner Pool) - letzter Fallback:
        // der gesamte Pool, unabhaengig von der Schwierigkeit.
        for (size_t i = 0; i < pool_.size(); ++i) {
            candidates.push_back(i);
        }
    }
    if (candidates.empty()) {
        return false;
    }

    size_t choice = candidates[esp_random() % candidates.size()];
    if (candidates.size() > 1) {
        while (static_cast<int>(choice) == lastIndex_) {
            choice = candidates[esp_random() % candidates.size()];
        }
    }
    lastIndex_ = static_cast<int>(choice);
    out = pool_[choice];
    return true;
}
