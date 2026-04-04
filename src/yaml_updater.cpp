#include "raptor_midi_usb_watcher/yaml_updater.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace raptor::midi_usb_watcher {
namespace {

YAML::Node controller_node(const UsbControllerState& st) {
    YAML::Node n;
    n["id"] = st.id;
    n["match_name"] = st.match_name;
    n["enabled"] = st.enabled;
    return n;
}

}  // namespace

bool update_midi_io_config_usb_controllers(
    const std::string& midi_io_config_path,
    const std::vector<UsbControllerState>& controllers) {

    YAML::Node root = YAML::LoadFile(midi_io_config_path);
    if (!root || !root.IsMap()) {
        return false;
    }

    YAML::Node list(YAML::NodeType::Sequence);
    for (const auto& st : controllers) {
        list.push_back(controller_node(st));
    }
    root["usb_midi_controllers"] = list;

    YAML::Emitter out;
    out << root;
    if (!out.good()) {
        return false;
    }

    const std::filesystem::path cfg_path {midi_io_config_path};
    const std::filesystem::path tmp_path = cfg_path.parent_path() / (cfg_path.filename().string() + ".tmp");

    {
        std::ofstream f(tmp_path, std::ios::binary | std::ios::trunc);
        f << out.c_str() << "\n";
        f.flush();
        if (!f.good()) {
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmp_path, cfg_path, ec);
    if (ec) {
        std::filesystem::remove(tmp_path, ec);
        return false;
    }

    return true;
}

}  // namespace raptor::midi_usb_watcher
