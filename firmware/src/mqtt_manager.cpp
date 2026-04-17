#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "mqtt_manager.h"
#include "types.h"

void connectMQTT(PubSubClient &client) {
    while (!client.connected()) {
        Serial.println("[MQTT] Connecting...");

        if (client.connect("esp32-client")) {
            Serial.println("[MQTT] Connected");
        } else {
            Serial.print("[MQTT] Failed: ");
            Serial.println(client.state());
            delay(2000);
        }
    }
}

void publishTelemetry(PubSubClient &client, const Metrics &m) {
    StaticJsonDocument<128> doc;
    doc["cpu"] = m.cpu;
    doc["memory"] = m.memory;

    char buffer[128];
    serializeJson(doc, buffer);

    client.publish(TOPIC_TELEMETRY, buffer);
}

void publishAlert(PubSubClient &client, const Metrics &m) {
    StaticJsonDocument<128> doc;
    doc["cpu"] = m.cpu;
    doc["memory"] = m.memory;
    doc["status"] = "HIGH_USAGE";

    char buffer[128];
    serializeJson(doc, buffer);

    client.publish(TOPIC_ALERT, buffer);

    Serial.println("[ALERT] High usage triggered");
}
