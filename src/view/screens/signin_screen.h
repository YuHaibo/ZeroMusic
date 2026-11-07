/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_screen.h"

namespace screen {

// SignInScreen shows the QR a plugin's sign-in asks for, and what to do with it.
class SignInScreen : public BaseScreen {
public:
    using BaseScreen::BaseScreen;

    bool handle_key(std::uint32_t key, bool long_pressed) override;

protected:
    void build_content(lv_obj_t* content) override;

private:
    void refresh();
    lv_obj_t* qr_{nullptr};
    lv_obj_t* text_{nullptr};
};

} // namespace screen
