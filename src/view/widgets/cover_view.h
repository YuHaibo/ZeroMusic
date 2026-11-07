/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include "base_viewmodel.h"
#include "base_widget.h"

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace app {
class AssetManager;
}

namespace view::widgets {

// CoverView is the current album's picture, or a default mark while it loads.
// It owns the RGB565 descriptors LVGL draws.
class CoverView : public BaseWidgets {
public:
    CoverView(lv_obj_t* parent,
              viewmodel::BaseViewModel& view_model,
              app::AssetManager& assets,
              int size);

    void build() override;
    void refresh();

private:
    struct Cover {
        lv_image_dsc_t dsc{};
        std::vector<std::uint8_t> data;
    };

    viewmodel::BaseViewModel& view_model_;
    app::AssetManager& assets_;
    int size_;
    lv_obj_t* image_{nullptr};
    lv_obj_t* initials_{nullptr};
    std::vector<std::unique_ptr<Cover>> covers_;
    std::string default_src_;
};

} // namespace view::widgets
