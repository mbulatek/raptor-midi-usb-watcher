#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace raptor::midi_usb_watcher {

struct LoggingConfig {
    std::string level {"info"};
};

struct ScanConfig {
    std::uint32_t period_ms {1000};
    std::uint32_t stable_cycles {2};
};

struct MidiIoConfig {
    std::string config_path {"/etc/raptor-midi-io-service/raptor-midi-io-service.yaml"};
    std::string control_endpoint {"ipc:///run/raptor/midi-io-control.sock"};
    std::string reload_command {"reload-config"};
};

struct ControllerRule {
    std::string id;
    std::string match_name;
};

struct ServiceConfig {
    std::string config_path;
    LoggingConfig logging;
    ScanConfig scan;
    MidiIoConfig midi_io;
    std::vector<ControllerRule> controllers;
};

ServiceConfig load_config(const std::string& path);

}  // namespace raptor::midi_usb_watcher
