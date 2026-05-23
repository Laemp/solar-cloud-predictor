# Solar Cloud Predictor

Лабораторная работа №4 по дисциплине "Архитектура информационных систем" (МАИ).

Камера через OpenCV распознаёт состояние неба и через MQTT управляет роботом-генератором AlphaBot: когда видит облако — робот крутит колесо (имитация запуска резервного генератора), когда видит ясное небо — колесо останавливается (работают солнечные панели).

## Архитектура

```
   camera.py                                  alphabot_listener.py
   (на ноуте)                                 (на роботе AlphaBot)
       │                                              ▲
       │   publish "cloudy"/"clear"                   │ subscribe
       │   в топик "sky/state"                        │
       ▼                                              │
   ┌─────────────────────────────────────────────────┴──┐
   │             MQTT-брокер mosquitto                  │
   │              (на ноуте, порт 1883)                 │
   └────────────────────┬───────────────────────────────┘
                        │
                        │ subscribe (на том же ноуте)
                        ▼
                  C++ MQTT-клиент
                       motor
                  (логирует события)
```

## Файлы

| Файл | Где запускается | Что делает |
|---|---|---|
| `camera.py` | Ноут | OpenCV анализирует видео и публикует `cloudy`/`clear` в MQTT |
| `motor.cpp` | Ноут (бинарник `motor`) | Точка входа C++ MQTT-клиента |
| `MotorMqttClient.hpp/.cpp` | Ноут | C++ класс — обёртка над libmosquitto |
| `mosquitto.conf` | Ноут | Конфиг брокера (порт 1883, anonymous) |
| `alphabot_listener.py` | Робот AlphaBot | Listener: крутит колесо по командам из MQTT |
| `CMakeLists.txt` | Ноут | Сборка C++ части |

## Установка зависимостей

### На ноуте (Arch Linux)

```bash
sudo pacman -S cmake gcc mosquitto

python -m venv .venv
source .venv/bin/activate
pip install paho-mqtt opencv-python numpy imutils
```

### На роботе AlphaBot (Raspbian)

```bash
pip3 install paho-mqtt
# RPi.GPIO обычно уже установлен
```

## Сборка C++ части

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
cd ..
```

В `build/` появится бинарник `motor`.

## Подготовка робота

1. Подключись к WiFi `TP-Link_846C` (пароль `21928338`).
2. Узнай свой IP на ноуте:
   ```bash
   ip addr show wlan0 | grep "inet "
   ```
3. Закинь файл на робота:
   ```bash
   scp alphabot_listener.py user@192.168.1.100:~/
   ```
4. Подправь IP на роботе:
   ```bash
   ssh user@192.168.1.100
   sed -i 's/192.168.1.107/192.168.1.<твой_IP>/' alphabot_listener.py
   ```

## Запуск

**ВАЖНО:** перед запуском выключи VPN — он перехватывает локальный трафик и ломает MQTT.

Если брокер уже запущен через systemd, останови его (иначе порт занят):

```bash
sudo systemctl stop mosquitto
```

Открой **4 терминала** и запускай по порядку:

### Терминал 1 — брокер (на ноуте)
```bash
mosquitto -v -c mosquitto.conf
```

### Терминал 2 — C++ клиент (на ноуте)
```bash
cd build
./motor
```

### Терминал 3 — робот (через SSH)
```bash
ssh user@192.168.1.100
sudo python3 alphabot_listener.py
```

### Терминал 4 — камера (на ноуте)
```bash
source .venv/bin/activate
python camera.py
```

## Что должно произойти

Когда камера видит облако:
- В терминале 4: `cloudy -> published to MQTT`
- В терминале 2: `>>> MOTOR ON`
- В терминале 3: `>>> GENERATOR ON (wheel spinning)`
- Колесо AlphaBot крутится

Когда видит ясное небо — наоборот, всё переключается в OFF, колесо останавливается.

## Возможные проблемы

| Проблема | Решение |
|---|---|
| `Address already in use` (брокер) | `sudo systemctl stop mosquitto` |
| MQTT не работает | Выключи VPN/Tun Mode |
| `Connection refused` при SSH на робота | Проверь WiFi и `ping 192.168.1.100` |
| Колесо не крутится | Запускай `alphabot_listener.py` через `sudo` |
| `nano: Error xterm-kitty` через SSH | Используй `TERM=xterm nano <файл>` |