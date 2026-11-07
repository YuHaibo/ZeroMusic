/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include "base_screen.h"

#include <array>
#include <string>
#include <vector>

namespace screen {

// PluginsScreen turns installed plugins on and off, grants the process
// permission, and starts their sign-in.
class PluginsScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;
    bool text_input_active() const override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    enum class Mode {
        List,
        Settings,
    };

    void build_list();
    void build_settings();
    void refresh();
    void refresh_settings();

    Mode mode_ = Mode::List;
    lv_obj_t* screen_content_{nullptr};
    int edit_selected_ = 0;
    bool editing_ = false;
    bool auto_settings_ = false;
    bool searching_ = false;
    std::string filter_;
    std::vector<int> visible_;
    std::string edit_buffer_;
    std::array<lv_obj_t*, 6> setting_rows_{};
    std::array<lv_obj_t*, 6> setting_labels_{};
    std::array<lv_obj_t*, 6> setting_values_{};
    std::array<lv_obj_t*, 6> rows_{};
    std::array<lv_obj_t*, 6> names_{};
    std::array<lv_obj_t*, 6> states_{};
    lv_obj_t* empty_{nullptr};
    lv_obj_t* title_{nullptr};
    lv_obj_t* hint_{nullptr};
    lv_obj_t* status_{nullptr};
    int pending_delete_ = -1;
    int selected_ = 0;
    int offset_ = 0;
};

} // namespace screen
