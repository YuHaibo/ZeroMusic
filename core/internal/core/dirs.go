// Package core is the Zero Music core: the local library, the player, plugin
// sources, settings and the session, all behind the IPC methods in
// internal/ipc.
package core

import (
	"fmt"
	"os"
	"path/filepath"
)

// Dirs are where the core keeps what outlives a run.
type Dirs struct {
	Config  string
	Data    string
	Cache   string
	State   string
	Runtime string
}

// DefaultDirs follows XDG, with the fallbacks a device without those
// variables can use.
func DefaultDirs() Dirs {
	home, _ := os.UserHomeDir()
	cfg := os.Getenv("XDG_CONFIG_HOME")
	if cfg == "" {
		cfg = filepath.Join(home, ".config")
	}
	data := os.Getenv("XDG_DATA_HOME")
	if data == "" {
		data = filepath.Join(home, ".local", "share")
	}
	cache := os.Getenv("XDG_CACHE_HOME")
	if cache == "" {
		cache = filepath.Join(home, ".cache")
	}
	state := os.Getenv("XDG_STATE_HOME")
	if state == "" {
		state = filepath.Join(home, ".local", "state")
	}
	runtime := os.Getenv("XDG_RUNTIME_DIR")
	if runtime == "" {
		runtime = os.TempDir()
	}
	return Dirs{
		Config:  filepath.Join(cfg, "zero-music"),
		Data:    filepath.Join(data, "zero-music"),
		Cache:   filepath.Join(cache, "zero-music"),
		State:   filepath.Join(state, "zero-music"),
		Runtime: runtime,
	}
}

// SettingsFile is where the settings are.
func (d Dirs) SettingsFile() string { return filepath.Join(d.Config, "settings.json") }

// SessionFile is where where-playback-was is.
func (d Dirs) SessionFile() string { return filepath.Join(d.Data, "session.json") }

// LocalDir is the local source's data: library.json, index.json and covers.
func (d Dirs) LocalDir() string { return filepath.Join(d.Data, "local") }

// PluginsDir holds installed plugins, one directory each.
func (d Dirs) PluginsDir() string { return filepath.Join(d.Data, "plugins") }

// PluginDataDir holds what plugins keep for themselves.
func (d Dirs) PluginDataDir() string { return filepath.Join(d.Data, "plugin-data") }

// SongCache is where played songs are kept. Under data, not cache, so a
// cache cleaner or a reboot doesn't take the listener's music away.
func (d Dirs) SongCache() string { return filepath.Join(d.Data, "songs") }

// CoverCache is where covers decoded by the image service are kept, if any.
func (d Dirs) CoverCache() string { return filepath.Join(d.Cache, "covers") }

// LogsDir is where logs go, when the core writes them.
func (d Dirs) LogsDir() string { return filepath.Join(d.State, "logs") }

// Socket is the default socket path, which only this user can reach.
func (d Dirs) Socket() string {
	if d.Runtime != "" {
		return filepath.Join(d.Runtime, "zero-music.sock")
	}
	return filepath.Join(os.TempDir(), fmt.Sprintf("zero-music-%d.sock", os.Getuid()))
}

// abs makes every path absolute, as a plugin's host.dir must be for its file
// URLs, and a socket path for the wrapper.
func (d Dirs) abs() Dirs {
	set := func(s string) string {
		if s == "" || filepath.IsAbs(s) {
			return s
		}
		if abs, err := filepath.Abs(s); err == nil {
			return abs
		}
		return s
	}
	d.Config, d.Data, d.Cache, d.State = set(d.Config), set(d.Data), set(d.Cache), set(d.State)
	d.Runtime = set(d.Runtime)
	return d
}

// Ensure makes every directory, private to the user.
func (d Dirs) Ensure() error {
	for _, dir := range []string{
		d.Config, d.Data, d.Cache, d.State,
		d.LocalDir(), d.PluginsDir(), d.PluginDataDir(), d.SongCache(), d.CoverCache(), d.LogsDir(),
	} {
		if err := os.MkdirAll(dir, 0o700); err != nil {
			return fmt.Errorf("core: making %s: %w", dir, err)
		}
	}
	return nil
}

// MusicDir is where to look for music when nothing was chosen: $XDG_MUSIC_DIR,
// or else ~/Music.
func MusicDir() string {
	if dir := os.Getenv("XDG_MUSIC_DIR"); dir != "" {
		return dir
	}
	home, err := os.UserHomeDir()
	if err != nil {
		return ""
	}
	return filepath.Join(home, "Music")
}

// SocketPath returns the socket named by flags or environment, or the default.
func SocketPath(flag string, dirs Dirs) string {
	if flag != "" {
		return flag
	}
	if p := os.Getenv("ZERO_MUSIC_SOCKET"); p != "" {
		return p
	}
	return dirs.Socket()
}
