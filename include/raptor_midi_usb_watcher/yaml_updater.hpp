#pragma once

#include <string>
#include <vector>

namespace raptor::midi_usb_watcher {

struct UsbControllerState {
    std::string id;
    std::string match_name;
    bool enabled {false};
};

bool update_midi_io_config_usb_controllers(
    const std::string& midi_io_config_path,
    const std::vector<UsbControllerState>& controllers);

}  // namespace raptor::midi_usb_watcher
