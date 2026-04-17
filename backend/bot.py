import os
import json
import time
import paho.mqtt.client as mqtt
import requests

MQTT_HOST = os.getenv("MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))

BOT_TOKEN = os.getenv("BOT_TOKEN")
CHAT_ID = os.getenv("CHAT_ID")

TOPIC_ALERT = "server/alert"

def send_telegram(message):
    url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"
    try:
        requests.post(url, json={
            "chat_id": CHAT_ID,
            "text": message
        })
    except Exception as e:
        print("Telegram error:", e)

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT:", rc)
    client.subscribe(TOPIC_ALERT)

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())

        cpu = data.get("cpu")
        mem = data.get("memory")

        message = f"🚨 ALERT!\nCPU: {cpu}%\nMemory: {mem}%"

        print(message)
        send_telegram(message)

    except Exception as e:
        print("Error:", e)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

while True:
    try:
        client.connect(MQTT_HOST, MQTT_PORT, 60)
        client.loop_forever()
    except Exception as e:
        print("MQTT reconnect...", e)
        time.sleep(5)
