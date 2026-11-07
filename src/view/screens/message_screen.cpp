/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "message_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <utility>

namespace screen {

MessageScreen::MessageScreen(viewmodel::BaseViewModel& view_model,
                             app::AssetManager& assets,
                             std::string title,
                             std::string message)
    : BaseScreen(view_model, assets), title_(std::move(title)), message_(std::move(message)) {}

void MessageScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* big = assets().load_standard_font(18);
    auto* body = assets().load_standard_font(12);

    auto* heading = lv_label_create(content);
    lv_label_set_text(heading, title_.c_str());
    lv_obj_set_style_text_font(heading, big ? big : &lv_font_montserrat_18, 0);
    lv_obj_align(heading, LV_ALIGN_TOP_LEFT, 14, 14);
    lv_obj_set_style_text_color(heading, colors.text, 0);

    auto* text = lv_label_create(content);
    lv_label_set_text(text, message_.c_str());
    lv_obj_set_style_text_font(text, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_set_width(text, 292);
    lv_label_set_long_mode(text, LV_LABEL_LONG_WRAP);
    lv_obj_align(text, LV_ALIGN_TOP_LEFT, 14, 44);
    lv_obj_set_style_text_color(text, colors.text_disabled, 0);

    auto* hint = lv_label_create(content);
    lv_label_set_text(hint, "ESC back");
    lv_obj_set_style_text_font(hint, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 14, -8);
    lv_obj_set_style_text_color(hint, colors.primary, 0);
}

} // namespace screen
