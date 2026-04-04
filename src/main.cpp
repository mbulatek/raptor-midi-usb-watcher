#include "raptor_midi_usb_watcher/application.hpp"
#include "raptor_midi_usb_watcher/config.hpp"

#include <cstdlib>
#include <exception>
#include <string>

#include <spdlog/spdlog.h>

namespace {

spdlog::level::level_enum level_from_string(const std::string& s) {
    if (s == "trace") return spdlog::level::trace;
    if (s == "debug") return spdlog::level::debug;
    if (s == "info") return spdlog::level::info;
    if (s == "warn" || s == "warning") return spdlog::level::warn;
    if (s == "err" || s == "error") return spdlog::level::err;
    if (s == "critical") return spdlog::level::critical;
    if (s == "off") return spdlog::level::off;
    return spdlog::level::info;
}

}  // namespace

int main(int argc, char** argv) {
    std::string config_path = "/etc/raptor-midi-usb-watcher/raptor-midi-usb-watcher.yaml";
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        }
    }

    try {
        auto config = raptor::midi_usb_watcher::load_config(config_path);
        spdlog::set_level(level_from_string(config.logging.level));
        raptor::midi_usb_watcher::Application app {std::move(config)};
        return app.run();
    } catch (const std::exception& ex) {
        spdlog::error("fatal: {}", ex.what());
        return 1;
    }
}
