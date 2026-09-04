# Tamagotchi-Lernspielgerät

Firmware für ein tamagotchiartiges Lerngerät auf Basis von zwei
**M5Stack Core2**-Geräten (Arduino/PlatformIO, `M5Unified`). Der vollständige
Projektplan inkl. Review-Anmerkungen steht in
[`docs/projektplan.md`](docs/projektplan.md) – dort auch die Begründung für
alle hier genannten Design-Entscheidungen.

**Aktueller Stand: alle 6 Phasen (0–5) sind umgesetzt** (siehe Abschnitt 14
des Plans) – State-Machine, Storage-Layer, Erststart-Einrichtung mit
Kind-Profil-Auswahl, Home-Screen mit einem Charakter, der als **Menschenkind**
über 6 Stufen zum Kampfkünstler-Kind heranwächst und trainiert (32×32-PNG-
Sprites von der SD-Karte, Gürtelfarben weiss→gelb→grün→braun→schwarz, mit
frei wählbarer Hautfarbe/Haarfarbe/Kleidungsfarbe – siehe "Sprite-Grafik"
unten), Aufgaben-Modus für alle vier Multiple-Choice-Fächer inkl. Spaced
Repetition und Schwierigkeitsanstieg, Gedächtnistraining, alle 9 Spiele,
Alltagsfunktionen (Uhr/Wecker, Timer, Checkliste, Steckbrief, Aussehen),
Nachtmodus, Eltern-PIN-geschützte Einstellungen (inkl. Uhrzeit einstellen),
ein durchgängiges, knalliges 80er-Jahre-/Arcade-Neon-Farbschema und ein
Web-Interface (Fortschrittsansicht, Aufgaben-Verwaltung, Tageslimit,
ArduinoOTA). Damit ist die komplette im Plan vorgesehene Funktionalität im
Code vorhanden – **und inzwischen auf echter Hardware kompiliert, geflasht
und getestet**, siehe "Bekannte Setup-Probleme" und "Bekannte
Laufzeit-Probleme (behoben)" unten für die dabei gefundenen und behobenen
Fehler.

## Vor dem ersten Flashen: zwei Dinge anpassen

1. **Eigene Kinder eintragen:** [`include/KidProfiles.h`](include/KidProfiles.h)
   öffnen und `kKidProfiles` mit den echten Namen, dem Alter **und dem
   Geburtstag** (Monat/Tag, kein Jahr nötig) der Kinder befüllen (aktuell
   Platzhalter "Kind 1"/"Kind 2" mit Platzhalter-Geburtstagen). Der
   Tamagotchi-Charakter trägt danach diesen Namen; die Klassenstufe (und
   damit die Aufgaben-Schwierigkeit) wird automatisch aus dem Alter
   hergeleitet (`klasseForAge()`, Schwelle in derselben Datei anpassbar);
   der Geburtstag speist den Countdown im Alltagsfunktionen-Menü (siehe
   unten).
2. **Aufgaben-Content auf die SD-Karte kopieren:** den Ordner
   [`sdcard/tasks/`](sdcard/tasks) auf die microSD-Karte kopieren (siehe
   "SD-Karte" unten). Ohne diese Dateien zeigt der Aufgaben-Modus "Keine
   Aufgaben gefunden" an. **Hinweis:** Der mitgelieferte Content ist
   selbst verfasster Übungscontent (25–30 Fragen pro Fach/Klasse) zum
   Ausprobieren der Engine, kein geprüfter Lehrplan-21/Passepartout-Stoff – siehe
   Abschnitt 16 des Plans.

## Hardware

- M5Stack **Core2** (nicht Fire, nicht CoreS3) – jede Revision (V1.0/V1.1)
  funktioniert, siehe `docs/projektplan.md` Abschnitt 2
- USB-C-Kabel zum Flashen/Laden
- microSD-Karte (wird für den Aufgaben-Content benötigt, siehe oben)

## SD-Karte

Die Firmware liest Aufgaben-Content und Charakter-Sprites von der SD-Karte,
schreibt aber auch Profil-/Fortschritts-/Spaced-Repetition-/Highscore-Daten
dorthin (`/profile.json`, `/progress.json`, `/progress/aufgaben_<fach>.json`,
`/highscores.json` – alle atomar via `JsonStore`, siehe Abschnitt 6 des
Plans). Karte vor dem ersten Start mit FAT32 formatieren und den Inhalt
von `sdcard/` (aus diesem Repo) ins Wurzelverzeichnis kopieren, sodass
`/tasks/mathe_1.json` und `/sprites/character/ei_idle1.png` etc. existieren.
Das Verzeichnis `/progress/` legt die Firmware beim ersten Start selbst an.
**Ohne Sprite-Dateien** (Karte fehlt, `/sprites/` nicht kopiert) zeigt der
Home-Screen automatisch die alte Kreis-Platzhalter-Grafik statt eines
Sprites – kein Absturz, nur weniger hübsch.

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

### Bekannte Setup-Probleme

**`ModuleNotFoundError: No module named 'intelhex'`:** Falls der Build beim
Bootloader-/Partitions-Schritt mit einer fehlenden `intelhex`-Python-
Abhängigkeit von `esptool.py` abbricht, in der von PlatformIO verwendeten
Python-Umgebung nachinstallieren:
```
pip install intelhex
```
Danach `pio run` erneut ausführen – Toolchain und Libraries sind dann schon
lokal gecacht, der zweite Build ist entsprechend schnell.

**`'std::make_unique' is only available from C++14 onwards` (behoben):**
Trat beim allerersten echten Kompilierversuch auf – `platformio.ini` legte
keinen C++-Standard fest, wodurch der Toolchain-Default (C++11) griff. Ist
bereits in `platformio.ini` gefixt (`build_unflags`/`build_flags` für
C++17). Falls diese Meldung trotzdem erscheint: sicherstellen, dass der
lokale Checkout auf dem aktuellen Stand von
`claude/tamagotchi-device-review-v8dxml` ist (`git pull`), und
`.pio/build/` löschen, um einen sauberen Neu-Build zu erzwingen.

**SD-Karte wird nicht erkannt, obwohl korrekt FAT32-formatiert und befüllt:**
Zwei getrennte mögliche Ursachen:
1. **SPI-Bus-Fehlkonfiguration (behoben):** Core2s SD-Karte hängt am selben
   SPI-Bus, den `M5.begin()` bereits mit den Core2-spezifischen Pins
   konfiguriert. `SD.begin(cs)` ohne explizite Angabe des `SPI`-Objekts
   nutzte stattdessen dessen (nicht zu Core2 passende) Default-Belegung und
   meldete die Karte fälschlicherweise als fehlend. Behoben in
   `BootScreen.cpp` (`SD.begin(config::kSdChipSelectPin, SPI)`).
2. **Karten-/Formatierungskompatibilität (weiterhin offen, siehe unten):**
   auch nach obigem Fix meldete eine 128GB-SDXC-Karte (FAT32-formatiert)
   weiterhin "nicht erkannt". `BootScreen` zeigt seit dieser Runde bei
   einem SD-Fehlschlag eine deutliche, garantiert helle Fehlermeldung mit
   Troubleshooting-Hinweisen (statt eines stillen Hängenbleibens) und
   versucht automatisch alle paar Sekunden erneut. Zusätzlich protokolliert
   `BootScreen` jetzt über `Serial` (115200 Baud, `pio device monitor`) den
   `SD.begin()`-Erfolg sowie `SD.cardType()`/`SD.cardSize()` bei Erfolg –
   das liefert bei einem erneuten Fehlschlag konkrete Diagnosedaten statt
   eines reinen "geht nicht". **Empfehlung, bis geklärt:** zum Test eine
   SDHC-Karte mit ≤ 32GB probieren (die Arduino-ESP32-`SD`-Bibliothek ist
   mit grösseren SDXC-Karten über SPI bekanntermassen manchmal unzuverlässig,
   unabhängig vom SPI-Bus-Fix), und/oder die 128GB-Karte mit dem offiziellen
   [SD-Association-Formatierungstool](https://www.sdcard.org/downloads/formatter/)
   (nicht dem Windows-Standarddialog, der über 32GB gar kein FAT32 anbietet)
   neu formatieren.

**`/dev/ttyACM0`: Permission denied beim Flashen (Linux):** Der aktuelle
Nutzer ist noch nicht (oder erst nach Neuanmeldung) Mitglied der Gruppe
`dialout`, die den seriellen Port besitzt. Prüfen mit
`getent group dialout` und `groups`; fehlt die Mitgliedschaft:
`sudo usermod -aG dialout $USER`, dann **komplett aus- und wieder
einloggen** (ein neues Terminal allein reicht nicht) – für den aktuellen
Terminal reicht notfalls `newgrp dialout` als Sofort-Workaround. Existiert
zusätzlich `/etc/udev/rules.d/99-platformio-udev.rules` noch nicht, die
PlatformIO-udev-Regeln nachinstallieren (`pio run -t udev` bzw. gemäss
PlatformIO-Doku) und den Rechner neu starten.

## Bedienung (Stand nach Phase 5, mit Hardware-Fixes)

- **Startlogo:** beim Einschalten erscheint für ca. 1,8 Sekunden ein
  80er-/Synthwave-Neon-Logo ("Henri & Theo" – anpassbar in
  `src/screens/BootScreen.cpp`, Konstante `kLogoText`) vor demselben
  Sonne+Gitter-Hintergrund wie die übrige Retro-Optik, bevor SD-Karte/Profil
  geladen werden. Schlägt die SD-Erkennung fehl, erscheint statt eines
  stillen Hängenbleibens eine deutliche, garantiert helle Fehlermeldung mit
  Troubleshooting-Hinweisen (siehe "Bekannte Setup-Probleme" oben) – neue
  Versuche laufen automatisch alle paar Sekunden, ein manueller Neustart ist
  nicht nötig, sobald die Karte wieder korrekt sitzt.
- **Home-Screen:** zeigt Charakter + Namen, Uhrzeit, verfügbare Spielzeit.
  Untere Icon-Leiste (4 Zonen): Stift-Icon → Fach-Auswahl, Play-Dreieck →
  Spiele-Menü (nur mit Spielzeitguthaben aktiv), Uhr-Icon →
  Alltagsfunktionen-Menü, Zahnrad-Icon → Eltern-PIN-Eingabe →
  Einstellungen.
  - **Sprite-Grafik:** der Charakter ist ein echtes 32×32-Pixel-Art-Sprite
    von der SD-Karte (`sdcard/sprites/character/`) – ein Menschenkind, kein
    Tier/keine Fantasiekreatur. Pro Entwicklungsstufe (Ei/Baby/Kind/Junior/
    Experte/Meister) trägt es einen Karate-Gi mit zum Trainingsfortschritt
    passender Gürtelfarbe (weiss→gelb→grün→braun→schwarz) und wird
    zunehmend kampfbereiter (Stirnband ab Junior, Schlagpose ab Experte,
    Kampfhaltung mit beiden Fäusten ab Meister) – angelehnt an 90er-
    Arcade-Kampfspiele wie Street Fighter II/Mortal Kombat. Dazu blinzelnde
    Idle-Animation (wechselt sekündlich) und eigene traurige Variante bei
    Inaktivität (Abschnitt 9). Die Dateien wurden prozedural erzeugt (siehe
    [`tools/generate_sprites.py`](tools/generate_sprites.py)) – Skript
    erneut ausführen (`python3 tools/generate_sprites.py`, benötigt
    `pip install pillow`), um Formen anzupassen, oder die PNGs unter
    `sdcard/sprites/character/<stufe>_<idle1|idle2|sad>.png` durch eigene
    32×32-Artworks mit transparentem Hintergrund ersetzen (Markerfarben aus
    `src/core/CharacterTraits.h` beachten) – keine Codeänderung nötig,
    solange die Dateinamen gleich bleiben.
  - **Aussehen frei wählbar:** Hautfarbe (4 Voreinstellungen), Haarfarbe (6)
    und Kleidungsfarbe (6, nur der Gi – der Gürtel bleibt fest) lassen sich
    jederzeit über den Screen "Aussehen" (Alltagsfunktionen-Menü, siehe
    unten) mit </>-Pfeilen durchblättern; die Vorschau aktualisiert sich
    live, jede Änderung wird sofort gespeichert. Technisch ein
    Palette-Swap zur Laufzeit (`src/core/CharacterRenderer.*`): die
    Sprite-Dateien enthalten an diesen Stellen reservierte Markerfarben
    statt fester Farben, die beim Zeichnen durch die gewählte Trait-Farbe
    ersetzt werden – deshalb reichen 18 Bilddateien statt hunderter
    Farbkombinationen.
  - **Farbschema:** die komplette Nicht-Spiel-UI (inkl. der Kopf-/
    Statusleisten aller 9 Spiele) nutzt ein durchgängiges, auf maximale
    Knalligkeit finalisiertes 80er-Jahre-/Arcade-Neon-Farbschema (fast
    schwarzes Indigo/Lila + Neonpink/-cyan/-orange/-gold, siehe
    [`src/core/Theme.h`](src/core/Theme.h)) statt einzelner `TFT_*`-Farben –
    zentral an einer Stelle anpassbar. Ausnahmen bewusst unverändert:
    Text/Icon-Linien bleiben weiss (Lesbarkeit), und ein paar
    "wiedererkennungskritische" Farben (Französisch-Flagge, Basketball-/
    Fussball-Icon, die vier Merkspiel-Farben) folgen nicht dem Theme.
  - **Retro-Hintergründe (SNES-/Street-Fighter-Stage-Look):** Home-Screen,
    alle drei Icon-Menüs (Spiele/Alltag/Fach-Auswahl) und der Kampf-Modus
    zeichnen zusätzlich einen prozeduralen "Synthwave"-Hintergrund –
    untergehende Sonne (Gold→Pink-Verlauf) über einem perspektivischen
    Bodengitter, das zum unteren Bildschirmrand aufweitet
    ([`src/core/RetroBackdrop.*`](src/core/RetroBackdrop.h)). Reine
    Vektorgrafik aus Theme-Farben, keine zusätzlichen Bild-Dateien nötig.
    Liegt bewusst hinter Icons/Charakter/Panels, damit die Lesbarkeit für
    die junge Zielgruppe erhalten bleibt.
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
- **Spiele-Menü:** 3×3-Icon-Grid aller 9 Spiele; gesperrte Spiele (noch
  nicht erreichte Charakterstufe, Abschnitt 9) sind ausgegraut mit
  Schloss-Symbol und lassen sich nicht öffnen.
  - **Snake** (ab Baby): Antippen relativ zur Bildschirmmitte steuert die
    Richtung.
  - **Tetris** (ab Kind): linkes/mittleres/rechtes Bildschirmdrittel =
    links bewegen/drehen/rechts bewegen, nach unten swipen = Hard-Drop.
  - **Puzzle** (ab Kind): Kachel antippen und auf eine andere Position
    ziehen (Drag&Drop), Ziel: Zahlen 1–9 aufsteigend anordnen.
  - **Space Invaders** (ab Junior): linkes/rechtes Drittel halten = Schiff
    bewegen, mittleres Drittel antippen = Feuer-Button.
  - **Moorhuhn-Jagd** (ab Junior): Gerät neigen bewegt das Fadenkreuz,
    Antippen löst den Schuss aus.
  - **Pinball** (ab Experte): linke/rechte Bildschirmhälfte halten =
    Flipper, Neigen gibt der Kugel einen kleinen seitlichen Schubs.
  - **Basketball** (ab Experte): Ball antippen, in Wurfrichtung ziehen und
    loslassen (Swipe = Richtung + Stärke).
  - **Fussball** (ab Meister): wie Basketball, aber nach oben aufs Tor
    schiessen; ein Torwart hält, wenn er rechtzeitig davor steht.
  - **Kampf-Modus** (ab Meister): linkes/rechtes Bildschirmdrittel unten =
    bewegen, "S"/"T"-Buttons = Schlag/Tritt, Swipe = Spezialattacke.
  - Kein Spiel generiert EP oder Spielzeit – sie verbrauchen nur
    Spielzeitguthaben und kehren bei Null automatisch zu Home zurück.
    Highscores (wo sinnvoll) werden lokal in `/highscores.json`
    gespeichert.
  - **Zurück-Button:** alle 9 Spiele haben oben rechts ein Haus-Icon zum
    sofortigen Zurückkehren zu Home (**behoben:** fehlte zuvor bei Snake
    komplett; bei den anderen 8 Spielen gab es das Icon zwar schon, aber in
    einer schwer erkennbaren reinen Weiss-Umriss-Optik – jetzt einheitlich
    als deutlich sichtbares goldenes Badge).
- **Alltagsfunktionen-Menü:** Icon-Grid (3×2, vollständig gefüllt) zu
  Uhr/Wecker, Timer, Checkliste, Steckbrief, Aussehen und Geburtstag.
  - **Uhr/Wecker:** grosse Digitaluhr, eine einstellbare Alarmzeit (Glocke
    antippen = an/aus, +/- für Stunde/Minute). Der Wecker löst auch aus,
    wenn gerade ein anderer Screen aktiv ist (`AlarmService`). Die
    Uhrzeit/das Datum selbst stellt man in den Einstellungen unter
    "Uhrzeit einstellen" (siehe unten).
  - **Timer:** drei Presets (2/5/10 Min) antippen zum Starten, piept +
    vibriert bei Ablauf.
  - **Checkliste:** drei feste Morgen-Routine-Punkte abhaken; sind alle
    abgehakt, gibt es einmal pro Tag eine kleine EP-Belohnung.
  - **Steckbrief:** Übersicht über Name, Stufe, EP, Klasse, freigeschaltete
    Spiele und Tage seit der letzten Pflege.
  - **Aussehen:** live Vorschau des Charakters, darunter drei Zeilen
    (Haut/Haare/Kleidung) mit </>-Pfeilen zum Durchblättern der
    Farbvoreinstellungen. Nicht Eltern-PIN-geschützt – das Kind kann
    seinen Charakter jederzeit selbst anpassen.
  - **Geburtstag:** zeigt die Tage bis zum nächsten Geburtstag (aus
    `include/KidProfiles.h`, dort wo auch der Name steht) mit einer kleinen
    Torten-Grafik; am Geburtstag selbst erscheint statt der Tageszahl eine
    Glückwunsch-Meldung. Ist am Profil kein Geburtstag hinterlegt, erscheint
    stattdessen ein Hinweis.
- **Nachtmodus:** dimmt den Bildschirm automatisch zwischen 20 und 7 Uhr
  (Default-Zeitfenster, RTC-basiert, `NightModeService`) – **standardmässig
  jetzt AUS** (Default vor dem Hardware-Test war fälschlich AN, siehe
  "Bekannte Laufzeit-Probleme" unten), in den Einstellungen an/aus
  schaltbar.
- **Einstellungen (Eltern-PIN-geschützt):** Zahnrad-Icon auf dem
  Home-Screen antippen, PIN eingeben (Standard `0000`, siehe
  `config::kDefaultParentalCode` – vor "produktivem" Einsatz über "PIN
  ändern" in den Einstellungen anpassen; ein geänderter PIN übersteht jetzt
  zuverlässig einen Neustart, siehe "Bekannte Laufzeit-Probleme" unten).
  Dort: Tageslimit anpassen (±5 Min), Bonus-Spielzeit vergeben (+10 Min),
  Nachtmodus an/aus, **Uhrzeit einstellen** (Jahr/Monat/Tag/Stunde/Minute
  per Stepper, `DateTimeSetScreen`), PIN ändern, Web-Sync starten.
- **Power-Taste (Gerät):** kurz drücken schaltet nur das Display an/aus
  (Akku sparen, ohne die App zu verlassen – Nachtmodus/Spiellogik
  pausieren währenddessen), langes Drücken (≥ 2 Sekunden) sichert den
  Fortschritt und schaltet das Gerät **komplett aus** (`M5.Power.powerOff()`,
  nicht nur ein Neustart). Erneutes Drücken der Power-Taste schaltet es
  wieder ein – aller Fortschritt (Charakterstufe, EP, Spielzeitkonto, PIN
  usw.) bleibt erhalten, da er laufend auf der SD-Karte gespeichert wird.
  Es gibt bewusst **keine** Reset-/"Fortschritt löschen"-Funktion in der
  Firmware – der einzige Weg dafür ist, die Fortschrittsdateien manuell von
  der SD-Karte zu entfernen (siehe "SD-Karte" oben für die Dateinamen).
- **Web-Sync** (Abschnitt 12): startet einen geräteeigenen WLAN-
  Access-Point (SSID/Passwort/IP werden auf dem Gerät angezeigt) mit
  einem kleinen Web-Interface unter `http://192.168.4.1` – Fortschritt
  einsehen, Aufgaben pro Fach hinzufügen/löschen, Tageslimit anpassen,
  Firmware-Update per `pio run -t upload --upload-port <Geräte-IP>`
  (ArduinoOTA). Läuft nur, solange der Screen offen ist (Akku), siehe
  Abschnitt 12/16 für Details und offene Punkte (keine eigene
  Authentifizierung ausser dem AP-Passwort, kein Bearbeiten bestehender
  Aufgaben nur Hinzufügen/Löschen).

## Projektstruktur

```
platformio.ini             PlatformIO-Projektdefinition (Board, Libraries)
include/config.h            Zentrale Konfigurationswerte (Pfade, Limits, Pins)
include/KidProfiles.h       Eigene Kinder eintragen (Name + Alter) - siehe oben
sdcard/tasks/                Aufgaben-Content zum Kopieren auf die SD-Karte
sdcard/sprites/character/    Charakter-Sprites zum Kopieren auf die SD-Karte
tools/generate_sprites.py    Erzeugt die Sprite-PNGs (pip install pillow)
src/main.cpp                 Einstiegspunkt (setup()/loop())
src/core/                    Screen-unabhängige Module
  Screen.h / ScreenId.h / StateMachine.*   Zustandsautomat (Abschnitt 5)
  CharacterEngine.*         Tamagotchi-Charaktersystem (Abschnitt 9)
  CharacterRenderer.*        Sprite-Rendering + Trait-Palette-Swap (Abschnitt 4)
  CharacterTraits.h          Trait-Farbvoreinstellungen + Markerfarben
  Theme.h                    Zentrales 80er-/Arcade-Neon-Farbschema (Abschnitt 4)
  RetroBackdrop.*            Prozeduraler Synthwave-Hintergrund (Sonne+Gitter, SNES-Look)
  PlaytimeAccount.*         Spielzeitkonto (Abschnitt 7)
  PlaytimeTicker.*            Gemeinsame Spielzeit-Verbrauchslogik fuer alle Spiele
  HighscoreStore.*             Lokale Highscores je Spiel (/highscores.json)
  Subject.h                  Die vier Multiple-Choice-Faecher
  TaskEngine.*                Aufgaben-Engine (Pool laden, naechste Aufgabe waehlen)
  SpacedRepetitionStore.*      Leitner-5-Boxen je Fach (Abschnitt 8.3)
  DifficultyTracker.*          Schwierigkeitsanstieg je Fach (Abschnitt 8.4)
  RtcClock.*                RTC-Wrapper, Kalenderarithmetik, NTP-Sync-Stub
  AlarmService.*             Screen-unabhängige Wecker-Pruefung
  NightModeService.*          Screen-unabhängige Nachtmodus-Dimmung
  WebServerService.*           Web-Interface + ArduinoOTA (nur waehrend WebSyncScreen aktiv)
  PinCode.*                 Gehashter Eltern-PIN (Abschnitt 6, Review)
  AppContext.*               Gemeinsamer Laufzeit-Zustand fuer alle Screens
  storage/                   Atomare JSON-Persistenz auf der SD-Karte
    JsonStore.*               .tmp/.bak-Rotation gegen Stromausfall-Korruption
    ProfileStore.*            /profile.json
    ProgressStore.*           /progress.json
src/screens/                 Konkrete Screens (je eine Klasse pro Screen)
  BootScreen.*               Startlogo (Synthwave-Neon-Schriftzug), dann Init/Laedt Profil/Fortschritt
  ProfileSetupScreen.*       Erststart: Kind-Profil waehlen (icon-first)
  HomeScreen.*                Charakter + Statusleiste + Navigation
  SubjectSelectScreen.*        Fach-Auswahl (icon-first)
  TaskScreen.*                Aufgaben-Modus (alle vier Faecher)
  GedaechtnisScreen.*           Karten-Memory (Klasse 1) / Sequenzspiel (Klasse 3)
  GamesMenuScreen.*             Spiele-Menue mit Freischalt-Anzeige
  SnakeScreen.* / TetrisScreen.* / SpaceInvadersScreen.* / PinballScreen.* /
  BasketballScreen.* / FussballScreen.* / PuzzleScreen.* /
  MoorhuhnJagdScreen.* / KampfModusScreen.*   Die 9 Spiele (Abschnitt 10)
  AlltagMenuScreen.*            Alltagsfunktionen-Menue (icon-first)
  ClockScreen.*                 Uhr/Wecker-Einstellung
  TimerScreen.*                 Countdown-Timer (2/5/10 Min Presets)
  ChecklistScreen.*             Morgen-Routine-Checkliste
  SteckbriefScreen.*            Charakter-Uebersicht (nur lesend)
  CharacterCustomizeScreen.*     "Aussehen" - Haut/Haare/Kleidung waehlen (nicht PIN-geschuetzt)
  BirthdayScreen.*               Geburtstags-Countdown (Monat/Tag aus KidProfiles.h)
  PinEntryScreen.*               Eltern-PIN-Eingabe (pruefen/neu setzen)
  SettingsScreen.*               Eltern-Einstellungen (PIN-geschuetzt)
  DateTimeSetScreen.*            Datum/Uhrzeit der RTC einstellen (aus Settings)
  WebSyncScreen.*                 Zeigt SSID/Passwort/IP, startet/stoppt WebServerService
docs/projektplan.md          Vollstaendiger Projektplan inkl. Review
```

## Bekannte Laufzeit-Probleme (auf echter Hardware gefunden, behoben)

Der Code wurde inzwischen erfolgreich auf echter Core2-Hardware kompiliert,
geflasht und gespielt. Dabei gefundene und in dieser Runde behobene
Fehler:

- **Home-Screen flackerte sekündlich:** `HomeScreen` zeichnete mehrere
  aufeinanderfolgende Aufrufe direkt auf `M5.Display` statt (wie alle
  Spiele) über ein Offscreen-`M5Canvas`. Behoben, siehe Klassenkommentar in
  `HomeScreen.h`.
- **SD-Karte "nicht erkannt" trotz korrektem FAT32/Ordnern:** SPI-Bus-
  Fehlkonfiguration, siehe "Bekannte Setup-Probleme" oben.
- **Display viel zu dunkel / Nachtmodus fälschlich aktiv:** `nightModeEnabled`
  war default `true` statt `false`, zusätzlich konnte eine noch unplausible
  RTC-Werkszeit (Jahr < 2024) den Nachtmodus vor der ersten Zeiteinstellung
  auslösen. Beides behoben (`ProfileStore.h`, `NightModeService::check()`
  mit Plausibilitäts-Guard).
- **Eltern-PIN nach Neustart wieder auf Default zurückgesetzt:** Der neue
  PIN wurde beim Ändern zwar sofort im Speicher übernommen, aber
  `profilestore::save()`s Rückgabewert nie geprüft – schlug das Schreiben
  auf die (zu diesem Zeitpunkt wegen des SPI-Bus-Fehlers nicht
  funktionierende) SD-Karte fehl, wirkte die Änderung nur bis zum nächsten
  Neustart. `PinEntryScreen` rollt den PIN jetzt bei einem Speicherfehler
  zurück und zeigt einen Fehler, statt stillschweigend zu "vergessen".
- **Kein Zurück-/Home-Button in den Spielen:** siehe "Zurück-Button" oben.
- **Uhrzeit liess sich nicht einstellen:** neuer `DateTimeSetScreen`, siehe
  "Einstellungen" oben.
- **Power-Taste ohne Funktion:** kurzer/langer Druck jetzt belegt, siehe
  "Power-Taste" oben. Langer Druck schaltet das Gerät **aus** (nicht neu
  starten) – wieder einschalten per erneutem Tastendruck, kein
  Fortschrittsverlust, keine Reset-Funktion in der Firmware.

Die Spiele-Physik (Pinball/Basketball/Fussball/Moorhuhn-Jagd)-Konstanten
sind weiterhin Startwerte, die bei Bedarf nach mehr Spielzeit noch
nachjustiert werden können (Abschnitt 16 des Plans).

**Weiterhin am wenigsten auf echter Hardware verifiziert: das
Web-Interface (Abschnitt 12).** WLAN/Access-Point, der synchrone
`WebServer` und `ArduinoOTA` wurden in dieser Session nicht live
durchgespielt – vor Verlass darauf einmal real testen: Web-Sync im Gerät
öffnen, mit einem Handy/Laptop am angezeigten AP anmelden,
`http://192.168.4.1` aufrufen, eine Test-Aufgabe hinzufügen/löschen,
Tageslimit ändern, und `pio run -t upload --upload-port <angezeigte-IP>`
für ein OTA-Update probieren.
