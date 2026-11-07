/*
 * SPDX-FileCopyrightText: 2026 Zero Music contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "screen_manager.h"

#include "account_screen.h"
#include "browse_screen.h"
#include "detail_screen.h"
#include "home_screen.h"
#include "install_screen.h"
#include "lyrics_screen.h"
#include "message_screen.h"
#include "now_playing_screen.h"
#include "plugins_screen.h"
#include "queue_screen.h"
#include "recent_screen.h"
#include "settings_screen.h"
#include "search_screen.h"
#include "signin_screen.h"

namespace app {
namespace {

void flush_requested_page_async(void* user_data) {
    auto* manager = static_cast<ScreenManager*>(user_data);
    if (manager) {
        manager->flush_requested_page();
    }
}

void current_page_observer(lv_observer_t* observer, lv_subject_t* subject) {
    auto* manager = static_cast<ScreenManager*>(lv_observer_get_target(observer));
    if (!manager) {
        return;
    }
    manager->request_page(static_cast<model::AppPage>(lv_subject_get_int(subject)));
}

struct Placeholder {
    const char* title;
    const char* message;
};

Placeholder placeholder_for(model::AppPage page) {
    switch (page) {
        case model::AppPage::Browse:
            return {"Browse", "Plugin and library browsing arrives with the source pages."};
        case model::AppPage::Search:
            return {"Search", "Search arrives with the library pages."};
        case model::AppPage::Detail:
            return {"Detail", "Album, artist and playlist pages arrive with the library pages."};
        case model::AppPage::Lyrics:
            return {"Lyrics", "Lyrics arrive with the Now Playing pages."};
        case model::AppPage::Queue:
            return {"Queue", "The queue arrives with the player pages."};
        case model::AppPage::Settings:
            return {"Settings", "Settings arrives with the plugin manager."};
        case model::AppPage::Plugins:
            return {"Plugins", "Plugin install and settings arrive with the plugin manager."};
        case model::AppPage::SignIn:
            return {"Sign in", "QR sign-in arrives with the plugin manager."};
        case model::AppPage::Help:
            return {"Help", "Fn+H shows the keys on every page."};
        default:
            return {"Zero Music", ""};
    }
}

} // namespace

ScreenManager::ScreenManager(viewmodel::BaseViewModel& view_model, AssetManager& assets)
    : view_model_(view_model), assets_(assets) {}

ScreenManager::~ScreenManager() {
    lv_async_call_cancel(flush_requested_page_async, this);

    if (page_observer_) {
        lv_observer_remove(page_observer_);
        page_observer_ = nullptr;
    }
    // Deleting the active screen is what LVGL warns about; leave a blank one.
    if (current_screen_ && current_screen_->root() &&
        current_screen_->root() == lv_screen_active()) {
        lv_screen_load(lv_obj_create(nullptr));
    }
    current_screen_.reset();
}

void ScreenManager::start() {
    if (page_observer_) {
        return;
    }

    page_observer_ = lv_subject_add_observer_with_target(view_model_.current_page_subject(),
                                                         current_page_observer,
                                                         this,
                                                         nullptr);
    flush_requested_page();
}

void ScreenManager::request_page(model::AppPage page) {
    requested_page_ = page;

    if (page_switch_scheduled_) {
        return;
    }

    page_switch_scheduled_ = lv_async_call(flush_requested_page_async, this) == LV_RESULT_OK;
}

void ScreenManager::flush_requested_page() {
    page_switch_scheduled_ = false;

    if (has_loaded_page_ && requested_page_ == loaded_page_) {
        return;
    }

    switch (requested_page_) {
        case model::AppPage::Home:
            load_screen(std::make_unique<screen::HomeScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::NowPlaying:
            load_screen(std::make_unique<screen::NowPlayingScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Browse:
            load_screen(std::make_unique<screen::BrowseScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Search:
            load_screen(std::make_unique<screen::SearchScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Queue:
            load_screen(std::make_unique<screen::QueueScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Lyrics:
            load_screen(std::make_unique<screen::LyricsScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Detail:
            load_screen(std::make_unique<screen::DetailScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Settings:
            load_screen(std::make_unique<screen::SettingsScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Plugins:
            load_screen(std::make_unique<screen::PluginsScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::SignIn:
            load_screen(std::make_unique<screen::SignInScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Install:
            load_screen(std::make_unique<screen::InstallScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Account:
            load_screen(std::make_unique<screen::AccountScreen>(view_model_, assets_), requested_page_);
            break;
        case model::AppPage::Recent:
            load_screen(std::make_unique<screen::RecentScreen>(view_model_, assets_), requested_page_);
            break;
        default: {
            const auto text = placeholder_for(requested_page_);
            load_screen(std::make_unique<screen::MessageScreen>(view_model_, assets_, text.title, text.message),
                        requested_page_);
            break;
        }
    }
}

bool ScreenManager::handle_key(std::uint32_t key, bool long_pressed) {
    return current_screen_ && current_screen_->handle_key(key, long_pressed);
}

lv_obj_t* ScreenManager::current_screen() const {
    return current_screen_ ? current_screen_->root() : nullptr;
}

void ScreenManager::load_screen(std::unique_ptr<screen::BaseScreen> next_screen, model::AppPage page) {
    if (!next_screen) {
        return;
    }
    next_screen->init();
    if (!next_screen->root()) {
        return;
    }

    lv_screen_load(next_screen->root());
    current_screen_ = std::move(next_screen);
    loaded_page_ = page;
    has_loaded_page_ = true;
}

} // namespace app
