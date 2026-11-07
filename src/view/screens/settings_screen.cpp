/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "settings_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>
#include <array>

namespace screen {
namespace {

struct Row {
    const char* label;
};

constexpr std::array<Row, 6> kRows = {{
    {"Volume"},
    {"Quality"},
    {"Cache"},
    {"Language"},
    {"Music folders"},
    {"Rescan library"},
}};

void volume_changed(lv_observer_t* observer, lv_subject_t* subject) {
    auto* label = lv_observer_get_target_obj(observer);
    if (!label) {
        return;
    }
    char text[8];
    lv_snprintf(text, sizeof(text), "%d%%", static_cast<int>(lv_subject_get_int(subject)));
    lv_label_set_text(label, text);
}

} // namespace

void SettingsScreen::build_content(lv_obj_t* content) {
    selected_ = std::clamp(view_model().page_selection(model::AppPage::Settings), 0, static_cast<int>(kRows.size()) - 1);
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text_disabled, 0);

    for (int i = 0; i < static_cast<int>(kRows.size()); ++i) {
        auto* row = lv_obj_create(content);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 300, 17);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 10, 18 + i * 18);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row, 3, 0);
        rows_[i] = row;

        auto* label = lv_label_create(row);
        lv_label_set_text(label, kRows[i].label);
        lv_obj_set_style_text_font(label, body ? body : &lv_font_montserrat_12, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_text_color(label, colors.text, 0);

        auto* value = lv_label_create(row);
        lv_obj_set_width(value, 92);
        lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_font(value, caption ? caption : &lv_font_montserrat_10, 0);
        lv_obj_align(value, LV_ALIGN_RIGHT_MID, -6, 0);
        lv_obj_set_style_text_color(value, colors.primary, 0);

        switch (i) {
            case 0:
                lv_label_set_text(value, "80%");
                lv_subject_add_observer_obj(view_model().volume_subject(), volume_changed, value, nullptr);
                break;
            case 1:
                lv_label_bind_text(value, view_model().quality_subject(), nullptr);
                break;
            case 2:
                lv_label_bind_text(value, view_model().cache_subject(), nullptr);
                break;
            case 3:
                lv_label_bind_text(value, view_model().language_subject(), nullptr);
                break;
            case 4:
                lv_label_bind_text(value, view_model().folders_subject(), nullptr);
                break;
            case 5:
                lv_label_set_text(value, "Enter");
                break;
        }
    }

    auto* hint = lv_label_create(content);
    lv_label_set_text(hint, "UP/DOWN choose   LEFT/RIGHT change   ENTER run   ESC back");
    lv_obj_set_style_text_font(hint, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(hint, 300);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_color(hint, colors.text_disabled, 0);

    refresh_selection();
}

void SettingsScreen::refresh_selection() {
    const auto colors = view::palette(view_model().is_dark_mode());
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const bool on = static_cast<int>(i) == selected_;
        lv_obj_set_style_bg_opa(rows_[i], on ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(rows_[i], colors.primary, 0);
        lv_obj_set_style_border_width(rows_[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_side(rows_[i], LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_color(rows_[i], colors.primary, 0);
    }
}

void SettingsScreen::adjust(int direction) {
    switch (selected_) {
        case 0:
            view_model().volume_delta(direction * 5);
            break;
        case 1:
            view_model().cycle_quality();
            break;
        case 2:
            view_model().cycle_cache();
            break;
        case 3:
            view_model().cycle_language();
            break;
        default:
            break;
    }
}

bool SettingsScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    switch (key) {
        case LV_KEY_UP:
            selected_ = (selected_ + static_cast<int>(rows_.size()) - 1) % static_cast<int>(rows_.size());
            view_model().set_page_selection(model::AppPage::Settings, selected_);
            refresh_selection();
            return true;
        case LV_KEY_DOWN:
            selected_ = (selected_ + 1) % static_cast<int>(rows_.size());
            view_model().set_page_selection(model::AppPage::Settings, selected_);
            refresh_selection();
            return true;
        case LV_KEY_LEFT:
            adjust(-1);
            return true;
        case LV_KEY_RIGHT:
            adjust(1);
            return true;
        case LV_KEY_ENTER:
            if (selected_ == 4) {
                view_model().show_page(model::AppPage::Folders);
            }
            else if (selected_ == 5) {
                view_model().rescan();
            }
            else {
                adjust(1);
            }
            return true;
        default:
            return false;
    }
}

} // namespace screen
