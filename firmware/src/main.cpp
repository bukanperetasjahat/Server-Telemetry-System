#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "metrics_fetcher.h"
#include "processor.h"
#include "types.h"

// ===================== GLOBAL =====================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

QueueHandle_t metricsQueue;

// ===================== TASK: FETCH =====================
void taskFetch(void *pvParameters) {
    Metrics m;

    while (true) {
        if (fetchMetrics(m)) {
            xQueueSend(metricsQueue, &m, portMAX_DELAY);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ===================== TASK: PROCESS =====================
void taskProcess(void *pvParameters) {
    Metrics m;

    while (true) {
        if (xQueueReceive(metricsQueue, &m, portMAX_DELAY)) {

            if (!mqttClient.connected()) {
                connectMQTT(mqttClient);
            }

            // Telemetry
            publishTelemetry(mqttClient, m);

            // Alert
            if (isAlert(m)) {
                publishAlert(mqttClient, m);
            }
        }
    }
}

// ===================== TASK: MQTT LOOP =====================
void taskMQTT(void *pvParameters) {
    while (true) {
        if (!mqttClient.connected()) {
            connectMQTT(mqttClient);
        }

        mqttClient.loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ===================== TASK: WATCHDOG =====================
void taskWatchdog(void *pvParameters) {
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
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

    metricsQueue = xQueueCreate(10, sizeof(Metrics));

    xTaskCreatePinnedToCore(taskFetch, "FetchTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskProcess, "ProcessTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskMQTT, "MQTTTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskWatchdog, "WatchdogTask", 2048, NULL, 1, NULL, 0);

    Serial.println("[SYSTEM] Started");
}

void loop() {}
