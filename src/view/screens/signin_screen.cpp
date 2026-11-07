/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "signin_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <cstring>

namespace screen {
namespace {

void prompt_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<SignInScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        // The public read of the subjects happens in refresh, through the key.
        screen->handle_key(0, false);
    }
}

} // namespace

void SignInScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* title_font = assets().load_standard_font(16);

    auto* title = lv_label_create(content);
    lv_label_set_text(title, "Sign in");
    lv_obj_set_style_text_font(title, title_font ? title_font : &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 146, 8);
    lv_obj_set_style_text_color(title, colors.text, 0);

    text_ = lv_label_create(content);
    lv_label_set_text(text_, "Start sign-in from Plugins");
    lv_obj_set_width(text_, 160);
    lv_label_set_long_mode(text_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(text_, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(text_, LV_ALIGN_TOP_LEFT, 146, 32);
    lv_obj_set_style_text_color(text_, colors.text_disabled, 0);

    qr_ = lv_qrcode_create(content);
    lv_qrcode_set_size(qr_, 116);
    lv_qrcode_set_dark_color(qr_, colors.text);
    lv_qrcode_set_light_color(qr_, colors.surface);
    lv_obj_align(qr_, LV_ALIGN_LEFT_MID, 10, 0);

    lv_subject_add_observer_obj(view_model().prompt_qr_subject(), prompt_changed, content, this);
    lv_subject_add_observer_obj(view_model().prompt_text_subject(), prompt_changed, content, this);
    refresh();
}

void SignInScreen::refresh() {
    const char* qr = lv_subject_get_string(view_model().prompt_qr_subject());
    const char* text = lv_subject_get_string(view_model().prompt_text_subject());
    lv_label_set_text(text_, text && *text ? text : "Scan the code");
    if (qr && *qr) {
        lv_obj_remove_flag(qr_, LV_OBJ_FLAG_HIDDEN);
        lv_qrcode_update(qr_, qr, static_cast<uint32_t>(std::strlen(qr)));
    }
    else {
        lv_obj_add_flag(qr_, LV_OBJ_FLAG_HIDDEN);
    }
}

bool SignInScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (key == 0) {
        refresh();
        return true;
    }
    return false;
}

} // namespace screen
