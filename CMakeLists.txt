cmake_minimum_required(VERSION 3.15)
project(Robot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- httplib через FetchContent (нет в основных репах Arch, лежит в AUR) ---
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(httplib)

# --- nlohmann_json и libmosquitto из системы ---
# sudo pacman -S nlohmann-json mosquitto
find_package(nlohmann_json REQUIRED)
find_library(MOSQUITTO_LIB mosquitto REQUIRED)

# --- Таргет 1: REST-сервер ---
add_executable(myexample main.cpp)
target_link_libraries(myexample PRIVATE httplib)
target_link_libraries(myexample PRIVATE nlohmann_json::nlohmann_json)

# --- Таргет 2: MQTT-клиент двигателя ---
add_executable(motor
    motor.cpp
    MotorMqttClient.cpp
)
target_link_libraries(motor PRIVATE ${MOSQUITTO_LIB})
