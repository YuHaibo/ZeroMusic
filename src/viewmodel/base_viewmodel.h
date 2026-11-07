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
#include <utility>
#include <vector>

namespace viewmodel {

// TrackItem is a row the list screens show, with what the core needs to play
// it later.
struct TrackItem {
    std::string id;
    std::string name;
    std::string artist;
    std::string album;
    std::string source;
    std::int64_t duration_ms = 0;
    ipc::Json json;
};

// CollectionKind is what a Browse list holds besides songs.
enum class CollectionKind {
    Albums,
    Artists,
    Playlists,
};

// CollectionItem is an album, artist or playlist row.
struct CollectionItem {
    std::string id;
    std::string title;
    std::string subtitle;
};

// PluginSetting is one value a plugin asks the listener for.
struct PluginSetting {
    std::string key;
    std::string label;
    std::string value;
};

// PluginItem is a row of the plugin manager.
struct PluginItem {
    std::string id;
    std::string name;
    std::string version;
    bool enabled = false;
    bool granted = false;
    bool process = false;
};

// LyricItem is a timed line, as the core parsed it.
struct LyricItem {
    std::int64_t time_ms = 0;
    std::string text;
};

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

    void cycle_quality();
    void toggle_normalize();
    void cycle_cache();
    void cycle_language();
    void rescan();
    void cycle_source();

    void load_cover(const std::string& url, int size = 120);
    const std::vector<std::uint8_t>& cover_data() const { return cover_data_; }
    int cover_width() const { return cover_width_; }
    int cover_height() const { return cover_height_; }
    lv_subject_t* cover_revision_subject();

    const std::vector<PluginItem>& plugins() const { return plugins_; }
    void load_plugins();
    void set_plugin_enabled(std::size_t index, bool on);
    void grant_plugin(std::size_t index, bool on);
    void sign_in_plugin(std::size_t index);
    void install_plugin(const std::string& repo);
    void load_plugin_settings(std::size_t index);
    void save_plugin_setting(std::size_t index, const std::string& value);
    const std::vector<PluginSetting>& plugin_settings() const { return plugin_settings_; }
    lv_subject_t* plugins_revision_subject();
    lv_subject_t* plugin_settings_revision_subject();
    lv_subject_t* prompt_qr_subject();
    lv_subject_t* prompt_text_subject();
    const std::string& quality() const { return quality_; }
    bool normalize() const { return normalize_; }
    int cache_mb() const { return cache_mb_; }
    const std::string& language() const { return language_; }

    // browse opens a source page, such as "songs"; search queries the active
    // source. Both fill tracks(), and bump list_revision() when they do.
    void browse(const std::string& page = "songs");
    void load_collection(CollectionKind kind);
    void open_collection_item(std::size_t index);
    const std::vector<CollectionItem>& collection() const { return collection_; }
    lv_subject_t* collection_revision_subject();
    lv_subject_t* sources_revision_subject();
    void search(const std::string& query);
    const std::vector<TrackItem>& tracks() const { return tracks_; }
    std::size_t track_count() const { return tracks_.size(); }
    const std::string& list_title() const { return list_title_; }
    void play_at(std::size_t index);

    // queue is what plays, in play order; queue_at is the current row. Lyrics
    // are the current track's, when the source had any.
    const std::vector<TrackItem>& queue() const { return queue_view_; }
    int queue_at() const { return queue_at_; }
    void play_queue_at(std::size_t order_index);
    void remove_queue_at(std::size_t order_index);
    const std::vector<LyricItem>& lyrics() const { return lyrics_; }
    std::int64_t position_ms() const { return position_ms_; }

    lv_subject_t* list_title_subject();
    lv_subject_t* list_revision_subject();
    lv_subject_t* queue_revision_subject();
    lv_subject_t* lyrics_revision_subject();
    lv_subject_t* quality_subject();
    lv_subject_t* normalize_subject();
    lv_subject_t* cache_subject();
    lv_subject_t* language_subject();

private:
    void on_event(const std::string& name, const ipc::Json& data);
    void on_connected();
    void apply_state(const ipc::Json& state);
    void apply_sources(const ipc::Json& sources);
    void apply_settings(const ipc::Json& settings);
    void apply_browse(const ipc::Json& browse);
    void apply_tracks(const ipc::Json& tracks, const std::string& title);
    void apply_connection_status();
    void fail(const std::string& code, const std::string& message);
    void publish_all();

    model::BaseModel model_;
    ipc::Client client_;
    std::vector<model::AppPage> stack_;
    std::string active_source_ = "local";
    std::string preferred_source_;
    std::vector<std::pair<std::string, std::string>> sources_;
    std::int64_t position_ms_ = 0;
    int volume_ = 80;
    int repeat_ = 0;
    bool shuffle_ = false;
    bool hello_sent_ = false;
    std::string quality_ = "high";
    bool normalize_ = false;
    int cache_mb_ = 512;
    std::string language_ = "system";
    std::string list_title_ = "Songs";
    std::vector<TrackItem> tracks_;
    std::vector<TrackItem> queue_view_;
    std::vector<int> queue_indices_;
    int queue_at_ = -1;
    std::vector<LyricItem> lyrics_;
    std::string track_id_;
    std::vector<PluginItem> plugins_;
    std::vector<PluginSetting> plugin_settings_;
    std::string plugin_settings_id_;
    std::vector<CollectionItem> collection_;
    CollectionKind collection_kind_ = CollectionKind::Albums;
    std::vector<std::uint8_t> cover_data_;
    int cover_width_ = 0;
    int cover_height_ = 0;
    std::string cover_url_;

    reactive::StringSubject<32> title_subject_;
    reactive::StringSubject<32> source_subject_;
    reactive::StringSubject<96> track_title_subject_;
    reactive::StringSubject<96> track_artist_subject_;
    reactive::StringSubject<96> track_album_subject_;
    reactive::StringSubject<16> position_text_subject_;
    reactive::StringSubject<16> duration_text_subject_;
    reactive::StringSubject<64> status_subject_;
    reactive::StringSubject<32> list_title_subject_;
    reactive::IntSubject list_revision_subject_;
    reactive::IntSubject queue_revision_subject_;
    reactive::IntSubject lyrics_revision_subject_;
    reactive::StringSubject<32> quality_subject_;
    reactive::StringSubject<32> normalize_subject_;
    reactive::StringSubject<32> cache_subject_;
    reactive::StringSubject<32> language_subject_;
    reactive::IntSubject plugins_revision_subject_;
    reactive::IntSubject plugin_settings_revision_subject_;
    reactive::StringSubject<512> prompt_qr_subject_;
    reactive::StringSubject<256> prompt_text_subject_;
    reactive::IntSubject cover_revision_subject_;
    reactive::IntSubject collection_revision_subject_;
    reactive::IntSubject sources_revision_subject_;
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
