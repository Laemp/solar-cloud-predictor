"""
Тестовый клиент для сервера управления двигателем солнечных панелей.

Симулирует две роли:
  camera() — программа распознавания: шлёт состояние неба ("cloudy" / "clear").
  motor()  — двигатель: периодически спрашивает должен ли он работать.

Запускать: python client.py [cloudy|clear|motor|state]
"""

import sys
import time
import requests

URL = "http://localhost:8080"


def camera(sky: str):
    """Камера сообщает что видит."""
    data = {"sky": sky}
    try:
        resp = requests.post(f"{URL}/sky", json=data, timeout=5)
    except requests.exceptions.RequestException as e:
        print(f"[camera] Connection failed: {e}")
        return

    if resp.status_code == 200:
        print(f"[camera] sent sky={sky} -> {resp.json()}")
    else:
        print(f"[camera] FAIL ({resp.status_code}): {resp.text}")


def motor():
    """Симуляция двигателя: раз в 2 сек опрашиваем сервер."""
    print("[motor] Polling server every 2s. Ctrl+C to stop.")
    last_state = None
    try:
        while True:
            try:
                resp = requests.get(f"{URL}/motor", timeout=5)
                if resp.status_code == 200:
                    m = resp.json().get("motor", "?")
                    if m != last_state:
                        # печатаем только при изменении, чтобы не спамить
                        print(f"[motor] -> {m.upper()}")
                        last_state = m
                else:
                    print(f"[motor] HTTP {resp.status_code}: {resp.text}")
            except requests.exceptions.RequestException as e:
                print(f"[motor] Connection failed: {e}")
            time.sleep(2)
    except KeyboardInterrupt:
        print("\n[motor] Stopped.")


def state():
    """Просто посмотреть полное состояние системы."""
    try:
        resp = requests.get(f"{URL}/state", timeout=5)
        print(f"[state] {resp.status_code}: {resp.json()}")
    except requests.exceptions.RequestException as e:
        print(f"[state] Connection failed: {e}")


if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "state"

    if mode == "cloudy":
        camera("cloudy")
    elif mode == "clear":
        camera("clear")
    elif mode == "motor":
        motor()
    elif mode == "state":
        state()
    else:
        print(f"Unknown mode: {mode}")
        print("Usage: python client.py [cloudy|clear|motor|state]")