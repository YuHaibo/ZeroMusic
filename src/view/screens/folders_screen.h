/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"

#include <array>

namespace screen {

// FoldersScreen lists the local music folders, adds ~/Music, removes one, and
// rescans them.
class FoldersScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    void refresh();

    std::array<lv_obj_t*, 6> rows_{};
    std::array<lv_obj_t*, 6> names_{};
    int selected_ = 0;
    int offset_ = 0;
    lv_obj_t* empty_{nullptr};
};

} // namespace screen
