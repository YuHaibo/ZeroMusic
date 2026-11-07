/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "browse_screen.h"

#include "asset_manager.h"

namespace screen {

void BrowseScreen::build_content(lv_obj_t* content) {
    list_ = std::make_unique<view::widgets::TrackList>(content, view_model(), assets());
    list_->build();
    view_model().browse("songs");
}

bool BrowseScreen::handle_key(std::uint32_t key, bool long_pressed) {
    return list_ && list_->handle_key(key, long_pressed);
}

} // namespace screen
