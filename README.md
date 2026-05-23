# Solar Cloud Predictor

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
| `motor.cpp` | Ноут | Точка входа C++ MQTT-клиента |
| `MotorMqttClient.hpp/.cpp` | Ноут | C++ класс — обёртка над libmosquitto |
| `mosquitto.conf` | Ноут | Конфиг брокера (порт 1883, anonymous) |
| `alphabot_listener.py` | Робот | Listener: крутит колесо по командам из MQTT |
| `CMakeLists.txt` | Ноут | Сборка C++ части |

## Установка зависимостей

### На ноуте (Arch Linux)

```bash
sudo pacman -S cmake gcc mosquitto

python -m venv .venv
source .venv/bin/activate
pip install paho-mqtt opencv-python numpy imutils
```

## Сборка C++ части

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
cd ..
```

## Подготовка робота

1. Подключись к WiFi робота.
2. Узнать свой IP на ноуте:
   ```bash
   ip addr show wlan0 | grep "inet "
   ```
3. Закинуть файл на робота:
   ```bash
   scp alphabot_listener.py user@192.168.1.XXX:~/
   ```
5. Подправить IP на роботе:
   ```bash
   ssh user@192.168.1.XXX
   ```

## Запуск

Если брокер уже запущен через systemd, остановить его:

```bash
sudo systemctl stop mosquitto
```

Открываем **4 терминала** и запускаем по порядку:

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
ssh user@192.168.1.XXX
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
