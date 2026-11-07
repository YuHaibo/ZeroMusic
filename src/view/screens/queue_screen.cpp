/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "queue_screen.h"

#include "asset_manager.h"

namespace screen {

void QueueScreen::build_content(lv_obj_t* content) {
    lv_subject_copy_string(view_model().list_title_subject(), "Queue");
    list_ = std::make_unique<view::widgets::TrackList>(content,
                                                       view_model(),
                                                       assets(),
                                                       view::widgets::TrackList::Kind::Queue);
    list_->build();
    view_model().refresh();
}

bool QueueScreen::handle_key(std::uint32_t key, bool long_pressed) {
    return list_ && list_->handle_key(key, long_pressed);
}

} // namespace screen
