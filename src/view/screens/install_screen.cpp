/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "install_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>

namespace screen {
namespace {

void available_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<InstallScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

} // namespace

void InstallScreen::build_content(lv_obj_t* content) {
    content_ = content;
    build_list();
}

void InstallScreen::build_list() {
    mode_ = Mode::List;
    lv_obj_clean(content_);
    rows_ = {};
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content_);
    lv_label_set_text(title, "Install");
    lv_obj_set_style_text_font(title, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text, 0);

    status_ = lv_label_create(content_);
    lv_obj_set_width(status_, 215);
    lv_obj_set_style_text_align(status_, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_long_mode(status_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(status_, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(status_, LV_ALIGN_TOP_RIGHT, -8, 3);
    lv_obj_set_style_text_color(status_, colors.primary, 0);
    lv_label_bind_text(status_, view_model().status_subject(), nullptr);

    for (int i = 0; i < 6; ++i) {
        auto* row = lv_obj_create(content_);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 300, 16);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 10, 20 + i * 18);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row, 3, 0);
        rows_[i] = row;

        repos_[i] = lv_label_create(row);
        lv_obj_set_width(repos_[i], 288);
        lv_obj_set_style_text_font(repos_[i], body ? body : &lv_font_montserrat_12, 0);
        lv_label_set_long_mode(repos_[i], LV_LABEL_LONG_DOT);
        lv_obj_align(repos_[i], LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_text_color(repos_[i], colors.text, 0);
        descriptions_[i] = nullptr;
    }

    empty_ = lv_label_create(content_);
    lv_label_set_text(empty_, "Looking for plugins...");
    lv_obj_set_style_text_font(empty_, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(empty_, LV_ALIGN_TOP_LEFT, 10, 30);
    lv_obj_set_style_text_color(empty_, colors.text_disabled, 0);

    auto* hint = lv_label_create(content_);
    lv_label_set_text(hint, "UP/DOWN choose  ENTER install  T type  ESC back");
    lv_obj_set_style_text_font(hint, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(hint, 300);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_color(hint, colors.text_disabled, 0);

    lv_subject_add_observer_obj(view_model().available_plugins_revision_subject(), available_changed, content_, this);
    view_model().load_available_plugins();
    refresh();
}

void InstallScreen::refresh() {
    const auto colors = view::palette(view_model().is_dark_mode());
    const auto& plugins = view_model().available_plugins();
    const int count = static_cast<int>(plugins.size());
    lv_obj_add_flag(empty_, LV_OBJ_FLAG_HIDDEN);
    if (count == 0) {
        lv_obj_remove_flag(empty_, LV_OBJ_FLAG_HIDDEN);
        for (auto* row : rows_) {
            if (row) {
                lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }
    selected_ = std::clamp(selected_, 0, count - 1);
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
        const auto& plugin = plugins[static_cast<std::size_t>(index)];
        lv_label_set_text(repos_[i], plugin.repo.c_str());
        const bool on = index == selected_;
        lv_label_set_long_mode(repos_[i], on ? LV_LABEL_LONG_SCROLL_CIRCULAR : LV_LABEL_LONG_DOT);
        lv_obj_set_style_bg_opa(rows_[i], on ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(rows_[i], colors.primary, 0);
        lv_obj_set_style_border_width(rows_[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_side(rows_[i], LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_color(rows_[i], colors.primary, 0);
    }
}

void InstallScreen::build_manual() {
    mode_ = Mode::Manual;
    lv_obj_clean(content_);
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content_);
    lv_label_set_text(title, "Install from GitHub");
    lv_obj_set_style_text_font(title, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text, 0);

    field_ = lv_label_create(content_);
    lv_obj_set_width(field_, 300);
    lv_obj_set_style_text_font(field_, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(field_, LV_ALIGN_TOP_LEFT, 10, 26);
    lv_obj_set_style_text_color(field_, colors.primary, 0);

    auto* hint = lv_label_create(content_);
    lv_label_set_text(hint, "owner/repo, ENTER install, ESC back");
    lv_obj_set_style_text_font(hint, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(hint, 300);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_text_color(hint, colors.text_disabled, 0);

    show();
}

void InstallScreen::show() {
    lv_label_set_text(field_, repo_.empty() ? "owner/repo" : (repo_ + "_").c_str());
}

bool InstallScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (mode_ == Mode::List) {
        if (key == LV_KEY_ESC) {
            return false;
        }
        if (key == 't' || key == 'T') {
            build_manual();
            return true;
        }
        const int count = static_cast<int>(view_model().available_plugins().size());
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
                refresh();
                return true;
            case LV_KEY_DOWN:
                selected_ = (selected_ + 1) % count;
                refresh();
                return true;
            case LV_KEY_ENTER:
                view_model().install_plugin(view_model().available_plugins()[static_cast<std::size_t>(selected_)].repo);
                return true;
            default:
                return false;
        }
    }

    if (key == LV_KEY_ESC) {
        build_list();
        return true;
    }
    if (key == LV_KEY_ENTER) {
        if (!repo_.empty()) {
            view_model().install_plugin(repo_);
        }
        return true;
    }
    if (key == LV_KEY_BACKSPACE) {
        if (!repo_.empty()) {
            repo_.pop_back();
        }
        show();
        return true;
    }
    if (key == '/' || key == '\\') {
        if (repo_.size() < 96) {
            repo_.push_back('/');
            show();
        }
        return true;
    }
    if (key >= 32 && key < 127 && repo_.size() < 96) {
        repo_.push_back(static_cast<char>(key));
        show();
        return true;
    }
    return false;
}

} // namespace screen
