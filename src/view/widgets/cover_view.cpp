/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "cover_view.h"

#include "asset_manager.h"
#include "bindings.h"
#include "theme.h"

namespace view::widgets {
namespace {

void cover_changed(lv_observer_t* observer, lv_subject_t*) {
    auto* view = static_cast<CoverView*>(lv_observer_get_user_data(observer));
    if (view) {
        view->refresh();
    }
}

} // namespace

CoverView::CoverView(lv_obj_t* parent,
                     viewmodel::BaseViewModel& view_model,
                     app::AssetManager& assets,
                     int size)
    : BaseWidgets(parent), view_model_(view_model), assets_(assets), size_(size) {}

void CoverView::build() {
    if (core_obj_) {
        return;
    }
    const auto colors = view::palette(view_model_.is_dark_mode());
    core_obj_ = lv_obj_create(parent_);
    lv_obj_remove_style_all(core_obj_);
    lv_obj_set_size(core_obj_, size_, size_);
    lv_obj_set_style_radius(core_obj_, 6, 0);
    lv_obj_set_style_bg_opa(core_obj_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(core_obj_, colors.primary, 0);
    lv_obj_set_style_bg_grad_color(core_obj_, colors.surface, 0);
    lv_obj_set_style_bg_grad_dir(core_obj_, LV_GRAD_DIR_VER, 0);
    lv_obj_clear_flag(core_obj_, LV_OBJ_FLAG_SCROLLABLE);

    initials_ = lv_label_create(core_obj_);
    lv_label_set_text(initials_, "ZM");
    auto* font = assets_.load_standard_font(20);
    lv_obj_set_style_text_font(initials_, font ? font : &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(initials_, lv_color_hex(0x101014), 0);
    lv_obj_center(initials_);

    image_ = lv_image_create(core_obj_);
    lv_obj_set_size(image_, size_, size_);
    lv_obj_center(image_);
    lv_obj_add_flag(image_, LV_OBJ_FLAG_HIDDEN);

    if (const auto cover = assets_.resolve("images/zero-music.png"); !cover.empty()) {
        default_src_ = "A:" + cover.string();
    }

    lv_subject_add_observer_obj(view_model_.cover_revision_subject(), cover_changed, core_obj_, this);
    refresh();
}

void CoverView::refresh() {
    const auto& data = view_model_.cover_data();
    if (data.empty() || view_model_.cover_width() <= 0) {
        if (!default_src_.empty()) {
            lv_image_set_src(image_, default_src_.c_str());
            lv_image_set_scale(image_, static_cast<std::uint32_t>(size_ * 256 / 512));
            lv_obj_remove_flag(image_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(initials_, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(image_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(initials_, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    auto cover = std::make_unique<Cover>();
    cover->data = data;
    cover->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    cover->dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    cover->dsc.header.w = view_model_.cover_width();
    cover->dsc.header.h = view_model_.cover_height();
    cover->dsc.header.stride = view_model_.cover_width() * 2;
    cover->dsc.data_size = static_cast<std::uint32_t>(cover->data.size());
    cover->dsc.data = cover->data.data();
    lv_image_set_src(image_, &cover->dsc);
    lv_image_set_scale(image_, 256);
    lv_obj_remove_flag(image_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(initials_, LV_OBJ_FLAG_HIDDEN);
    covers_.push_back(std::move(cover));
    if (covers_.size() > 8) {
        covers_.erase(covers_.begin());
    }
}

} // namespace view::widgets
