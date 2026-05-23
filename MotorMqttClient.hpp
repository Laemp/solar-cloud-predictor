#pragma once

#include <string>
#include <mosquitto.h>

// Класс для управления двигателем солнечных панелей через MQTT.
// Подписывается на топик sky/state и переключает состояние двигателя
// в зависимости от того что прислала камера: cloudy -> motor on, clear -> motor off.
//
// Под капотом обёртка над сишной libmosquitto:
// libmosquitto использует C-стиль callback'ов (указатели на статические функции),
// поэтому мы регистрируем static методы класса как callback'и, а внутри них
// достаём указатель на конкретный объект через void* userdata.
class MotorMqttClient {
public:
    // host — адрес брокера (обычно "localhost")
    // port — порт (по умолчанию у mosquitto 1883)
    // client_id — имя нашего клиента в брокере (просто для логов)
    // topic — на какой топик подписаться
    MotorMqttClient(const std::string& host,
                    int port,
                    const std::string& client_id,
                    const std::string& topic);

    ~MotorMqttClient();

    // Запретим копирование — у нас внутри сишный указатель,
    // если его скопировать и удалить дважды — программа упадёт.
    MotorMqttClient(const MotorMqttClient&) = delete;
    MotorMqttClient& operator=(const MotorMqttClient&) = delete;

    // Подключиться к брокеру и подписаться на топик.
    // Возвращает true если успешно.
    bool connect();

    // Запустить бесконечный цикл обработки сообщений.
    // Внутри loop_forever() от libmosquitto — он сам обрабатывает входящие
    // сообщения и вызывает наши callback'и. Блокирующий вызов.
    void run();

    // Текущее состояние двигателя (для отладки/тестов)
    bool is_motor_on() const { return motor_on_; }

private:
    // Сишные callback'и должны быть статическими.
    // libmosquitto передаёт в них наш userdata (this), и мы делегируем
    // вызов на нестатический метод конкретного объекта.
    static void on_connect_static(struct mosquitto* mosq, void* userdata, int rc);
    static void on_message_static(struct mosquitto* mosq, void* userdata,
                                  const struct mosquitto_message* msg);

    // Обработчики уровня объекта — вся реальная логика тут
    void on_connect(int rc);
    void on_message(const struct mosquitto_message* msg);

    // "Бизнес-логика" двигателя
    void turn_motor_on();
    void turn_motor_off();

    std::string         host_;
    int                 port_;
    std::string         client_id_;
    std::string         topic_;
    struct mosquitto*   mosq_       = nullptr;
    bool                motor_on_   = false;  // текущее состояние двигателя
};
