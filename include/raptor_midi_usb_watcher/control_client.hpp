#pragma once

#include <optional>
#include <string>

namespace raptor::midi_usb_watcher {

struct ControlReply {
    bool ok {false};
    std::string service;
    std::string command;
    std::string error_message;
};

class ControlClient {
public:
    explicit ControlClient(std::string endpoint);
    std::optional<ControlReply> request_reload(const std::string& command, const std::string& request_id) const;

private:
    std::string endpoint_;
};

}  // namespace raptor::midi_usb_watcher
