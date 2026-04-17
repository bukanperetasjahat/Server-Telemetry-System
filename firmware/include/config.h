#ifndef CONFIG_H
#define CONFIG_H

// WiFi
#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASS "YOUR_PASS"

// Server
#define SERVER_URL "http://YOUR_SERVER_IP:8000/metrics.json"

// MQTT
#define MQTT_HOST "YOUR_MQTT_IP"
#define MQTT_PORT 1883

#define TOPIC_TELEMETRY "server/telemetry"
#define TOPIC_ALERT "server/alert"

#endif
