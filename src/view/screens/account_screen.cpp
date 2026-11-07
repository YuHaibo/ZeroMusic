/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "account_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

namespace screen {

void AccountScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* big = assets().load_standard_font(16);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content);
    lv_label_set_text(title, "Account");
    lv_obj_set_style_text_font(title, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text_disabled, 0);

    auto* name = lv_label_create(content);
    lv_label_bind_text(name, view_model().account_subject(), nullptr);
    lv_obj_set_style_text_font(name, big ? big : &lv_font_montserrat_16, 0);
    lv_obj_align(name, LV_ALIGN_TOP_LEFT, 10, 28);
    lv_obj_set_style_text_color(name, colors.text, 0);

    auto* hint = lv_label_create(content);
    lv_label_set_text(hint, "X sign out   ESC back");
    lv_obj_set_style_text_font(hint, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 10, -8);
    lv_obj_set_style_text_color(hint, colors.text_disabled, 0);
}

bool AccountScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (key == 'x' || key == 'X') {
        view_model().sign_out_account();
        return true;
    }
    return false;
}

} // namespace screen
