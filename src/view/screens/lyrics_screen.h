/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include "base_screen.h"

#include <array>

namespace screen {

// LyricsScreen shows the current line, with the ones around it, and follows
// the song unless the listener scrolled away.
class LyricsScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;
    ~LyricsScreen() override;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    void refresh();
    int current_line();
    static void tick(lv_timer_t* timer);

    std::array<lv_obj_t*, 5> lines_{};
    lv_obj_t* empty_{nullptr};
    lv_timer_t* timer_{nullptr};
    int scroll_ = 0;
};

} // namespace screen
