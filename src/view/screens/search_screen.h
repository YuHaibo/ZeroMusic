/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"
#include "track_list.h"

#include <array>
#include <memory>
#include <string>

namespace screen {

// SearchScreen takes a query from the keys and asks the active source. Tab
// changes what it looks for: tracks, albums, artists or playlists.
class SearchScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;
    void reload();

protected:
    void build_content(lv_obj_t* content) override;

private:
    void show_query();
    void search_current();
    void build_tracks();
    void build_collection();
    void refresh_collection();

    lv_obj_t* content_{nullptr};
    lv_obj_t* header_{nullptr};
    std::unique_ptr<view::widgets::TrackList> list_;
    std::array<lv_obj_t*, 6> rows_{};
    std::array<lv_obj_t*, 6> titles_{};
    std::array<lv_obj_t*, 6> subtitles_{};
    std::string query_;
    bool editing_ = true;
    int kind_ = 0; // 0 tracks, 1 albums, 2 artists, 3 playlists
    int selected_ = 0;
    int offset_ = 0;
};

} // namespace screen
