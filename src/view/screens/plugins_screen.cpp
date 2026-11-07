/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "plugins_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>

namespace screen {
namespace {

void plugins_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<PluginsScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

} // namespace

void PluginsScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content);
    lv_label_set_text(title, "Plugins");
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
        lv_obj_set_width(names_[i], 190);
        lv_obj_set_style_text_font(names_[i], body ? body : &lv_font_montserrat_12, 0);
        lv_label_set_long_mode(names_[i], LV_LABEL_LONG_DOT);
        lv_obj_align(names_[i], LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_text_color(names_[i], colors.text, 0);

        states_[i] = lv_label_create(row);
        lv_obj_set_width(states_[i], 90);
        lv_obj_set_style_text_align(states_[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_font(states_[i], caption ? caption : &lv_font_montserrat_10, 0);
        lv_obj_align(states_[i], LV_ALIGN_RIGHT_MID, -6, 0);
        lv_obj_set_style_text_color(states_[i], colors.primary, 0);
    }

    empty_ = lv_label_create(content);
    lv_label_set_text(empty_, "No plugins installed");
    lv_obj_set_style_text_font(empty_, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(empty_, LV_ALIGN_TOP_LEFT, 10, 30);
    lv_obj_set_style_text_color(empty_, colors.text_disabled, 0);

    auto* hint = lv_label_create(content);
    lv_label_set_text(hint, "UP/DOWN choose   ENTER on/off   G grant   S sign in");
    lv_obj_set_style_text_font(hint, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(hint, 300);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_color(hint, colors.text_disabled, 0);

    lv_subject_add_observer_obj(view_model().plugins_revision_subject(), plugins_changed, content, this);
    view_model().load_plugins();
    refresh();
}

void PluginsScreen::refresh() {
    const auto colors = view::palette(view_model().is_dark_mode());
    const auto& plugins = view_model().plugins();
    const int count = static_cast<int>(plugins.size());
    if (count == 0) {
        if (empty_) {
            lv_obj_remove_flag(empty_, LV_OBJ_FLAG_HIDDEN);
        }
        for (auto* row : rows_) {
            if (row) {
                lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }
    if (empty_) {
        lv_obj_add_flag(empty_, LV_OBJ_FLAG_HIDDEN);
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
        std::string name = plugin.name;
        if (!plugin.version.empty()) {
            name += "  " + plugin.version;
        }
        lv_label_set_text(names_[i], name.c_str());
        std::string state = plugin.enabled ? "On" : "Off";
        if (plugin.process) {
            state += plugin.granted ? "  granted" : "  app";
        }
        lv_label_set_text(states_[i], state.c_str());
        const bool on = index == selected_;
        lv_obj_set_style_bg_opa(rows_[i], on ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(rows_[i], colors.primary, 0);
        lv_obj_set_style_border_width(rows_[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_side(rows_[i], LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_color(rows_[i], colors.primary, 0);
    }
}

bool PluginsScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (key == 0) {
        refresh();
        return true;
    }
    const int count = static_cast<int>(view_model().plugins().size());
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
            view_model().set_plugin_enabled(static_cast<std::size_t>(selected_),
                                            !view_model().plugins()[static_cast<std::size_t>(selected_)].enabled);
            return true;
        case 'g':
        case 'G': {
            const auto& plugin = view_model().plugins()[static_cast<std::size_t>(selected_)];
            view_model().grant_plugin(static_cast<std::size_t>(selected_), !plugin.granted);
            return true;
        }
        case 's':
        case 'S':
            view_model().sign_in_plugin(static_cast<std::size_t>(selected_));
            return true;
        default:
            return false;
    }
}

} // namespace screen
