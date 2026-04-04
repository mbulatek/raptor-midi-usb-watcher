#include "raptor_midi_usb_watcher/application.hpp"

#include "raptor_midi_usb_watcher/alsa_enumerator.hpp"
#include "raptor_midi_usb_watcher/control_client.hpp"
#include "raptor_midi_usb_watcher/yaml_updater.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

namespace raptor::midi_usb_watcher {
namespace {

bool contains_case_insensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    auto it = std::search(
        haystack.begin(), haystack.end(), needle.begin(), needle.end(),
        [](char left, char right) {
            return std::tolower(static_cast<unsigned char>(left)) ==
                   std::tolower(static_cast<unsigned char>(right));
        });
    return it != haystack.end();
}

std::string join_detected_key(const std::vector<AlsaPort>& ports) {
    std::vector<std::string> keys;
    keys.reserve(ports.size());
    for (const auto& p : ports) {
        keys.push_back(p.full_name);
    }
    std::sort(keys.begin(), keys.end());
    std::string out;
    for (const auto& k : keys) {
        out.append(k);
        out.push_back('\n');
    }
    return out;
}

}  // namespace

Application::Application(ServiceConfig config) : config_(std::move(config)) {}

int Application::run() {
    spdlog::info(
        "starting scan_period_ms={} stable_cycles={} midi_io_cfg={} control={}",
        config_.scan.period_ms,
        config_.scan.stable_cycles,
        config_.midi_io.config_path,
        config_.midi_io.control_endpoint);

    AlsaEnumerator enumerator;
    ControlClient control(config_.midi_io.control_endpoint);

    std::string last_key;
    std::string stable_key;
    std::uint32_t stable_count = 0;

    while (true) {
        const auto ports = enumerator.scan();
        const auto key = join_detected_key(ports);

        if (key != stable_key) {
            stable_key = key;
            stable_count = 1;
        } else {
            ++stable_count;
        }

        if (stable_key != last_key && stable_count >= std::max<std::uint32_t>(1, config_.scan.stable_cycles)) {
            last_key = stable_key;

            spdlog::info("usb midi port set changed (ports={})", ports.size());
            for (const auto& p : ports) {
                spdlog::debug("detected alsa client={} port={} name={}", p.client_id, p.port_id, p.full_name);
            }

            std::vector<UsbControllerState> states;
            states.reserve(config_.controllers.size());
            for (const auto& rule : config_.controllers) {
                bool present = false;
                for (const auto& port : ports) {
                    if (contains_case_insensitive(port.full_name, rule.match_name)) {
                        present = true;
                        break;
                    }
                }
                states.push_back(UsbControllerState {
                    .id = rule.id,
                    .match_name = rule.match_name,
                    .enabled = present,
                });
            }

            if (!update_midi_io_config_usb_controllers(config_.midi_io.config_path, states)) {
                spdlog::error("failed to update midi-io config at {}", config_.midi_io.config_path);
            } else {
                const auto reply = control.request_reload(config_.midi_io.reload_command, "usb-watcher-reload");
                if (!reply.has_value()) {
                    spdlog::warn("midi-io reload request failed (no reply)");
                } else if (!reply->ok) {
                    spdlog::warn("midi-io reload rejected command={} err={}", reply->command, reply->error_message);
                } else {
                    spdlog::info("midi-io reload ok");
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(config_.scan.period_ms));
    }
}

}  // namespace raptor::midi_usb_watcher
