/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "base_viewmodel.h"

#include "logger.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace viewmodel {
namespace {

int page_to_int(model::AppPage page) {
    return static_cast<int>(page);
}

std::string join_artists(const ipc::Json& track) {
    std::string names;
    if (!track.contains("artists") || !track["artists"].is_array()) {
        return names;
    }
    for (const auto& artist : track["artists"]) {
        if (!names.empty()) {
            names += ", ";
        }
        names += artist.value("name", "");
    }
    return names;
}

std::string format_time(std::int64_t ms) {
    if (ms < 0) {
        ms = 0;
    }
    const auto seconds = ms / 1000;
    char text[16];
    std::snprintf(text, sizeof(text), "%lld:%02lld",
                  static_cast<long long>(seconds / 60),
                  static_cast<long long>(seconds % 60));
    return text;
}

} // namespace

BaseViewModel::BaseViewModel()
    : stack_{model::AppPage::Home},
      title_subject_("Zero Music"),
      source_subject_("Local Files"),
      track_title_subject_("Nothing playing"),
      track_artist_subject_("Choose a source in the core"),
      track_album_subject_(""),
      position_text_subject_("0:00"),
      duration_text_subject_("0:00"),
      status_subject_("Waiting for core…"),
      progress_subject_(0),
      playing_subject_(false),
      connected_subject_(false),
      has_track_subject_(false),
      volume_subject_(80),
      repeat_subject_(0),
      shuffle_subject_(false),
      dark_mode_subject_(model_.dark_mode()),
      current_page_subject_(page_to_int(model_.current_page())),
      quit_requested_subject_(false) {
    if (const char* start = std::getenv("ZERO_MUSIC_START_PAGE"); start && std::string(start) == "now-playing") {
        stack_ = {model::AppPage::NowPlaying};
        model_.set_current_page(model::AppPage::NowPlaying);
        current_page_subject_.set(page_to_int(model::AppPage::NowPlaying));
    }
    client_.set_event_handler([this](const std::string& name, const ipc::Json& data) {
        on_event(name, data);
    });
}

BaseViewModel::~BaseViewModel() {
    client_.stop();
}

void BaseViewModel::start(std::string socket_path) {
    client_.start(std::move(socket_path));
}

void BaseViewModel::poll() {
    client_.poll();
}

lv_subject_t* BaseViewModel::title_subject() { return title_subject_.native(); }
lv_subject_t* BaseViewModel::source_subject() { return source_subject_.native(); }
lv_subject_t* BaseViewModel::track_title_subject() { return track_title_subject_.native(); }
lv_subject_t* BaseViewModel::track_artist_subject() { return track_artist_subject_.native(); }
lv_subject_t* BaseViewModel::track_album_subject() { return track_album_subject_.native(); }
lv_subject_t* BaseViewModel::position_text_subject() { return position_text_subject_.native(); }
lv_subject_t* BaseViewModel::duration_text_subject() { return duration_text_subject_.native(); }
lv_subject_t* BaseViewModel::status_subject() { return status_subject_.native(); }
lv_subject_t* BaseViewModel::progress_subject() { return progress_subject_.native(); }
lv_subject_t* BaseViewModel::playing_subject() { return playing_subject_.native(); }
lv_subject_t* BaseViewModel::connected_subject() { return connected_subject_.native(); }
lv_subject_t* BaseViewModel::has_track_subject() { return has_track_subject_.native(); }
lv_subject_t* BaseViewModel::volume_subject() { return volume_subject_.native(); }
lv_subject_t* BaseViewModel::repeat_subject() { return repeat_subject_.native(); }
lv_subject_t* BaseViewModel::shuffle_subject() { return shuffle_subject_.native(); }
lv_subject_t* BaseViewModel::dark_mode_subject() { return dark_mode_subject_.native(); }
lv_subject_t* BaseViewModel::current_page_subject() { return current_page_subject_.native(); }
lv_subject_t* BaseViewModel::quit_requested_subject() { return quit_requested_subject_.native(); }

bool BaseViewModel::is_dark_mode() const { return model_.dark_mode(); }

void BaseViewModel::set_dark_mode(bool enabled) {
    model_.set_dark_mode(enabled);
    dark_mode_subject_.set(enabled);
}

void BaseViewModel::toggle_dark_mode() {
    set_dark_mode(!model_.dark_mode());
}

model::AppPage BaseViewModel::current_page() const {
    return model_.current_page();
}

void BaseViewModel::show_page(model::AppPage page) {
    if (model_.current_page() == page) {
        return;
    }
    stack_.push_back(page);
    model_.set_current_page(page);
    current_page_subject_.set(page_to_int(page));
}

void BaseViewModel::go_back() {
    if (stack_.size() > 1) {
        stack_.pop_back();
    }
    model_.set_current_page(stack_.empty() ? model::AppPage::Home : stack_.back());
    current_page_subject_.set(page_to_int(model_.current_page()));
}

void BaseViewModel::request_quit() {
    quit_requested_subject_.set(true);
}

void BaseViewModel::toggle_play() {
    client_.call("player.toggle");
}

void BaseViewModel::next() {
    client_.call("player.next");
}

void BaseViewModel::previous() {
    client_.call("player.prev");
}

void BaseViewModel::seek_relative(int delta_ms) {
    std::int64_t target = position_ms_ + delta_ms;
    target = std::max<std::int64_t>(0, target);
    if (model_.duration_ms() > 0) {
        target = std::min(target, model_.duration_ms());
    }
    position_ms_ = target;
    client_.call("player.seek", ipc::Json{{"position", target}});
}

void BaseViewModel::volume_delta(int delta) {
    volume_ = std::clamp(volume_ + delta, 0, 100);
    model_.set_volume(volume_);
    volume_subject_.set(volume_);
    client_.call("player.volume", ipc::Json{{"volume", static_cast<double>(volume_) / 100.0}});
}

void BaseViewModel::cycle_repeat() {
    repeat_ = (repeat_ + 1) % 3;
    const char* mode = repeat_ == 0 ? "off" : (repeat_ == 1 ? "all" : "one");
    model_.set_repeat(repeat_);
    repeat_subject_.set(repeat_);
    client_.call("player.repeat", ipc::Json{{"mode", mode}});
}

void BaseViewModel::toggle_shuffle() {
    shuffle_ = !shuffle_;
    model_.set_shuffle(shuffle_);
    shuffle_subject_.set(shuffle_);
    client_.call("player.shuffle", ipc::Json{{"on", shuffle_}});
}

void BaseViewModel::refresh() {
    client_.call("player.state", ipc::Json::object(), [this](const ipc::Json& result,
                                                             const std::string& code,
                                                             const std::string& message) {
        if (!code.empty()) {
            fail(code, message);
        }
        else {
            apply_state(result);
        }
    });
}

void BaseViewModel::on_event(const std::string& name, const ipc::Json& data) {
    if (name == "connected") {
        on_connected();
    }
    else if (name == "disconnected") {
        model_.set_connected(false);
        hello_sent_ = false;
        connected_subject_.set(false);
        status_subject_.set("Core disconnected");
        model_.set_status("Core disconnected");
    }
    else if (name == "player.state") {
        apply_state(data);
    }
    else if (name == "sources.changed") {
        apply_sources(data);
    }
    else if (name == "toast") {
        const std::string text = data.value("text", "");
        model_.set_status(text);
        status_subject_.set(text.c_str());
    }
    else if (name == "plugin.prompt") {
        const std::string text = data.value("text", "Scan the code to sign in");
        model_.set_status(text);
        status_subject_.set(text.c_str());
    }
    else if (name == "error") {
        fail(data.value("code", "internal"), data.value("message", ""));
    }
}

void BaseViewModel::on_connected() {
    model_.set_connected(true);
    connected_subject_.set(true);
    status_subject_.set("Connected");
    model_.set_status("Connected");
    if (hello_sent_) {
        return;
    }
    hello_sent_ = true;
    client_.call("hello", ipc::Json{{"protocol", 1}, {"uiVersion", "0.1.0"}},
                 [this](const ipc::Json& result, const std::string& code, const std::string& message) {
        if (!code.empty()) {
            fail(code, message);
            hello_sent_ = false;
            return;
        }
        const int protocol = result.value("protocol", 0);
        if (protocol != 1) {
            fail("unsupported", "The core speaks another protocol");
            return;
        }
        status_subject_.set("Ready");
        model_.set_status("Ready");
        LOG_INFO("core connected: {} {}", result.value("app", "Zero Music"), result.value("version", ""));
        client_.call("sources.list", ipc::Json::object(),
                     [this](const ipc::Json& data, const std::string& error, const std::string& text) {
            if (error.empty()) {
                apply_sources(data);
            }
            else {
                fail(error, text);
            }
        });
        client_.call("settings.get", ipc::Json::object(),
                     [this](const ipc::Json& data, const std::string& error, const std::string& text) {
            if (error.empty()) {
                apply_settings(data);
            }
            else {
                fail(error, text);
            }
        });
        refresh();
    });
}

void BaseViewModel::apply_state(const ipc::Json& state) {
    model_.set_playing(state.value("playing", false));
    model_.set_loading(state.value("loading", false));
    playing_subject_.set(model_.playing());

    const auto& track = state.contains("track") ? state["track"] : ipc::Json();
    model::Track shown;
    if (track.is_object() && !track.empty()) {
        shown.valid = true;
        shown.name = track.value("name", "");
        shown.artist = join_artists(track);
        if (track.contains("album") && track["album"].is_object()) {
            shown.album = track["album"].value("name", "");
        }
        shown.source = track.value("source", "");
        shown.duration_ms = track.value("duration", static_cast<std::int64_t>(0));
    }
    model_.set_track(shown);
    has_track_subject_.set(shown.valid);
    if (shown.valid) {
        track_title_subject_.set(shown.name.c_str());
        track_artist_subject_.set(shown.artist.c_str());
        track_album_subject_.set(shown.album.c_str());
    }
    else {
        track_title_subject_.set("Nothing playing");
        track_artist_subject_.set("Choose a source in the core");
        track_album_subject_.set("");
    }
    position_ms_ = state.value("position", static_cast<std::int64_t>(0));
    const auto duration = state.value("duration", shown.duration_ms);
    model_.set_position_ms(position_ms_);
    model_.set_duration_ms(duration);
    position_text_subject_.set(format_time(position_ms_).c_str());
    duration_text_subject_.set(format_time(duration).c_str());
    const int progress = duration > 0 ? static_cast<int>(position_ms_ * 1000 / duration) : 0;
    progress_subject_.set(std::clamp(progress, 0, 1000));

    volume_ = static_cast<int>(state.value("volume", 0.8) * 100.0);
    model_.set_volume(volume_);
    volume_subject_.set(volume_);

    const std::string repeat = state.value("repeat", "off");
    repeat_ = repeat == "all" ? 1 : (repeat == "one" ? 2 : 0);
    model_.set_repeat(repeat_);
    repeat_subject_.set(repeat_);

    shuffle_ = state.value("shuffle", false);
    model_.set_shuffle(shuffle_);
    shuffle_subject_.set(shuffle_);

    const std::string error = state.value("error", "");
    model_.set_error(error);
    if (!error.empty()) {
        status_subject_.set(error.c_str());
        model_.set_status(error);
    }
}

void BaseViewModel::apply_sources(const ipc::Json& sources) {
    if (!sources.is_array()) {
        return;
    }
    bool chose = false;
    for (const auto& source : sources) {
        if (!source.value("enabled", true)) {
            continue;
        }
        const std::string id = source.value("id", "");
        const std::string name = source.value("name", id);
        if (!chose) {
            active_source_ = id;
            model_.set_source_name(name);
            source_subject_.set(name.c_str());
            chose = true;
        }
        if (id == active_source_) {
            model_.set_source_name(name);
            source_subject_.set(name.c_str());
        }
    }
}

void BaseViewModel::apply_settings(const ipc::Json& settings) {
    const double volume = settings.value("volume", 0.8);
    volume_ = static_cast<int>(volume * 100.0);
    model_.set_volume(volume_);
    volume_subject_.set(volume_);
    shuffle_ = settings.value("shuffle", false);
    model_.set_shuffle(shuffle_);
    shuffle_subject_.set(shuffle_);
    repeat_ = settings.value("repeat", 0);
    model_.set_repeat(repeat_);
    repeat_subject_.set(repeat_);
}

void BaseViewModel::apply_connection_status() {
    connected_subject_.set(model_.connected());
}

void BaseViewModel::fail(const std::string& code, const std::string& message) {
    std::string text;
    if (code == "signIn") {
        text = "Sign in needed";
    }
    else if (code == "setup") {
        text = "The plugin needs setting up";
    }
    else if (code == "unavailable") {
        text = message.empty() ? "Not available" : message;
    }
    else if (code == "unsupported") {
        text = message.empty() ? "Not supported" : message;
    }
    else {
        text = message.empty() ? "The core failed" : message;
    }
    LOG_WARN("core: {}: {}", code, message);
    model_.set_error(text);
    model_.set_status(text);
    status_subject_.set(text.c_str());
    apply_connection_status();
}

void BaseViewModel::publish_all() {
    title_subject_.set("Zero Music");
    source_subject_.set(model_.source_name().c_str());
    dark_mode_subject_.set(model_.dark_mode());
    current_page_subject_.set(page_to_int(model_.current_page()));
}

} // namespace viewmodel
