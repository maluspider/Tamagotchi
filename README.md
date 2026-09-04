# Tamagotchi-Lernspielgerät

Firmware für ein tamagotchiartiges Lerngerät auf Basis von zwei
**M5Stack Core2**-Geräten (Arduino/PlatformIO, `M5Unified`). Der vollständige
Projektplan inkl. Review-Anmerkungen steht in
[`docs/projektplan.md`](docs/projektplan.md) – dort auch die Begründung für
alle hier genannten Design-Entscheidungen.

**Aktueller Stand: Phase 0 + Phase 1 + Phase 2** (siehe Abschnitt 14 des
Plans) – State-Machine, Storage-Layer, Erststart-Einrichtung mit
Kind-Profil-Auswahl, Home-Screen mit Platzhalter-Charakter, Aufgaben-Modus
für alle vier Multiple-Choice-Fächer inkl. Spaced Repetition und
Schwierigkeitsanstieg, Gedächtnistraining, Snake und Uhr/Wecker. Die
übrigen 8 Spiele sowie Alltagsfunktionen-Menü/Einstellungen/Nachtmodus
folgen in den Phasen 3–4, das Web-Interface in Phase 5.

## Vor dem ersten Flashen: zwei Dinge anpassen

1. **Eigene Kinder eintragen:** [`include/KidProfiles.h`](include/KidProfiles.h)
   öffnen und `kKidProfiles` mit den echten Namen und dem Alter der Kinder
   befüllen (aktuell Platzhalter "Kind 1"/"Kind 2"). Der Tamagotchi-
   Charakter trägt danach diesen Namen; die Klassenstufe (und damit die
   Aufgaben-Schwierigkeit) wird automatisch aus dem Alter hergeleitet
   (`klasseForAge()`, Schwelle in derselben Datei anpassbar).
2. **Aufgaben-Content auf die SD-Karte kopieren:** den Ordner
   [`sdcard/tasks/`](sdcard/tasks) auf die microSD-Karte kopieren (siehe
   "SD-Karte" unten). Ohne diese Dateien zeigt der Aufgaben-Modus "Keine
   Aufgaben gefunden" an. **Hinweis:** Der mitgelieferte Content ist
   Platzhalter-Content (8–10 Fragen pro Fach/Klasse) zum Ausprobieren der
   Engine, kein geprüfter Lehrplan-21/Passepartout-Stoff – siehe
   Abschnitt 16 des Plans.

## Hardware

- M5Stack **Core2** (nicht Fire, nicht CoreS3) – jede Revision (V1.0/V1.1)
  funktioniert, siehe `docs/projektplan.md` Abschnitt 2
- USB-C-Kabel zum Flashen/Laden
- microSD-Karte (wird für den Aufgaben-Content benötigt, siehe oben)

## SD-Karte

Die Firmware liest Aufgaben-Content von der SD-Karte, schreibt aber auch
Profil-/Fortschritts-/Spaced-Repetition-Daten dorthin (`/profile.json`,
`/progress.json`, `/progress/aufgaben_<fach>.json` – alle atomar via
`JsonStore`, siehe Abschnitt 6 des Plans). Karte vor dem ersten Start mit
FAT32 formatieren und den Inhalt von `sdcard/` (aus diesem Repo) ins
Wurzelverzeichnis kopieren, sodass `/tasks/mathe_1.json` etc. existieren.
Das Verzeichnis `/progress/` legt die Firmware beim ersten Start selbst an.

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

## Bedienung (Stand nach Phase 2)

- **Home-Screen:** zeigt Charakter + Namen, Uhrzeit, verfügbare Spielzeit.
  Untere Icon-Leiste: Stift-Icon → Fach-Auswahl, Play-Dreieck → Snake (nur
  aktiv ab Charakterstufe "Baby" und mit Spielzeitguthaben), Uhr-Icon →
  Uhr/Wecker.
- **Fach-Auswahl:** Icon-Grid zu Mathe, Rechtschreibung, Französisch (nur
  ab Klasse 3), Quiz und Gedächtnistraining.
- **Aufgaben-Modus:** Multiple-Choice-Frage antippen; richtige Antwort gibt
  Erfahrungspunkte + 2 Minuten Spielzeit (Abschnitt 7/9) und schiebt das
  Item in der Spaced-Repetition-Historie eine Box weiter; falsche Antwort
  wirft es auf Box 1 zurück (Abschnitt 8.3). Die Schwierigkeit passt sich
  an die rollierende Trefferquote an (Abschnitt 8.4). Zurück zu Home über
  das Haus-Icon oben rechts.
- **Gedächtnistraining:** für Klasse-1-Profile ein Karten-Memory (Paare
  finden), für Klasse-3-Profile ein Simon-Says-Sequenzspiel (Reihenfolge
  nachtippen, wird jede Runde länger).
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
  Subject.h                  Die vier Multiple-Choice-Faecher
  TaskEngine.*                Aufgaben-Engine (Pool laden, naechste Aufgabe waehlen)
  SpacedRepetitionStore.*      Leitner-5-Boxen je Fach (Abschnitt 8.3)
  DifficultyTracker.*          Schwierigkeitsanstieg je Fach (Abschnitt 8.4)
  RtcClock.*                RTC-Wrapper, Kalenderarithmetik, NTP-Sync-Stub
  AlarmService.*             Screen-unabhängige Wecker-Pruefung
  PinCode.*                 Gehashter Eltern-PIN (Abschnitt 6, Review)
  AppContext.*               Gemeinsamer Laufzeit-Zustand fuer alle Screens
  storage/                   Atomare JSON-Persistenz auf der SD-Karte
    JsonStore.*               .tmp/.bak-Rotation gegen Stromausfall-Korruption
    ProfileStore.*            /profile.json
    ProgressStore.*           /progress.json
src/screens/                 Konkrete Screens (je eine Klasse pro Screen)
  BootScreen.*               Init, laedt Profil/Fortschritt
  ProfileSetupScreen.*       Erststart: Kind-Profil waehlen (icon-first)
  HomeScreen.*                Charakter + Statusleiste + Navigation
  SubjectSelectScreen.*        Fach-Auswahl (icon-first)
  TaskScreen.*                Aufgaben-Modus (alle vier Faecher)
  GedaechtnisScreen.*           Karten-Memory (Klasse 1) / Sequenzspiel (Klasse 3)
  SnakeScreen.*                Snake, ueber M5Canvas-Offscreen-Sprite gezeichnet
  ClockScreen.*                Uhr/Wecker-Einstellung
docs/projektplan.md          Vollstaendiger Projektplan inkl. Review
```

## Hinweis zum Kompilieren in dieser Session

Der Code wurde in dieser Cloud-Umgebung geschrieben, aber **nicht** über
`pio run` gebaut/verifiziert – der Netzwerkzugriff auf die
PlatformIO-Paketregistrierung (zum Laden der ESP32-Toolchain und
Libraries) ist hier durch die Organisations-Egress-Policy blockiert. Bitte
vor dem Flashen lokal `pio run` ausführen, um sicherzustellen, dass alles
sauber kompiliert.
