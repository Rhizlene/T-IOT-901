#include "wifi_manager.h"

bool connectWiFi() {
    Serial.println("Connexion WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("WiFi OK - IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    Serial.println("\nWiFi FAILED");
    return false;
}
