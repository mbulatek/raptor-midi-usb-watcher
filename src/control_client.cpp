#include "raptor_midi_usb_watcher/control_client.hpp"

#include <chrono>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <zmq.h>

namespace raptor::midi_usb_watcher {
namespace {

using json = nlohmann::json;

}  // namespace

ControlClient::ControlClient(std::string endpoint) : endpoint_(std::move(endpoint)) {}

std::optional<ControlReply> ControlClient::request_reload(const std::string& command, const std::string& request_id) const {
    void* ctx = zmq_ctx_new();
    if (ctx == nullptr) {
        return std::nullopt;
    }

    void* sock = zmq_socket(ctx, ZMQ_REQ);
    if (sock == nullptr) {
        zmq_ctx_term(ctx);
        return std::nullopt;
    }

    constexpr int timeout_ms = 200;
    constexpr int linger_ms = 0;
    (void)zmq_setsockopt(sock, ZMQ_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
    (void)zmq_setsockopt(sock, ZMQ_SNDTIMEO, &timeout_ms, sizeof(timeout_ms));
    (void)zmq_setsockopt(sock, ZMQ_LINGER, &linger_ms, sizeof(linger_ms));

    if (zmq_connect(sock, endpoint_.c_str()) != 0) {
        zmq_close(sock);
        zmq_ctx_term(ctx);
        return std::nullopt;
    }

    const auto req = json{{"command", command}, {"request_id", request_id}}.dump();
    if (zmq_send(sock, req.data(), req.size(), 0) < 0) {
        zmq_close(sock);
        zmq_ctx_term(ctx);
        return std::nullopt;
    }

    char buffer[4096];
    const auto size = zmq_recv(sock, buffer, sizeof(buffer), 0);
    zmq_close(sock);
    zmq_ctx_term(ctx);

    if (size <= 0) {
        return std::nullopt;
    }

    ControlReply reply;
    try {
        const auto root = json::parse(std::string(buffer, buffer + size), nullptr, false);
        if (!root.is_object()) {
            return reply;
        }
        reply.ok = root.value("ok", false);
        reply.service = root.value("service", "");
        reply.command = root.value("command", "");
        if (root.contains("error") && root["error"].is_object()) {
            reply.error_message = root["error"].value("message", "");
        }
        return reply;
    } catch (...) {
        return reply;
    }
}

}  // namespace raptor::midi_usb_watcher
