/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "nlohmann/json.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ipc {

using Json = nlohmann::json;

// Client speaks to zero-music-core over its Unix socket. Calls may be made
// from the LVGL thread; the socket lives on a thread of its own and answers
// are delivered by poll(), on the caller's thread.
class Client {
public:
    // Result is called with the answer, or with code and message on failure.
    using Result = std::function<void(const Json& result, const std::string& code, const std::string& message)>;
    // Event is called for pushed events, and for "connected"/"disconnected".
    using Event = std::function<void(const std::string& name, const Json& data)>;

    Client();
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void start(std::string socket_path);
    void stop();

    // call sends a request. An empty handler is fine for fire-and-forget.
    void call(const std::string& method, const Json& params = Json::object(), Result result = {});

    void set_event_handler(Event handler);

    // poll delivers answers and events queued since the last call. It must run
    // on the thread that owns the UI.
    void poll();

    bool connected() const { return connected_.load(); }
    const std::string& socket_path() const { return socket_path_; }

private:
    void run();
    bool connect_socket();
    void disconnect_socket(const std::string& why);
    void read_available();
    void write_pending();
    void handle_frame(const std::string& frame);
    void push_event(const std::string& name, const Json& data);
    void push_call(std::function<void()> fn);
    void fail_pending(const std::string& code, const std::string& message);

    std::string socket_path_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};

    int fd_{-1};
    std::string in_;
    std::deque<std::string> out_;
    uint64_t next_id_{1};
    std::map<uint64_t, Result> pending_;
    std::mutex mutex_;
    std::condition_variable wake_;

    Event event_;
    std::vector<std::function<void()>> inbox_;
    std::mutex inbox_mutex_;
};

// default_socket_path is where the wrapper and core put the socket.
std::string default_socket_path();

} // namespace ipc
