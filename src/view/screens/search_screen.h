/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"
#include "track_list.h"

#include <memory>
#include <string>

namespace screen {

// SearchScreen takes a query from the keys, asks the active source, and shows
// what it found. Enter asks; a second Enter plays the focused song.
class SearchScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    void show_query();

    std::unique_ptr<view::widgets::TrackList> list_;
    std::string query_;
    bool editing_ = true;
};

} // namespace screen
