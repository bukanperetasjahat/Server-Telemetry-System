#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ===================== CONFIG =====================
const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASS = "YOUR_PASSWORD";

const char* SERVER_URL = "http://YOUR_SERVER_IP:8000/metrics.json";

const char* MQTT_HOST = "YOUR_MQTT_IP";
const int   MQTT_PORT = 1883;

const char* TOPIC_TELEMETRY = "server/telemetry";
const char* TOPIC_ALERT     = "server/alert";

// ===================== GLOBALS =====================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

QueueHandle_t metricsQueue;

// ===================== DATA STRUCT =====================
typedef struct {
    float cpu;
    float memory;
} Metrics;

// ===================== WIFI =====================
void connectWiFi() {
    Serial.println("[WiFi] Connecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("\n[WiFi] Connected!");
}

// ===================== MQTT =====================
void connectMQTT() {
    while (!mqttClient.connected()) {
        Serial.println("[MQTT] Connecting...");
        if (mqttClient.connect("esp32-client")) {
            Serial.println("[MQTT] Connected!");
        } else {
            Serial.print("[MQTT] Failed, rc=");
            Serial.println(mqttClient.state());
            delay(2000);
        }
    }
}

// ===================== TASK: FETCH METRICS =====================
void taskFetch(void *pvParameters) {
    HTTPClient http;

    while (true) {
        if (WiFi.status() == WL_CONNECTED) {

            http.begin(SERVER_URL);
            int httpCode = http.GET();

            if (httpCode == HTTP_CODE_OK) {
                String payload = http.getString();

                StaticJsonDocument<256> doc;
                DeserializationError err = deserializeJson(doc, payload);

                if (!err) {
                    Metrics m;
                    m.cpu = doc["cpu"];
                    m.memory = doc["memory"];

                    xQueueSend(metricsQueue, &m, portMAX_DELAY);

                    Serial.printf("[FETCH] CPU: %.2f | MEM: %.2f\n", m.cpu, m.memory);
                } else {
                    Serial.println("[FETCH] JSON parse error");
                }
            } else {
                Serial.printf("[FETCH] HTTP error: %d\n", httpCode);
            }

            http.end();
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ===================== TASK: PROCESS + MQTT =====================
void taskProcess(void *pvParameters) {
    Metrics m;

    while (true) {
        if (xQueueReceive(metricsQueue, &m, portMAX_DELAY) == pdTRUE) {

            if (!mqttClient.connected()) {
                connectMQTT();
            }

            // ---- TELEMETRY ----
            StaticJsonDocument<128> doc;
            doc["cpu"] = m.cpu;
            doc["memory"] = m.memory;

            char buffer[128];
            serializeJson(doc, buffer);

            mqttClient.publish(TOPIC_TELEMETRY, buffer);

            Serial.println("[MQTT] Telemetry sent");

            // ---- ALERT ----
            if (m.cpu > 80 || m.memory > 80) {
                StaticJsonDocument<128> alertDoc;
                alertDoc["cpu"] = m.cpu;
                alertDoc["memory"] = m.memory;
                alertDoc["status"] = "HIGH_USAGE";

                char alertBuffer[128];
                serializeJson(alertDoc, alertBuffer);

                mqttClient.publish(TOPIC_ALERT, alertBuffer);

                Serial.println("[ALERT] High usage detected!");
            }
        }
    }
}

// ===================== TASK: MQTT LOOP =====================
void taskMQTT(void *pvParameters) {
    while (true) {
        if (!mqttClient.connected()) {
            connectMQTT();
        }

        mqttClient.loop();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ===================== TASK: WATCHDOG =====================
void taskWatchdog(void *pvParameters) {
    while (true) {

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WATCHDOG] WiFi disconnected. Reconnecting...");
            connectWiFi();
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// ===================== SETUP =====================
void setup() {
    Serial.begin(115200);

    connectWiFi();

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    // Create queue (store 10 metrics)
    metricsQueue = xQueueCreate(10, sizeof(Metrics));

    if (metricsQueue == NULL) {
        Serial.println("[ERROR] Failed to create queue");
        while (true);
    }

    // Create tasks
    xTaskCreatePinnedToCore(taskFetch, "FetchTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskProcess, "ProcessTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskMQTT, "MQTTTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskWatchdog, "WatchdogTask", 2048, NULL, 1, NULL, 0);

    Serial.println("[SYSTEM] Started");
}

// ===================== LOOP =====================
void loop() {
    // Not used (FreeRTOS handles scheduling)
}
