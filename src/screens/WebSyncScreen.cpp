#include "WebSyncScreen.h"

#include <M5Unified.h>

#include "../core/ScreenId.h"
#include "../core/Theme.h"
#include "../core/WebServerService.h"
#include "config.h"

namespace {
constexpr int kHomeIconSize = 28;
} // namespace

WebSyncScreen::WebSyncScreen(AppContext& app, StateMachine& stateMachine)
    : app_(app), stateMachine_(stateMachine) {}

void WebSyncScreen::onEnter() {
    webserverservice::start(&app_);
    draw();
}

void WebSyncScreen::onExit() {
    webserverservice::stop();
}

void WebSyncScreen::update(uint32_t) {
    // Treibt den synchronen WebServer + ArduinoOTA an (siehe
    // WebServerService.h fuer die Begruendung, warum das hier statt eines
    // Async-Servers geschieht) - muss jeden Frame laufen, solange dieser
    // Screen aktiv ist.
    webserverservice::handleClient();

    const auto touch = M5.Touch.getDetail();
    if (touch.wasPressed() && touchedHomeIcon(touch.x, touch.y)) {
        stateMachine_.requestSwitch(ScreenId::Settings);
    }
}

bool WebSyncScreen::touchedHomeIcon(int x, int y) const {
    return x >= M5.Display.width() - kHomeIconSize - 6 && y <= kHomeIconSize + 6;
}

void WebSyncScreen::drawHomeIcon() const {
    const int x = M5.Display.width() - kHomeIconSize - 6;
    const int y = 6;
    M5.Display.drawRoundRect(x, y, kHomeIconSize, kHomeIconSize, 4, theme::kText);
    M5.Display.fillTriangle(x + kHomeIconSize / 2, y + 4, x + 5, y + 14, x + kHomeIconSize - 5, y + 14, theme::kText);
    M5.Display.fillRect(x + 8, y + 13, kHomeIconSize - 16, kHomeIconSize - 17, theme::kText);
}

void WebSyncScreen::draw() {
    M5.Display.fillScreen(theme::kBackground);
    M5.Display.fillRect(0, 0, M5.Display.width(), 30, theme::kPanel);
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Web-Sync", 6, 4);

    int y = 46;
    M5.Display.setTextColor(theme::kText);
    M5.Display.setTextSize(2);
    M5.Display.drawString("WLAN verbinden mit:", 14, y);
    y += 26;
    M5.Display.setTextColor(theme::kAccentGold);
    M5.Display.drawString(webserverservice::apSsid(), 14, y);
    y += 32;

    M5.Display.setTextColor(theme::kText);
    M5.Display.drawString("Passwort:", 14, y);
    y += 26;
    M5.Display.setTextColor(theme::kAccentGold);
    M5.Display.drawString(config::kWebSyncApPassword, 14, y);
    y += 32;

    M5.Display.setTextColor(theme::kText);
    M5.Display.drawString("Dann im Browser oeffnen:", 14, y);
    y += 30;
    M5.Display.setTextColor(theme::kAccentCyan);
    M5.Display.setTextSize(3);
    M5.Display.drawString(String("http://") + webserverservice::apIp(), 14, y);

    drawHomeIcon();
}
