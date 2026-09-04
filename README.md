# Tamagotchi-Lernspielgerät

Firmware für ein tamagotchiartiges Lerngerät auf Basis von zwei
**M5Stack Core2**-Geräten (Arduino/PlatformIO, `M5Unified`). Der vollständige
Projektplan inkl. Review-Anmerkungen steht in
[`docs/projektplan.md`](docs/projektplan.md) – dort auch die Begründung für
alle hier genannten Design-Entscheidungen.

**Aktueller Stand: Phase 0** (siehe Abschnitt 14 des Plans) – Grundgerüst
mit State-Machine, Storage-Layer, Erststart-Einrichtung und Home-Screen mit
Platzhalter-Charakter. Aufgaben-Modus, Spiele und Alltagsfunktionen folgen
in den Phasen 1–4.

## Hardware

- M5Stack **Core2** (nicht Fire, nicht CoreS3) – jede Revision (V1.0/V1.1)
  funktioniert, siehe `docs/projektplan.md` Abschnitt 2
- USB-C-Kabel zum Flashen/Laden
- Optional: microSD-Karte (wird ab Phase 1 für Aufgaben-Content und
  Fortschrittsdaten benötigt)

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

## Projektstruktur

```
platformio.ini          PlatformIO-Projektdefinition (Board, Libraries)
include/config.h         Zentrale Konfigurationswerte (Pfade, Limits, Pins)
src/main.cpp              Einstiegspunkt (setup()/loop())
src/core/                 Screen-unabhängige Module
  Screen.h / ScreenId.h / StateMachine.*   Zustandsautomat (Abschnitt 5)
  CharacterEngine.*        Tamagotchi-Charaktersystem (Abschnitt 9)
  PlaytimeAccount.*        Spielzeitkonto (Abschnitt 7)
  RtcClock.*               RTC-Wrapper, Kalenderarithmetik, NTP-Sync-Stub
  PinCode.*                Gehashter Eltern-PIN (Abschnitt 6, Review)
  AppContext.*             Gemeinsamer Laufzeit-Zustand für alle Screens
  storage/                 Atomare JSON-Persistenz auf der SD-Karte
    JsonStore.*             .tmp/.bak-Rotation gegen Stromausfall-Korruption
    ProfileStore.*           /profile.json
    ProgressStore.*          /progress.json
src/screens/               Konkrete Screens (je eine Klasse pro Screen)
  BootScreen.*              Init, lädt Profil/Fortschritt
  ProfileSetupScreen.*      Erststart: Klassenstufe wählen (icon-first)
  HomeScreen.*              Charakter + Statusleiste
docs/projektplan.md        Vollständiger Projektplan inkl. Review
```

## Hinweis zum Kompilieren in dieser Session

Der Code wurde in dieser Cloud-Umgebung geschrieben, aber **nicht** über
`pio run` gebaut/verifiziert – der Netzwerkzugriff auf die
PlatformIO-Paketregistrierung (zum Laden der ESP32-Toolchain und
Libraries) ist hier durch die Organisations-Egress-Policy blockiert. Bitte
vor dem Flashen lokal `pio run` ausführen, um sicherzustellen, dass alles
sauber kompiliert.
