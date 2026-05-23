#!/usr/bin/env python3
"""
MQTT-listener для AlphaBot.

Подключается к брокеру mosquitto на ноуте, подписывается на топик 'sky/state'.
  cloudy  -> крутится ЛЕВОЕ колесо (генератор работает)
  clear   -> колесо останавливается (солнечно, генератор не нужен)
"""

import paho.mqtt.client as mqtt
import RPi.GPIO as GPIO

# === КОНФИГ ===
# IP ноутбука в WiFi-сети. ВАЖНО: подставь свой IP!
# Узнать на ноуте: ip addr show wlan0 | grep "inet "
BROKER_HOST = "192.168.1.107"   # <-- ЗАМЕНИТЬ
BROKER_PORT = 1883
TOPIC       = "sky/state"

# Пины для левого мотора (мотор A):
# IN1, IN2 — направление вращения
# ENA      — включение питания мотора
LEFT_IN1 = 12
LEFT_IN2 = 13
LEFT_ENA = 6


def setup_gpio():
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)
    for pin in (LEFT_IN1, LEFT_IN2, LEFT_ENA):
        GPIO.setup(pin, GPIO.OUT)
    # Питание мотора всегда подаём
    GPIO.output(LEFT_ENA, GPIO.HIGH)
    # На старте мотор выключен
    stop_wheel()


def start_wheel():
    """Крутим левое колесо вперёд."""
    GPIO.output(LEFT_IN1, GPIO.HIGH)
    GPIO.output(LEFT_IN2, GPIO.LOW)


def stop_wheel():
    """Останавливаем колесо."""
    GPIO.output(LEFT_IN1, GPIO.LOW)
    GPIO.output(LEFT_IN2, GPIO.LOW)


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[AlphaBot] Connected to broker {BROKER_HOST}:{BROKER_PORT}")
        client.subscribe(TOPIC)
        print(f"[AlphaBot] Subscribed to '{TOPIC}'")
    else:
        print(f"[AlphaBot] Connection failed with code {rc}")


def on_message(client, userdata, msg):
    payload = msg.payload.decode("utf-8").strip()
    print(f"[AlphaBot] Got message on '{msg.topic}': {payload}")

    if payload == "cloudy":
        print("[AlphaBot] >>> GENERATOR ON  (wheel spinning)")
        start_wheel()
    elif payload == "clear":
        print("[AlphaBot] >>> GENERATOR OFF (wheel stopped)")
        stop_wheel()
    else:
        print(f"[AlphaBot] Unknown payload, ignoring")


def main():
    setup_gpio()

    client = mqtt.Client(client_id="alphabot_generator")
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(BROKER_HOST, BROKER_PORT, 60)
    except Exception as e:
        print(f"[AlphaBot] Failed to connect: {e}")
        print(f"[AlphaBot] Check that broker is running on {BROKER_HOST}:{BROKER_PORT}")
        GPIO.cleanup()
        return

    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n[AlphaBot] Stopping...")
    finally:
        stop_wheel()
        GPIO.cleanup()
        client.disconnect()
        print("[AlphaBot] Bye")


if __name__ == "__main__":
    main()