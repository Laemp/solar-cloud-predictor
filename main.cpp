#include <iostream>
#include <mutex>
#include <string>
#include <chrono>
#include "httplib.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Состояние неба, которое присылает программа распознавания
enum class SkyState {
    UNKNOWN,    // ещё не получали данных
    CLOUDY,     // облака
    CLEAR       // ясно
};

// Состояние двигателя
enum class MotorState {
    OFF,        // выключен (по умолчанию пока не пришли данные)
    ON          // включён
};

// Глобальное состояние системы
struct SystemState {
    SkyState    sky          = SkyState::UNKNOWN;
    MotorState  motor        = MotorState::OFF;
    long long   last_update  = 0;
};

SystemState  state;
std::mutex   state_mutex;   // блокировка для безопасной работы из нескольких потоков

const char* sky_to_str(SkyState s) {
    switch (s) {
        case SkyState::CLOUDY:  return "cloudy";
        case SkyState::CLEAR:   return "clear";
        default:                return "unknown";
    }
}

const char* motor_to_str(MotorState m) {
    return m == MotorState::ON ? "on" : "off";
}

// POST /sky — программа распознавания (камера) сообщает что видит небо.
// Ожидаемый JSON: {"sky": "cloudy"} или {"sky": "clear"}
void post_sky(const httplib::Request& req, httplib::Response& res) {
    if (req.get_header_value("Content-Type") != "application/json") {
        res.status = 415;
        res.set_content("Expected Content-Type: application/json", "text/plain");
        return;
    }

    try {
        json req_json = json::parse(req.body);
        std::string sky_str = req_json.value("sky", "");

        SkyState   new_sky;
        MotorState new_motor;

        if (sky_str == "cloudy") {
            // Облака -> двигатель включаем
            new_sky   = SkyState::CLOUDY;
            new_motor = MotorState::ON;
        } else if (sky_str == "clear") {
            // Ясно -> двигатель выключаем
            new_sky   = SkyState::CLEAR;
            new_motor = MotorState::OFF;
        } else {
            res.status = 400;
            json err;
            err["status"]  = "error";
            err["message"] = "Field 'sky' must be 'cloudy' or 'clear', got: '" + sky_str + "'";
            res.set_content(err.dump(), "application/json");
            return;
        }

        long long ts = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        {
            std::lock_guard<std::mutex> lock(state_mutex);
            state.sky         = new_sky;
            state.motor       = new_motor;
            state.last_update = ts;
        }

        std::cout << "[POST /sky] sky=" << sky_to_str(new_sky)
                  << " -> motor=" << motor_to_str(new_motor) << "\n";

        json resp_json;
        resp_json["status"] = "success";
        resp_json["sky"]    = sky_to_str(new_sky);
        resp_json["motor"]  = motor_to_str(new_motor);
        res.set_content(resp_json.dump(), "application/json");
        res.status = 200;

    } catch (const json::parse_error& e) {
        res.status = 400;
        res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
    }
}

// GET /motor — двигатель спрашивает: я должен быть включён или выключен?
void get_motor(const httplib::Request& /*req*/, httplib::Response& res) {
    MotorState m;
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        m = state.motor;
    }
    json resp_json;
    resp_json["motor"] = motor_to_str(m);
    res.set_content(resp_json.dump(), "application/json");
    res.status = 200;
}

// GET /state — посмотреть полное состояние системы (для отладки)
void get_state(const httplib::Request& /*req*/, httplib::Response& res) {
    json resp_json;
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        resp_json["sky"]         = sky_to_str(state.sky);
        resp_json["motor"]       = motor_to_str(state.motor);
        resp_json["last_update"] = state.last_update;
    }
    resp_json["status"] = "alive";
    res.set_content(resp_json.dump(), "application/json");
    res.status = 200;
}

int main() {
    httplib::Server svr;

    svr.Post("/sky",   post_sky);
    svr.Get ("/motor", get_motor);
    svr.Get ("/state", get_state);

    std::cout << "Solar panel motor server on http://localhost:8080\n";
    std::cout << "Endpoints:\n";
    std::cout << "  POST /sky    - camera reports sky condition (cloudy|clear)\n";
    std::cout << "  GET  /motor  - motor asks if it should run (on|off)\n";
    std::cout << "  GET  /state  - full system state\n\n";
    std::cout << "Test with:\n";
    std::cout << "  curl -X POST -H \"Content-Type: application/json\" \\\n"
              << "       -d '{\"sky\":\"cloudy\"}' http://localhost:8080/sky\n";

    svr.listen("0.0.0.0", 8080);
    return 0;
}