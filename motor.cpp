#include "MotorMqttClient.hpp"

#include <iostream>

int main() {
    // Адрес локального брокера mosquitto и наш клиент
    MotorMqttClient motor(
        /*host=*/      "localhost",
        /*port=*/      1883,
        /*client_id=*/ "solar_motor_client",
        /*topic=*/     "sky/state"
    );

    if (!motor.connect()) {
        std::cerr << "Failed to connect to broker. Is mosquitto running?\n";
        return 1;
    }
    
    motor.run();
    return 0;
}
