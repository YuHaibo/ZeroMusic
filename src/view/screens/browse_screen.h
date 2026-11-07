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

namespace screen {

// BrowseScreen starts with the kinds the active source has, then lists songs,
// albums, artists or playlists. Enter on a collection opens its detail page.
class BrowseScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;
    void reload();

protected:
    void build_content(lv_obj_t* content) override;

private:
    enum class Mode {
        Menu,
        Home,
        Songs,
        Collection,
        Liked,
    };

    void build_menu();
    void build_home();
    void build_songs();
    void build_collection();
    void build_liked();
    void refresh_menu();
    void refresh_collection();

    Mode mode_ = Mode::Menu;
    lv_obj_t* content_{nullptr};
    std::unique_ptr<view::widgets::TrackList> list_;
    std::array<lv_obj_t*, 6> menu_rows_{};
    std::array<lv_obj_t*, 6> rows_{};
    std::array<lv_obj_t*, 6> titles_{};
    std::array<lv_obj_t*, 6> subtitles_{};
    lv_obj_t* status_{nullptr};
    int menu_selected_ = 0;
    int selected_ = 0;
    int offset_ = 0;
    bool auto_opened_ = false;
};

} // namespace screen
