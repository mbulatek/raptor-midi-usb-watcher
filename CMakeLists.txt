cmake_minimum_required(VERSION 3.22)
project(raptor_midi_usb_watcher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(RAPTOR_MIDI_USB_WATCHER_INSTALL_SYSTEMD_UNIT "Install the systemd service file" ON)
option(RAPTOR_MIDI_USB_WATCHER_INSTALL_DEFAULT_CONFIG "Install the default YAML configuration file" ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(ZMQ REQUIRED libzmq)

find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

add_executable(raptor-midi-usb-watcher
  src/main.cpp
  src/application.cpp
  src/config.cpp
  src/alsa_enumerator.cpp
  src/control_client.cpp
  src/yaml_updater.cpp
)

target_include_directories(raptor-midi-usb-watcher PRIVATE
  ${ALSA_INCLUDE_DIRS}
  ${ZMQ_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(raptor-midi-usb-watcher PRIVATE
  -Wall -Wextra -Wpedantic
)

target_link_libraries(raptor-midi-usb-watcher PRIVATE
  ${ALSA_LIBRARIES}
  ${ZMQ_LIBRARIES}
  yaml-cpp::yaml-cpp
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)

install(TARGETS raptor-midi-usb-watcher RUNTIME DESTINATION bin)

if (RAPTOR_MIDI_USB_WATCHER_INSTALL_DEFAULT_CONFIG)
  install(FILES config/raptor-midi-usb-watcher.example.yaml DESTINATION /etc/raptor-midi-usb-watcher RENAME raptor-midi-usb-watcher.yaml)
endif()

if (RAPTOR_MIDI_USB_WATCHER_INSTALL_SYSTEMD_UNIT)
  install(FILES packaging/raptor-midi-usb-watcher.service DESTINATION /usr/lib/systemd/system)
endif()
