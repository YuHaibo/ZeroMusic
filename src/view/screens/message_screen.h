/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include "base_screen.h"

#include <string>

namespace screen {

// MessageScreen stands in for a page that is not built yet, so the keyboard
// and the frame can be tried before every page exists.
class MessageScreen : public BaseScreen {
public:
    MessageScreen(viewmodel::BaseViewModel& view_model,
                  app::AssetManager& assets,
                  std::string title,
                  std::string message);

protected:
    void build_content(lv_obj_t* content) override;

private:
    std::string title_;
    std::string message_;
};

} // namespace screen
