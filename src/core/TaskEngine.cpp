#include "TaskEngine.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <esp_random.h>

// Aufgabenpools sind vom Content-Team/Eltern gepflegter, statischer Inhalt
// (Abschnitt 8.2/13) - im Unterschied zu profile.json/progress.json wird
// hier bewusst nicht ueber JsonStore mit .tmp/.bak-Rotation geschrieben,
// denn diese Dateien werden von der Firmware nur gelesen, nie geschrieben.

bool TaskEngine::loadMathePool(uint8_t klasse) {
    pool_.clear();
    lastIndex_ = -1;

    const String path = String("/tasks/mathe_") + String(klasse) + ".json";
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

bool TaskEngine::pickRandomTask(Task& out) {
    if (pool_.empty()) {
        return false;
    }
    if (pool_.size() == 1) {
        out = pool_[0];
        lastIndex_ = 0;
        return true;
    }

    int index;
    do {
        index = static_cast<int>(esp_random() % pool_.size());
    } while (index == lastIndex_);

    lastIndex_ = index;
    out = pool_[static_cast<size_t>(index)];
    return true;
}
