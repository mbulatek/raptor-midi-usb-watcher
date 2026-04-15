#include "raptor_midi_usb_watcher/config.hpp"

#include <stdexcept>

#include <yaml-cpp/yaml.h>

namespace raptor::midi_usb_watcher {
namespace {

std::string scalar_or(const YAML::Node& node, const char* key, const std::string& fallback = {}) {
    if (node && node[key] && node[key].IsScalar()) {
        return node[key].as<std::string>();
    }
    return fallback;
}

std::uint32_t u32_or(const YAML::Node& node, const char* key, const std::uint32_t fallback) {
    if (node && node[key] && node[key].IsScalar()) {
        return node[key].as<std::uint32_t>();
    }
    return fallback;
}

}  // namespace

ServiceConfig load_config(const std::string& path) {
    auto root = YAML::LoadFile(path);
    if (!root || !root.IsMap()) {
        throw std::runtime_error("invalid config: expected a YAML map");
    }

    ServiceConfig cfg;
    cfg.config_path = path;

    cfg.logging.level = scalar_or(root["logging"], "level", "info");

    cfg.scan.period_ms = u32_or(root["scan"], "period_ms", 1000);
    cfg.scan.stable_cycles = u32_or(root["scan"], "stable_cycles", 2);

    cfg.midi_io.config_path = scalar_or(root["midi_io"], "config_path", cfg.midi_io.config_path);
    cfg.midi_io.control_endpoint = scalar_or(root["midi_io"], "control_endpoint", cfg.midi_io.control_endpoint);
    cfg.midi_io.reload_command = scalar_or(root["midi_io"], "reload_command", cfg.midi_io.reload_command);

    if (root["controllers"] && root["controllers"].IsSequence()) {
        for (const auto& item : root["controllers"]) {
            if (!item.IsMap()) {
                continue;
            }
            ControllerRule rule;
            rule.id = scalar_or(item, "id");
            rule.match_name = scalar_or(item, "match_name");
            if (!rule.id.empty() && !rule.match_name.empty()) {
                cfg.controllers.push_back(std::move(rule));
            }
        }
    }

    return cfg;
}

}  // namespace raptor::midi_usb_watcher
