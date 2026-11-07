/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "now_playing_screen.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

namespace screen {
namespace {

void progress_observer(lv_observer_t* observer, lv_subject_t* subject) {
    auto* bar = lv_observer_get_target_obj(observer);
    if (!bar) {
        return;
    }
    lv_bar_set_value(bar, lv_subject_get_int(subject), LV_ANIM_OFF);
}

void cover_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* screen = static_cast<NowPlayingScreen*>(lv_observer_get_user_data(observer));
    if (screen) {
        screen->handle_key(0, false);
    }
}

} // namespace

void NowPlayingScreen::build_content(lv_obj_t* content) {
    const auto colors = view::palette(view_model().is_dark_mode());
    auto* big = assets().load_standard_font(16);
    auto* body = assets().load_standard_font(12);
    auto* caption = assets().load_standard_font(10);

    auto* cover = lv_obj_create(content);
    lv_obj_remove_style_all(cover);
    lv_obj_set_size(cover, 78, 78);
    lv_obj_align(cover, LV_ALIGN_TOP_LEFT, 10, 8);
    lv_obj_set_style_radius(cover, 6, 0);
    lv_obj_set_style_bg_opa(cover, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cover, colors.primary, 0);
    lv_obj_set_style_bg_grad_color(cover, colors.surface, 0);
    lv_obj_set_style_bg_grad_dir(cover, LV_GRAD_DIR_VER, 0);
    lv_obj_clear_flag(cover, LV_OBJ_FLAG_SCROLLABLE);
    cover_initials_ = lv_label_create(cover);
    lv_label_set_text(cover_initials_, "ZM");
    lv_obj_set_style_text_font(cover_initials_, big ? big : &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(cover_initials_, lv_color_hex(0x101014), 0);
    lv_obj_center(cover_initials_);

    cover_image_ = lv_image_create(cover);
    lv_obj_set_size(cover_image_, 78, 78);
    lv_obj_center(cover_image_);
    lv_obj_add_flag(cover_image_, LV_OBJ_FLAG_HIDDEN);

    auto* title = lv_label_create(content);
    lv_label_bind_text(title, view_model().track_title_subject(), nullptr);
    lv_obj_set_style_text_font(title, big ? big : &lv_font_montserrat_16, 0);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title, 216);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 98, 10);
    lv_obj_set_style_text_color(title, colors.text, 0);

    auto* artist = lv_label_create(content);
    lv_label_bind_text(artist, view_model().track_artist_subject(), nullptr);
    lv_obj_set_style_text_font(artist, body ? body : &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(artist, LV_LABEL_LONG_DOT);
    lv_obj_set_width(artist, 216);
    lv_obj_align(artist, LV_ALIGN_TOP_LEFT, 98, 32);
    lv_obj_set_style_text_color(artist, colors.text, 0);

    auto* album = lv_label_create(content);
    lv_label_bind_text(album, view_model().track_album_subject(), nullptr);
    lv_obj_set_style_text_font(album, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(album, LV_LABEL_LONG_DOT);
    lv_obj_set_width(album, 216);
    lv_obj_align(album, LV_ALIGN_TOP_LEFT, 98, 50);
    lv_obj_set_style_text_color(album, colors.text_disabled, 0);

    state_label_ = lv_label_create(content);
    reactive::bind_bool_text(state_label_, view_model().playing_subject(), "Paused", "Playing");
    lv_obj_set_style_text_font(state_label_, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(state_label_, LV_ALIGN_TOP_LEFT, 98, 66);
    lv_obj_set_style_text_color(state_label_, colors.primary, 0);

    progress_ = lv_bar_create(content);
    lv_obj_remove_style_all(progress_);
    lv_obj_set_size(progress_, 300, 4);
    lv_obj_align(progress_, LV_ALIGN_TOP_MID, 0, 94);
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
    lv_obj_align(position, LV_ALIGN_TOP_LEFT, 10, 100);
    lv_obj_set_style_text_color(position, colors.text_disabled, 0);

    auto* duration = lv_label_create(content);
    lv_label_bind_text(duration, view_model().duration_text_subject(), nullptr);
    lv_obj_set_style_text_font(duration, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(duration, LV_ALIGN_TOP_RIGHT, -10, 100);
    lv_obj_set_style_text_color(duration, colors.text_disabled, 0);

    auto* hints = lv_label_create(content);
    lv_label_set_text(hints, "SPACE play   LEFT/RIGHT seek   UP/DOWN volume");
    lv_obj_set_style_text_font(hints, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(hints, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(hints, 300);
    lv_obj_align(hints, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_color(hints, colors.text_disabled, 0);

    auto* like = lv_label_create(content);
    reactive::bind_bool_text(like, view_model().liked_subject(), "LIKE", "LIKED");
    lv_obj_set_style_text_font(like, caption ? caption : &lv_font_montserrat_10, 0);
    lv_obj_align(like, LV_ALIGN_TOP_RIGHT, -10, 66);
    lv_obj_set_style_text_color(like, colors.primary, 0);

    lv_subject_add_observer_obj(view_model().cover_revision_subject(), cover_changed, content, this);
    refresh_cover();

    auto* more = lv_label_create(content);
    lv_label_set_text(more, "5/7 track   8 repeat   HOLD ESC exit");
    lv_obj_set_style_text_font(more, caption ? caption : &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(more, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(more, 300);
    lv_obj_align(more, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_text_color(more, colors.text_disabled, 0);
}

void NowPlayingScreen::refresh_cover() {
    const auto& data = view_model().cover_data();
    if (data.empty() || view_model().cover_width() <= 0) {
        if (cover_image_) {
            lv_obj_add_flag(cover_image_, LV_OBJ_FLAG_HIDDEN);
        }
        if (cover_initials_) {
            lv_obj_remove_flag(cover_initials_, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    auto cover = std::make_unique<Cover>();
    cover->data = data;
    cover->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    cover->dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    cover->dsc.header.w = view_model().cover_width();
    cover->dsc.header.h = view_model().cover_height();
    cover->dsc.header.stride = view_model().cover_width() * 2;
    cover->dsc.data_size = static_cast<std::uint32_t>(cover->data.size());
    cover->dsc.data = cover->data.data();
    lv_image_set_src(cover_image_, &cover->dsc);
    lv_obj_remove_flag(cover_image_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cover_initials_, LV_OBJ_FLAG_HIDDEN);
    covers_.push_back(std::move(cover));
    if (covers_.size() > 8) {
        covers_.erase(covers_.begin());
    }
}

bool NowPlayingScreen::handle_key(std::uint32_t key, bool long_pressed) {
    if (long_pressed) {
        return false;
    }
    if (key == 0) {
        refresh_cover();
        return true;
    }
    switch (key) {
        case LV_KEY_LEFT:
            view_model().seek_relative(-5000);
            return true;
        case LV_KEY_RIGHT:
            view_model().seek_relative(5000);
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
