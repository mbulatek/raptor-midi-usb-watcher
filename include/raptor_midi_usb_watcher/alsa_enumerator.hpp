#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace raptor::midi_usb_watcher {

struct AlsaPort {
    int client_id {0};
    int port_id {0};
    std::string client_name;
    std::string port_name;
    std::string full_name;  // client + ":" + port
};

class AlsaEnumerator {
public:
    std::vector<AlsaPort> scan() const;
};

}  // namespace raptor::midi_usb_watcher
