#include "raptor_midi_usb_watcher/application.hpp"

#include "raptor_midi_usb_watcher/alsa_enumerator.hpp"
#include "raptor_midi_usb_watcher/control_client.hpp"
#include "raptor_midi_usb_watcher/yaml_updater.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>
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

std::string slugify_id(const std::string& s) {
    std::string out;
    out.reserve(s.size());

    bool last_dash = false;
    for (const char c : s) {
        const unsigned char uc = static_cast<unsigned char>(c);
        const bool is_alnum =
            (uc >= 'A' && uc <= 'Z') ||
            (uc >= 'a' && uc <= 'z') ||
            (uc >= '0' && uc <= '9');

        if (is_alnum) {
            out.push_back(static_cast<char>(std::tolower(uc)));
            last_dash = false;
        } else {
            if (!out.empty() && !last_dash) {
                out.push_back('-');
                last_dash = true;
            }
        }
    }

    while (!out.empty() && out.back() == '-') {
        out.pop_back();
    }

    return out.empty() ? std::string{"usb-midi"} : out;
}

bool is_ignored_port(const AlsaPort& p) {
    // Filter well-known non-controller ports.
    if (p.client_name == "System") {
        return true;
    }
    if (p.client_name == "Midi Through") {
        return true;
    }
    if (p.port_name == "Announce") {
        return true;
    }
    return false;
}

std::vector<AlsaPort> filter_ports(std::vector<AlsaPort> ports) {
    ports.erase(
        std::remove_if(ports.begin(), ports.end(), [](const AlsaPort& p) { return is_ignored_port(p); }),
        ports.end());
    return ports;
}

std::vector<UsbControllerState> build_auto_states(const std::vector<AlsaPort>& ports) {
    std::vector<UsbControllerState> states;
    states.reserve(ports.size());

    std::unordered_map<std::string, std::uint32_t> used;

    for (const auto& p : ports) {
        const std::string match_name = p.client_name + " " + p.port_name;

        std::string id = slugify_id(match_name);
        auto& n = used[id];
        ++n;
        if (n > 1) {
            id += "-" + std::to_string(n);
        }

        states.push_back(UsbControllerState {
            .id = std::move(id),
            .match_name = match_name,
            .enabled = true,
        });
    }

    return states;
}

std::vector<UsbControllerState> build_allowlist_states(const std::vector<ControllerRule>& rules, const std::vector<AlsaPort>& ports) {
    std::vector<UsbControllerState> states;
    states.reserve(rules.size());

    for (const auto& rule : rules) {
        bool present = false;
        for (const auto& port : ports) {
            const std::string full_name = port.client_name + " " + port.port_name;
            if (contains_case_insensitive(full_name, rule.match_name)) {
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

    return states;
}

}  // namespace

Application::Application(ServiceConfig config) : config_(std::move(config)) {}

int Application::run() {
    spdlog::info(
        "starting scan_period_ms={} stable_cycles={} midi_io_cfg={} control={} mode={}",
        config_.scan.period_ms,
        config_.scan.stable_cycles,
        config_.midi_io.config_path,
        config_.midi_io.control_endpoint,
        config_.controllers.empty() ? "auto" : "allowlist");

    AlsaEnumerator enumerator;
    ControlClient control(config_.midi_io.control_endpoint);

    std::string last_key;
    std::string stable_key;
    std::uint32_t stable_count = 0;

    while (true) {
        auto ports = filter_ports(enumerator.scan());
        std::sort(ports.begin(), ports.end(), [](const AlsaPort& a, const AlsaPort& b) {
            return a.full_name < b.full_name;
        });
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
            if (config_.controllers.empty()) {
                states = build_auto_states(ports);
            } else {
                states = build_allowlist_states(config_.controllers, ports);
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