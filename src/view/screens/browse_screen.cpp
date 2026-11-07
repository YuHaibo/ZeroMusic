/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "browse_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace screen {
namespace {

struct Entry {
    const char* label;
    viewmodel::CollectionKind kind;
};

constexpr std::array<Entry, 3> kCollections = {{
    {"Albums", viewmodel::CollectionKind::Albums},
    {"Artists", viewmodel::CollectionKind::Artists},
    {"Playlists", viewmodel::CollectionKind::Playlists},
}};

constexpr std::array<const char*, 5> kMenu = {{"Songs", "Albums", "Artists", "Playlists", "Liked"}};

void connected_changed(lv_observer_t* observer, lv_subject_t* subject) {
    auto* screen = static_cast<BrowseScreen*>(lv_observer_get_user_data(observer));
    if (screen && lv_subject_get_int(subject)) {
        screen->reload();
    }
}

void collection_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<BrowseScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

} // namespace

void BrowseScreen::build_content(lv_obj_t* content) {
    content_ = content;
    if (const char* start = std::getenv("ZERO_MUSIC_BROWSE"); start && *start) {
        const std::string page(start);
        if (page == "songs") {
            menu_selected_ = 0;
            build_songs();
            return;
        }
        if (page == "liked") {
            menu_selected_ = 4;
            build_liked();
            return;
        }
        menu_selected_ = page == "artists" ? 2 : (page == "playlists" ? 3 : 1);
        build_collection();
        return;
    }
    build_menu();
}

void BrowseScreen::build_menu() {
    mode_ = Mode::Menu;
    list_.reset();
    lv_obj_clean(content_);
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(13);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content_);
    lv_label_set_text(title, "Browse");
    lv_obj_set_style_text_font(title, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text_disabled, 0);

    for (int i = 0; i < static_cast<int>(kMenu.size()); ++i) {
        auto* row = lv_obj_create(content_);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 300, 22);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 10, 20 + i * 23);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row, 3, 0);
        menu_rows_[i] = row;
        auto* label = lv_label_create(row);
        lv_label_set_text(label, kMenu[i]);
        lv_obj_set_style_text_font(label, body ? body : &lv_font_montserrat_14, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 8, 0);
        lv_obj_set_style_text_color(label, colors.text, 0);
    }
    refresh_menu();
}

void BrowseScreen::refresh_menu() {
    const auto colors = view::palette(view_model().is_dark_mode());
    for (std::size_t i = 0; i < menu_rows_.size(); ++i) {
        const bool on = static_cast<int>(i) == menu_selected_;
        lv_obj_set_style_bg_opa(menu_rows_[i], on ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(menu_rows_[i], colors.primary, 0);
        lv_obj_set_style_border_width(menu_rows_[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_side(menu_rows_[i], LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_color(menu_rows_[i], colors.primary, 0);
    }
}

void BrowseScreen::build_songs() {
    mode_ = Mode::Songs;
    lv_obj_clean(content_);
    menu_rows_ = {};
    list_ = std::make_unique<view::widgets::TrackList>(content_, view_model(), assets());
    list_->build();
    lv_subject_add_observer_obj(view_model().sources_revision_subject(), connected_changed, content_, this);
    view_model().browse("songs");
}

void BrowseScreen::build_liked() {
    mode_ = Mode::Liked;
    lv_obj_clean(content_);
    menu_rows_ = {};
    list_ = std::make_unique<view::widgets::TrackList>(content_, view_model(), assets());
    list_->build();
    lv_subject_add_observer_obj(view_model().sources_revision_subject(), connected_changed, content_, this);
    view_model().load_liked();
}

void BrowseScreen::build_collection() {
    mode_ = Mode::Collection;
    list_.reset();
    lv_obj_clean(content_);
    menu_rows_ = {};
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    const int kind_index = menu_selected_ - 1;
    const char* title_text = kCollections[std::clamp(kind_index, 0, 2)].label;
    auto* title = lv_label_create(content_);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_font(title, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text_disabled, 0);

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
        lv_obj_set_style_text_color(subtitles_[i], colors.text_disabled, 0);
        lv_obj_align(subtitles_[i], LV_ALIGN_RIGHT_MID, -6, 0);
    }

    lv_subject_add_observer_obj(view_model().collection_revision_subject(), collection_changed, content_, this);
    lv_subject_add_observer_obj(view_model().sources_revision_subject(), connected_changed, content_, this);
    view_model().load_collection(kCollections[std::clamp(kind_index, 0, 2)].kind);
    refresh_collection();
}

void BrowseScreen::refresh_collection() {
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
    if (std::getenv("ZERO_MUSIC_OPEN_FIRST") && !auto_opened_ && count > 0) {
        auto_opened_ = true;
        view_model().open_collection_item(0);
    }
}

void BrowseScreen::reload() {
    if (mode_ == Mode::Songs) {
        view_model().browse("songs");
    }
    else if (mode_ == Mode::Liked) {
        view_model().load_liked();
    }
    else if (mode_ == Mode::Collection) {
        menu_selected_ = std::clamp(menu_selected_, 1, 3);
        view_model().load_collection(kCollections[menu_selected_ - 1].kind);
    }
}

bool BrowseScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (key == 0) {
        if (mode_ == Mode::Menu) {
            refresh_menu();
        }
        else if (mode_ == Mode::Collection) {
            refresh_collection();
        }
        return true;
    }
    if (key == LV_KEY_ESC) {
        if (mode_ != Mode::Menu) {
            menu_selected_ = 0;
            build_menu();
            return true;
        }
        return false;
    }
    if (mode_ == Mode::Menu) {
        switch (key) {
            case LV_KEY_UP:
                menu_selected_ = (menu_selected_ + static_cast<int>(kMenu.size()) - 1) % static_cast<int>(kMenu.size());
                refresh_menu();
                return true;
            case LV_KEY_DOWN:
                menu_selected_ = (menu_selected_ + 1) % static_cast<int>(kMenu.size());
                refresh_menu();
                return true;
            case LV_KEY_ENTER:
                if (menu_selected_ == 0) {
                    build_songs();
                }
                else if (menu_selected_ == 4) {
                    build_liked();
                }
                else {
                    build_collection();
                }
                return true;
            default:
                return false;
        }
    }
    if (mode_ == Mode::Songs || mode_ == Mode::Liked) {
        return list_ && list_->handle_key(key, false);
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
        case LV_KEY_ENTER:
            if (count > 0) {
                view_model().open_collection_item(static_cast<std::size_t>(selected_));
            }
            return true;
        default:
            return false;
    }
}

} // namespace screen
