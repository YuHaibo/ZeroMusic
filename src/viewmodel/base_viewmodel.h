/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_model.h"
#include "client.h"
#include "subjects.h"

#include "lvgl.h"

#include <string>
#include <vector>

namespace viewmodel {

// BaseViewModel is what the screens bind to and the keys act on. It keeps the
// core's answers in the model and publishes them as subjects.
class BaseViewModel {
public:
    BaseViewModel();
    ~BaseViewModel();

    BaseViewModel(const BaseViewModel&) = delete;
    BaseViewModel& operator=(const BaseViewModel&) = delete;

    // start opens the core socket; poll delivers what came back, on the LVGL
    // thread, and is called from the app's timer.
    void start(std::string socket_path);
    void poll();

    lv_subject_t* title_subject();
    lv_subject_t* source_subject();
    lv_subject_t* track_title_subject();
    lv_subject_t* track_artist_subject();
    lv_subject_t* track_album_subject();
    lv_subject_t* position_text_subject();
    lv_subject_t* duration_text_subject();
    lv_subject_t* status_subject();
    lv_subject_t* progress_subject();
    lv_subject_t* playing_subject();
    lv_subject_t* connected_subject();
    lv_subject_t* has_track_subject();
    lv_subject_t* volume_subject();
    lv_subject_t* repeat_subject();
    lv_subject_t* shuffle_subject();
    lv_subject_t* dark_mode_subject();
    lv_subject_t* current_page_subject();
    lv_subject_t* quit_requested_subject();

    bool is_dark_mode() const;
    void set_dark_mode(bool enabled);
    void toggle_dark_mode();

    model::AppPage current_page() const;
    void show_page(model::AppPage page);
    void go_back();
    void request_quit();

    void toggle_play();
    void next();
    void previous();
    void seek_relative(int delta_ms);
    void volume_delta(int delta);
    void cycle_repeat();
    void toggle_shuffle();
    void refresh();

private:
    void on_event(const std::string& name, const ipc::Json& data);
    void on_connected();
    void apply_state(const ipc::Json& state);
    void apply_sources(const ipc::Json& sources);
    void apply_settings(const ipc::Json& settings);
    void apply_connection_status();
    void fail(const std::string& code, const std::string& message);
    void publish_all();

    model::BaseModel model_;
    ipc::Client client_;
    std::vector<model::AppPage> stack_;
    std::string active_source_ = "local";
    std::int64_t position_ms_ = 0;
    int volume_ = 80;
    int repeat_ = 0;
    bool shuffle_ = false;
    bool hello_sent_ = false;

    reactive::StringSubject<32> title_subject_;
    reactive::StringSubject<32> source_subject_;
    reactive::StringSubject<96> track_title_subject_;
    reactive::StringSubject<96> track_artist_subject_;
    reactive::StringSubject<96> track_album_subject_;
    reactive::StringSubject<16> position_text_subject_;
    reactive::StringSubject<16> duration_text_subject_;
    reactive::StringSubject<64> status_subject_;
    reactive::IntSubject progress_subject_;
    reactive::BoolSubject playing_subject_;
    reactive::BoolSubject connected_subject_;
    reactive::BoolSubject has_track_subject_;
    reactive::IntSubject volume_subject_;
    reactive::IntSubject repeat_subject_;
    reactive::BoolSubject shuffle_subject_;
    reactive::BoolSubject dark_mode_subject_;
    reactive::IntSubject current_page_subject_;
    reactive::BoolSubject quit_requested_subject_;
};

} // namespace viewmodel
