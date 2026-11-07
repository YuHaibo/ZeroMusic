/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include "base_screen.h"
#include "track_list.h"

#include <memory>

namespace screen {

// DetailScreen is an album, artist or playlist: its name, then its tracks.
class DetailScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    std::unique_ptr<view::widgets::TrackList> list_;
};

} // namespace screen
