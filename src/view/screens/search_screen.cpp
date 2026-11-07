/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "search_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>
#include <cstdlib>

namespace screen {
namespace {

constexpr std::array<const char*, 4> kKinds = {{"tracks", "albums", "artists", "playlists"}};

viewmodel::CollectionKind to_kind(int kind) {
    switch (kind) {
        case 1:
            return viewmodel::CollectionKind::Albums;
        case 2:
            return viewmodel::CollectionKind::Artists;
        case 3:
            return viewmodel::CollectionKind::Playlists;
        default:
            return viewmodel::CollectionKind::Tracks;
    }
}

void sources_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<SearchScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->reload();
    }
}

void results_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<SearchScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

} // namespace

void SearchScreen::build_content(lv_obj_t* content) {
    content_ = content;
    if (const char* query = std::getenv("ZERO_MUSIC_SEARCH"); query && *query) {
        query_ = query;
        editing_ = false;
    }
    if (const char* kind = std::getenv("ZERO_MUSIC_SEARCH_KIND"); kind && *kind) {
        const std::string wanted(kind);
        kind_ = wanted == "albums" ? 1 : (wanted == "artists" ? 2 : (wanted == "playlists" ? 3 : 0));
    }
    if (kind_ == 0) {
        build_tracks();
    }
    else {
        build_collection();
    }
    if (!query_.empty()) {
        search_current();
    }
}

void SearchScreen::build_tracks() {
    list_.reset();
    lv_obj_clean(content_);
    rows_ = {};
    header_ = nullptr;
    list_ = std::make_unique<view::widgets::TrackList>(content_, view_model(), assets());
    list_->build();
    lv_subject_add_observer_obj(view_model().sources_revision_subject(), sources_changed, content_, this);
    show_query();
}

void SearchScreen::build_collection() {
    list_.reset();
    lv_obj_clean(content_);
    rows_ = {};
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    header_ = lv_label_create(content_);
    lv_obj_set_width(header_, 300);
    lv_obj_set_style_text_font(header_, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(header_, LV_LABEL_LONG_DOT);
    lv_obj_align(header_, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(header_, colors.primary, 0);

    for (int i = 0; i < 6; ++i) {
        auto* row = lv_obj_create(content_);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 300, 20);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 10, 20 + i * 21);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row, 3, 0);
        rows_[i] = row;

        titles_[i] = lv_label_create(row);
        lv_obj_set_width(titles_[i], 230);
        lv_obj_set_style_text_font(titles_[i], body ? body : &lv_font_montserrat_12, 0);
        lv_label_set_long_mode(titles_[i], LV_LABEL_LONG_DOT);
        lv_obj_align(titles_[i], LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_text_color(titles_[i], colors.text, 0);

        subtitles_[i] = lv_label_create(row);
        lv_obj_set_width(subtitles_[i], 60);
        lv_obj_set_style_text_align(subtitles_[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_font(subtitles_[i], caption ? caption : &lv_font_montserrat_10, 0);
        lv_obj_align(subtitles_[i], LV_ALIGN_RIGHT_MID, -6, 0);
        lv_obj_set_style_text_color(subtitles_[i], colors.text_disabled, 0);
    }

    lv_subject_add_observer_obj(view_model().collection_revision_subject(), results_changed, content_, this);
    lv_subject_add_observer_obj(view_model().sources_revision_subject(), sources_changed, content_, this);
    show_query();
    refresh_collection();
}

void SearchScreen::refresh_collection() {
    const auto colors = view::palette(view_model().is_dark_mode());
    const auto& items = view_model().collection();
    const int count = static_cast<int>(items.size());
    if (selected_ >= count) {
        selected_ = std::max(0, count - 1);
    }
    if (selected_ < offset_) {
        offset_ = selected_;
    }
    if (selected_ >= offset_ + 6) {
        offset_ = selected_ - 5;
    }
    for (int i = 0; i < 6; ++i) {
        const int index = offset_ + i;
        if (index >= count) {
            lv_obj_add_flag(rows_[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(rows_[i], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(titles_[i], items[static_cast<std::size_t>(index)].title.c_str());
        lv_label_set_text(subtitles_[i], items[static_cast<std::size_t>(index)].subtitle.c_str());
        const bool on = index == selected_;
        lv_obj_set_style_bg_opa(rows_[i], on ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(rows_[i], colors.primary, 0);
        lv_obj_set_style_border_width(rows_[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_side(rows_[i], LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_color(rows_[i], colors.primary, 0);
    }
}

void SearchScreen::show_query() {
    std::string title = "Search: " + query_ + "  [" + kKinds[kind_] + "]";
    if (editing_) {
        title += "  (Enter)";
    }
    if (header_) {
        lv_label_set_text(header_, title.c_str());
    }
    else {
        lv_subject_copy_string(view_model().list_title_subject(), title.c_str());
    }
}

void SearchScreen::search_current() {
    if (query_.empty()) {
        return;
    }
    view_model().search(query_, to_kind(kind_));
}

void SearchScreen::reload() {
    if (!query_.empty()) {
        search_current();
    }
}

bool SearchScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (key == LV_KEY_NEXT) {
        kind_ = (kind_ + 1) % 4;
        if (kind_ == 0) {
            build_tracks();
        }
        else {
            build_collection();
        }
        if (!query_.empty()) {
            search_current();
        }
        return true;
    }
    if (key == LV_KEY_ENTER) {
        if (editing_) {
            editing_ = false;
            search_current();
            show_query();
            return true;
        }
        if (kind_ == 0) {
            return list_ && list_->handle_key(key, false);
        }
        if (selected_ < static_cast<int>(view_model().collection().size())) {
            view_model().open_collection_item(static_cast<std::size_t>(selected_));
        }
        return true;
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
    if (kind_ == 0) {
        return list_ && list_->handle_key(key, false);
    }
    if (key == 0) {
        refresh_collection();
        return true;
    }
    const int count = static_cast<int>(view_model().collection().size());
    switch (key) {
        case LV_KEY_UP:
            if (count > 0) {
                selected_ = (selected_ + count - 1) % count;
                refresh_collection();
            }
            return true;
        case LV_KEY_DOWN:
            if (count > 0) {
                selected_ = (selected_ + 1) % count;
                refresh_collection();
            }
            return true;
        default:
            return false;
    }
}

} // namespace screen
