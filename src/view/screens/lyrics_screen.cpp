/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "lyrics_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>

namespace screen {
namespace {

constexpr int kLines = 5;

void lyrics_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<LyricsScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

} // namespace

LyricsScreen::~LyricsScreen() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
}

void LyricsScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    auto* title = lv_label_create(content);
    lv_label_set_text(title, "Lyrics");
    lv_obj_set_style_text_font(title, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 2);
    lv_obj_set_style_text_color(title, colors.text_disabled, 0);

    for (int i = 0; i < kLines; ++i) {
        auto* line = lv_label_create(content);
        lv_obj_set_width(line, 300);
        lv_label_set_long_mode(line, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(line, body ? body : &lv_font_montserrat_12, 0);
        lv_obj_align(line, LV_ALIGN_TOP_LEFT, 10, 22 + i * 21);
        lv_obj_set_style_text_color(line, colors.text, 0);
        lines_[i] = line;
    }

    empty_ = lv_label_create(content);
    lv_label_set_text(empty_, "No lyrics for this track");
    lv_obj_set_style_text_font(empty_, body ? body : &lv_font_montserrat_12, 0);
    lv_obj_center(empty_);
    lv_obj_set_style_text_color(empty_, colors.text_disabled, 0);

    lv_subject_add_observer_obj(view_model().lyrics_revision_subject(), lyrics_changed, content, this);

    timer_ = lv_timer_create(
        [](lv_timer_t* timer) {
            static_cast<LyricsScreen*>(lv_timer_get_user_data(timer))->refresh();
        },
        250,
        this);

    refresh();
}

int LyricsScreen::current_line() {
    const auto& lyrics = view_model().lyrics();
    if (lyrics.empty()) {
        return -1;
    }
    const auto position = view_model().position_ms();
    int found = 0;
    for (std::size_t i = 0; i < lyrics.size(); ++i) {
        if (lyrics[i].time_ms <= position) {
            found = static_cast<int>(i);
        }
        else {
            break;
        }
    }
    return found;
}

void LyricsScreen::refresh() {
    const auto& lyrics = view_model().lyrics();
    const int count = static_cast<int>(lyrics.size());
    if (count == 0) {
        if (empty_) {
            lv_obj_remove_flag(empty_, LV_OBJ_FLAG_HIDDEN);
        }
        for (auto* line : lines_) {
            if (line) {
                lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }
    if (empty_) {
        lv_obj_add_flag(empty_, LV_OBJ_FLAG_HIDDEN);
    }
    const int current = current_line();
    if (scroll_ == 0) {
        const int start = std::max(0, current - 2);
        for (int i = 0; i < kLines; ++i) {
            const int index = start + i;
            if (!lines_[i]) {
                continue;
            }
            if (index >= count) {
                lv_obj_add_flag(lines_[i], LV_OBJ_FLAG_HIDDEN);
                continue;
            }
            lv_obj_remove_flag(lines_[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(lines_[i], lyrics[static_cast<std::size_t>(index)].text.c_str());
            const bool on = index == current;
            const auto colors = view::palette(view_model().is_dark_mode());
            lv_obj_set_style_text_color(lines_[i], on ? colors.primary : colors.text_disabled, 0);
        }
    }
    else {
        const int start = std::max(0, std::min(scroll_, std::max(0, count - kLines)));
        for (int i = 0; i < kLines; ++i) {
            const int index = start + i;
            if (!lines_[i]) {
                continue;
            }
            if (index >= count) {
                lv_obj_add_flag(lines_[i], LV_OBJ_FLAG_HIDDEN);
                continue;
            }
            lv_obj_remove_flag(lines_[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(lines_[i], lyrics[static_cast<std::size_t>(index)].text.c_str());
            lv_obj_set_style_text_color(lines_[i],
                                        index == current
                                            ? view::palette(view_model().is_dark_mode()).primary
                                            : view::palette(view_model().is_dark_mode()).text_disabled,
                                        0);
        }
    }
}

bool LyricsScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    const int count = static_cast<int>(view_model().lyrics().size());
    switch (key) {
        case 0:
            refresh();
            return true;
        case LV_KEY_UP:
            scroll_ = std::max(0, scroll_ - 1);
            if (scroll_ == 0) {
                scroll_ = 0;
            }
            refresh();
            return true;
        case LV_KEY_DOWN:
            if (count > 0) {
                const int last = std::max(0, count - kLines);
                scroll_ = std::min(last, scroll_ + 1);
                if (scroll_ == last) {
                    scroll_ = 0; // back to following the song
                }
            }
            refresh();
            return true;
        default:
            return false;
    }
}

} // namespace screen
