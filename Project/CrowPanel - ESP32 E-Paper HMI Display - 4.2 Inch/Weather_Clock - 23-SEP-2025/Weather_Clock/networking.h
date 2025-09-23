// networking.h - Networking utility functions for ESP32 Weather Clock
#pragma once
#include <WiFi.h>
#include "config.h"

// Ensures WiFi is connected, attempts to reconnect if not
inline void ensureWiFiConnected() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost. Attempting to reconnect...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        unsigned long wifiReconnectStart = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - wifiReconnectStart < 10000) {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi reconnected.");
        } else {
            Serial.println("\nWiFi reconnection failed.");
        }
    }
}
