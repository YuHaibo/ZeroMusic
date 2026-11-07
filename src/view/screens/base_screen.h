/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "base_viewmodel.h"
#include "titlebar.h"
#include "lvgl.h"

#include <cstdint>
#include <memory>

namespace app {
class AssetManager;
}

namespace screen {

// BaseScreen is the frame every page shares: the status bar and a content
// area under it. Keys arrive from the app, which gives Esc and the transport
// keys its own meaning first.
class BaseScreen {
public:
    BaseScreen(viewmodel::BaseViewModel& view_model, app::AssetManager& assets);
    virtual ~BaseScreen();

    BaseScreen(const BaseScreen&) = delete;
    BaseScreen& operator=(const BaseScreen&) = delete;

    void init();
    lv_obj_t* root() const;

    // handle_key reports whether the screen used the key.
    virtual bool handle_key(std::uint32_t key, bool long_pressed);

    // text_input_active is true while a field takes letters, so the app sends
    // F/Z/X/C as text rather than as the arrows they stand for in navigation.
    virtual bool text_input_active() const { return false; }

protected:
    virtual void build_content(lv_obj_t* content) = 0;

    viewmodel::BaseViewModel& view_model();
    app::AssetManager& assets();

private:
    viewmodel::BaseViewModel& view_model_;
    app::AssetManager& assets_;
    lv_obj_t* root_{nullptr};
    lv_obj_t* content_{nullptr};
    std::unique_ptr<view::widgets::TitleBar> title_bar_;
};

} // namespace screen
