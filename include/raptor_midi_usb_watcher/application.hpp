#pragma once

#include "raptor_midi_usb_watcher/config.hpp"

namespace raptor::midi_usb_watcher {

class Application {
public:
    explicit Application(ServiceConfig config);
    int run();

private:
    ServiceConfig config_;
};

}  // namespace raptor::midi_usb_watcher
