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
#include <cstdlib>

namespace screen {
namespace {

void plugins_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<PluginsScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

void settings_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<PluginsScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

} // namespace

bool PluginsScreen::text_input_active() const {
    return mode_ == Mode::Settings && editing_;
}

void PluginsScreen::build_content(lv_obj_t* content) {
    screen_content_ = content;
    mode_ = Mode::List;
    build_list();
}

void PluginsScreen::build_list() {
    mode_ = Mode::List;
    lv_obj_clean(screen_content_);
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(screen_content_);
    lv_label_set_text(title, "Plugins");
    lv_obj_set_style_text_font(title, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text_disabled, 0);

    for (int i = 0; i < 6; ++i) {
        auto* row = lv_obj_create(screen_content_);
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

    empty_ = lv_label_create(screen_content_);
    lv_label_set_text(empty_, "No plugins installed");
    lv_obj_set_style_text_font(empty_, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(empty_, LV_ALIGN_TOP_LEFT, 10, 30);
    lv_obj_set_style_text_color(empty_, colors.text_disabled, 0);

    auto* hint = lv_label_create(screen_content_);
    lv_label_set_text(hint, "ENTER on/off  U use  S sign in  A acct  E set  I install");
    lv_obj_set_style_text_font(hint, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(hint, 300);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_color(hint, colors.text_disabled, 0);

    lv_subject_add_observer_obj(view_model().plugins_revision_subject(), plugins_changed, screen_content_, this);
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
    if (std::getenv("ZERO_MUSIC_PLUGIN_SETTINGS") && !auto_settings_) {
        auto_settings_ = true;
        mode_ = Mode::Settings;
        edit_selected_ = 0;
        view_model().load_plugin_settings(static_cast<std::size_t>(selected_));
        build_settings();
        return;
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

void PluginsScreen::build_settings() {
    lv_obj_clean(screen_content_);
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    std::string title = "Settings";
    if (selected_ < static_cast<int>(view_model().plugins().size())) {
        title = view_model().plugins()[static_cast<std::size_t>(selected_)].name + " settings";
    }
    auto* heading = lv_label_create(screen_content_);
    lv_label_set_text(heading, title.c_str());
    lv_obj_set_style_text_font(heading, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_align(heading, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(heading, colors.text, 0);

    for (int i = 0; i < 6; ++i) {
        auto* row = lv_obj_create(screen_content_);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 300, 17);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 10, 20 + i * 18);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row, 3, 0);
        setting_rows_[i] = row;

        setting_labels_[i] = lv_label_create(row);
        lv_obj_set_width(setting_labels_[i], 120);
        lv_obj_set_style_text_font(setting_labels_[i], body ? body : &lv_font_montserrat_12, 0);
        lv_label_set_long_mode(setting_labels_[i], LV_LABEL_LONG_DOT);
        lv_obj_align(setting_labels_[i], LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_text_color(setting_labels_[i], colors.text, 0);

        setting_values_[i] = lv_label_create(row);
        lv_obj_set_width(setting_values_[i], 160);
        lv_obj_set_style_text_align(setting_values_[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_font(setting_values_[i], caption ? caption : &lv_font_montserrat_10, 0);
        lv_label_set_long_mode(setting_values_[i], LV_LABEL_LONG_DOT);
        lv_obj_align(setting_values_[i], LV_ALIGN_RIGHT_MID, -6, 0);
        lv_obj_set_style_text_color(setting_values_[i], colors.primary, 0);
    }

    auto* hint = lv_label_create(screen_content_);
    lv_label_set_text(hint, "UP/DOWN choose   ENTER edit   ESC back");
    lv_obj_set_style_text_font(hint, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(hint, 300);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_color(hint, colors.text_disabled, 0);

    lv_subject_add_observer_obj(view_model().plugin_settings_revision_subject(), settings_changed, screen_content_, this);
    refresh_settings();
}

void PluginsScreen::refresh_settings() {
    const auto colors = view::palette(view_model().is_dark_mode());
    const auto& settings = view_model().plugin_settings();
    const int count = static_cast<int>(settings.size());
    if (edit_selected_ >= count) {
        edit_selected_ = std::max(0, count - 1);
    }
    for (int i = 0; i < 6; ++i) {
        if (i >= count) {
            lv_obj_add_flag(setting_rows_[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(setting_rows_[i], LV_OBJ_FLAG_HIDDEN);
        const auto& setting = settings[static_cast<std::size_t>(i)];
        lv_label_set_text(setting_labels_[i], setting.label.c_str());
        if (editing_ && i == edit_selected_) {
            lv_label_set_text(setting_values_[i], (edit_buffer_ + "_").c_str());
        }
        else {
            lv_label_set_text(setting_values_[i], setting.value.empty() ? "(empty)" : setting.value.c_str());
        }
        const bool on = i == edit_selected_;
        lv_obj_set_style_bg_opa(setting_rows_[i], on ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(setting_rows_[i], colors.primary, 0);
        lv_obj_set_style_border_width(setting_rows_[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_side(setting_rows_[i], LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_color(setting_rows_[i], colors.primary, 0);
    }
}

bool PluginsScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (mode_ == Mode::Settings) {
        const int count = static_cast<int>(view_model().plugin_settings().size());
        if (key == LV_KEY_ESC) {
            if (editing_) {
                editing_ = false;
                refresh_settings();
            }
            else {
                build_list();
            }
            return true;
        }
        if (editing_) {
            if (key == LV_KEY_ENTER) {
                view_model().save_plugin_setting(static_cast<std::size_t>(edit_selected_), edit_buffer_);
                editing_ = false;
                refresh_settings();
                return true;
            }
            if (key == LV_KEY_BACKSPACE) {
                if (!edit_buffer_.empty()) {
                    edit_buffer_.pop_back();
                }
                refresh_settings();
                return true;
            }
            if (key >= 32 && key < 127 && edit_buffer_.size() < 96) {
                edit_buffer_.push_back(static_cast<char>(key));
                refresh_settings();
                return true;
            }
            return true;
        }
        if (key == 0) {
            refresh_settings();
            return true;
        }
        switch (key) {
            case LV_KEY_UP:
                if (count > 0) {
                    edit_selected_ = (edit_selected_ + count - 1) % count;
                    refresh_settings();
                }
                return true;
            case LV_KEY_DOWN:
                if (count > 0) {
                    edit_selected_ = (edit_selected_ + 1) % count;
                    refresh_settings();
                }
                return true;
            case LV_KEY_ENTER:
                if (count > 0) {
                    editing_ = true;
                    edit_buffer_ = view_model().plugin_settings()[static_cast<std::size_t>(edit_selected_)].value;
                    refresh_settings();
                }
                return true;
            default:
                return false;
        }
    }

    if (key == 0) {
        refresh();
        return true;
    }
    if (key == 'i' || key == 'I') {
        view_model().show_page(model::AppPage::Install);
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
        case 'u':
        case 'U':
            view_model().use_plugin(static_cast<std::size_t>(selected_));
            return true;
        case 'a':
        case 'A':
            view_model().load_account(static_cast<std::size_t>(selected_));
            view_model().show_page(model::AppPage::Account);
            return true;
        case 'e':
        case 'E': {
            mode_ = Mode::Settings;
            edit_selected_ = 0;
            editing_ = false;
            view_model().load_plugin_settings(static_cast<std::size_t>(selected_));
            build_settings();
            return true;
        }
        default:
            return false;
    }
}

} // namespace screen
