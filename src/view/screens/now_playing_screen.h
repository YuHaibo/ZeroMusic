/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace screen {

// NowPlayingScreen shows the current track, how far it is, and the keys that
// control it.
class NowPlayingScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    struct Cover {
        lv_image_dsc_t dsc{};
        std::vector<std::uint8_t> data;
    };

    void refresh_cover();

    lv_obj_t* progress_{nullptr};
    lv_obj_t* state_label_{nullptr};
    lv_obj_t* cover_image_{nullptr};
    lv_obj_t* cover_initials_{nullptr};
    std::vector<std::unique_ptr<Cover>> covers_;
};

} // namespace screen
