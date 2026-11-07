/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_viewmodel.h"
#include "base_widget.h"

#include <array>
#include <cstdint>

namespace app {
class AssetManager;
}

namespace view::widgets {

// TrackList is six rows of what a source last answered: the focused one, and
// the keys that move through them and play one.
class TrackList : public BaseWidgets {
public:
    enum class Kind {
        Tracks,
        Queue,
    };

    TrackList(lv_obj_t* parent,
              viewmodel::BaseViewModel& view_model,
              app::AssetManager& assets,
              Kind kind = Kind::Tracks);

    void build() override;
    bool handle_key(std::uint32_t key, bool long_pressed);

private:
    void refresh();

    viewmodel::BaseViewModel& view_model_;
    app::AssetManager& assets_;
    Kind kind_;
    lv_obj_t* title_{nullptr};
    std::array<lv_obj_t*, 6> rows_{};
    std::array<lv_obj_t*, 6> names_{};
    std::array<lv_obj_t*, 6> artists_{};
    std::array<lv_obj_t*, 6> times_{};
    int selected_ = 0;
    int offset_ = 0;
};

} // namespace view::widgets
