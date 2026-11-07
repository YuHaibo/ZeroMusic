/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "client.h"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace ipc {
namespace {

constexpr std::size_t kMaxFrame = 8U * 1024U * 1024U;

uint32_t read_u32(const char* p) {
    return (static_cast<uint32_t>(static_cast<unsigned char>(p[0])) << 24) |
           (static_cast<uint32_t>(static_cast<unsigned char>(p[1])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(p[2])) << 8) |
           static_cast<uint32_t>(static_cast<unsigned char>(p[3]));
}

void append_u32(std::string& out, uint32_t n) {
    out.push_back(static_cast<char>((n >> 24) & 0xff));
    out.push_back(static_cast<char>((n >> 16) & 0xff));
    out.push_back(static_cast<char>((n >> 8) & 0xff));
    out.push_back(static_cast<char>(n & 0xff));
}

} // namespace

Client::Client() = default;

Client::~Client() {
    stop();
}

void Client::start(std::string socket_path) {
    if (running_.exchange(true)) {
        return;
    }
    socket_path_ = std::move(socket_path);
    thread_ = std::thread([this] { run(); });
}

void Client::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    wake_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    connected_.store(false);
}

void Client::set_event_handler(Event handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    event_ = std::move(handler);
}

void Client::call(const std::string& method, const Json& params, Result result) {
    uint64_t id = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        id = next_id_++;
        if (result) {
            pending_[id] = std::move(result);
        }
        Json request;
        request["id"] = id;
        request["method"] = method;
        if (!params.is_null()) {
            request["params"] = params;
        }
        std::string body = request.dump();
        std::string frame;
        frame.reserve(4 + body.size());
        append_u32(frame, static_cast<uint32_t>(body.size()));
        frame += body;
        out_.push_back(std::move(frame));
    }
    wake_.notify_one();
}

void Client::poll() {
    std::vector<std::function<void()>> work;
    {
        std::lock_guard<std::mutex> lock(inbox_mutex_);
        work.swap(inbox_);
    }
    for (auto& fn : work) {
        fn();
    }
}

void Client::push_event(const std::string& name, const Json& data) {
    std::lock_guard<std::mutex> lock(inbox_mutex_);
    inbox_.push_back([this, name, data] {
        Event handler;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            handler = event_;
        }
        if (handler) {
            handler(name, data);
        }
    });
}

void Client::push_call(std::function<void()> fn) {
    std::lock_guard<std::mutex> lock(inbox_mutex_);
    inbox_.push_back(std::move(fn));
}

void Client::fail_pending(const std::string& code, const std::string& message) {
    std::map<uint64_t, Result> pending;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending.swap(pending_);
    }
    for (auto& [id, result] : pending) {
        auto fn = std::move(result);
        push_call([fn = std::move(fn), code, message] {
            if (fn) {
                fn(Json(), code, message);
            }
        });
    }
}

void Client::run() {
    bool announced = false;
    while (running_.load()) {
        if (fd_ < 0) {
            if (!connect_socket()) {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait_for(lock, std::chrono::milliseconds(250), [this] { return !running_.load(); });
                continue;
            }
            announced = false;
        }
        if (!announced) {
            connected_.store(true);
            push_event("connected", Json::object());
            announced = true;
        }

        bool want_write = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            want_write = !out_.empty();
        }
        pollfd pfd{};
        pfd.fd = fd_;
        pfd.events = POLLIN | (want_write ? POLLOUT : 0);
        const int ready = ::poll(&pfd, 1, 50);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            disconnect_socket(std::strerror(errno));
            continue;
        }
        if (ready == 0) {
            continue;
        }
        if (pfd.revents & (POLLERR | POLLNVAL)) {
            disconnect_socket("socket error");
            continue;
        }
        if (pfd.revents & POLLIN) {
            read_available();
            if (fd_ < 0) {
                announced = false;
                continue;
            }
        }
        if (pfd.revents & POLLOUT) {
            write_pending();
            if (fd_ < 0) {
                announced = false;
            }
        }
        if (pfd.revents & POLLHUP) {
            disconnect_socket("core closed the socket");
            announced = false;
        }
    }
}

bool Client::connect_socket() {
    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return false;
    }
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (socket_path_.size() >= sizeof(addr.sun_path)) {
        ::close(fd);
        return false;
    }
    std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(fd);
        return false;
    }
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    fd_ = fd;
    in_.clear();
    return true;
}

void Client::disconnect_socket(const std::string& why) {
    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        fd = fd_;
        fd_ = -1;
        in_.clear();
    }
    if (fd >= 0) {
        ::close(fd);
    }
    if (connected_.exchange(false)) {
        push_event("disconnected", Json{{"reason", why}});
    }
    fail_pending("unavailable", "not connected to the core");
}

void Client::read_available() {
    char buf[64 * 1024];
    for (;;) {
        const ssize_t n = ::recv(fd_, buf, sizeof(buf), 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            disconnect_socket(std::strerror(errno));
            return;
        }
        if (n == 0) {
            disconnect_socket("core closed the connection");
            return;
        }
        in_.append(buf, static_cast<std::size_t>(n));
        while (in_.size() >= 4) {
            const uint32_t length = read_u32(in_.data());
            if (length == 0 || length > kMaxFrame) {
                disconnect_socket("bad frame from the core");
                return;
            }
            if (in_.size() < 4 + length) {
                break;
            }
            const std::string body = in_.substr(4, length);
            in_.erase(0, 4 + length);
            handle_frame(body);
        }
    }
}

void Client::handle_frame(const std::string& body) {
    Json message;
    try {
        message = Json::parse(body);
    }
    catch (const std::exception& error) {
        std::fprintf(stderr, "zero-music-ui: bad JSON from core: %s\n", error.what());
        return;
    }
    if (message.contains("event")) {
        push_event(message.value("event", ""), message.value("data", Json::object()));
        return;
    }
    if (!message.contains("id")) {
        return;
    }
    const uint64_t id = message.value("id", 0);
    Result result;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = pending_.find(id);
        if (it == pending_.end()) {
            return;
        }
        result = std::move(it->second);
        pending_.erase(it);
    }
    if (!result) {
        return;
    }
    if (message.contains("error")) {
        const auto& error = message["error"];
        push_call([result = std::move(result),
                   code = error.value("code", "internal"),
                   text = error.value("message", "")] { result(Json(), code, text); });
    }
    else {
        Json data = message.value("result", Json());
        push_call([result = std::move(result), data = std::move(data)] { result(data, "", ""); });
    }
}

void Client::write_pending() {
    for (;;) {
        std::string frame;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (out_.empty()) {
                return;
            }
            frame = std::move(out_.front());
            out_.pop_front();
        }
        std::size_t sent = 0;
        while (sent < frame.size()) {
#ifdef MSG_NOSIGNAL
            constexpr int flags = MSG_NOSIGNAL;
#else
            constexpr int flags = 0;
#endif
            const ssize_t n = ::send(fd_, frame.data() + sent, frame.size() - sent, flags);
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    continue;
                }
                disconnect_socket(std::strerror(errno));
                return;
            }
            sent += static_cast<std::size_t>(n);
        }
    }
}

std::string default_socket_path() {
    if (const char* path = std::getenv("ZERO_MUSIC_SOCKET"); path && *path) {
        return path;
    }
    if (const char* runtime = std::getenv("XDG_RUNTIME_DIR"); runtime && *runtime) {
        return std::string(runtime) + "/zero-music.sock";
    }
    return "/tmp/zero-music-" + std::to_string(::getuid()) + ".sock";
}

} // namespace ipc
