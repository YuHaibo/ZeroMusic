/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "search_screen.h"

#include "asset_manager.h"

#include <cstdlib>

namespace screen {

void SearchScreen::build_content(lv_obj_t* content) {
    list_ = std::make_unique<view::widgets::TrackList>(content, view_model(), assets());
    list_->build();
    if (const char* query = std::getenv("ZERO_MUSIC_SEARCH"); query && *query) {
        query_ = query;
        editing_ = false;
        lv_subject_copy_string(view_model().list_title_subject(), ("Results: " + query_).c_str());
        view_model().search(query_);
        return;
    }
    show_query();
}

void SearchScreen::show_query() {
    std::string title = "Search: " + query_;
    title += "  (Enter)";
    lv_subject_copy_string(view_model().list_title_subject(), title.c_str());
}

bool SearchScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (key == LV_KEY_ENTER) {
        if (editing_) {
            editing_ = false;
            view_model().search(query_);
            const std::string title = query_.empty() ? "Search" : ("Results: " + query_);
            lv_subject_copy_string(view_model().list_title_subject(), title.c_str());
            return true;
        }
        return list_ && list_->handle_key(key, false);
    }
    if (key == LV_KEY_BACKSPACE) {
        if (!query_.empty()) {
            query_.pop_back();
        }
        editing_ = true;
        show_query();
        return true;
    }
    if (key >= 32 && key < 127) {
        if (query_.size() < 48) {
            query_.push_back(static_cast<char>(key));
        }
        editing_ = true;
        show_query();
        return true;
    }
    return list_ && list_->handle_key(key, long_pressed);
}

} // namespace screen
