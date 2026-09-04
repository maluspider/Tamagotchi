#include "WebServerService.h"

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <SD.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_random.h>

#include "CharacterEngine.h"
#include "Subject.h"
#include "config.h"
#include "storage/JsonStore.h"
#include "storage/ProfileStore.h"

namespace webserverservice {

namespace {

WebServer server(80);
AppContext* g_app = nullptr;
bool g_running = false;

String htmlEscape(const String& in) {
    String out;
    out.reserve(in.length());
    for (size_t i = 0; i < in.length(); ++i) {
        const char c = in[i];
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c; break;
        }
    }
    return out;
}

String pageHeader(const String& title) {
    String html = "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<title>" + htmlEscape(title) + "</title>";
    html +=
        "<style>body{font-family:sans-serif;margin:16px;max-width:640px;}"
        "table{border-collapse:collapse;width:100%;}"
        "td,th{border:1px solid #ccc;padding:4px 8px;text-align:left;}"
        "input,select{padding:4px;margin:2px 0;}"
        "a.button,button{display:inline-block;padding:6px 12px;background:#2a5;color:#fff;"
        "text-decoration:none;border:none;border-radius:4px;cursor:pointer;}"
        "a.danger{background:#c33;}</style></head><body>";
    html += "<p><a href=\"/\">&larr; Uebersicht</a></p>";
    html += "<h1>" + htmlEscape(title) + "</h1>";
    return html;
}

String pageFooter() {
    return "</body></html>";
}

void sendHtml(const String& body) {
    server.send(200, "text/html; charset=utf-8", body);
}

void redirectTo(const String& location) {
    server.sendHeader("Location", location);
    server.send(303, "text/plain", "");
}

// Aufgabenpool-Dateizugriff - Lese-Pendant zu TaskEngine::loadPool(), aber
// bewusst eigenstaendig gehalten statt TaskEngine wiederzuverwenden: die
// Web-Handler brauchen Schreibzugriff (add/delete), TaskEngine ist als
// reiner Leser fuer den Spielbetrieb konzipiert (siehe dortigen Kommentar).

String taskFilePath(const String& fach, const String& klasse) {
    return "/tasks/" + fach + "_" + klasse + ".json";
}

bool loadTaskDoc(const String& path, JsonDocument& doc) {
    if (!SD.exists(path)) {
        return false;
    }
    File file = SD.open(path, FILE_READ);
    if (!file) {
        return false;
    }
    const DeserializationError err = deserializeJson(doc, file);
    file.close();
    return err == DeserializationError::Ok;
}

void handleRoot() {
    String html =
        pageHeader(String("Tamagotchi") + (g_app->profile.name.length() ? " - " + g_app->profile.name : ""));

    html += "<h2>Fortschritt</h2><ul>";
    html += "<li>Stufe: " + String(CharacterEngine::stageName(g_app->character.stage())) + "</li>";
    html += "<li>Erfahrungspunkte: " + String(g_app->character.xp()) + "</li>";
    html += "<li>Spielzeit heute: " + String(g_app->playtime.spentMinutesToday()) + " / " +
            String(g_app->playtime.earnedMinutesToday()) + " Min verbraucht (Tageslimit " +
            String(g_app->playtime.dailyLimitMinutes()) + " Min)</li>";
    html += "</ul>";

    html += "<h2>Aufgaben-Faecher (Klasse " + String(g_app->profile.klasse) + ")</h2><ul>";
    for (size_t i = 0; i < kSubjectCount; ++i) {
        const Subject subject = static_cast<Subject>(i);
        if (subject == Subject::Franzoesisch && g_app->profile.klasse < 3) {
            continue;
        }
        const String slug = subjectSlug(subject);
        html += "<li><a href=\"/tasks?fach=" + slug + "&klasse=" + String(g_app->profile.klasse) + "\">" + slug +
                "</a></li>";
    }
    html += "</ul>";

    html += "<h2>Tageslimit</h2>";
    html += "<form method=\"POST\" action=\"/settings\">";
    html += "Minuten pro Tag: <input type=\"number\" name=\"limit\" min=\"15\" max=\"180\" value=\"" +
            String(g_app->playtime.dailyLimitMinutes()) + "\"> ";
    html += "<button type=\"submit\">Speichern</button></form>";

    html += "<h2>Firmware-Update (OTA)</h2>";
    html += "<p>Mit PlatformIO vom Rechner, der mit diesem WLAN verbunden ist:</p>";
    html += "<pre>pio run -t upload --upload-port " + WiFi.softAPIP().toString() + "</pre>";

    html += pageFooter();
    sendHtml(html);
}

void handleTasksList() {
    if (!server.hasArg("fach") || !server.hasArg("klasse")) {
        server.send(400, "text/plain", "fach und klasse erforderlich");
        return;
    }
    const String fach = server.arg("fach");
    const String klasse = server.arg("klasse");
    const String path = taskFilePath(fach, klasse);

    JsonDocument doc;
    const bool loaded = loadTaskDoc(path, doc);

    String html = pageHeader("Aufgaben: " + fach + " (Klasse " + klasse + ")");

    if (!loaded) {
        html += "<p>Keine Aufgaben gefunden oder Datei nicht lesbar.</p>";
    } else {
        html += "<table><tr><th>Frage</th><th>Antworten</th><th>Richtig</th><th></th></tr>";
        for (JsonObject item : doc.as<JsonArray>()) {
            const String id = item["id"] | "";
            const String frage = item["frage"] | "";
            const int richtig = item["richtig"] | 0;

            String antworten;
            int idx = 0;
            for (JsonVariant a : item["antworten"].as<JsonArray>()) {
                if (idx > 0) {
                    antworten += ", ";
                }
                antworten += a.as<String>();
                ++idx;
            }
            const String richtigText =
                (richtig >= 0 && richtig < idx) ? item["antworten"][richtig].as<String>() : String("?");

            html += "<tr><td>" + htmlEscape(frage) + "</td><td>" + htmlEscape(antworten) + "</td><td>" +
                    htmlEscape(richtigText) +
                    "</td><td><a class=\"button danger\" href=\"/tasks/delete?fach=" + fach + "&klasse=" + klasse +
                    "&id=" + id + "\" onclick=\"return confirm('Wirklich loeschen?')\">Loeschen</a></td></tr>";
        }
        html += "</table>";
    }

    html += "<h2>Neue Aufgabe</h2>";
    html += "<form method=\"POST\" action=\"/tasks/add\">";
    html += "<input type=\"hidden\" name=\"fach\" value=\"" + fach + "\">";
    html += "<input type=\"hidden\" name=\"klasse\" value=\"" + klasse + "\">";
    html += "<div>Frage: <input name=\"frage\" style=\"width:100%\"></div>";
    html += "<div>Antwort 1: <input name=\"a0\"> Antwort 2: <input name=\"a1\"> Antwort 3: <input name=\"a2\"></div>";
    html +=
        "<div>Richtige Antwort: <select name=\"richtig\"><option value=\"0\">1</option>"
        "<option value=\"1\">2</option><option value=\"2\">3</option></select></div>";
    html +=
        "<div>Schwierigkeit (1-5): <input type=\"number\" name=\"schwierigkeit\" min=\"1\" max=\"5\" "
        "value=\"1\"></div>";
    html += "<button type=\"submit\">Hinzufuegen</button>";
    html += "</form>";

    html += pageFooter();
    sendHtml(html);
}

void handleTasksAdd() {
    if (!server.hasArg("fach") || !server.hasArg("klasse") || !server.hasArg("frage")) {
        server.send(400, "text/plain", "Pflichtfelder fehlen");
        return;
    }
    const String fach = server.arg("fach");
    const String klasse = server.arg("klasse");
    const String path = taskFilePath(fach, klasse);

    JsonDocument doc;
    loadTaskDoc(path, doc); // ok, wenn die Datei (noch) nicht existiert - doc bleibt dann leer
    if (doc.isNull() || !doc.is<JsonArray>()) {
        doc.to<JsonArray>();
    }
    JsonArray arr = doc.as<JsonArray>();

    JsonObject item = arr.add<JsonObject>();
    const String newId = fach + "_" + klasse + "_web_" + String(static_cast<unsigned long>(esp_random()), HEX);
    item["id"] = newId;
    item["fach"] = fach;
    item["klasse"] = klasse.toInt();
    item["schwierigkeit"] = server.hasArg("schwierigkeit") ? server.arg("schwierigkeit").toInt() : 1;
    item["typ"] = "multiple_choice";
    item["frage"] = server.arg("frage");
    JsonArray answers = item["antworten"].to<JsonArray>();
    answers.add(server.arg("a0"));
    answers.add(server.arg("a1"));
    answers.add(server.arg("a2"));
    item["richtig"] = server.hasArg("richtig") ? server.arg("richtig").toInt() : 0;

    storage::saveJsonAtomic(path.c_str(), doc);

    redirectTo("/tasks?fach=" + fach + "&klasse=" + klasse);
}

void handleTasksDelete() {
    if (!server.hasArg("fach") || !server.hasArg("klasse") || !server.hasArg("id")) {
        server.send(400, "text/plain", "fach, klasse und id erforderlich");
        return;
    }
    const String fach = server.arg("fach");
    const String klasse = server.arg("klasse");
    const String id = server.arg("id");
    const String path = taskFilePath(fach, klasse);

    JsonDocument doc;
    if (loadTaskDoc(path, doc)) {
        JsonArray arr = doc.as<JsonArray>();
        for (size_t i = 0; i < arr.size(); ++i) {
            const String itemId = arr[i]["id"] | "";
            if (itemId == id) {
                arr.remove(i);
                break;
            }
        }
        storage::saveJsonAtomic(path.c_str(), doc);
    }

    redirectTo("/tasks?fach=" + fach + "&klasse=" + klasse);
}

void handleSettings() {
    if (server.hasArg("limit")) {
        int limit = server.arg("limit").toInt();
        if (limit < 15) {
            limit = 15;
        }
        if (limit > 180) {
            limit = 180;
        }
        g_app->profile.dailyLimitMinutesOverride = static_cast<uint16_t>(limit);
        g_app->playtime.setDailyLimitMinutes(static_cast<uint16_t>(limit));
        profilestore::save(g_app->profile);
    }
    redirectTo("/");
}

void handleNotFound() {
    server.send(404, "text/plain", "Nicht gefunden");
}

void registerRoutes() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/tasks", HTTP_GET, handleTasksList);
    server.on("/tasks/add", HTTP_POST, handleTasksAdd);
    server.on("/tasks/delete", HTTP_GET, handleTasksDelete);
    server.on("/settings", HTTP_POST, handleSettings);
    server.onNotFound(handleNotFound);
}

} // namespace

void start(AppContext* app) {
    if (g_running) {
        return;
    }
    g_app = app;

    const String ssid = String("Tamagotchi-") + (app->profile.name.length() ? app->profile.name : String("Geraet"));
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), config::kWebSyncApPassword);

    registerRoutes();
    server.begin();

    ArduinoOTA.setHostname(config::kOtaHostname);
    ArduinoOTA.begin();

    g_running = true;
}

void stop() {
    if (!g_running) {
        return;
    }
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    g_app = nullptr;
    g_running = false;
}

void handleClient() {
    if (!g_running) {
        return;
    }
    server.handleClient();
    ArduinoOTA.handle();
}

bool isRunning() {
    return g_running;
}

String apSsid() {
    return WiFi.softAPSSID();
}

String apIp() {
    return WiFi.softAPIP().toString();
}

} // namespace webserverservice
