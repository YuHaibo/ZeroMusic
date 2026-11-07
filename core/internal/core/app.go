package core

import (
	"context"
	"encoding/json"
	"errors"
	"log"
	"os"
	"sync"
	"time"

	"zeromusic/core/internal/i18n"
	"zeromusic/core/internal/local"
	"zeromusic/core/internal/music"
	"zeromusic/core/internal/player"
	"zeromusic/core/internal/plugin"
)

// Options set up an App.
type Options struct {
	Dirs   Dirs
	Logger *log.Logger
	// SkipDefaultFolders leaves the local source alone at startup, for tests
	// that need only the folders they chose.
	SkipDefaultFolders bool
}

// App is the core's state, which only its loop's goroutine touches: requests
// are brought there by Do, and the player's loads come back there through post.
type App struct {
	dirs Dirs
	log  *log.Logger

	settings     Settings
	settingsPath string
	sessionPath  string

	local       *local.Source
	cache       *player.Cache
	pl          *player.Player
	plugins     []*plugin.Plugin
	pluginsByID map[string]*plugin.Plugin
	srcMu       sync.RWMutex
	settingsMu  sync.RWMutex
	installMu   sync.Mutex

	tasks chan task
	quit  chan struct{}
	once  sync.Once

	publishMu sync.RWMutex
	publish   func(event string, data any)

	dirty    bool
	lastSave time.Time
}

type task struct {
	fn   func()
	done chan struct{}
}

// New prepares the core: directories, settings, the local source and the
// player. It starts nothing until Run.
func New(opts Options) (*App, error) {
	dirs := opts.Dirs
	if dirs.Config == "" && dirs.Data == "" {
		dirs = DefaultDirs()
	}
	dirs = dirs.abs()
	if err := dirs.Ensure(); err != nil {
		return nil, err
	}
	logger := opts.Logger
	if logger == nil {
		logger = log.New(os.Stderr, "zero-music-core: ", log.LstdFlags)
	}
	a := &App{
		dirs:         dirs,
		log:          logger,
		settingsPath: dirs.SettingsFile(),
		sessionPath:  dirs.SessionFile(),
		tasks:        make(chan task, 128),
		quit:         make(chan struct{}),
	}
	settings, err := LoadSettings(a.settingsPath)
	if err != nil {
		logger.Printf("settings: %v; using defaults", err)
	}
	a.settings = settings
	i18n.SetLanguage(i18n.Resolve(settings.Language, []string{os.Getenv("LC_ALL"), os.Getenv("LANG")}))

	a.cache = player.NewCache(dirs.SongCache(), int64(settings.CacheMB)<<20)
	a.pl = player.New(func(fn func()) { a.post(fn) })
	a.pl.Cache = a.cache
	a.pl.Source = a.sourceOf
	a.pl.Quality = func() string { return a.settings.Quality }
	a.pl.Normalize = func() bool { return a.settings.Normalize }
	a.pl.Changed = a.changed
	a.pl.Failed = func(source string, err error) {
		a.log.Printf("player: %s: %v", source, err)
		if errors.Is(err, music.ErrSignIn) {
			a.publishEvent("toast", map[string]any{"text": i18n.T("Sign in to play this song"), "kind": "error"})
		}
	}
	a.pl.SetVolume(settings.Volume)
	a.pl.SetShuffle(settings.Shuffle) // only if such a setter exists; see below
	a.pl.SetRepeat(player.RepeatMode(settings.Repeat))

	a.loadPlugins()

	a.local = local.New(dirs.LocalDir())
	a.local.OnChange(func() {
		a.post(func() {
			a.publishEvent("sources.changed", a.sources())
		})
	})
	if !opts.SkipDefaultFolders && len(a.local.Folders()) == 0 {
		if dir := MusicDir(); dir != "" {
			if st, err := os.Stat(dir); err == nil && st.IsDir() {
				a.local.AddFolders(dir)
			}
		}
	}

	// Where playback was is picked up paused, once the loop can take the load.
	a.post(func() {
		var s player.Session
		if ok, err := readJSON(a.sessionPath, &s); err != nil {
			a.log.Printf("session: %v", err)
		} else if ok {
			a.pl.Restore(s)
		}
	})
	return a, nil
}

// Run answers requests and the player's posts until ctx ends or Quit.
func (a *App) Run(ctx context.Context) {
	ticker := time.NewTicker(500 * time.Millisecond)
	defer ticker.Stop()
	for {
		select {
		case t := <-a.tasks:
			t.fn()
			if t.done != nil {
				close(t.done)
			}
		case <-ticker.C:
			a.tick()
		case <-ctx.Done():
			a.saveSession()
			return
		case <-a.quit:
			a.saveSession()
			return
		}
	}
}

// Quit asks Run to finish, after saving the session.
func (a *App) Quit() {
	a.once.Do(func() { close(a.quit) })
}

// Done is closed when Quit was called.
func (a *App) Done() <-chan struct{} { return a.quit }

// SetPublisher sets where events go. Call it before Run starts.
func (a *App) SetPublisher(fn func(event string, data any)) {
	a.publishMu.Lock()
	a.publish = fn
	a.publishMu.Unlock()
}

// Do runs fn on the core's goroutine and waits for it. It must not be called
// from that goroutine.
func (a *App) Do(fn func()) {
	done := make(chan struct{})
	select {
	case a.tasks <- task{fn: fn, done: done}:
		<-done
	case <-a.quit:
	}
}

// DoValue is Do for an answer.
func DoValue[T any](a *App, fn func() T) T {
	var v T
	a.Do(func() { v = fn() })
	return v
}

// post brings fn to the loop without waiting, as the player does with what
// its goroutines load.
func (a *App) post(fn func()) {
	select {
	case a.tasks <- task{fn: fn}:
	case <-a.quit:
	}
}

func (a *App) publishEvent(event string, data any) {
	a.publishMu.RLock()
	fn := a.publish
	a.publishMu.RUnlock()
	if fn != nil {
		fn(event, data)
	}
}

// changed hears the player: something shown changed, and the session is worth
// saving soon.
func (a *App) changed() {
	a.dirty = true
	a.emitState(true)
}

func (a *App) tick() {
	if a.pl.Playing() {
		a.emitState(false)
	}
	if a.dirty && time.Since(a.lastSave) >= 2*time.Second {
		a.saveSession()
	}
}

func (a *App) saveSession() {
	a.dirty = false
	a.lastSave = time.Now()
	s, ok := a.pl.Session()
	if !ok {
		os.Remove(a.sessionPath)
		return
	}
	if err := writeJSON(a.sessionPath, s); err != nil {
		a.log.Printf("session: %v", err)
	}
	a.settings.Shuffle = s.Shuffle
	a.settings.Repeat = int(s.Repeat)
	if err := a.settings.Save(a.settingsPath); err != nil {
		a.log.Printf("settings: %v", err)
	}
}

func (a *App) clearSession() {
	a.dirty = false
	os.Remove(a.sessionPath)
}

func (a *App) sourceOf(id string) music.Source {
	if id == "" || id == local.ID {
		return a.local
	}
	a.srcMu.RLock()
	defer a.srcMu.RUnlock()
	if !a.pluginEnabled(id) {
		return nil
	}
	p := a.pluginsByID[id]
	if p == nil {
		return nil
	}
	return p
}

// sourceInfo is one row of sources.list.
type sourceInfo struct {
	ID       string   `json:"id"`
	Name     string   `json:"name"`
	Kind     string   `json:"kind"`
	Playback string   `json:"playback"`
	Can      []string `json:"can"`
	Enabled  bool     `json:"enabled"`
	Version  string   `json:"version,omitempty"`
}

var sourceMethods = []string{
	"search", "suggest", "playlists", "playlist", "album", "artist", "liked", "recent",
	"isLiked", "like", "home", "browse", "lyrics", "stream",
	"createPlaylist", "renamePlaylist", "deletePlaylist", "addToPlaylist",
	"removeFromPlaylist", "movePlaylistTrack",
}

func (a *App) sources() []sourceInfo {
	a.srcMu.RLock()
	plugins := append([]*plugin.Plugin(nil), a.plugins...)
	a.srcMu.RUnlock()
	list := []sourceInfo{{
		ID:       local.ID,
		Name:     a.local.Info().Name,
		Kind:     "local",
		Playback: string(a.local.Info().Playback),
		Can:      canList(a.local),
		Enabled:  true,
	}}
	for _, p := range plugins {
		info := p.Info()
		list = append(list, sourceInfo{
			ID:       info.ID,
			Name:     info.Name,
			Kind:     "plugin",
			Playback: string(info.Playback),
			Can:      canList(p),
			Enabled:  a.pluginEnabled(info.ID),
			Version:  info.Version,
		})
	}
	return list
}

func canList(src music.Source) []string {
	var can []string
	for _, m := range sourceMethods {
		if src.Can(m) {
			can = append(can, m)
		}
	}
	return can
}

// Settings returns a copy of the settings.
func (a *App) Settings() Settings { return a.settings }

// applySettings makes the player and cache follow the settings.
func (a *App) applySettings() {
	a.pl.SetVolume(a.settings.Volume)
	a.cache.SetLimit(int64(a.settings.CacheMB) << 20)
	switch a.settings.Repeat {
	case 0, 1, 2:
		a.pl.SetRepeat(player.RepeatMode(a.settings.Repeat))
	}
	if a.settings.Shuffle != a.pl.Shuffle() {
		a.pl.SetShuffle(a.settings.Shuffle)
	}
	i18n.SetLanguage(i18n.Resolve(a.settings.Language, []string{os.Getenv("LC_ALL"), os.Getenv("LANG")}))
}

// Handle answers one IPC request.
func (a *App) Handle(method string, params json.RawMessage) (any, error) {
	return a.dispatch(method, params)
}
