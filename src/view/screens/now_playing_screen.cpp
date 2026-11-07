/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "now_playing_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

#include <algorithm>

namespace screen {
namespace {

void progress_observer(lv_observer_t* observer, lv_subject_t* subject) {
    auto* bar = lv_observer_get_target_obj(observer);
    if (!bar) {
        return;
    }
    lv_bar_set_value(bar, lv_subject_get_int(subject), LV_ANIM_OFF);
}

void lyrics_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<NowPlayingScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

void volume_changed(lv_observer_t* observer, lv_subject_t* subject) {
    auto* label = lv_observer_get_target_obj(observer);
    if (!label) {
        return;
    }
    char text[16];
    lv_snprintf(text, sizeof(text), "VOL %d%%", static_cast<int>(lv_subject_get_int(subject)));
    lv_label_set_text(label, text);
}

} // namespace

void NowPlayingScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* big = assets().load_standard_font(16);
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);
    auto* lyric_font = assets().load_standard_font(11);

    cover_ = std::make_unique<view::widgets::CoverView>(content, view_model(), assets(), 100);
    cover_->build();
    lv_obj_align(cover_->root(), LV_ALIGN_TOP_LEFT, 8, 6);

    auto* title = lv_label_create(content);
    lv_label_bind_text(title, view_model().track_title_subject(), nullptr);
    lv_obj_set_style_text_font(title, big ? big : &lv_font_montserrat_16, 0);
    lv_label_set_long_mode(title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(title, 194);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 118, 6);
    lv_obj_set_style_text_color(title, colors.text, 0);

    auto* artist = lv_label_create(content);
    lv_label_bind_text(artist, view_model().track_artist_subject(), nullptr);
    lv_obj_set_style_text_font(artist, body ? body : &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(artist, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(artist, 194);
    lv_obj_align(artist, LV_ALIGN_TOP_LEFT, 118, 30);
    lv_obj_set_style_text_color(artist, colors.text, 0);

    auto* album = lv_label_create(content);
    lv_label_bind_text(album, view_model().track_album_subject(), nullptr);
    lv_obj_set_style_text_font(album, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(album, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(album, 194);
    lv_obj_align(album, LV_ALIGN_TOP_LEFT, 118, 46);
    lv_obj_set_style_text_color(album, colors.text_disabled, 0);

    state_label_ = lv_label_create(content);
    reactive::bind_bool_text(state_label_, view_model().playing_subject(), "Paused", "Playing");
    lv_obj_set_style_text_font(state_label_, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(state_label_, LV_ALIGN_TOP_LEFT, 118, 62);
    lv_obj_set_style_text_color(state_label_, colors.primary, 0);

    auto* like = lv_label_create(content);
    reactive::bind_bool_text(like, view_model().liked_subject(), "LIKE", "LIKED");
    lv_obj_set_style_text_font(like, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(like, LV_ALIGN_TOP_RIGHT, -10, 62);
    lv_obj_set_style_text_color(like, colors.primary, 0);

    volume_ = lv_label_create(content);
    lv_obj_set_style_text_font(volume_, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(volume_, LV_ALIGN_TOP_RIGHT, -10, 74);
    lv_obj_set_style_text_color(volume_, colors.text_disabled, 0);
    lv_subject_add_observer_obj(view_model().volume_subject(), volume_changed, volume_, nullptr);

    progress_ = lv_bar_create(content);
    lv_obj_remove_style_all(progress_);
    lv_obj_set_size(progress_, 194, 4);
    lv_obj_align(progress_, LV_ALIGN_TOP_LEFT, 118, 88);
    lv_obj_set_style_radius(progress_, 2, 0);
    lv_obj_set_style_bg_opa(progress_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(progress_, colors.border, LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_, colors.primary, LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress_, 2, LV_PART_INDICATOR);
    lv_bar_set_range(progress_, 0, 1000);
    lv_bar_set_value(progress_, lv_subject_get_int(view_model().progress_subject()), LV_ANIM_OFF);
    lv_subject_add_observer_obj(view_model().progress_subject(), progress_observer, progress_, nullptr);

    auto* position = lv_label_create(content);
    lv_label_bind_text(position, view_model().position_text_subject(), nullptr);
    lv_obj_set_style_text_font(position, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(position, LV_ALIGN_TOP_LEFT, 118, 94);
    lv_obj_set_style_text_color(position, colors.text_disabled, 0);

    auto* duration = lv_label_create(content);
    lv_label_bind_text(duration, view_model().duration_text_subject(), nullptr);
    lv_obj_set_style_text_font(duration, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(duration, LV_ALIGN_TOP_RIGHT, -10, 94);
    lv_obj_set_style_text_color(duration, colors.text_disabled, 0);

    for (int i = 0; i < 2; ++i) {
        lyric_lines_[i] = lv_label_create(content);
        lv_obj_set_width(lyric_lines_[i], 304);
        lv_label_set_long_mode(lyric_lines_[i], LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(lyric_lines_[i], lyric_font ? lyric_font : &lv_font_montserrat_10, 0);
        lv_obj_align(lyric_lines_[i], LV_ALIGN_TOP_LEFT, 8, 108 + i * 15);
        lv_obj_set_style_text_color(lyric_lines_[i], colors.text_disabled, 0);
    }

    lv_subject_add_observer_obj(view_model().lyrics_revision_subject(), lyrics_changed, content, this);
    lv_subject_add_observer_obj(view_model().progress_subject(), lyrics_changed, content, this);
    refresh_lyrics();
}

void NowPlayingScreen::refresh_lyrics() {
    const auto& lyrics = view_model().lyrics();
    if (lyrics.empty()) {
        lv_label_set_text(lyric_lines_[0], "No lyrics for this track");
        lv_obj_set_style_text_color(lyric_lines_[0],
                                    view::palette(view_model().is_dark_mode()).text_disabled,
                                    0);
        lv_obj_add_flag(lyric_lines_[1], LV_OBJ_FLAG_HIDDEN);
        return;
    }
    const auto position = view_model().position_ms();
    int current = 0;
    for (std::size_t i = 0; i < lyrics.size(); ++i) {
        if (lyrics[i].time_ms <= position) {
            current = static_cast<int>(i);
        }
        else {
            break;
        }
    }
    const auto colors = view::palette(view_model().is_dark_mode());
    lv_obj_remove_flag(lyric_lines_[0], LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(lyric_lines_[0], lyrics[static_cast<std::size_t>(current)].text.c_str());
    lv_obj_set_style_text_color(lyric_lines_[0], colors.primary, 0);
    const int next = current + 1;
    if (next < static_cast<int>(lyrics.size())) {
        lv_obj_remove_flag(lyric_lines_[1], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lyric_lines_[1], lyrics[static_cast<std::size_t>(next)].text.c_str());
        lv_obj_set_style_text_color(lyric_lines_[1], colors.text_disabled, 0);
    }
    else {
        lv_obj_add_flag(lyric_lines_[1], LV_OBJ_FLAG_HIDDEN);
    }
}

bool NowPlayingScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (key == 0) {
        refresh_lyrics();
        return true;
    }
    switch (key) {
        case LV_KEY_LEFT:
            view_model().seek_relative(-10000);
            return true;
        case LV_KEY_RIGHT:
            view_model().seek_relative(10000);
            return true;
        case LV_KEY_UP:
            view_model().volume_delta(5);
            return true;
        case LV_KEY_DOWN:
            view_model().volume_delta(-5);
            return true;
        case LV_KEY_ENTER:
            view_model().toggle_play();
            return true;
        case 'l':
        case 'L':
            view_model().toggle_like();
            return true;
        default:
            return false;
    }
}

} // namespace screen
