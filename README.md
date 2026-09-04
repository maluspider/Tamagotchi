# Tamagotchi-Lernspielgerät

Firmware für ein tamagotchiartiges Lerngerät auf Basis von zwei
**M5Stack Core2**-Geräten (Arduino/PlatformIO, `M5Unified`). Der vollständige
Projektplan inkl. Review-Anmerkungen steht in
[`docs/projektplan.md`](docs/projektplan.md) – dort auch die Begründung für
alle hier genannten Design-Entscheidungen.

**Aktueller Stand: Phase 0 + erster Teil von Phase 1** (siehe Abschnitt 14
des Plans) – State-Machine, Storage-Layer, Erststart-Einrichtung mit
Kind-Profil-Auswahl, Home-Screen mit Platzhalter-Charakter, Aufgaben-Modus
(nur Mathe, ohne Spaced Repetition), Snake und Uhr/Wecker. Restliche
Fächer, Spaced Repetition, die übrigen 8 Spiele sowie Alltagsfunktionen-
Menü/Einstellungen folgen in den Phasen 2–4.

## Vor dem ersten Flashen: zwei Dinge anpassen

1. **Eigene Kinder eintragen:** [`include/KidProfiles.h`](include/KidProfiles.h)
   öffnen und `kKidProfiles` mit den echten Namen und dem Alter der Kinder
   befüllen (aktuell Platzhalter "Kind 1"/"Kind 2"). Der Tamagotchi-
   Charakter trägt danach diesen Namen; die Klassenstufe (und damit die
   Aufgaben-Schwierigkeit) wird automatisch aus dem Alter hergeleitet
   (`klasseForAge()`, Schwelle in derselben Datei anpassbar).
2. **Aufgaben-Content auf die SD-Karte kopieren:** den Ordner
   [`sdcard/tasks/`](sdcard/tasks) auf die microSD-Karte kopieren, sodass
   dort `/tasks/mathe_1.json` und `/tasks/mathe_3.json` liegen (siehe
   "SD-Karte" unten). Ohne diese Dateien zeigt der Aufgaben-Modus "Keine
   Aufgaben gefunden" an.

## Hardware

- M5Stack **Core2** (nicht Fire, nicht CoreS3) – jede Revision (V1.0/V1.1)
  funktioniert, siehe `docs/projektplan.md` Abschnitt 2
- USB-C-Kabel zum Flashen/Laden
- microSD-Karte (wird für den Aufgaben-Content benötigt, siehe oben)

## SD-Karte

Die Firmware liest Aufgaben-Content von der SD-Karte, schreibt aber auch
Profil-/Fortschrittsdaten dorthin (`/profile.json`, `/progress.json`,
atomar via `JsonStore`, siehe Abschnitt 6 des Plans). Karte vor dem ersten
Start mit FAT32 formatieren und den Inhalt von `sdcard/` (aus diesem Repo)
ins Wurzelverzeichnis kopieren, sodass `/tasks/mathe_1.json` etc. existieren.

## Setup

1. [PlatformIO Core](https://platformio.org/install/cli) installieren (per
   `pip install platformio` oder die PlatformIO-IDE-Extension für VS Code).
2. Repo klonen, dann im Projektordner:
   ```
   pio run
   ```
   Beim ersten Build lädt PlatformIO die ESP32-Toolchain herunter, das kann
   ein paar Minuten dauern.
3. Core2 per USB anschliessen und flashen:
   ```
   pio run --target upload
   ```
4. Seriellen Monitor öffnen (optional, für Debug-Ausgaben):
   ```
   pio device monitor
   ```

### Bekanntes Setup-Problem: `ModuleNotFoundError: No module named 'intelhex'`

Falls der Build beim Bootloader-/Partitions-Schritt mit einer fehlenden
`intelhex`-Python-Abhängigkeit von `esptool.py` abbricht, in der von
PlatformIO verwendeten Python-Umgebung nachinstallieren:
```
pip install intelhex
```
Danach `pio run` erneut ausführen – Toolchain und Libraries sind dann schon
lokal gecacht, der zweite Build ist entsprechend schnell.

## Bedienung (Phase-1-Umfang)

- **Home-Screen:** zeigt Charakter + Namen, Uhrzeit, verfügbare Spielzeit.
  Untere Icon-Leiste: Stift-Icon → Aufgaben-Modus, Play-Dreieck → Snake
  (nur aktiv ab Charakterstufe "Baby" und mit Spielzeitguthaben), Uhr-Icon
  → Uhr/Wecker.
- **Aufgaben-Modus:** Multiple-Choice-Frage antippen; richtige Antwort gibt
  Erfahrungspunkte + 2 Minuten Spielzeit (Abschnitt 7/9). Zurück zu Home
  über das Haus-Icon oben rechts.
- **Snake:** Antippen relativ zur Bildschirmmitte steuert die Richtung
  (oben/unten/links/rechts). Verbraucht laufend Spielzeitguthaben; ist es
  aufgebraucht, geht es automatisch zurück zu Home.
- **Uhr/Wecker:** grosse Digitaluhr, eine einstellbare Alarmzeit (Glocke
  antippen = an/aus, +/- für Stunde/Minute). Der Wecker löst auch aus,
  wenn gerade ein anderer Screen aktiv ist (`AlarmService`).

## Projektstruktur

```
platformio.ini             PlatformIO-Projektdefinition (Board, Libraries)
include/config.h            Zentrale Konfigurationswerte (Pfade, Limits, Pins)
include/KidProfiles.h       Eigene Kinder eintragen (Name + Alter) - siehe oben
sdcard/tasks/                Aufgaben-Content zum Kopieren auf die SD-Karte
src/main.cpp                 Einstiegspunkt (setup()/loop())
src/core/                    Screen-unabhängige Module
  Screen.h / ScreenId.h / StateMachine.*   Zustandsautomat (Abschnitt 5)
  CharacterEngine.*         Tamagotchi-Charaktersystem (Abschnitt 9)
  PlaytimeAccount.*         Spielzeitkonto (Abschnitt 7)
  TaskEngine.*               Aufgaben-Engine (Phase 1: nur Mathe)
  RtcClock.*                RTC-Wrapper, Kalenderarithmetik, NTP-Sync-Stub
  AlarmService.*             Screen-unabhängige Wecker-Prüfung
  PinCode.*                 Gehashter Eltern-PIN (Abschnitt 6, Review)
  AppContext.*               Gemeinsamer Laufzeit-Zustand für alle Screens
  storage/                   Atomare JSON-Persistenz auf der SD-Karte
    JsonStore.*               .tmp/.bak-Rotation gegen Stromausfall-Korruption
    ProfileStore.*            /profile.json
    ProgressStore.*           /progress.json
src/screens/                 Konkrete Screens (je eine Klasse pro Screen)
  BootScreen.*               Init, lädt Profil/Fortschritt
  ProfileSetupScreen.*       Erststart: Kind-Profil wählen (icon-first)
  HomeScreen.*                Charakter + Statusleiste + Navigation
  TaskScreen.*                Aufgaben-Modus (Mathe)
  SnakeScreen.*                Snake, ueber M5Canvas-Offscreen-Sprite gezeichnet
  ClockScreen.*                Uhr/Wecker-Einstellung
docs/projektplan.md          Vollständiger Projektplan inkl. Review
```

## Hinweis zum Kompilieren in dieser Session

Der Code wurde in dieser Cloud-Umgebung geschrieben, aber **nicht** über
`pio run` gebaut/verifiziert – der Netzwerkzugriff auf die
PlatformIO-Paketregistrierung (zum Laden der ESP32-Toolchain und
Libraries) ist hier durch die Organisations-Egress-Policy blockiert. Bitte
vor dem Flashen lokal `pio run` ausführen, um sicherzustellen, dass alles
sauber kompiliert.
