/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "track_list.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>
#include <cstdio>

namespace view::widgets {
namespace {

constexpr int kRows = 6;
constexpr int kRowHeight = 20;

std::string clock(std::int64_t ms) {
    if (ms <= 0) {
        return "";
    }
    const auto seconds = ms / 1000;
    char text[8];
    std::snprintf(text, sizeof(text), "%lld:%02lld",
                  static_cast<long long>(seconds / 60),
                  static_cast<long long>(seconds % 60));
    return text;
}

void revision_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* list = static_cast<TrackList*>(lv_observer_get_user_data(observer));
    if (list) {
        // The private refresh is reached through a key press with no key.
        list->handle_key(0, false);
    }
}

} // namespace

TrackList::TrackList(lv_obj_t* parent,
                     viewmodel::BaseViewModel& view_model,
                     app::AssetManager& assets,
                     Kind kind)
    : BaseWidgets(parent), view_model_(view_model), assets_(assets), kind_(kind) {}

void TrackList::build() {
    if (core_obj_) {
        return;
    }
    core_obj_ = lv_obj_create(parent_);
    lv_obj_remove_style_all(core_obj_);
    lv_obj_set_size(core_obj_, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(core_obj_, LV_OBJ_FLAG_SCROLLABLE);

    const auto colors = view::palette(view_model_.is_dark_mode());
    auto* small = assets_.load_standard_font(11);
    auto* body = assets_.load_standard_font(12);
    auto* title_font = assets_.load_standard_font(13);

    title_ = lv_label_create(core_obj_);
    lv_label_bind_text(title_, view_model_.list_title_subject(), nullptr);
    lv_obj_set_style_text_font(title_, title_font ? title_font : &lv_font_montserrat_14, 0);
    lv_obj_align(title_, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title_, colors.text, 0);

    status_ = lv_label_create(core_obj_);
    lv_obj_set_width(status_, 280);
    lv_obj_set_style_text_align(status_, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(status_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(status_, small ? small : &lv_font_montserrat_10, 0);
    lv_obj_align(status_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(status_, colors.text_disabled, 0);
    lv_label_bind_text(status_, view_model_.list_status_subject(), nullptr);
    lv_obj_add_flag(status_, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < kRows; ++i) {
        auto* row = lv_obj_create(core_obj_);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 300, kRowHeight - 2);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 10, 21 + i * kRowHeight);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row, 3, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        rows_[i] = row;

        names_[i] = lv_label_create(row);
        lv_obj_set_width(names_[i], 176);
        lv_obj_set_style_text_font(names_[i], body ? body : &lv_font_montserrat_12, 0);
        lv_label_set_long_mode(names_[i], LV_LABEL_LONG_DOT);
        lv_obj_align(names_[i], LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_text_color(names_[i], colors.text, 0);

        artists_[i] = lv_label_create(row);
        lv_obj_set_width(artists_[i], 80);
        lv_obj_set_style_text_font(artists_[i], small ? small : &lv_font_montserrat_10, 0);
        lv_label_set_long_mode(artists_[i], LV_LABEL_LONG_DOT);
        lv_obj_align(artists_[i], LV_ALIGN_LEFT_MID, 184, 0);
        lv_obj_set_style_text_color(artists_[i], colors.text_disabled, 0);

        times_[i] = lv_label_create(row);
        lv_obj_set_width(times_[i], 32);
        lv_obj_set_style_text_font(times_[i], small ? small : &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_align(times_[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(times_[i], LV_ALIGN_RIGHT_MID, -4, 0);
        lv_obj_set_style_text_color(times_[i], colors.text_disabled, 0);
    }

    if (kind_ != Kind::Queue) {
        selected_ = std::max(0, view_model_.page_selection(view_model_.current_page()));
    }
    lv_subject_add_observer_obj(kind_ == Kind::Queue ? view_model_.queue_revision_subject()
                                                     : view_model_.list_revision_subject(),
                                revision_changed,
                                core_obj_,
                                this);
    refresh();
}

void TrackList::refresh() {
    const auto colors = view::palette(view_model_.is_dark_mode());
    const auto& tracks = kind_ == Kind::Queue ? view_model_.queue() : view_model_.tracks();
    const int count = static_cast<int>(tracks.size());
    if (kind_ == Kind::Queue && view_model_.queue_at() >= 0) {
        selected_ = view_model_.queue_at();
    }
    if (status_) {
        const char* status = lv_subject_get_string(view_model_.list_status_subject());
        if (count == 0 && status && *status) {
            lv_obj_remove_flag(status_, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(status_, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (count > 0) {
        selected_ = std::clamp(selected_, 0, count - 1);
        if (selected_ < offset_) {
            offset_ = selected_;
        }
        if (selected_ >= offset_ + kRows) {
            offset_ = selected_ - kRows + 1;
        }
    }
    else {
        selected_ = 0;
        offset_ = 0;
    }

    for (int i = 0; i < kRows; ++i) {
        const int index = offset_ + i;
        const bool present = index < count;
        if (!present) {
            lv_obj_add_flag(rows_[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(rows_[i], LV_OBJ_FLAG_HIDDEN);
        const auto& track = tracks[static_cast<std::size_t>(index)];
        lv_label_set_text(names_[i], track.name.c_str());
        const bool on = index == selected_;
        lv_label_set_long_mode(names_[i], on ? LV_LABEL_LONG_SCROLL_CIRCULAR : LV_LABEL_LONG_DOT);
        lv_label_set_text(artists_[i], track.artist.c_str());
        const auto time = clock(track.duration_ms);
        lv_label_set_text(times_[i], time.c_str());
        lv_obj_set_style_bg_opa(rows_[i], on ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(rows_[i], colors.primary, 0);
        lv_obj_set_style_border_width(rows_[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_side(rows_[i], LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_color(rows_[i], colors.primary, 0);
    }
}

bool TrackList::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    const int count = static_cast<int>((kind_ == Kind::Queue ? view_model_.queue() : view_model_.tracks()).size());
    if (key == 0) {
        refresh();
        return true;
    }
    if (count == 0) {
        return false;
    }
    switch (key) {
        case LV_KEY_UP:
            selected_ = (selected_ + count - 1) % count;
            if (kind_ != Kind::Queue) {
                view_model_.set_page_selection(view_model_.current_page(), selected_);
            }
            refresh();
            return true;
        case LV_KEY_DOWN:
            selected_ = (selected_ + 1) % count;
            if (kind_ != Kind::Queue) {
                view_model_.set_page_selection(view_model_.current_page(), selected_);
            }
            refresh();
            return true;
        case LV_KEY_BACKSPACE:
            if (kind_ == Kind::Queue) {
                view_model_.remove_queue_at(static_cast<std::size_t>(selected_));
                return true;
            }
            return false;
        case LV_KEY_ENTER:
            if (kind_ == Kind::Queue) {
                view_model_.play_queue_at(static_cast<std::size_t>(selected_));
            }
            else {
                view_model_.play_at(static_cast<std::size_t>(selected_));
            }
            return true;
        default:
            return false;
    }
}

} // namespace view::widgets
