/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include "base_screen.h"
#include "cover_view.h"

#include <memory>
#include <vector>

namespace screen {

// HomeScreen is the way in: the current cover and source, and the pages the
// listener can open.
class HomeScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    void refresh_selection();
    void activate();

    std::vector<lv_obj_t*> rows_;
    std::unique_ptr<view::widgets::CoverView> cover_;
    int selected_ = 0;

};

} // namespace screen
