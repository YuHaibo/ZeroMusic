/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"

#include <array>
#include <string>

namespace screen {

// InstallScreen lists the public repositories tagged mygo-music-plugin, and
// can take an owner/repo by hand for one that isn't listed yet.
class InstallScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;
    bool text_input_active() const override { return mode_ == Mode::Manual; }

protected:
    void build_content(lv_obj_t* content) override;

private:
    enum class Mode {
        List,
        Manual,
    };

    void build_list();
    void build_manual();
    void refresh();
    void show();

    Mode mode_ = Mode::List;
    lv_obj_t* content_{nullptr};
    std::array<lv_obj_t*, 6> rows_{};
    std::array<lv_obj_t*, 6> repos_{};
    std::array<lv_obj_t*, 6> descriptions_{};
    lv_obj_t* empty_{nullptr};
    lv_obj_t* field_{nullptr};
    int selected_ = 0;
    int offset_ = 0;
    std::string repo_;
};

} // namespace screen
