package core

import (
	"fmt"
	"os/exec"
	"runtime"
	"slices"

	"zeromusic/core/internal/music"
	"zeromusic/core/internal/plugin"
)

// loadPlugins discovers the installed plugins, keeping the ones that loaded.
// It replaces any loaded before.
func (a *App) loadPlugins() {
	plugin.SetAppVersion(Version)
	plugin.SetOpenURL(openInBrowser)
	for _, p := range a.plugins {
		p.Close()
	}
	a.plugins = nil
	a.pluginsByID = map[string]*plugin.Plugin{}
	for _, p := range plugin.Discover(a.dirs.PluginsDir(), a.dirs.PluginDataDir()) {
		id := p.Info().ID
		if id == "" || id == "local" || a.pluginsByID[id] != nil {
			a.log.Printf("plugin: ignoring one with id %q", id)
			continue
		}
		a.plugins = append(a.plugins, p)
		a.pluginsByID[id] = p
		p.OnSignInPrompt(func(prompt music.SignInPrompt) {
			a.publishEvent("plugin.prompt", map[string]any{
				"plugin": id,
				"qr":     prompt.QR,
				"text":   prompt.Text,
			})
		})
		a.log.Printf("plugin: loaded %s %s", id, p.Info().Version)
	}
}

func openInBrowser(address string) error {
	cmd := "xdg-open"
	if runtime.GOOS == "darwin" {
		cmd = "open"
	}
	return exec.Command(cmd, address).Start()
}

// pluginEnabled reports whether the listener left the source on.
func (a *App) pluginEnabled(id string) bool {
	return !slices.Contains(a.settings.DisabledSources, id)
}

// pluginsInfo is what plugins.list shows.
func (a *App) pluginsInfo() []map[string]any {
	out := make([]map[string]any, 0, len(a.plugins))
	for _, p := range a.plugins {
		info := p.Info()
		enabled := a.pluginEnabled(info.ID)
		out = append(out, map[string]any{
			"id":          info.ID,
			"name":        info.Name,
			"version":     info.Version,
			"playback":    string(info.Playback),
			"enabled":     enabled,
			"granted":     p.Granted(),
			"permissions": p.Permissions(),
			"settings":    info.Settings,
		})
	}
	return out
}

// setSourceEnabled turns a plugin off or on, keeping the choice and closing
// its runtime when it goes off.
func (a *App) setSourceEnabled(id string, on bool) error {
	p := a.pluginMap()[id]
	if p == nil {
		if id == "local" {
			if on {
				return nil
			}
			return fmt.Errorf("the local source cannot be disabled")
		}
		return fmt.Errorf("no such source: %s", id)
	}
	disabled := slices.Clone(a.settings.DisabledSources)
	if on {
		disabled = slices.DeleteFunc(disabled, func(s string) bool { return s == id })
	} else if !slices.Contains(disabled, id) {
		disabled = append(disabled, id)
		p.Close()
	}
	a.settings.DisabledSources = uniqueStrings(disabled)
	if err := a.settings.Save(a.settingsPath); err != nil {
		return err
	}
	a.publishEvent("sources.changed", a.sources())
	return nil
}

func (a *App) pluginMap() map[string]*plugin.Plugin { return a.pluginsByID }
