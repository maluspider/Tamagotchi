# Projektplan: Tamagotchi-Lernspielgerät auf 2× M5Stack Core2

> Diese Fassung enthält die Ergebnisse eines Plan-Reviews (siehe Abschnitt 15).
> Stellen, an denen sich der ursprüngliche Plan inhaltlich geändert hat, sind
> mit **»Review:«** markiert. Rein technische Klarstellungen ohne
> Verhaltensänderung sind mit **»Klarstellung:«** markiert.

## 1. Zielsetzung

Zwei M5Stack Core2 (je eines für die Kinder, 1. und 3. Klasse Baselland) werden zu einem tamagotchiartigen Lerngerät: Ein Charakter entwickelt sich weiter, wenn altersgerechte Schulaufgaben (Mathe, Rechtschreibung, Französisch-Vokabeln, Quiz, Gedächtnistraining) richtig gelöst werden. Richtige Antworten geben Spielzeit für 9 vollständige Mini-Games im Stil japanischer 90er-Jahre-Videospiele. Zusätzlich: Uhr, Wecker und weitere Alltagsfunktionen.

## 2. Hardware

- 2× M5Stack **Core2** (nicht Fire, nicht CoreS3 – Begründung siehe Projektverlauf)
- Je ein USB-C-Kabel/Netzteil zum Laden
- Optional: microSD-Karte pro Gerät (Aufgaben-Content, Sprites, Fortschrittsdaten-Backup)
- Optional (Phase 2): stabile Ladestation/Dock fürs Kinderzimmer

**Kernspezifikationen Core2** (für die Codeplanung relevant):
- ESP32-D0WDQ6-V3, 240 MHz Dual-Core
- 16 MB Flash, 8 MB PSRAM
- 320×240 IPS-Touchscreen (kapazitiv, FT6336U)
- **6-Achsen-IMU** (MPU6886 bzw. BMI270): 3-Achsen-Beschleunigungsmesser + 3-Achsen-Gyroskop, Vibrationsmotor, RTC (BM8563)
- Lautsprecher (I2S, NS4168-Verstärker), Mikrofon, microSD-Slot
- Akku ca. 390–500 mAh (je nach Revision)

**Klarstellung (Core2-Revision):** Welche Core2-Hardware-Revision (V1.0/V1.1) verbaut ist, spielt für den Code keine Rolle – `M5Unified` erkennt Power-Management- und IMU-Chip automatisch und abstrahiert sie über dieselbe API. Einzige relevante Unterscheidung ist "Core2" vs. "Core2 for AWS IoT EduKit" (anderer IMU-Chip, daher "MPU6886 bzw. BMI270" oben) – auch diese Variante läuft mit `M5Unified` einwandfrei, ist für den Kauf aber nicht nötig.

**Bewegungssensorik – Antwort auf die Frage "sind Bewegungssensoren vorhanden?":**
Ja. Der eingebaute 6-Achsen-IMU liefert sowohl Neigung (Tilt, über den Beschleunigungsmesser) als auch Drehrate (Gyroskop). Das reicht für:
- **Zielsteuerung per Neigen** (z. B. Moorhuhn-Jagd, Abschnitt 10) – Gerät kippen bewegt ein Fadenkreuz
- **Ball-/Flipper-Beeinflussung** (Pinball) durch leichtes Schütteln/Neigen
- Grobe Bewegungserkennung, aber **keine präzise Gestenerkennung im Raum** wie bei einer Spielekonsole mit Kamera – für die geplanten Spiele reicht das völlig aus.

## 3. Software-Stack

- **Framework:** Arduino über PlatformIO (bessere Versionierung/Struktur als Arduino IDE)
  **Update (erster echter Kompilierfehler, nach Phase 5):** Der erste tatsächliche Kompilierversuch auf echter Hardware/Toolchain (siehe unten, "nichts kompiliert in dieser Session" galt bis dahin) schlug fehl: `std::make_unique`/Lambda-zu-`std::function`-Konvertierung sind erst ab C++14 verfügbar, doch `platformio.ini` legte bis dahin keinen C++-Standard fest, wodurch der Toolchain-Default (C++11) griff. Behoben durch `build_unflags = -std=gnu++11` + `build_flags = -std=gnu++17` in `platformio.ini`. Zeigt, dass trotz sorgfältigem Selbstreview (Klammerbalance, Includes, Signaturen) echte Compiler-Fehler bis zum ersten tatsächlichen Build unentdeckt bleiben können – weitere Fehler beim nächsten Kompilierversuch sind nicht ausgeschlossen.
- **Hardware-Abstraktion:** `M5Unified` + `M5GFX` (offizielle Bibliothek, deckt Touch, IMU, RTC, Power, Lautsprecher einheitlich ab)
- **Grafik:** Sprite-basiertes Rendering direkt über `M5GFX`-Sprites (Offscreen-Buffer im PSRAM) für flüssige Pixel-Art-Grafik – kein LVGL nötig, das wäre für Spiele eher hinderlich
- **Daten:** `ArduinoJson` für Speicherformate, `Preferences` (NVS) für kleine/häufige Werte (z. B. aktuelles Spielzeitkonto), microSD für grössere Datenmengen (Aufgabenpools, Fortschrittshistorie, Sprite-Assets)
- **Web-Interface (Phase 5):** `ESPAsyncWebServer` + `AsyncTCP`
  **Review:** Das ursprüngliche `me-no-dev/ESPAsyncWebServer` ist praktisch unmaintained. Für Phase 5 den aktiv gepflegten Fork **`ESP32Async/ESPAsyncWebServer`** (inkl. `ESP32Async/AsyncTCP`) verwenden, sonst drohen Kompatibilitätsprobleme mit aktuellen ESP32-Arduino-Cores.

  **Klarstellung (Umsetzung, Phase 5 – Abweichung von der Review-Empfehlung):** Tatsächlich umgesetzt wurde stattdessen der **synchrone** `WebServer` aus dem arduino-esp32-Kern (keine zusätzliche Bibliotheksabhängigkeit, kein `ESP32Async`-Fork nötig). Grund: Async-Handler von ESPAsyncWebServer laufen in einem eigenen FreeRTOS-Task (AsyncTCP) parallel zum Hauptloop, der ebenfalls laufend auf die SD-Karte zugreift (JsonStore, TaskEngine, HighscoreStore, …) – ohne sorgfältig geprüften Mutex wäre das ein Data Race auf dem SPI-Bus. Ein solcher Mutex lässt sich ohne physisches Testgerät nicht zuverlässig verifizieren. Der synchrone `WebServer` verarbeitet Anfragen ausschliesslich innerhalb von `handleClient()` im Hauptloop – zu jedem Zeitpunkt greift dadurch nur ein einziger Kontrollfluss auf die SD-Karte zu, das Race-Problem entfällt strukturell statt nur "wahrscheinlich in Ordnung" zu sein. Für ein Admin-Panel mit einem Elternteil zur Zeit ist der Verzicht auf echte Nebenläufigkeit kein spürbarer Nachteil. Siehe `src/core/WebServerService.h` für die ausführliche Begründung im Code.
- **Versionsverwaltung:** Ein gemeinsames Repo, Geräte-Profil (1. Klasse / 3. Klasse) als Kompilier- oder Laufzeit-Konfiguration, damit dieselbe Codebasis beide Geräte bedient
  **Review (entschieden):** Laufzeit-Konfiguration. Eine gemeinsame Firmware für beide Geräte; das Kind wählt beim allerersten Start per Touch sein eigenes, im Code hinterlegtes Profil (siehe `ProfileSetupScreen`, Abschnitt 5) und das Ergebnis wird danach in `profile.json` persistiert. Vorteil: nur eine Firmware-Version pflegen/flashen, kein Risiko, versehentlich das falsche PlatformIO-Environment aufs falsche Gerät zu flashen.

  **Ergänzung (Name/Alter statt nur Klassenstufe):** Statt direkt "1. Klasse"/"3. Klasse" zu wählen, werden in `include/KidProfiles.h` die eigenen Kinder mit **Name + Alter** eingetragen (compile-time, vom Elternteil im Code gepflegt). Beim Erststart tippt das Kind auf sein eigenes Profil (Name + kleiner Platzhalter-Avatar); der Tamagotchi-Charakter trägt danach diesen Namen (angezeigt auf dem Home-Screen), und die Klassenstufe – die die Aufgaben-Schwierigkeit steuert (Abschnitt 8.1) – wird über `klasseForAge()` aus dem hinterlegten Alter hergeleitet (Schwelle: ab 8 Jahren → 3.-Klasse-Content, sonst 1.-Klasse-Content; bei abweichendem Schulweg im Code anpassbar). Der Charakter selbst wächst wie bisher ausschliesslich über Erfahrungspunkte aus gelösten Aufgaben (Abschnitt 9) – Alter/Name bestimmen nur Namen und Startschwierigkeit, nicht das Fortschrittstempo.
- **OTA-Updates. Review (neu):** Der ursprüngliche Plan sah keinen Update-Weg nach der Erstinstallation vor. Sobald die Geräte in den Kinderzimmern "deployed" sind, ist erneutes Flashen per USB-Kabel für jeden Bugfix unnötige Reibung. Da ohnehin ein Webserver in Phase 5 gebaut wird, sollte `ArduinoOTA` bzw. Web-basiertes OTA möglichst früh (spätestens mit Phase 5, idealerweise schon als kleiner Baustein in Phase 4) ergänzt werden, statt es ganz wegzulassen.

## 4. Grafikstil-Spezifikation

Gewünschter Stil: **japanische Videospiele der 90er Jahre** (SNES-/frühe PS1-Ära, z. B. Chrono Trigger, Secret of Mana, Dragon Quest, Street Fighter II als Referenzen).

**Konkrete Vorgaben für die Asset-Erstellung:**
- **Proportionen:** Chibi/SD-Stil (überproportional grosser Kopf, kompakter Körper) – typisch für die Ära, wirkt kindgerecht und ist einfacher zu zeichnen/animieren als realistische Proportionen
- **Farbpalette:** kräftig, gesättigt, begrenzt (ca. 16–32 Farben pro Sprite), wie bei 16-Bit-Konsolen üblich
- **Outlines:** klare, meist dunkle/schwarze Umrisslinien um Sprites – typisches Merkmal der Ära
- **Auflösung Sprites:** Standard-Objekte/Charakter (Home-Screen, Aufgaben) 32×32 px; Kampf-Modus-Charaktere grösser, 48×48 bis 64×64 px, da dort mehr Bilddetail für Angriffe/Treffer gebraucht wird
- **Animationsphasen (Richtwert):** Idle 2–4 Frames, Lauf/Bewegung 4–6 Frames, Angriff/Aktion 3–5 Frames
- **Hintergründe:** einfache, sich wiederholende Pixel-Art-Hintergründe (Parallax nicht nötig, Bildschirm ist klein)

**Wichtig für die Architektur:** Sprites werden als Dateien (z. B. Bitmaps/Sprite-Sheets) von der SD-Karte geladen, nicht im Code hardcodiert. So können Platzhalter-Grafiken (einfache Formen) in Phase 0/1 verwendet und später ohne Codeänderung durch finale Artworks ersetzt werden. **Klarstellung:** In Phase 0 gab es noch keine Sprite-Dateien; der Home-Screen zeichnete den Charakter prozedural (Kreis/Formen per `M5GFX`-Primitiven, Farbe/Grösse abhängig von Entwicklungsstufe) als Platzhalter (`HomeScreen::drawPlaceholderCharacter()`).

**Update (Sprite-Grafik):** Der Home-Screen-Charakter nutzt echte 32×32-PNG-Sprites von der SD-Karte (`sdcard/sprites/character/<stufe>_<idle1|idle2|sad>.png`, 6 Stufen × 3 Varianten = 18 Dateien) statt der Platzhalter-Grafik. Da in dieser Session weder ein klassisches Zeichenprogramm noch handgemalte Artworks verfügbar waren, wurden die Sprites **prozedural als echte Bilddateien erzeugt**: `tools/generate_sprites.py` baut jede Form pixelweise aus Ellipsen/Rechtecken/Linien auf einem 32×32-Raster zusammen (keine PIL-Anti-Aliasing-Kanten, dadurch echter Pixel-Art-Look) und exportiert sie als PNG mit transparentem Hintergrund. `HomeScreen::drawSpriteCharacter()` laedt die passende Datei ueber `CharacterRenderer` (siehe unten); existiert sie nicht (SD-Karte fehlt, Datei nicht kopiert), faellt der Code automatisch auf `drawPlaceholderCharacter()` zurueck – die Platzhalter-Grafik ist also nicht entfernt, sondern bleibt als Fallback aktiv.

**Update 2 (Menschenkind statt Kreatur, definierbare Traits, Kampfkuenstler-Motiv):** Auf expliziten Wunsch stellt der Charakter ein **Menschenkind** dar, das ueber die 6 Stufen hinweg sichtbar heranwaechst und trainiert – kein Tier/keine Fantasiekreatur. Konkret: von einem Kleinkind (Stufe "Ei") ueber ein Kind im Karate-Gi mit Weiss-Guertel ("Baby"), Gelb-Guertel ("Kind"), Gruen-Guertel + Stirnband ("Junior"), Braun-Guertel + Schlagpose ("Experte") bis zum Schwarzguertel mit Kampfhaltung ("Meister") – angelehnt an die in Abschnitt 4 ohnehin schon referenzierten 90er-Arcade-Kampfspiele (Street Fighter II, Mortal Kombat). Die Guertelfarbe ist bewusst **nicht** anpassbar (sie zeigt den Trainings-/Lernfortschritt, nicht den persoenlichen Look).

**Definierbare Traits (Hautfarbe/Haarfarbe/Kleidungsfarbe):** Die Sprite-Vorlagen enthalten an diesen drei Stellen reservierte, reine Markerfarben (`0,255,0` / `0,255,255` / `255,255,0`) statt fest eingefaerbter Pixel (siehe `traits::kSkinMarker` usw. in `src/core/CharacterTraits.h`). `CharacterRenderer` (`src/core/CharacterRenderer.*`) laedt die PNG-Datei in ein internes `M5Canvas`, ersetzt dort per direktem Puffer-Zugriff jedes Markerfarb-Pixel durch die im Profil gespeicherte Trait-Farbe (klassisches Palette-Swap-Verfahren) und zeichnet das Ergebnis skaliert + zentriert per `pushRotateZoom(..., transparentColor)`. Die Markerfarben sind bewusst reine 0/255-Kanalwerte gewaehlt, damit die verlustbehaftete 888->565-Quantisierung beim PNG-Decodieren keine Rundungsunschaerfe erzeugt und der exakte Pixelvergleich zuverlaessig bleibt. Jedes Kind kann Hautfarbe (4 Voreinstellungen), Haarfarbe (6, inkl. zwei "Fantasie"-Farben) und Kleidungsfarbe (6) frei waehlen – ueber den neuen, **nicht Eltern-PIN-geschuetzten** `CharacterCustomizeScreen` ("Aussehen", erreichbar ueber das Alltagsfunktionen-Menue, Abschnitt 11): live Vorschau plus </>-Pfeil je Zeile, sofortiges Speichern in `profile.json` (neues `aussehen`-Objekt). Da der Swap zur Laufzeit auf dem Geraet passiert (nicht als vorgerenderte Dateimatrix), bleiben es weiterhin nur 18 Bilddateien statt hunderter Farbkombinationen.

**80er-Jahre-/Synthwave-Farbschema:** Ein zentrales `src/core/Theme.h` definiert ein durchgaengiges Neon-vor-dunklem-Indigo-Farbschema (Hintergrund/Panels dunkles Lila, Akzente in Neonpink/-cyan/-orange/-gold, Erfolg/Gefahr in Neongruen/-rot), das alle Nicht-Spiel-UI-Screens sowie die Kopf-/Statusleisten der 9 Spiele ersetzt, was zuvor direkte `TFT_*`-Farbkonstanten waren. Bewusste Ausnahmen vom Sweep: Text/Icon-Linien bleiben ueberwiegend Weiss (Lesbarkeit fuer die junge Zielgruppe geht vor Theme-Treue), und "wiedererkennungskritische" Farben – die franzoesische Flagge als Fach-Icon, Basketball-Orange/Fussball-Schwarzweiss als Spiel-Icons, sowie die vier Farben des Simon-Says-/Memory-Merkspiels (dort sind unterscheidbare Farben pädagogisch/spielerisch notwendig) – bleiben unveraendert.

**Offen:** Spiel-interne Charaktergrafiken (Kampf-Modus 48–64 px eigene Kampf-Sprites, Puzzle-Bildmotive) nutzen weiterhin keine Sprites, siehe Abschnitt 16.

## 5. Systemarchitektur (Screens/State Machine)

```
BOOT
 └─ Home (Tamagotchi-Charakter, Status, Uhrzeit)
     ├─ Aufgaben-Modus (Fach wählen → Aufgabe → Auswertung → zurück zu Home)
     ├─ Spiele-Menü (nur mit vorhandenem Zeitguthaben betretbar)
     │    └─ [Snake | Tetris | Space Invaders | Pinball | Basketball | Fussball | Puzzle | Moorhuhn-Jagd | Kampf-Modus]
     ├─ Alltagsfunktionen-Menü (Uhr/Wecker/Timer/Kalender/...)
     └─ Einstellungen (Eltern-PIN-geschützt)
```

Jeder Screen als eigene Klasse mit `update()`/`draw()`, zentrale State-Machine im Hauptloop schaltet zwischen Screens um. Charakter-Engine, Aufgaben-Engine und Spielzeitkonto sind eigenständige Module, die von mehreren Screens genutzt werden (nicht an einen Screen gebunden).

**Klarstellung (Stand nach Phase 5):** Implementiert sind `BootScreen` → `ProfileSetupScreen` (nur beim allerersten Start, legt `profile.json` an) → `HomeScreen` mit einer unteren 4-Zonen-Icon-Leiste zu `SubjectSelectScreen` (Fach-Auswahl), `GamesMenuScreen` (nur mit Spielzeitguthaben erreichbar), `AlltagMenuScreen` (Uhr/Timer/Checkliste/Steckbrief) und `PinEntryScreen` (Eltern-PIN, führt zu `SettingsScreen` – inkl. `WebSyncScreen`). `TaskScreen` bedient alle vier Multiple-Choice-Fächer über `TaskEngine` inkl. Spaced Repetition und Schwierigkeitsanstieg. `GedaechtnisScreen` implementiert Gedächtnistraining. Alle 9 Spiele sind über `GamesMenuScreen` nach Charakterstufe gestaffelt erreichbar. `AlarmService` und `NightModeService` prüfen screen-unabhängig aus `main.cpp::loop()`, ob Wecker bzw. Nachtmodus gerade greifen sollen. `WebServerService` (Abschnitt 12) liefert Fortschrittsansicht, Aufgaben-Verwaltung und ArduinoOTA, aktiv nur solange `WebSyncScreen` offen ist. Damit ist die komplette in Abschnitt 14 geplante Phasenfolge im Code umgesetzt.

**Klarstellung (Spiele-Infrastruktur, Phase 3):** Alle Spiele generieren nie EP oder Spielzeit selbst - das bleibt der Aufgaben-Engine und dem Gedächtnistraining vorbehalten (Abschnitt 7/9). Sie sind ausschliesslich die "Verbrauchsseite" der Spielzeit-Ökonomie: `PlaytimeTicker` (`src/core/PlaytimeTicker.*`) buendelt die minutenweise Spielzeit-Abbuchung, die vorher in `SnakeScreen` dupliziert war, in einer gemeinsamen Klasse, die jeder Spiel-Screen einmal pro `update()` aufruft. `HighscoreStore` (`src/core/HighscoreStore.*`) persistiert lokale Bestwerte je Spiel in `/highscores.json` (Abschnitt 10: "Highscore lokal gespeichert" - war in der ersten `SnakeScreen`-Fassung noch nicht umgesetzt, jetzt nachgeholt und auf mehrere Spiele ausgeweitet). Spiele ohne sinnvollen "Highscore"-Begriff (Puzzle: Zuege bis geloest; Kampf-Modus: Sieg/Niederlage) verzichten bewusst auf `HighscoreStore`.

**Review (State-Machine-Sicherheit):** Ein Screen kann nicht direkt `switchTo()` aus seiner eigenen `update()`-Methode heraus sicher aufrufen, weil dabei das eigene Objekt zerstört würde, während sein Code noch auf dem Aufruf-Stack liegt (Use-after-free). Die `StateMachine` bietet deshalb zwei Methoden: `switchTo()` für den einmaligen Erststart aus `main.cpp` heraus, und `requestSwitch()` für den sicheren, verzögerten Wechsel aus einem laufenden Screen heraus (wird erst zu Beginn des nächsten `update()`-Aufrufs angewendet).

**Klarstellung (Zeichnen/Flackern):** Die `StateMachine` ruft absichtlich nur `update()` auf dem aktiven Screen auf, kein begleitendes `draw()`. Jeder Screen entscheidet in seinem eigenen `update()`/`onEnter()`, wann tatsächlich neu gezeichnet wird (z. B. `HomeScreen`/`ClockScreen` höchstens 1×/Sekunde für die Uhrzeit, `TaskScreen` nur bei Zustandswechseln, `SnakeScreen` jeden Frame fürs Animationstempo). Ein zusätzlicher, bedingungsloser `draw()`-Aufruf aus dem Hauptloop würde diese Drosselung wirkungslos machen und auf echter Hardware bei den nicht Sprite-gepufferten Screens sichtbar flackern. `SnakeScreen` zeichnet deshalb konsequent über ein `M5Canvas`-Offscreen-Sprite (Abschnitt 3: "Sprite-basiertes Rendering ... für flüssige Pixel-Art-Grafik") statt direkt auf `M5.Display`.

## 6. Datenmodell

**Review (Abgleich mit Abschnitt 13):** Die ursprüngliche Beispiel-JSON unten bündelte Profil, Charakter, Spielzeit und Aufgaben-Fortschritt in einem Objekt, während Abschnitt 13 sie auf mehrere Dateien aufteilte. Diese Uneindeutigkeit wurde aufgelöst – die Aufteilung ist jetzt verbindlich:

| Datei | Inhalt | Änderungsfrequenz |
|---|---|---|
| `/profile.json` | `profil` (Name, Alter, Klasse, Geräte-ID, Geburtstag Monat/Tag) + `guard` (gehashter Eltern-PIN) + `wecker` (Alarmzeit/an-aus) + `aussehen` (Hautfarbe/Haarfarbe/Kleidungsfarbe-Indizes, Abschnitt 4/9) | selten (i. d. R. einmalig bei Ersteinrichtung bzw. bei Wecker-/Aussehen-Änderung) |
| `/progress.json` | `charakter` (Stufe/EP/letzte Pflege) + `spielzeitkonto` | häufig (bei jeder gelösten Aufgabe / jedem Spiel) |
| `/progress/aufgaben_<fach>.json` | `aufgaben_fortschritt` je Fach (Leitner-Box-Zustand, Abschnitt 8.3) | häufig, aber pro Fach unabhängig |
| `/tasks/<fach>_<klasse>.json` | statische Aufgabenpools (Content, siehe Abschnitt 8.2/13) | selten, i. d. R. nur bei Content-Pflege (Phase 5) |

Begründung für die Trennung von `progress.json` und den Aufgaben-Fortschrittsdateien: `spielzeitkonto` ändert sich sehr häufig (mehrmals pro Session), `aufgaben_fortschritt` kann mit der Zeit gross werden (hunderte Items, Abschnitt 15.5). Beides bei jeder kleinen Änderung gemeinsam neu zu schreiben wäre unnötig teuer und vergrössert das Risiko-Fenster für den Stromausfall-Fall (siehe unten).

**Review (Datenintegrität – Stromausfall):** Kinder trennen das Gerät auch mal mitten im Speichervorgang von der Stromversorgung (leerer Akku, Kabel rausgezogen). Ein direktes Überschreiben der Zieldatei würde in diesem Fall eine abgeschnittene/kaputte JSON-Datei hinterlassen und im schlimmsten Fall den gesamten Fortschritt eines Kindes zerstören. Alle JSON-Dateien werden deshalb **atomar** geschrieben: erst in eine `.tmp`-Datei, dann die bisherige gültige Version nach `.bak` rotieren, dann `.tmp` atomar auf den Zielnamen umbenennen. Lesen fällt automatisch auf `.bak` zurück, falls die Zieldatei fehlt oder beschädigt ist. Implementiert in `src/core/storage/JsonStore.{h,cpp}`, von `ProfileStore`/`ProgressStore` genutzt – siehe Abschnitt 15.1.

**Review (Eltern-PIN-Speicherung):** SD-Karten sind entnehmbar – läge der PIN im Klartext in `profile.json`, könnte ihn jedes Kind am PC auslesen. Der PIN wird deshalb nie im Klartext gespeichert, sondern als Salt + iterierter Streuwert unter dem unauffälligen Feldnamen `guard` (statt z. B. `eltern_pin`). Bewusst keine "echte" Kryptografie (dafür ist ein 4-stelliger Kinder-PIN kein Ziel), aber genug Obskurität gegen zufälliges Auslesen. Implementiert in `src/core/PinCode.{h,cpp}`.

Aktualisiertes Beispiel für `/progress.json` (Phase-0-Umfang; `statistik`/`aufgaben_fortschritt` kommen mit der Aufgaben-Engine in Phase 1/2 dazu):

```json
{
  "charakter": {
    "erfahrungspunkte": 340,
    "letzte_pflege": "2026-09-02"
  },
  "spielzeitkonto": {
    "datum": "2026-09-03",
    "heute_verdient_min": 24,
    "heute_verbraucht_min": 10
  }
}
```

**Review (Freischaltungen nicht redundant speichern):** Das Feld `faehigkeiten` aus der ursprünglichen Beispiel-JSON (z. B. `"basketball_freigeschaltet"`) wird bewusst **nicht** persistiert. Welche Spiele/Skins frei sind, ergibt sich deterministisch aus der Charakterstufe (Abschnitt 9) und wird bei Bedarf berechnet statt dupliziert gespeichert – das vermeidet, dass gespeicherter und tatsächlicher Zustand auseinanderlaufen können.

`stufe` wird ebenfalls nicht separat gespeichert, sondern aus `erfahrungspunkte` über dieselbe Schwellentabelle wie beim Runtime-Zustand hergeleitet (`CharacterEngine::stageForXp`) – aus demselben Grund.

## 7. Spielzeit-Ökonomie

- **2 Minuten Spielzeit** pro richtig gelöster Aufgabe, gutgeschrieben aufs Tageskonto
- **Tageslimit: 60 Minuten verbrauchbare Spielzeit** (Rest verfällt am Tagesende, kein Übertrag – hält es einfach und fair zwischen den Kindern)
- Reset des Tageskontos um Mitternacht via RTC (funktioniert auch offline zuverlässig – Vorteil des Core2)
- ~~**Rückschritt bei Fehler:** falsche Antwort kostet Erfahrungspunkte des Charakters (kein Spielzeitabzug, das wäre demotivierend) – Charakter kann dadurch eine Stufe zurückfallen, wenn EP unter die Stufenschwelle fällt~~

  **Review (geändert):** Falsche Antworten kosten **keine** Erfahrungspunkte mehr, und der Charakter kann dadurch **nicht** mehr eine Stufe zurückfallen. Begründung: EP-Verlust mit sichtbarem Stufen-Downgrade ist trotz des schon vermiedenen Spielzeitabzugs weiterhin ein Bestrafungssignal für falsche Antworten – das entmutigt gerade bei jüngeren Kindern das Raten/Ausprobieren, wo viel Lernen stattfindet. Es riskiert zudem ein "Flackern" der Stufe direkt an einer XP-Schwelle (richtig → Stufenaufstieg → falsch → Rückstufung → ...), was für ein Kind willkürlich wirkt. Neue Regel: **kein Rückschritt**, nur langsameres Vorwärtskommen bei vielen Fehlern (weniger neue EP). "Traurig" wirkt der Charakter ausschliesslich bei mehrtägiger Inaktivität (siehe Abschnitt 9), nie durch einzelne falsche Antworten.

## 8. Aufgaben-Engine

### 8.1 Fächer je Klasse (Baselland, Lehrplan 21/Passepartout – als Ausgangsraster, Feininhalte später mit echtem Schulmaterial abgleichen)

**1. Klasse:**
- Mathe: Zahlenraum bis 20, Addition/Subtraktion, Mengen erfassen
- Rechtschreibung/Deutsch: Buchstaben, einfache Wörter lesen/erkennen, Anlaute
- Quiz: einfache Sachfragen (Tiere, Farben, Formen)
- Gedächtnistraining: Bild-Paare merken (Memory-Prinzip)
- *(Kein Französisch – Fremdsprachenbeginn in Baselland/Passepartout ist die 3. Klasse)*

**3. Klasse:**
- Mathe: Zahlenraum bis 1000, kleines Einmaleins, einfache Textaufgaben
- Rechtschreibung: Wortfamilien, einfache Rechtschreibregeln, Diktat-artige Aufgaben
- Französisch-Vokabeln: Grundwortschatz (Begrüssung, Zahlen, Farben, Familie) – passend zum Fremdsprachenbeginn in der 3. Klasse
- Quiz: altersgerechtes Allgemeinwissen
- Gedächtnistraining: Sequenzen merken, Zuordnungsspiele

### 8.2 Aufgaben-Datenstruktur

```json
{
  "id": "mathe_1kl_add_014",
  "fach": "mathe",
  "klasse": 1,
  "schwierigkeit": 2,
  "typ": "multiple_choice",
  "frage": "5 + 3 = ?",
  "antworten": ["7", "8", "9"],
  "richtig": 1
}
```
Aufgabenpools als separate JSON-Dateien pro Fach/Klasse auf der SD-Karte (`/tasks/mathe_1.json` etc.) – einfach erweiterbar, ohne Neu-Flashen.

### 8.3 Spaced Repetition (vereinfachtes Leitner-System, 5 Boxen)

| Box | Intervall bis Wiederholung |
|---|---|
| 1 | täglich |
| 2 | alle 2 Tage |
| 3 | alle 4 Tage |
| 4 | wöchentlich |
| 5 | alle 2 Wochen |

Richtig beantwortet → eine Box aufsteigen. Falsch → zurück auf Box 1. Bei jeder Aufgaben-Session: fällige Items aus den Boxen zuerst, dazu 1–2 neue Items gemischt.

**Klarstellung (Umsetzung, Phase 2):** Implementiert in `src/core/SpacedRepetitionStore.{h,cpp}`, ein Objekt pro Fach-Session, persistiert unter `/progress/aufgaben_<fach>.json` (Abschnitt 6/13). `TaskEngine::pickNextTask()` baut den Kandidatenkreis aus fälligen bekannten Items + bis zu 2 zufälligen neuen Items und filtert zusätzlich auf die aktuelle Schwierigkeitsstufe (8.4); ist der gefilterte Kreis leer (z. B. sehr kleiner Pool), fällt die Auswahl zunächst auf alle Items der aktuellen Stufe, notfalls auf den gesamten Pool zurück, statt keine Aufgabe mehr anzubieten.

### 8.4 Schwierigkeitsanstieg (Zeit **und** Trefferquote)

- Rollierende Trefferquote der letzten 10 Aufgaben pro Fach wird getrackt
- **Trefferquote ≥ 85 %** → Schwierigkeitsstufe steigt um 1 (bis zum Maximum der Klassenstufe)
- **Trefferquote < 50 %** → Schwierigkeitsstufe bleibt oder sinkt leicht, mehr Wiederholungen leichterer Items
- Zusätzlich langsamer automatischer Anstieg über die Zeit (z. B. +1 Basisstufe pro Schulmonat), damit das Gerät auch bei durchschnittlicher Leistung mit dem Schuljahr mitwächst
- Schwierigkeitsstufe wird bei Auswahl der nächsten Aufgabe aus dem Pool als Filter genutzt

**Review (Präzedenz geklärt):** Die performance-basierte Anpassung und der monatliche Auto-Anstieg konnten sich bisher widersprechen – bei einem Kind mit Trefferquote < 50 % hätte der monatliche Anstieg die Schwierigkeit trotzdem nach oben ziehen können, gegen die Schutzregel für schwache Trefferquoten. **Entschieden:** Der monatliche Auto-Anstieg hebt ausschliesslich die **Obergrenze** (die maximal erreichbare Schwierigkeitsstufe der Klassenstufe), nie die aktuell wirksame Schwierigkeit direkt. Die tatsächlich verwendete Schwierigkeit bleibt weiterhin performance-gesteuert (85 %/50 %-Regel) und kann dadurch nie gegen eine aktuell gedrückte Schwierigkeit "gewinnen" – sie kann höchstens bis zur (jetzt höheren) Obergrenze steigen, wenn die Trefferquote das hergibt.

**Klarstellung (Umsetzung, Phase 2):** Implementiert in `src/core/DifficultyTracker.{h,cpp}`, ein `DifficultyState` je Fach (persistiert in `progress.json` unter `statistik.schwierigkeit.<fach>`, Abschnitt 6). Die rollierende Trefferquote selbst (die letzten bis zu 10 Antworten) wird bewusst NICHT persistiert – nach einem Neustart beginnt das Fenster leer, was für ein Gerät, das selten mitten am Tag neu startet, unkritisch ist und eine unhandliche Array-Serialisierung erspart. Mindestens 5 Antworten werden gesammelt, bevor die 85 %/50 %-Regel überhaupt greift (zu wenige Datenpunkte sonst zu verrauscht). Der monatliche Auto-Anstieg (`applyMonthlyCeilingBump()`) wird bei jedem Betreten des Aufgaben-Modus geprüft und ist durch einen gespeicherten "letzter Monat"-Wert idempotent (löst pro Kalendermonat höchstens einmal aus).

## 9. Tamagotchi-Charaktersystem

**Entwicklungsstufen:** Ei → Baby → Kind → Junior → Experte → Meister (6 Stufen, EP-Schwellen z. B. 0/100/300/700/1500/3000)

**Freischaltungen pro Stufe (9 Spiele total):**
- Baby: Snake
- Kind: Tetris, Puzzle
- Junior: Space Invaders, Moorhuhn-Jagd, neuer Skin
- Experte: Pinball, Basketball
- Meister: Fussball, Kampf-Modus, Sonder-Skin/Umgebung

**Update (Optik, siehe Abschnitt 4 fuer die technischen Details):** "neuer Skin"/"Sonder-Skin" oben sind durch die tatsaechliche Umsetzung abgedeckt, wenn auch anders als urspruenglich gedacht: statt eines separaten Freischalt-Mechanismus fuer Skins aendert sich das Sprite bei jedem Stufenaufstieg automatisch (Guertelfarbe, Stirnband, Kampfhaltung) – das *ist* der neue Skin je Stufe. Zusaetzlich kann das Kind jederzeit Hautfarbe/Haarfarbe/Kleidungsfarbe frei waehlen (`CharacterCustomizeScreen`), unabhaengig vom Stufenfortschritt.

~~**Rückschritt:** EP-Verlust bei Fehlern kann eine Stufe zurückfallen lassen (nicht sofort ein Spiel wieder sperren, aber visuelles Feedback – Charakter wirkt "trauriger").~~ **Review (geändert, siehe Abschnitt 7):** Kein Rückschritt mehr durch Fehler – Stufen steigen ausschliesslich vorwärts. Mehrtägige Inaktivität lässt den Charakter "müde/traurig" wirken (kein harter Rückschritt, aber Anreiz zum täglichen Nutzen) – das bleibt der **einzige** Auslöser für den "traurig"-Zustand.

## 10. Spiele (vollständige Varianten, Steuerungskonzept für Core2)

Für alle Spiele: On-Screen-Touch-Zonen (grosse Tap-Flächen) als primäre Steuerung, IMU-Neigung als optionale bzw. bei manchen Spielen zentrale Alternative.

1. **Snake** – Touch-Zonen links/rechts/oben/unten am Bildschirmrand oder Swipe-Richtung; klassisches Wachstums-/Kollisionsprinzip, Highscore lokal gespeichert
2. **Tetris** – Touch-Zonen: links/rechts bewegen, tippen = drehen, unten swipen = fallen lassen; Standard-7-Steine-Set, steigende Fallgeschwindigkeit
3. **Space Invaders** – links/rechts-Zonen + Feuer-Button, Wellen mit steigendem Schwierigkeitsgrad
4. **Pinball** – zwei Touch-Zonen unten links/rechts als Flipper, Neigungssteuerung (Tilt/IMU) für leichte Balleinflussnahme, wie beim Original
5. **Basketball** – Swipe-Geste (Richtung + Stärke) zum Werfen, Wurfwinkel/Distanz variieren
6. **Fussball** – Swipe zum Schiessen aufs Tor, einfacher Torwart-Gegner mit steigender Reaktionsgeschwindigkeit
7. **Puzzle** – Direktes Touch-Ziehen von Puzzleteilen (Drag&Drop), Bildmotive aus dem Charakter-Universum
8. **Moorhuhn-Jagd (Shooting Gallery)** – Ziele (Hühner o. Ä., thematisch austauschbar) bewegen sich nach Skript-Pfaden über den Bildschirm. **Steuerung: Neigungssensor (IMU) bewegt ein Fadenkreuz**, Antippen des Bildschirms löst den Schuss aus; alternativ Touch-Drag als ruhigere Zielmethode. Score nach Trefferzahl/Zeit, steigende Geschwindigkeit/mehr Ziele als Schwierigkeitskurve
9. **Kampf-Modus (Street-Fighter-inspiriert)** – Vereinfachte 1-gegen-1-Variante gegen eine KI (kein Splitscreen/2-Spieler-Modus, dafür ist der 2"-Bildschirm zu klein): Touch-Zonen links/rechts für Bewegung, zwei grosse Buttons für Schlag/Tritt, eine Swipe-Geste für eine Spezialattacke.
   > **Scope-Hinweis:** Ein vollwertiges Fighting-Game mit klassischen Combo-Eingaben (Viertelkreis-Bewegungen etc.) ist auf einem 2"-Touchscreen kaum sauber und kindgerecht umsetzbar. Geplant ist eine bewusst vereinfachte Variante mit wenigen, klar erkennbaren Aktionen statt komplexer Eingabefolgen – im Look wie Street Fighter, aber in der Steuerungstiefe für Kinder passend reduziert.

Freischalt-Reihenfolge folgt dem Charaktersystem (Abschnitt 9) – hält Motivation über die Zeit aufrecht statt alles von Anfang an verfügbar zu machen.

**Klarstellung (Umsetzung, Phase 3):** Alle 9 Spiele sind implementiert, mit folgenden bewussten Vereinfachungen gegenüber einer "perfekten" Umsetzung (Priorität: kindgerecht spielbar und wartbar statt feature-vollstaendig):
- **Tetris:** Rotation ist eine generische 90°-Drehung des 4x4-Rasters statt vier hartcodierter Rotationszustaende pro Stein (SRS) – kein Wandkick, eine Drehung am Spielfeldrand wird bei Kollision einfach verworfen.
- **Space Invaders:** wie das Original-Arcade-Spiel nur ein Spieler-Geschoss gleichzeitig in der Luft.
- **Pinball:** vereinfachte Physik (Kugel + Schwerkraft + zwei "Flipper"-Touch-Zonen + Bumper), keine echte Fluepper-Rotation; Feintuning der Physik-Konstanten war ohne Testgeraet in dieser Session nicht möglich, siehe Abschnitt 16.
- **Basketball:** klassischer Wurf mit Schwerkraft-Bogen. **Fussball:** bewusst *ohne* Bogen (geradliniger Schuss), um das Torwart-Timing klarer lesbar zu machen – beide nutzen dieselbe Swipe-Grundmechanik, aber unterschiedliche Flugbahnen.
- **Puzzle:** die "Bildmotive" sind weiterhin nummerierte Farbkacheln (1–9) statt Charakter-Grafiken – das Home-Screen-Sprite-Update (Abschnitt 4) deckt bewusst nur den Tamagotchi-Charakter ab, nicht die Spiel-internen Bildmotive.
- **Moorhuhn-Jagd:** Fadenkreuz-Position ist eine geglättete *absolute* Zuordnung von Neigungswinkel zu Bildschirmposition (kein Aufintegrieren von Geschwindigkeit, das würde bei einer IMU über die Zeit wegdriften).
- **Kampf-Modus:** KI-Schwierigkeit ist bewusst fest (skaliert nicht mit dem Charakterlevel) – siehe offener Punkt in Abschnitt 16.

Keines der Spiele generiert EP oder Spielzeit (das bleibt der Aufgaben-Engine/Gedächtnistraining vorbehalten, Abschnitt 7/9) – sie verbrauchen ausschliesslich Spielzeitguthaben ueber `PlaytimeTicker` (siehe Review-Anmerkung nach Abschnitt 5).

**Review (Scope/Ressourcen):** 9 vollständige Spiele + Aufgaben-Engine + Spaced Repetition + Charaktersystem + Web-Interface sind viel Fläche für ein Hobbyprojekt auf einem Mikrocontroller. Jedes Spiel ist für sich ein kleines eigenständiges Projekt (v. a. Tetris/Pinball/Kampf-Modus). Die Phasenplanung (Abschnitt 14) staffelt das bereits sinnvoll – "Phase 3: alle 9 Spiele vollständig" ist trotzdem realistisch der grösste Zeitblock im gesamten Plan und sollte entsprechend budgetiert werden.

## 11. Alltagsfunktionen

**Kernanforderung:**
- Uhr (grosse, kindgerechte Anzeige, analog oder digital wählbar)
- Wecker (mit Vibration + Sound, mehrere Alarme möglich)

**Zusätzliche Vorschläge:**
- **Timer/Stoppuhr** – z. B. für Zähneputzen (2-Min-Timer mit Countdown-Animation des Charakters)
- **Tages-/Routine-Checkliste** – z. B. "Anziehen, Zähneputzen, Znüni einpacken" zum Abhaken am Morgen, gibt kleine EP-Belohnung
- **Geburtstags-/Ereignis-Countdown** – Countdown-Anzeige zu Ereignissen wie Geburtstag, Ferienbeginn
- **Einfacher Taschenrechner** – für den 3.-Klässler auch als "Übungswerkzeug" nutzbar
- **Charakter-Steckbrief** – Übersicht über Stufe, Fähigkeiten, Statistiken ("mein Tamagotchi")
- **Nachtmodus** – Bildschirm dimmt/schaltet sich nach einstellbarer Uhrzeit automatisch ab (RTC-basiert, auch ohne WLAN zuverlässig)
- **Eltern-PIN** – für Einstellungen, Tageslimit-Anpassung, evtl. manuelles Zeitguthaben

**Review (RTC-Drift/NTP, neu):** Die BM8563-RTC hält die Zeit offline zuverlässig, driftet aber wie jede Quarz-RTC über Monate hinweg und kennt keine automatischen CEST/CET-Umstellungen (relevant für einen Schweizer Haushalt). Sobald WLAN verfügbar ist (spätestens Phase 5), sollte ein NTP-Sync-Pfad die RTC nachstellen – die RTC bleibt dabei die primäre, offline-taugliche Zeitquelle, NTP ist nur eine gelegentliche Korrektur. Vorbereitet in `src/core/RtcClock.{h,cpp}` (`syncFromNtpIfAvailable()`, aktuell ein No-op ohne WLAN-Konfiguration – Anknüpfungspunkt für Phase 5).

**Klarstellung (Umsetzung, Phase 4):**
- **Timer:** drei feste Presets (2/5/10 Min) statt freier Zeiteingabe – deckt den genannten Zähneputz-Anwendungsfall direkt ab (`TimerScreen`).
- **Checkliste:** feste, im Code hinterlegte Morgen-Routine ("Anziehen", "Zähne putzen", "Znüni einpacken", `ChecklistScreen`), EP-Belohnung einmal pro Tag beim vollständigen Abhaken. Zustand ist bewusst nur In-Memory (kein Reboot-Schutz) – siehe Abschnitt 16.
- **Charakter-Steckbrief:** `SteckbriefScreen`, rein lesende Übersicht (Name/Stufe/EP/Klasse/freigeschaltete Spiele/Tage seit letzter Pflege).
- **Update (Aussehen, nach Phase 5):** `AlltagMenuScreen` bekam ein fuenftes Feld ("Look", 3x2-Raster statt 2x2) zu `CharacterCustomizeScreen` – Hautfarbe/Haarfarbe/Kleidungsfarbe frei waehlen, siehe Abschnitt 4/9. Bewusst NICHT Eltern-PIN-geschuetzt (reine Optik, kein Limit-/Sicherheitsthema) und direkt vom Kind bedienbar.
- **Update (Geburtstags-Countdown, nach Phase 5):** Sechstes und letztes Feld im jetzt vollstaendig gefuellten 3x2-Raster ("Torte") zu `BirthdayScreen`. Zaehlt die Tage bis zum naechsten Geburtstag anhand von Monat/Tag aus `include/KidProfiles.h` (kein Jahr noetig, siehe dort) - beim Erststart in `profile.json` uebernommen (`ProfileSetupScreen::commitSelection()`). Zeigt am Geburtstag selbst eine Glueckwunsch-Meldung statt einer Tageszahl; fehlt der Geburtstag (0/0, z. B. bei einem vor dieser Funktion angelegten Profil), erscheint ein Hinweis statt eines falschen Countdowns. Bewusst kein Alarm/keine Benachrichtigung - das Kind schaut aktiv nach, kein zusaetzlicher Signalweg noetig (Abschnitt 11 nannte urspruenglich auch "Ferienbeginn" als moegliches Ereignis; umgesetzt ist bewusst nur der Geburtstag, da explizit angefragt).
- **Nachtmodus:** `NightModeService` dimmt den Bildschirm screen-unabhängig zwischen einstellbaren Start-/Endstunden (Default 20–7 Uhr, Wraparound über Mitternacht unterstützt). Ohne "kurz antippen zum Aufhellen" – siehe Abschnitt 16.
- **Eltern-PIN:** `PinEntryScreen` (Ziffernblock, kein Text) gated `SettingsScreen`; dort Tageslimit anpassen (±5 Min, 15–180 Min), Bonus-Zeit vergeben (+10 Min), Nachtmodus an/aus, PIN ändern (führt zurück zu `PinEntryScreen` im "neuen PIN festlegen"-Modus). Eine UI zum Anpassen der Nachtmodus-Uhrzeiten selbst ist nicht umgesetzt.
- **Nicht umgesetzt** (Zeitbudget, siehe Abschnitt 16): Taschenrechner, mehrere Wecker gleichzeitig (aktuell weiterhin nur einer, wie in Phase 1 angelegt).

## 12. Web-Interface (Phase 5, nicht von Anfang an nötig)

**Zweck:**
- Fortschritt/Statistik einsehen (für Eltern)
- Neue Aufgaben hinzufügen/bearbeiten, ohne Gerät neu zu flashen
- Tageslimit, Minuten-pro-Aufgabe zentral anpassen

**Technischer Ansatz:** ESP32 im Access-Point- oder Stationsmodus, `ESPAsyncWebServer` liefert einfache HTML-Seite (Tabelle mit Aufgaben, Formular zum Hinzufügen), Daten als JSON über die vorhandene SD-Struktur – kein separates Backend nötig, das Core2 ist Server und Speicher zugleich. **Siehe Abschnitt 3 (Review) zur Bibliothekswahl (`ESP32Async`-Fork statt `me-no-dev`) und zu OTA-Updates.**

**Klarstellung (Umsetzung, Phase 5):** `WebServerService` (`src/core/WebServerService.{h,cpp}`) implementiert alle drei Zwecke:
- **Fortschritt/Statistik:** `GET /` zeigt Stufe, EP, heutige Spielzeit/Tageslimit.
- **Aufgaben verwalten:** `GET /tasks?fach=…&klasse=…` listet den Pool einer Datei als Tabelle mit Löschen-Link je Zeile, plus Formular zum Hinzufügen (`POST /tasks/add`). Schreibt über `storage::saveJsonAtomic()` – die Aufgabenpool-Dateien sind damit ab Phase 5 nicht mehr rein lesend, sondern nutzen dieselbe Stromausfall-sichere Schreiblogik wie `profile.json`/`progress.json` (Abschnitt 6).
- **Tageslimit zentral anpassen:** `POST /settings` – dieselbe Wirkung wie die Tageslimit-Zeile in `SettingsScreen` (Abschnitt 11), nur vom Browser aus.
- **OTA:** `ArduinoOTA` statt eines selbstgebauten Datei-Upload-Formulars (Abschnitt 3) – einfacher und ohne eigene Multipart-Parsing-Logik, die ohne Testgerät schwer zuverlässig zu verifizieren gewesen wäre. Firmware-Update per `pio run -t upload --upload-port <Geräte-IP>`, sichtbar auf `GET /` sobald `WebSyncScreen` aktiv ist.

Der Access Point (SSID `Tamagotchi-<Name>`, siehe `include/config.h`) läuft bewusst nur, solange `WebSyncScreen` geöffnet ist (`onEnter()`/`onExit()` starten/stoppen ihn) – dauerhaftes WLAN-Senden würde den kleinen Akku unnötig belasten (Abschnitt 2, Review).

## 13. Speicherung (SD-Karten-Struktur)

```
/profile.json
/progress.json
/progress/aufgaben_mathe.json
/progress/aufgaben_rechtschreibung.json
/progress/aufgaben_franzoesisch.json
/progress/aufgaben_quiz.json
/progress/aufgaben_gedaechtnis.json
/tasks/mathe_1.json
/tasks/mathe_3.json
/tasks/rechtschreibung_1.json
/tasks/rechtschreibung_3.json
/tasks/franzoesisch_3.json
/tasks/quiz_1.json
/tasks/quiz_3.json
/tasks/gedaechtnis_1.json
/tasks/gedaechtnis_3.json
/sprites/character/ei_idle1.png
/sprites/character/ei_idle2.png
/sprites/character/ei_sad.png
/sprites/character/baby_idle1.png … meister_sad.png (6 Stufen × 3 Varianten, siehe Abschnitt 4)
```

**Review:** Struktur an das aufgelöste Datenmodell aus Abschnitt 6 angepasst (Aufgaben-Fortschritt pro Fach statt in `progress.json` gebündelt; siehe dortige Tabelle für Begründung). Jede hier gelistete `.json`-Datei existiert zur Laufzeit zusätzlich potenziell als `.tmp`- (während des Schreibens) bzw. `.bak`-Variante (Backup der letzten gültigen Version) – siehe Abschnitt 6, Review zur Datenintegrität.

## 14. Projektphasen

| Phase | Inhalt |
|---|---|
| 0 | Hardware-Setup, PlatformIO-Projekt, Grundgerüst State-Machine, Home-Screen mit Platzhalter-Charakter |
| 1 (MVP) | Ein Fach (z. B. Mathe) inkl. Aufgaben-Engine ohne Spaced Repetition, Spielzeitkonto, 1 Spiel (Snake), Uhr/Wecker |
| 2 | Alle Fächer, Spaced Repetition, Schwierigkeitsanstieg, Charaktersystem mit Stufen |
| 3 | Alle 9 Spiele vollständig |
| 4 | Erweiterte Alltagsfunktionen, Nachtmodus, Eltern-PIN, (idealerweise) erster OTA-Baustein |
| 5 | Web-Interface für Sync/Content-Pflege, OTA-Updates spätestens hier vollständig |

**Stand:** Alle 6 Phasen (0–5) sind im Code umgesetzt: State-Machine, Storage-Layer mit atomaren Schreibvorgängen, `BootScreen`, `ProfileSetupScreen` (Kind-Profil-Auswahl aus `include/KidProfiles.h`), `HomeScreen` mit echtem Sprite-Charakter (Menschenkind, waechst/trainiert ueber die Stufen, `sdcard/sprites/character/`, Fallback auf Platzhalter-Grafik ohne SD-Karte/Dateien – siehe Abschnitt 4) + Namen + 4-Zonen-Navigation, `SubjectSelectScreen`, `TaskScreen`/`TaskEngine` (alle vier Multiple-Choice-Fächer, mit Spaced Repetition und Schwierigkeitsanstieg), `GedaechtnisScreen` (Karten-Memory/Sequenz-Merkspiel), `GamesMenuScreen` + alle 9 Spiele, `AlltagMenuScreen` (Uhr/Wecker, Timer, Checkliste, Steckbrief, Aussehen/`CharacterCustomizeScreen`, Geburtstags-Countdown/`BirthdayScreen`), `PinEntryScreen` + `SettingsScreen` (Eltern-PIN-geschützt: Tageslimit, Bonus-Zeit, Nachtmodus, PIN ändern, Web-Sync), `AlarmService` + `NightModeService`, `WebServerService` + `WebSyncScreen` (Fortschrittsansicht, Aufgaben-Verwaltung, Tageslimit, ArduinoOTA). Siehe `README.md` für den aktuellen Stand und Build-Anleitung – und den dortigen Hinweis, dass nichts davon in dieser Session kompiliert/getestet werden konnte.

## 15. Review-Zusammenfassung

Kurzüberblick über alle inhaltlichen Änderungen gegenüber der ursprünglichen Planfassung (Details jeweils an der zitierten Stelle oben):

1. **Datenintegrität (Abschnitt 6):** Atomares Schreiben (`.tmp` → `.bak`-Rotation → Umbenennen) für alle JSON-Dateien auf der SD-Karte, gegen Datenverlust durch Stromausfall/Batterie-leer mitten im Schreibvorgang.
2. **Datenmodell-Aufteilung (Abschnitt 6/13):** Eindeutige, dokumentierte Aufteilung auf `profile.json` / `progress.json` / `progress/aufgaben_<fach>.json` / `tasks/<fach>_<klasse>.json` statt widersprüchlicher Beispiele.
3. **Eltern-PIN (Abschnitt 6):** Nie im Klartext gespeichert, sondern gesalzen/gehasht unter unauffälligem Feldnamen.
4. **Schwierigkeits-Präzedenz (Abschnitt 8.4):** Monatlicher Auto-Anstieg hebt nur die Obergrenze, überschreibt nie eine performance-bedingt gedrückte Schwierigkeit.
5. **Kein EP-/Stufen-Rückschritt mehr (Abschnitt 7/9):** Falsche Antworten kosten keine EP und lösen keinen Stufenabstieg mehr aus – vermeidet Bestrafungssignale beim Lernen und "Flackern" der Stufe an XP-Schwellen. "Traurig" ausschliesslich durch Inaktivität.
6. **Geräte-Profil (Abschnitt 3):** Laufzeit-Konfiguration (eine Firmware, Auswahl beim Erststart) statt Kompilierzeit-Flag.
7. **ESPAsyncWebServer-Fork (Abschnitt 3/12):** `ESP32Async`-Fork statt des unmaintained `me-no-dev`-Originals empfohlen. **Tatsächlich umgesetzt (Phase 5) wurde stattdessen der synchrone `WebServer`-Kern ohne externe Abhängigkeit** – Begründung siehe Abschnitt 3/12 (Nebenläufigkeits-Risiko beim SD-Zugriff ohne Testgerät).
8. **OTA-Updates (Abschnitt 3/14, neu):** Als expliziter Punkt ergänzt statt implizit wegzulassen.
9. **RTC-Drift/NTP (Abschnitt 11, neu):** NTP-Sync-Pfad für verfügbares WLAN ergänzt, RTC bleibt Offline-Primärquelle.
10. **Icon-first-Navigation (Abschnitt 5, neu):** Für die jüngere Zielgruppe (1. Klasse, eingeschränkte Lesefähigkeit) Ziffern-/Icon-basierte statt textbasierte Auswahl-Screens, ab `ProfileSetupScreen` umgesetzt.
11. **Akku/Low-Battery (Abschnitt 2, neu):** Home-Screen zeigt eine Akku-Warnung bei niedrigem Ladestand und stösst bei kritischem Ladestand einen sichernden Speichervorgang an, statt nur zu hoffen, dass der Akku nicht mitten im nächsten Schreibvorgang leer wird.
12. **Sprite-Grafik (Abschnitt 4, neu, nach Phase 5):** Home-Screen-Charakter durch echte, prozedural generierte PNG-Sprites ersetzt (`tools/generate_sprites.py`, 6 Stufen × 3 Varianten). `HomeScreen::drawPlaceholderCharacter()` bleibt als automatischer Fallback bestehen, falls SD-Karte/Sprite-Dateien fehlen.
13. **Menschenkind statt Kreatur + definierbare Traits + 80er-Theme (Abschnitt 4/9, neu):** Charakter-Sprites komplett neu gezeichnet als Menschenkind, das ueber die Stufen zum Kampfkuenstler-Kind heranwaechst (Guertelfarben weiss->gelb->gruen->braun->schwarz), statt der urspruenglichen Kreatur-Optik. Hautfarbe/Haarfarbe/Kleidungsfarbe sind jetzt per `CharacterCustomizeScreen` frei waehlbar (Palette-Swap zur Laufzeit via `CharacterRenderer`, siehe Abschnitt 4). Ein zentrales `Theme.h` mit Neon-vor-Indigo-Farbschema (Street-Fighter-/Mortal-Kombat-Arcade-Anmutung) ersetzt die bisherigen direkten `TFT_*`-Farben in praktisch allen Nicht-Spiel-Screens.
14. **Geburtstags-Countdown + erweiterter Aufgaben-Content (Abschnitt 8/11, neu):** `BirthdayScreen` zaehlt die Tage bis zum naechsten Geburtstag (Monat/Tag aus `include/KidProfiles.h`, dort wo auch der Name steht - wie angefragt), erreichbar ueber das nun vollstaendig gefuellte 3x2-Alltagsfunktionen-Raster. Alle 7 Aufgabenpools unter `sdcard/tasks/` von 8–10 auf 25–30 Fragen je Datei erweitert (alle Antworten rechnerisch/inhaltlich gegengeprueft).
15. **C++-Standard in `platformio.ini` (Abschnitt 3, erster echter Kompilierfehler):** siehe Update-Hinweis in Abschnitt 3 - `build_unflags`/`build_flags` fuer C++17 ergaenzt, da `std::make_unique` sonst nicht verfuegbar war.
12. **Freischaltungen nicht redundant gespeichert (Abschnitt 6):** `faehigkeiten`/`stufe` werden aus der Charakterstufe/EP hergeleitet statt separat persistiert (vermeidet Drift zwischen gespeichertem und tatsächlichem Zustand).
13. **Kind-Profile mit Name/Alter statt reiner Klassenwahl (Abschnitt 3, neu):** Name und Alter der Kinder werden in `include/KidProfiles.h` im Code hinterlegt statt beim Erststart nur abstrakt "1./3. Klasse" zu wählen. Der Charakter trägt den Namen des Kindes, die Klassenstufe (und damit die Aufgaben-Schwierigkeit) wird aus dem Alter hergeleitet.

Weiterhin offen/nicht Teil des Reviews (Scope/Ressourcen-Hinweise ohne konkrete Planänderung): Umfang von 9 vollständigen Spielen (Abschnitt 10) und Content-Autorenschaft für die Aufgabenpools (Abschnitt 8) bleiben die grössten Zeitrisiken des Projekts und sind bewusst nicht reduziert worden, sondern nur explizit benannt.

## 16. Kleinere offene Punkte für die Code-Phase

- Genaue EP-Schwellen/Stufenwerte final festlegen (Vorschlagswerte oben als Startpunkt). EP pro richtig gelöster Aufgabe ist aktuell ebenfalls ein Platzhalter (`config::kXpPerCorrectAnswer = 15`), ebenso die maximale Schwierigkeitsstufe (`config::kMaxDifficultyStage = 5`).
- Aufgaben-Content ist erweitert (Mathe: 30, alle anderen Fächer/Klassen: 25 Fragen je Datei unter `sdcard/tasks/`, gegenüber urspruenglich 8–10) und damit spürbar weniger repetitiv als der erste Platzhalter-Umfang, aber weiterhin selbst verfasst statt curriculumsgeprüfter Lehrplan-21/Passepartout-Stoff – Content-Pflege/-Erweiterung bleibt der grösste Zeitfaktor auf lange Sicht (siehe Abschnitt 10, Scope/Ressourcen-Hinweis).
- Home-Screen-Charakter nutzt seit dem Sprite-Update (Abschnitt 4) echte, prozedural generierte PNG-Sprites (`tools/generate_sprites.py`) statt der reinen Formen-Platzhalter – als Menschenkind mit definierbaren Traits (Hautfarbe/Haarfarbe/Kleidungsfarbe ueber `CharacterCustomizeScreen`) statt der urspruenglichen Kreatur-Optik. Offen bleibt: (a) Kampf-Modus-Sprites (48–64 px, mit Angriffs-/Treffer-Frames) und Puzzle-Bildmotive nutzen weiterhin keine Sprites; (b) die generierten Sprites sind einfacher als handgezeichnete 90er-Jahre-Referenzsprites (kein Dithering, keine Lauf-/Angriffsanimation); (c) Traits sind auf je 4–6 Voreinstellungen begrenzt (kein freier Farbwaehler) – bewusste Vereinfachung fuer eine kindgerechte, mit zwei Fingertipps bedienbare UI. Bei Bedarf sind sowohl Sprites als auch Trait-Paletten ohne Codeänderung an der Logik ersetzbar: Sprites durch Austausch der Dateien unter `sdcard/sprites/character/` (gleiches Namensschema `<stufe>_<idle1|idle2|sad>.png`, Markerfarben aus `src/core/CharacterTraits.h` beachten), Paletten durch Anpassen der `kSkinTones`/`kHairColors`/`kClothingColors`-Arrays in derselben Datei.
- Soll der Eltern-PIN pro Gerät gleich oder unterschiedlich sein? (Aktuell: identischer Default-PIN auf beiden Geräten, siehe `config::kDefaultParentalCode` – muss vor "produktivem" Einsatz in den Einstellungen geändert werden, sobald Abschnitt 11/Einstellungen in Phase 4 existiert.)
- Gegner-KI-Schwierigkeit im Kampf-Modus und beim Fussball-Torwart: wie stark soll sie mit dem Charakterlevel mitwachsen? Aktuell fest (Fussball-Torwart wird nur innerhalb einer Session schneller, je mehr Tore fallen; Kampf-Modus-KI ist komplett fest).
- Physik-/Gameplay-Konstanten der Phase-3-Spiele (Schwerkraft, Wurf-/Schuss-Geschwindigkeiten, Neigungsempfindlichkeit bei Moorhuhn-Jagd/Pinball, Fallgeschwindigkeit bei Tetris) sind plausible Startwerte ohne Testgeraet – nach dem ersten Spielen auf echter Hardware wahrscheinlich nachzujustieren.
- Web-Sync-Passwort (`config::kWebSyncApPassword`) ist ein Platzhalter im Klartext im Code – für den Eigenbedarf unkritisch (kein Internet-exponierter Dienst, nur lokaler AP), sollte aber vor Weitergabe des Codes an Dritte angepasst werden.
- Web-Interface hat keine eigene Authentifizierung ausser dem AP-WLAN-Passwort – wer im AP ist, kann Aufgaben bearbeiten und das Tageslimit ändern. Für den Familiengebrauch (Eltern kontrollieren, wer sich mit dem kurzzeitig laufenden AP verbindet) ausreichend, aber kein Ersatz für die Eltern-PIN am Gerät selbst.
- Web-Interface unterstützt nur Hinzufügen/Löschen von Aufgaben, kein Bearbeiten bestehender Einträge (Löschen + neu Hinzufügen ist der Workaround) – Zeitbudget-Entscheidung.
- Die gesamte Phase-5-Implementierung (WLAN/AP, WebServer-Routing, ArduinoOTA) ist der am wenigsten verifizierbare Teil dieses Projekts, da diese Session weder kompilieren noch auf echter Hardware testen konnte – vor Verlass auf die Web-Sync-Funktion unbedingt einmal real durchspielen (AP verbinden, Aufgabe hinzufügen/löschen, Tageslimit ändern, `pio run -t upload --upload-port <ip>` versuchen).
- Checklisten-Zustand (Abschnitt 11) ist nur In-Memory – ein Neustart mitten in der Morgenroutine verliert die bereits abgehakten Punkte fuer diesen Tag. Falls das in der Praxis stoert, waere eine kleine Persistenz in progress.json nachruestbar.
- Nachtmodus-Start-/Endzeit ist in profile.json vorbereitet, aber ohne eigene Anpassen-UI in SettingsScreen (aktuell nur an/aus umschaltbar, Zeiten bleiben beim Default 20-7 Uhr). Ebenso fehlt ein "kurz antippen zum Aufhellen" waehrend der Nachtstunden.
- Taschenrechner aus Abschnitt 11 ("Zusaetzliche Vorschlaege") ist nicht umgesetzt. Geburtstags-Countdown ist seit dem `BirthdayScreen`-Update umgesetzt (siehe Abschnitt 11); ein allgemeinerer Ereignis-Countdown (z. B. Ferienbeginn) bleibt offen, da nur der Geburtstag explizit angefragt wurde.
- Mehrere gleichzeitige Wecker (Abschnitt 11 nennt "mehrere Alarme moeglich") sind nicht umgesetzt - aktuell weiterhin nur ein Alarm pro Geraet, wie bereits in Phase 1 angelegt.
- **Vor dem Flashen:** `include/KidProfiles.h` mit den echten Namen/Altern/Geburtstagen der eigenen Kinder befüllen (aktuell Platzhalter "Kind 1"/"Kind 2" mit Platzhalter-Geburtstagen) und `sdcard/tasks/mathe_1.json`/`mathe_3.json` auf die SD-Karte unter `/tasks/` kopieren (siehe README).
- Klassen-Alters-Schwelle (`include/KidProfiles.h::kKlasse3AgeThreshold`, aktuell 8 Jahre) ist ein grober Richtwert und ggf. an den tatsächlichen Schuleintritt der Kinder anzupassen.
- Das 80er-/Synthwave-Farbschema (`src/core/Theme.h`) deckt bewusst alle Nicht-Spiel-Screens sowie die Kopf-/Statusleisten der 9 Spiele ab, nicht aber die gameplay-kritischen Objektfarben innerhalb der Spiele selbst (z. B. Tetris-Blockfarben, Space-Invaders-Gegner, Snake) – dort wuerde eine erzwungene Theme-Angleichung die farbliche Unterscheidbarkeit der Spielobjekte verschlechtern. Ebenfalls bewusst unveraendert: die franzoesische Flagge als Fach-Icon und die vier Farben des Memory-/Sequenz-Merkspiels (Wiedererkennbarkeit bzw. Spielmechanik wichtiger als Theme-Treue).
