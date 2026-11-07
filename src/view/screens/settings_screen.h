/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"

#include <array>

namespace screen {

// SettingsScreen changes what the core keeps: volume, quality, normalization,
// cache and language, and rescans the folders.
class SettingsScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    void refresh_selection();
    void adjust(int direction);

    std::array<lv_obj_t*, 5> rows_{};
    int selected_ = 0;
};

} // namespace screen
