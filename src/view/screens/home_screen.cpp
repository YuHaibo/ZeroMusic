/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "home_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>
#include <array>

namespace screen {
namespace {

struct Entry {
    const char* label;
    model::AppPage page;
};

constexpr std::array<Entry, 7> kEntries = {{
    {"Now Playing", model::AppPage::NowPlaying},
    {"Recent", model::AppPage::Recent},
    {"Browse", model::AppPage::Browse},
    {"Search", model::AppPage::Search},
    {"Settings", model::AppPage::Settings},
    {"Plugins", model::AppPage::Plugins},
    {"Help", model::AppPage::Help},
}};

} // namespace

void HomeScreen::build_content(lv_obj_t* content) {
    selected_ = std::clamp(view_model().page_selection(model::AppPage::Home), 0, static_cast<int>(kEntries.size()) - 1);
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* small = assets().load_standard_font(10);

    cover_ = std::make_unique<view::widgets::CoverView>(content, view_model(), assets(), 100);
    cover_->build();
    lv_obj_align(cover_->root(), LV_ALIGN_TOP_LEFT, 8, 6);

    auto* source = lv_label_create(content);
    lv_label_bind_text(source, view_model().source_subject(), nullptr);
    lv_obj_set_width(source, 104);
    lv_obj_set_style_text_align(source, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(source, small ? small : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(source, LV_LABEL_LONG_DOT);
    lv_obj_align(source, LV_ALIGN_TOP_LEFT, 6, 110);
    lv_obj_set_style_text_color(source, colors.primary, 0);

    auto* menu = lv_obj_create(content);
    lv_obj_remove_style_all(menu);
    lv_obj_set_size(menu, 190, LV_PCT(100));
    lv_obj_align(menu, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_pad_all(menu, 4, 0);
    lv_obj_set_style_pad_left(menu, 2, 0);
    lv_obj_set_flex_flow(menu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(menu, 1, 0);
    lv_obj_clear_flag(menu, LV_OBJ_FLAG_SCROLLABLE);

    auto* text = assets().load_standard_font(12);
    for (const auto& entry : kEntries) {
        auto* row = lv_obj_create(menu);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, LV_PCT(100), 18);
        lv_obj_set_style_radius(row, 3, 0);
        lv_obj_set_style_pad_left(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        auto* label = lv_label_create(row);
        lv_label_set_text(label, entry.label);
        lv_obj_set_style_text_font(label, text ? text : &lv_font_montserrat_12, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
        reactive::bind_theme(label, view_model().dark_mode_subject(), reactive::ThemeRole::Text);
        rows_.push_back(row);
    }

    refresh_selection();
}

void HomeScreen::refresh_selection() {
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

void HomeScreen::activate() {
    if (selected_ < 0 || selected_ >= static_cast<int>(kEntries.size())) {
        return;
    }
    view_model().show_page(kEntries[selected_].page);
}

bool HomeScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    switch (key) {
        case LV_KEY_UP:
            selected_ = (selected_ + static_cast<int>(rows_.size()) - 1) % static_cast<int>(rows_.size());
            view_model().set_page_selection(model::AppPage::Home, selected_);
            refresh_selection();
            return true;
        case LV_KEY_DOWN:
            selected_ = (selected_ + 1) % static_cast<int>(rows_.size());
            view_model().set_page_selection(model::AppPage::Home, selected_);
            refresh_selection();
            return true;
        case LV_KEY_ENTER:
            activate();
            return true;
        case LV_KEY_ESC:
            return false;
        default:
            return false;
    }
}

} // namespace screen
