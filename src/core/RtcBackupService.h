#pragma once

// Rettungsanker fuer die RTC-Uhrzeit (Nutzer-Feedback: "Uhrzeit geht
// verloren, wenn man das Geraet ausschaltet" - was in Folge auch den
// Spielzeit-Timer kaputt wirken liess, siehe unten).
//
// Manche Core2-Board-Revisionen haben keinen (ausreichend dimensionierten)
// Backup-Akku fuer die BM8563-RTC - der puffert normalerweise nur einen
// kurzen Stromausfall (z. B. Akku-Wechsel), nicht ein laengeres echtes
// Abschalten. Seit dem Wechsel von ESP.restart() auf M5.Power.powerOff()
// (Nutzerwunsch: "kein Reset, richtiges Ausschalten") kappt ein langer
// Tastendruck jetzt wirklich die komplette Stromversorgung - verliert die
// RTC dabei ihre Pufferspannung, faellt sie beim naechsten Einschalten auf
// ihr Chip-Default-Datum zurueck (z. B. Jahr 2000), was unter
// config::kRtcPlausibleMinYear faellt.
//
// Das ist nicht nur kosmetisch: PlaytimeAccount::rolloverIfNewDay()
// erkennt anhand des RTC-Datums einen neuen Tag und setzt dann verdiente
// Spielzeit zurueck - bei einem auf Jahr 2000 zurueckgefallenen Datum sieht
// JEDER Neustart wie ein neuer Tag aus, das Spielzeitguthaben verschwindet
// dadurch sofort wieder (das vom Nutzer gemeldete "Spielzeit-Timer
// funktioniert nicht").
//
// Dieses Modul sichert die zuletzt bekannte RTC-Zeit periodisch sowie
// unmittelbar vor M5.Power.powerOff() auf die SD-Karte und stellt sie beim
// naechsten Boot wieder her, falls die RTC eine unplausible Zeit zeigt.
// Keine perfekte Loesung (die waehrend der Abschaltzeit tatsaechlich
// vergangene Zeit bleibt unbekannt), aber verhindert den Sprung auf ein
// Fantasie-Datum und behandelt den Alltagsfall "kurz aus, wieder an"
// korrekt, statt das Spielzeitguthaben jedes Mal zu verlieren.
namespace rtcbackupservice {

// Sichert die aktuelle RTC-Zeit atomar auf die SD-Karte - Aufrufer:
// main.cpp::loop() periodisch (Schutz gegen unerwarteten Stromverlust,
// z. B. leerer Akku) und unmittelbar vor M5.Power.powerOff() (der
// wichtigste Zeitpunkt, siehe oben). No-op, wenn die RTC selbst gerade
// eine unplausible Zeit zeigt (Jahr < config::kRtcPlausibleMinYear) -
// sonst wuerde eine bereits kaputte Zeit "gesichert" werden.
void save();

// Prueft beim Boot, ob die RTC eine unplausible Zeit zeigt, und stellt in
// diesem Fall die zuletzt gesicherte Zeit wieder her, falls eine Sicherung
// existiert. Muss aufgerufen werden, bevor irgendein Screen/Service die
// RTC-Zeit liest (siehe BootScreen::update()). Gibt true zurueck, wenn
// eine Wiederherstellung stattgefunden hat (fuer Serial-Diagnose durch
// den Aufrufer).
bool restoreIfImplausible();

} // namespace rtcbackupservice
