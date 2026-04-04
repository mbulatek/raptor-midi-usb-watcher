#include "raptor_midi_usb_watcher/alsa_enumerator.hpp"

#include <alsa/asoundlib.h>

#include <string>
#include <vector>

namespace raptor::midi_usb_watcher {

std::vector<AlsaPort> AlsaEnumerator::scan() const {
    std::vector<AlsaPort> ports;

    snd_seq_t* seq = nullptr;
    const int open_rc = snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
    if (open_rc < 0 || seq == nullptr) {
        return ports;
    }

    snd_seq_client_info_t* client_info = nullptr;
    snd_seq_port_info_t* port_info = nullptr;
    snd_seq_client_info_alloca(&client_info);
    snd_seq_port_info_alloca(&port_info);

    snd_seq_client_info_set_client(client_info, -1);
    while (snd_seq_query_next_client(seq, client_info) >= 0) {
        const int client = snd_seq_client_info_get_client(client_info);
        const std::string client_name = snd_seq_client_info_get_name(client_info);

        snd_seq_port_info_set_client(port_info, client);
        snd_seq_port_info_set_port(port_info, -1);
        while (snd_seq_query_next_port(seq, port_info) >= 0) {
            const unsigned int caps = snd_seq_port_info_get_capability(port_info);
            // We want ports we can "read from" via a subscription (controller -> our input port).
            if ((caps & SND_SEQ_PORT_CAP_READ) == 0 || (caps & SND_SEQ_PORT_CAP_SUBS_READ) == 0) {
                continue;
            }

            const std::string port_name = snd_seq_port_info_get_name(port_info);
            AlsaPort port;
            port.client_id = client;
            port.port_id = snd_seq_port_info_get_port(port_info);
            port.client_name = client_name;
            port.port_name = port_name;
            port.full_name = client_name + ":" + port_name;
            ports.push_back(std::move(port));
        }
    }

    snd_seq_close(seq);
    return ports;
}

}  // namespace raptor::midi_usb_watcher
