/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "help_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <array>

namespace screen {
namespace {

struct Row {
    const char* key;
    const char* action;
};

constexpr std::array<Row, 7> kRows = {{
    {"SPACE / 6", "Play or pause"},
    {"5 / 7", "Previous / next track"},
    {"4 / 9 / 0", "Now Playing / Queue / Lyrics"},
    {"8 / L", "Repeat mode / Like"},
    {"F X Z C", "Arrows; type in a field"},
    {"ESC", "Back; hold to exit"},
    {"Fn+H / Fn+J", "Help / Screenshot"},
}};

} // namespace

void HelpScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(11);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content);
    lv_label_set_text(title, "Keyboard help");
    lv_obj_set_style_text_font(title, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text, 0);

    for (int i = 0; i < static_cast<int>(kRows.size()); ++i) {
        auto* key = lv_label_create(content);
        lv_label_set_text(key, kRows[i].key);
        lv_obj_set_width(key, 86);
        lv_obj_set_style_text_font(key, caption ? caption : &lv_font_montserrat_10, 0);
        lv_obj_align(key, LV_ALIGN_TOP_LEFT, 10, 20 + i * 17);
        lv_obj_set_style_text_color(key, colors.primary, 0);

        auto* action = lv_label_create(content);
        lv_label_set_text(action, kRows[i].action);
        lv_obj_set_width(action, 210);
        lv_label_set_long_mode(action, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(action, caption ? caption : &lv_font_montserrat_10, 0);
        lv_obj_align(action, LV_ALIGN_TOP_LEFT, 100, 20 + i * 17);
        lv_obj_set_style_text_color(action, colors.text, 0);
    }

    auto* footer = lv_label_create(content);
    lv_label_set_text(footer, "HELP / ESC close   PRSC screenshot");
    lv_obj_set_style_text_font(footer, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(footer, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(footer, 300);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_color(footer, colors.text_disabled, 0);
}

} // namespace screen
