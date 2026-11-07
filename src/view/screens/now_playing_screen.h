/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"

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
    lv_obj_t* progress_{nullptr};
    lv_obj_t* state_label_{nullptr};
};

} // namespace screen
