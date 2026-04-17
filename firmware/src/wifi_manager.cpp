#include <WiFi.h>
#include "config.h"
#include "wifi_manager.h"

void connectWiFi() {
    Serial.println("[WiFi] Connecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("\n[WiFi] Connected");
}
