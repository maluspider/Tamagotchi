#pragma once

#include <Arduino.h>

#include "AppContext.h"

// Web-Interface (docs/projektplan.md Abschnitt 12): Fortschritt/Statistik
// einsehen, Aufgaben hinzufuegen/bearbeiten ohne Neu-Flashen, Tageslimit
// zentral anpassen - plus Firmware-Updates ueber ArduinoOTA (Abschnitt 3,
// Review: OTA-Update-Pfad).
//
// Review-Abweichung: Der Plan empfahl bei tatsaechlicher Umsetzung von
// Phase 5 den `ESP32Async`-Fork von ESPAsyncWebServer statt des
// unmaintained `me-no-dev`-Originals. Diese Implementierung nutzt
// stattdessen bewusst den SYNCHRONEN `WebServer` aus dem arduino-esp32-
// Kern (keine zusaetzliche Bibliotheksabhaengigkeit). Grund: Async-
// Handler von ESPAsyncWebServer laufen in einem eigenen FreeRTOS-Task
// (AsyncTCP) parallel zum Hauptloop, der ebenfalls auf die SD-Karte
// zugreift (JsonStore, TaskEngine, HighscoreStore, ...) - ohne Mutex waere
// das ein Data Race auf dem SPI-Bus. Ein sauberer Mutex laesst sich ohne
// Testgeraet in dieser Session nicht zuverlaessig verifizieren. Der
// synchrone WebServer verarbeitet Anfragen ausschliesslich innerhalb von
// handleClient() im Hauptloop - dadurch greift zu jedem Zeitpunkt nur ein
// einziger Kontrollfluss auf die SD-Karte zu, das Race-Problem entfaellt
// strukturell. Für ein Admin-Panel mit einem Elternteil zur Zeit ist der
// Verzicht auf Nebenlaeufigkeit kein spuerbarer Nachteil.
namespace webserverservice {

// Startet den geraeteeigenen Access Point (SSID "Tamagotchi-<Name>",
// Passwort config::kWebSyncApPassword) + WebServer + ArduinoOTA. app muss
// mindestens so lange gueltig bleiben, bis stop() aufgerufen wird.
void start(AppContext* app);

void stop();

// Muss aus dem aktiven Screen heraus (WebSyncScreen) bei jedem update()
// aufgerufen werden, solange isRunning() true ist.
void handleClient();

bool isRunning();

String apSsid();
String apIp();

} // namespace webserverservice
