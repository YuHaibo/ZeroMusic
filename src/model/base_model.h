/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstdint>
#include <string>

namespace model {

enum class AppPage {
    Home = 0,
    NowPlaying = 1,
    Browse = 2,
    Search = 3,
    Detail = 4,
    Lyrics = 5,
    Queue = 6,
    Settings = 7,
    Plugins = 8,
    SignIn = 9,
    Help = 10,
    Install = 11,
    Account = 12,
};

struct Track {
    bool valid = false;
    std::string name;
    std::string artist;
    std::string album;
    std::string source;
    int64_t duration_ms = 0;
};

// BaseModel is the durable state the UI shows: where the listener is, what
// plays, and what the core last said.
class BaseModel {
public:
    bool dark_mode() const { return dark_mode_; }
    void set_dark_mode(bool enabled) { dark_mode_ = enabled; }
    void toggle_dark_mode() { dark_mode_ = !dark_mode_; }

    AppPage current_page() const { return current_page_; }
    void set_current_page(AppPage page) { current_page_ = page; }

    bool connected() const { return connected_; }
    void set_connected(bool connected) { connected_ = connected; }

    const std::string& source_name() const { return source_name_; }
    void set_source_name(std::string name) { source_name_ = std::move(name); }

    const Track& track() const { return track_; }
    void set_track(Track track) { track_ = std::move(track); }

    bool playing() const { return playing_; }
    void set_playing(bool playing) { playing_ = playing; }

    bool loading() const { return loading_; }
    void set_loading(bool loading) { loading_ = loading; }

    std::int64_t position_ms() const { return position_ms_; }
    void set_position_ms(std::int64_t position) { position_ms_ = position; }

    std::int64_t duration_ms() const { return duration_ms_; }
    void set_duration_ms(std::int64_t duration) { duration_ms_ = duration; }

    int volume() const { return volume_; }
    void set_volume(int volume) { volume_ = volume; }

    int repeat() const { return repeat_; }
    void set_repeat(int repeat) { repeat_ = repeat; }

    bool shuffle() const { return shuffle_; }
    void set_shuffle(bool shuffle) { shuffle_ = shuffle; }

    const std::string& status() const { return status_; }
    void set_status(std::string status) { status_ = std::move(status); }

    const std::string& error() const { return error_; }
    void set_error(std::string error) { error_ = std::move(error); }

private:
    bool dark_mode_ = true;
    bool connected_ = false;
    AppPage current_page_ = AppPage::Home;
    std::string source_name_ = "Local Files";
    Track track_;
    bool playing_ = false;
    bool loading_ = false;
    std::int64_t position_ms_ = 0;
    std::int64_t duration_ms_ = 0;
    int volume_ = 80;
    int repeat_ = 0;
    bool shuffle_ = false;
    std::string status_;
    std::string error_;
};

} // namespace model
