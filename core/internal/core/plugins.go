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
	var loaded []*plugin.Plugin
	byID := map[string]*plugin.Plugin{}
	for _, p := range plugin.Discover(a.dirs.PluginsDir(), a.dirs.PluginDataDir()) {
		id := p.Info().ID
		if id == "" || id == "local" || byID[id] != nil {
			a.log.Printf("plugin: ignoring one with id %q", id)
			p.Close()
			continue
		}
		loaded = append(loaded, p)
		byID[id] = p
		p.OnSignInPrompt(func(prompt music.SignInPrompt) {
			a.publishEvent("plugin.prompt", map[string]any{
				"plugin": id,
				"qr":     prompt.QR,
				"text":   prompt.Text,
			})
		})
		a.log.Printf("plugin: loaded %s %s", id, p.Info().Version)
	}
	a.srcMu.Lock()
	old := a.plugins
	a.plugins, a.pluginsByID = loaded, byID
	a.srcMu.Unlock()
	for _, p := range old {
		p.Close()
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
	a.settingsMu.RLock()
	defer a.settingsMu.RUnlock()
	return !slices.Contains(a.settings.DisabledSources, id)
}

// pluginsInfo is what plugins.list shows.
func (a *App) pluginsInfo() []map[string]any {
	a.srcMu.RLock()
	plugins := append([]*plugin.Plugin(nil), a.plugins...)
	a.srcMu.RUnlock()
	out := make([]map[string]any, 0, len(plugins))
	for _, p := range plugins {
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
			"signIn":      p.Can("signIn"),
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
	a.settingsMu.Lock()
	disabled := slices.Clone(a.settings.DisabledSources)
	if on {
		disabled = slices.DeleteFunc(disabled, func(s string) bool { return s == id })
	} else if !slices.Contains(disabled, id) {
		disabled = append(disabled, id)
		p.Close()
	}
	a.settings.DisabledSources = uniqueStrings(disabled)
	a.settingsMu.Unlock()
	if err := a.settings.Save(a.settingsPath); err != nil {
		return err
	}
	a.publishEvent("sources.changed", a.sources())
	return nil
}

func (a *App) pluginMap() map[string]*plugin.Plugin {
	a.srcMu.RLock()
	defer a.srcMu.RUnlock()
	out := make(map[string]*plugin.Plugin, len(a.pluginsByID))
	for id, p := range a.pluginsByID {
		out[id] = p
	}
	return out
}

// installPlugin downloads a GitHub repository's latest release in the
// background, then reloads the sources on the loop.
func (a *App) installPlugin(repo string) error {
	parsed, err := plugin.ParseRepo(repo)
	if err != nil {
		return err
	}
	if !a.installMu.TryLock() {
		return fmt.Errorf("another plugin is being installed")
	}
	a.publishEvent("plugin.install", map[string]any{"repo": parsed, "phase": "download", "progress": 0})
	go func() {
		defer a.installMu.Unlock()
		pending, err := plugin.Fetch(parsed, a.dirs.PluginsDir())
		if err != nil {
			a.publishEvent("plugin.install", map[string]any{"repo": parsed, "phase": "error", "error": err.Error()})
			return
		}
		id := pending.Manifest.ID
		if err := pending.Install(a.dirs.PluginsDir()); err != nil {
			a.publishEvent("plugin.install", map[string]any{"id": id, "phase": "error", "error": err.Error()})
			return
		}
		a.publishEvent("plugin.install", map[string]any{"id": id, "phase": "done", "progress": 1})
		a.post(func() {
			a.loadPlugins()
			a.publishEvent("sources.changed", a.sources())
		})
	}()
	return nil
}

func (a *App) updatePlugin(id string) error {
	pl := a.pluginMap()[id]
	if pl == nil {
		return fmt.Errorf("no such plugin: %s", id)
	}
	origin := pl.Origin()
	if origin == nil {
		return fmt.Errorf("%s was not installed from a repository", id)
	}
	return a.installPlugin(origin.Repo)
}

func (a *App) removePlugin(id string) error {
	pl := a.pluginMap()[id]
	if pl == nil {
		return fmt.Errorf("no such plugin: %s", id)
	}
	name := pl.Info().Name
	if err := pl.Remove(nil); err != nil {
		return err
	}
	a.loadPlugins()
	a.publishEvent("sources.changed", a.sources())
	a.publishEvent("toast", map[string]any{"text": "Removed " + name, "kind": "success"})
	return nil
}
