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

TrackItem parse_track(const ipc::Json& track, const std::string& source) {
    TrackItem item;
    item.id = track.value("id", "");
    item.name = track.value("name", "");
    item.artist = join_artists(track);
    if (track.contains("album") && track["album"].is_object()) {
        item.album = track["album"].value("name", "");
    }
    item.source = track.value("source", source);
    item.duration_ms = track.value("duration", static_cast<std::int64_t>(0));
    item.json = track;
    if (!item.json.contains("source") || item.json["source"].empty()) {
        item.json["source"] = item.source;
    }
    return item;
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
      list_title_subject_("Songs"),
      list_revision_subject_(0),
      queue_revision_subject_(0),
      lyrics_revision_subject_(0),
      quality_subject_("High"),
      normalize_subject_("Off"),
      cache_subject_("512 MB"),
      language_subject_("System"),
      plugins_revision_subject_(0),
      prompt_qr_subject_(""),
      prompt_text_subject_("Start sign-in from Plugins"),
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
    if (const char* start = std::getenv("ZERO_MUSIC_START_PAGE"); start) {
        const std::string page(start);
        if (page == "now-playing") {
            stack_ = {model::AppPage::NowPlaying};
            model_.set_current_page(model::AppPage::NowPlaying);
            current_page_subject_.set(page_to_int(model::AppPage::NowPlaying));
        }
        else if (page == "browse") {
            stack_ = {model::AppPage::Browse};
            model_.set_current_page(model::AppPage::Browse);
            current_page_subject_.set(page_to_int(model::AppPage::Browse));
        }
        else if (page == "search") {
            stack_ = {model::AppPage::Search};
            model_.set_current_page(model::AppPage::Search);
            current_page_subject_.set(page_to_int(model::AppPage::Search));
        }
        else if (page == "queue") {
            stack_ = {model::AppPage::Queue};
            model_.set_current_page(model::AppPage::Queue);
            current_page_subject_.set(page_to_int(model::AppPage::Queue));
        }
        else if (page == "lyrics") {
            stack_ = {model::AppPage::Lyrics};
            model_.set_current_page(model::AppPage::Lyrics);
            current_page_subject_.set(page_to_int(model::AppPage::Lyrics));
        }
        else if (page == "settings") {
            stack_ = {model::AppPage::Settings};
            model_.set_current_page(model::AppPage::Settings);
            current_page_subject_.set(page_to_int(model::AppPage::Settings));
        }
        else if (page == "plugins") {
            stack_ = {model::AppPage::Plugins};
            model_.set_current_page(model::AppPage::Plugins);
            current_page_subject_.set(page_to_int(model::AppPage::Plugins));
        }
        else if (page == "signin") {
            stack_ = {model::AppPage::SignIn};
            model_.set_current_page(model::AppPage::SignIn);
            current_page_subject_.set(page_to_int(model::AppPage::SignIn));
        }
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
lv_subject_t* BaseViewModel::list_title_subject() { return list_title_subject_.native(); }
lv_subject_t* BaseViewModel::list_revision_subject() { return list_revision_subject_.native(); }
lv_subject_t* BaseViewModel::queue_revision_subject() { return queue_revision_subject_.native(); }
lv_subject_t* BaseViewModel::lyrics_revision_subject() { return lyrics_revision_subject_.native(); }
lv_subject_t* BaseViewModel::quality_subject() { return quality_subject_.native(); }
lv_subject_t* BaseViewModel::normalize_subject() { return normalize_subject_.native(); }
lv_subject_t* BaseViewModel::cache_subject() { return cache_subject_.native(); }
lv_subject_t* BaseViewModel::language_subject() { return language_subject_.native(); }
lv_subject_t* BaseViewModel::plugins_revision_subject() { return plugins_revision_subject_.native(); }
lv_subject_t* BaseViewModel::prompt_qr_subject() { return prompt_qr_subject_.native(); }
lv_subject_t* BaseViewModel::prompt_text_subject() { return prompt_text_subject_.native(); }
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

void BaseViewModel::browse(const std::string& page) {
    const std::string source = active_source_;
    client_.call("source.call",
                 ipc::Json{{"source", source},
                           {"method", "browse"},
                           {"params", ipc::Json{{"page", page}}}},
                  [this](const ipc::Json& result, const std::string& code, const std::string& message) {
        if (!code.empty()) {
            fail(code, message);
            return;
        }
        apply_browse(result);
    });
}

void BaseViewModel::search(const std::string& query) {
    if (query.empty()) {
        apply_tracks(ipc::Json::array(), "Search");
        return;
    }
    const std::string source = active_source_;
    client_.call("source.call",
                 ipc::Json{{"source", source},
                           {"method", "search"},
                           {"params", ipc::Json{{"query", query}, {"kind", "tracks"}, {"offset", 0}}}},
                  [this, query](const ipc::Json& result, const std::string& code, const std::string& message) {
        if (!code.empty()) {
            fail(code, message);
            return;
        }
        apply_tracks(result.contains("tracks") ? result["tracks"] : ipc::Json::array(),
                     "Results: " + query);
    });
}

void BaseViewModel::play_at(std::size_t index) {
    if (index >= tracks_.size()) {
        return;
    }
    ipc::Json queue = ipc::Json::array();
    for (const auto& track : tracks_) {
        queue.push_back(track.json);
    }
    client_.call("player.playTracks",
                 ipc::Json{{"source", tracks_[index].source.empty() ? active_source_ : tracks_[index].source},
                           {"tracks", queue},
                           {"start", static_cast<std::int64_t>(index)}});
    show_page(model::AppPage::NowPlaying);
}

void BaseViewModel::play_queue_at(std::size_t order_index) {
    if (order_index >= queue_indices_.size()) {
        return;
    }
    const int index = queue_indices_[order_index];
    client_.call("player.jump", ipc::Json{{"index", index}});
    show_page(model::AppPage::NowPlaying);
}

namespace {

std::string quality_label(const std::string& quality) {
    if (quality == "standard") return "Standard";
    if (quality == "lossless") return "Lossless";
    return "High";
}

std::string language_label(const std::string& language) {
    if (language == "en") return "English";
    if (language == "zh-CN") return "简体中文";
    return "System";
}

} // namespace

void BaseViewModel::cycle_quality() {
    if (quality_ == "standard") quality_ = "high";
    else if (quality_ == "high") quality_ = "lossless";
    else quality_ = "standard";
    quality_subject_.set(quality_label(quality_).c_str());
    client_.call("settings.set", ipc::Json{{"quality", quality_}});
}

void BaseViewModel::toggle_normalize() {
    normalize_ = !normalize_;
    normalize_subject_.set(normalize_ ? "On" : "Off");
    client_.call("settings.set", ipc::Json{{"normalize", normalize_}});
}

void BaseViewModel::cycle_cache() {
    if (cache_mb_ == 512) cache_mb_ = 1024;
    else if (cache_mb_ == 1024) cache_mb_ = 256;
    else if (cache_mb_ == 256) cache_mb_ = 0;
    else cache_mb_ = 512;
    cache_subject_.set((std::to_string(cache_mb_) + " MB").c_str());
    client_.call("settings.set", ipc::Json{{"cacheMB", cache_mb_}});
}

void BaseViewModel::cycle_language() {
    if (language_ == "system") language_ = "en";
    else if (language_ == "en") language_ = "zh-CN";
    else language_ = "system";
    language_subject_.set(language_label(language_).c_str());
    client_.call("settings.set", ipc::Json{{"language", language_}});
}

void BaseViewModel::rescan() {
    client_.call("library.rescan", ipc::Json::object());
    status_subject_.set("Scanning…");
    model_.set_status("Scanning…");
}

void BaseViewModel::load_plugins() {
    client_.call("plugins.list", ipc::Json::object(),
                 [this](const ipc::Json& result, const std::string& code, const std::string& message) {
        if (!code.empty()) {
            fail(code, message);
            return;
        }
        std::vector<PluginItem> plugins;
        if (result.is_array()) {
            for (const auto& plugin : result) {
                PluginItem item;
                item.id = plugin.value("id", "");
                item.name = plugin.value("name", item.id);
                item.version = plugin.value("version", "");
                item.enabled = plugin.value("enabled", true);
                item.granted = plugin.value("granted", false);
                if (plugin.contains("permissions") && plugin["permissions"].contains("process")) {
                    const auto& process = plugin["permissions"]["process"];
                    item.process = process.is_array() ? !process.empty() : process.get<bool>();
                }
                plugins.push_back(std::move(item));
            }
        }
        plugins_ = std::move(plugins);
        plugins_revision_subject_.set(plugins_revision_subject_.value() + 1);
    });
}

void BaseViewModel::set_plugin_enabled(std::size_t index, bool on) {
    if (index >= plugins_.size()) {
        return;
    }
    const std::string id = plugins_[index].id;
    plugins_[index].enabled = on;
    plugins_revision_subject_.set(plugins_revision_subject_.value() + 1);
    client_.call("sources.setEnabled", ipc::Json{{"id", id}, {"on", on}},
                 [this](const ipc::Json&, const std::string& code, const std::string& message) {
        if (!code.empty()) {
            fail(code, message);
        }
        load_plugins();
    });
}

void BaseViewModel::grant_plugin(std::size_t index, bool on) {
    if (index >= plugins_.size()) {
        return;
    }
    const std::string id = plugins_[index].id;
    client_.call("plugin.grant", ipc::Json{{"id", id}, {"on", on}},
                 [this](const ipc::Json&, const std::string& code, const std::string& message) {
        if (!code.empty()) {
            fail(code, message);
        }
        load_plugins();
    });
}

void BaseViewModel::sign_in_plugin(std::size_t index) {
    if (index >= plugins_.size()) {
        return;
    }
    const std::string id = plugins_[index].id;
    prompt_qr_subject_.set("");
    prompt_text_subject_.set("Waiting for the code…");
    show_page(model::AppPage::SignIn);
    client_.call("plugin.signIn", ipc::Json{{"id", id}},
                 [this, id](const ipc::Json&, const std::string& code, const std::string& message) {
        if (!code.empty()) {
            fail(code, message);
        }
        else {
            model_.set_status("Signed in to " + id);
            status_subject_.set(model_.status().c_str());
        }
        load_plugins();
    });
}

void BaseViewModel::remove_queue_at(std::size_t order_index) {
    if (order_index >= queue_indices_.size()) {
        return;
    }
    client_.call("player.remove", ipc::Json{{"index", queue_indices_[order_index]}});
    refresh();
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
        prompt_qr_subject_.set(data.value("qr", "").c_str());
        prompt_text_subject_.set(text.c_str());
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
        load_plugins();
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
    const std::string new_id = track.is_object() ? track.value("id", "") : "";
    if (!new_id.empty() && new_id != track_id_) {
        lyrics_.clear();
        lyrics_revision_subject_.set(lyrics_revision_subject_.value() + 1);
    }
    track_id_ = new_id;
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

    if (state.contains("queue") && state["queue"].is_array()) {
        std::vector<TrackItem> raw;
        for (const auto& track : state["queue"]) {
            raw.push_back(parse_track(track, active_source_));
        }
        std::vector<int> order;
        if (state.contains("order") && state["order"].is_array()) {
            for (const auto& index : state["order"]) {
                order.push_back(index.get<int>());
            }
        }
        queue_view_.clear();
        queue_indices_.clear();
        if (!order.empty() && order.size() == raw.size()) {
            for (const int index : order) {
                if (index < 0 || index >= static_cast<int>(raw.size())) {
                    continue;
                }
                queue_view_.push_back(raw[static_cast<std::size_t>(index)]);
                queue_indices_.push_back(index);
            }
        }
        else {
            for (std::size_t i = 0; i < raw.size(); ++i) {
                queue_view_.push_back(raw[i]);
                queue_indices_.push_back(static_cast<int>(i));
            }
        }
        queue_at_ = state.value("at", -1);
        queue_revision_subject_.set(queue_revision_subject_.value() + 1);
    }

    if (state.contains("lyrics") && state["lyrics"].is_array()) {
        lyrics_.clear();
        for (const auto& line : state["lyrics"]) {
            LyricItem item;
            item.time_ms = static_cast<std::int64_t>(line.value("time", 0.0) * 1000.0);
            item.text = line.value("text", "");
            lyrics_.push_back(std::move(item));
        }
        lyrics_revision_subject_.set(lyrics_revision_subject_.value() + 1);
    }

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

void BaseViewModel::apply_browse(const ipc::Json& browse) {
    list_title_ = browse.value("title", "Songs");
    list_title_subject_.set(list_title_.c_str());
    std::vector<TrackItem> items;
    if (browse.contains("sections") && browse["sections"].is_array()) {
        for (const auto& section : browse["sections"]) {
            if (!section.contains("tracks") || !section["tracks"].is_array()) {
                continue;
            }
            for (const auto& track : section["tracks"]) {
                items.push_back(parse_track(track, active_source_));
            }
        }
    }
    tracks_ = std::move(items);
    list_revision_subject_.set(list_revision_subject_.value() + 1);
}

void BaseViewModel::apply_tracks(const ipc::Json& tracks, const std::string& title) {
    list_title_ = title;
    list_title_subject_.set(list_title_.c_str());
    std::vector<TrackItem> items;
    if (tracks.is_array()) {
        for (const auto& track : tracks) {
            items.push_back(parse_track(track, active_source_));
        }
    }
    tracks_ = std::move(items);
    list_revision_subject_.set(list_revision_subject_.value() + 1);
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
    quality_ = settings.value("quality", "high");
    quality_subject_.set(quality_label(quality_).c_str());
    normalize_ = settings.value("normalize", false);
    normalize_subject_.set(normalize_ ? "On" : "Off");
    cache_mb_ = settings.value("cacheMB", 512);
    cache_subject_.set((std::to_string(cache_mb_) + " MB").c_str());
    language_ = settings.value("language", "system");
    language_subject_.set(language_label(language_).c_str());
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
