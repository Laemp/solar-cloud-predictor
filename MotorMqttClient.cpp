#include "MotorMqttClient.hpp"

#include <cstring>
#include <iostream>

MotorMqttClient::MotorMqttClient(const std::string& host,
                                 int port,
                                 const std::string& client_id,
                                 const std::string& topic)
    : host_(host), port_(port), client_id_(client_id), topic_(topic)
{
    // Инициализируем библиотеку (можно вызвать несколько раз, безопасно)
    mosquitto_lib_init();

    // Создаём клиента.
    // 3-й аргумент — наш userdata (this). libmosquitto будет передавать
    // его в callback'и, чтобы мы знали к какому объекту это относится.
    bool clean_session = true;
    mosq_ = mosquitto_new(client_id_.c_str(), clean_session, this);

    if (!mosq_) {
        std::cerr << "[MotorMqttClient] Failed to create mosquitto instance\n";
        return;
    }

    // Регистрируем callback'и
    mosquitto_connect_callback_set(mosq_, on_connect_static);
    mosquitto_message_callback_set(mosq_, on_message_static);
}

MotorMqttClient::~MotorMqttClient() {
    if (mosq_) {
        mosquitto_disconnect(mosq_);
        mosquitto_destroy(mosq_);
        mosq_ = nullptr;
    }
    mosquitto_lib_cleanup();
}

bool MotorMqttClient::connect() {
    if (!mosq_) return false;

    // 4-й аргумент — keepalive в секундах. Если связь теряется,
    // через это время брокер поймёт что клиент отвалился.
    int rc = mosquitto_connect(mosq_, host_.c_str(), port_, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "[MotorMqttClient] Connect failed: "
                  << mosquitto_strerror(rc) << "\n";
        return false;
    }
    return true;
}

void MotorMqttClient::run() {
    if (!mosq_) return;

    std::cout << "[MotorMqttClient] Listening for messages on topic '"
              << topic_ << "'... (Ctrl+C to stop)\n";

    // Бесконечный цикл: обрабатывает сетевой ввод/вывод и вызывает callback'и.
    // -1 — без таймаута, 1 — обрабатывать максимум 1 пакет за итерацию.
    // При обрыве соединения сам пытается переподключиться.
    int rc = mosquitto_loop_forever(mosq_, -1, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "[MotorMqttClient] Loop ended with error: "
                  << mosquitto_strerror(rc) << "\n";
    }
}

// --- Статические переходники: достают this из userdata и зовут метод объекта ---

void MotorMqttClient::on_connect_static(struct mosquitto* /*mosq*/,
                                        void* userdata, int rc) {
    auto* self = static_cast<MotorMqttClient*>(userdata);
    self->on_connect(rc);
}

void MotorMqttClient::on_message_static(struct mosquitto* /*mosq*/,
                                        void* userdata,
                                        const struct mosquitto_message* msg) {
    auto* self = static_cast<MotorMqttClient*>(userdata);
    self->on_message(msg);
}

// --- Реальная логика обработчиков ---

void MotorMqttClient::on_connect(int rc) {
    if (rc != 0) {
        std::cerr << "[MotorMqttClient] Connect failed (rc=" << rc << ")\n";
        return;
    }
    std::cout << "[MotorMqttClient] Connected to broker " << host_ << ":" << port_ << "\n";

    // Подписываемся на топик. QoS=0 (at most once) — для нашего случая хватает.
    int sub_rc = mosquitto_subscribe(mosq_, nullptr, topic_.c_str(), 0);
    if (sub_rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "[MotorMqttClient] Subscribe failed: "
                  << mosquitto_strerror(sub_rc) << "\n";
    } else {
        std::cout << "[MotorMqttClient] Subscribed to '" << topic_ << "'\n";
    }
}

void MotorMqttClient::on_message(const struct mosquitto_message* msg) {
    if (!msg || !msg->payload || msg->payloadlen <= 0) {
        return;
    }

    // payload — это просто байты, превращаем в строку C++
    std::string payload(static_cast<const char*>(msg->payload),
                        static_cast<size_t>(msg->payloadlen));

    std::cout << "[MotorMqttClient] Got message on '" << msg->topic
              << "': " << payload << "\n";

    if (payload == "cloudy") {
        turn_motor_on();
    } else if (payload == "clear") {
        turn_motor_off();
    } else {
        std::cerr << "[MotorMqttClient] Unknown payload, ignoring\n";
    }
}

void MotorMqttClient::turn_motor_on() {
    if (motor_on_) {
        std::cout << "[Motor] already ON, nothing to do\n";
        return;
    }
    motor_on_ = true;
    std::cout << "[Motor] >>> MOTOR ON  (clouds detected, retracting panels)\n";
}

void MotorMqttClient::turn_motor_off() {
    if (!motor_on_) {
        std::cout << "[Motor] already OFF, nothing to do\n";
        return;
    }
    motor_on_ = false;
    std::cout << "[Motor] >>> MOTOR OFF (clear sky, panels stay deployed)\n";
}
