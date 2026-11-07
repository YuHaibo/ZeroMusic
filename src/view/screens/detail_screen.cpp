/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "detail_screen.h"

#include "asset_manager.h"

namespace screen {

void DetailScreen::build_content(lv_obj_t* content) {
    list_ = std::make_unique<view::widgets::TrackList>(content, view_model(), assets());
    list_->build();
}

bool DetailScreen::handle_key(std::uint32_t key, bool long_pressed) {
    return list_ && list_->handle_key(key, long_pressed);
}

} // namespace screen
