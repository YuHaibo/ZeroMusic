/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"
#include "track_list.h"

#include <memory>

namespace screen {

// BrowseScreen lists what the active source has, starting with its songs.
class BrowseScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    std::unique_ptr<view::widgets::TrackList> list_;
};

} // namespace screen
