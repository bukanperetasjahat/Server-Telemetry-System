#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "metrics_fetcher.h"

bool fetchMetrics(Metrics &out) {
    HTTPClient http;

    http.begin(SERVER_URL);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, payload);

        if (!err) {
            out.cpu = doc["cpu"];
            out.memory = doc["memory"];

            Serial.printf("[FETCH] CPU: %.2f | MEM: %.2f\n", out.cpu, out.memory);

            http.end();
            return true;
        }
    }

    Serial.println("[FETCH] Failed");
    http.end();
    return false;
}
