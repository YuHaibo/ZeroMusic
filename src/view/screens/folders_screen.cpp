/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "folders_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>

namespace screen {
namespace {

void folders_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<FoldersScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

} // namespace

void FoldersScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content);
    lv_label_set_text(title, "Music folders");
    lv_obj_set_style_text_font(title, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text_disabled, 0);

    for (int i = 0; i < 6; ++i) {
        auto* row = lv_obj_create(content);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 300, 18);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 10, 20 + i * 19);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row, 3, 0);
        rows_[i] = row;
        names_[i] = lv_label_create(row);
        lv_obj_set_width(names_[i], 288);
        lv_obj_set_style_text_font(names_[i], body ? body : &lv_font_montserrat_12, 0);
        lv_label_set_long_mode(names_[i], LV_LABEL_LONG_DOT);
        lv_obj_align(names_[i], LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_text_color(names_[i], colors.text, 0);
    }

    empty_ = lv_label_create(content);
    lv_label_set_text(empty_, "No folders; A adds ~/Music");
    lv_obj_set_style_text_font(empty_, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(empty_, LV_ALIGN_TOP_LEFT, 10, 30);
    lv_obj_set_style_text_color(empty_, colors.text_disabled, 0);

    auto* hint = lv_label_create(content);
    lv_label_set_text(hint, "A add ~/Music  D remove  R rescan  ESC back");
    lv_obj_set_style_text_font(hint, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(hint, 300);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_color(hint, colors.text_disabled, 0);

    lv_subject_add_observer_obj(view_model().folders_subject(), folders_changed, content, this);
    view_model().load_folders();
    refresh();
}

void FoldersScreen::refresh() {
    const auto colors = view::palette(view_model().is_dark_mode());
    const auto& folders = view_model().folders();
    const int count = static_cast<int>(folders.size());
    if (count == 0) {
        lv_obj_remove_flag(empty_, LV_OBJ_FLAG_HIDDEN);
        for (auto* row : rows_) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    lv_obj_add_flag(empty_, LV_OBJ_FLAG_HIDDEN);
    selected_ = std::clamp(selected_, 0, count - 1);
    if (selected_ < offset_) offset_ = selected_;
    if (selected_ >= offset_ + 6) offset_ = selected_ - 5;
    for (int i = 0; i < 6; ++i) {
        const int index = offset_ + i;
        if (index >= count) {
            lv_obj_add_flag(rows_[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(rows_[i], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(names_[i], folders[static_cast<std::size_t>(index)].c_str());
        const bool on = index == selected_;
        lv_label_set_long_mode(names_[i], on ? LV_LABEL_LONG_SCROLL_CIRCULAR : LV_LABEL_LONG_DOT);
        lv_obj_set_style_bg_opa(rows_[i], on ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(rows_[i], colors.primary, 0);
        lv_obj_set_style_border_width(rows_[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_side(rows_[i], LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_color(rows_[i], colors.primary, 0);
    }
}

bool FoldersScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) return false;
    if (key == 0) { refresh(); return true; }
    if (key == 'a' || key == 'A') { view_model().add_music_folder(); return true; }
    if (key == 'r' || key == 'R') { view_model().rescan(); return true; }
    if (key == 'd' || key == 'D') {
        view_model().remove_folder(static_cast<std::size_t>(selected_));
        return true;
    }
    const int count = static_cast<int>(view_model().folders().size());
    if (key == LV_KEY_UP && count > 0) {
        selected_ = (selected_ + count - 1) % count;
        refresh();
        return true;
    }
    if (key == LV_KEY_DOWN && count > 0) {
        selected_ = (selected_ + 1) % count;
        refresh();
        return true;
    }
    return false;
}

} // namespace screen
