/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"
#include "cover_view.h"

#include <array>
#include <memory>

namespace screen {

// NowPlayingScreen shows the current track, cover, progress, volume and the
// current lyric, with the keys that control it.
class NowPlayingScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    void refresh_lyrics();

    lv_obj_t* progress_{nullptr};
    lv_obj_t* state_label_{nullptr};
    lv_obj_t* volume_{nullptr};
    std::unique_ptr<view::widgets::CoverView> cover_;
    std::array<lv_obj_t*, 2> lyric_lines_{};
};

} // namespace screen
