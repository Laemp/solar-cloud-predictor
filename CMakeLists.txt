cmake_minimum_required(VERSION 3.15)
project(SolarMotor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# libmosquitto из системы: sudo pacman -S mosquitto
find_library(MOSQUITTO_LIB mosquitto REQUIRED)

# MQTT-клиент двигателя (подписан на топик sky/state)
add_executable(motor
    motor.cpp
    MotorMqttClient.cpp
)
target_link_libraries(motor PRIVATE ${MOSQUITTO_LIB})