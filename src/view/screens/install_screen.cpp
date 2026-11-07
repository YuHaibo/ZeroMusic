/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "install_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

namespace screen {

void InstallScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content);
    lv_label_set_text(title, "Install plugin");
    lv_obj_set_style_text_font(title, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text, 0);

    field_ = lv_label_create(content);
    lv_obj_set_width(field_, 300);
    lv_obj_set_style_text_font(field_, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(field_, LV_ALIGN_TOP_LEFT, 10, 26);
    lv_obj_set_style_text_color(field_, colors.primary, 0);

    auto* hint = lv_label_create(content);
    lv_label_set_text(hint, "GitHub owner/repo, ENTER install, ESC back");
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
    if (key >= 32 && key < 127 && repo_.size() < 96) {
        repo_.push_back(static_cast<char>(key));
        show();
        return true;
    }
    return false;
}

} // namespace screen
