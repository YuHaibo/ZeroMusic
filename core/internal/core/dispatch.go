package core

import (
	"encoding/json"
	"errors"
	"time"

	"zeromusic/core/internal/ipc"
	"zeromusic/core/internal/music"
)

type helloParams struct {
	Protocol  int    `json:"protocol"`
	UIVersion string `json:"uiVersion"`
}

type idParams struct {
	ID string `json:"id"`
}

type folderParams struct {
	Path string `json:"path"`
}

type playTracksParams struct {
	Source string        `json:"source"`
	Tracks []music.Track `json:"tracks"`
	Start  int           `json:"start"`
}

type seekParams struct {
	Position int64 `json:"position"`
}

type volumeParams struct {
	Volume float64 `json:"volume"`
}

type shuffleParams struct {
	On *bool `json:"on"`
}

type repeatParams struct {
	Mode string `json:"mode"`
}

type jumpParams struct {
	Index int `json:"index"`
}

type enabledParams struct {
	ID string `json:"id"`
	On bool   `json:"on"`
}

type sourceCallParams struct {
	Source string          `json:"source"`
	Method string          `json:"method"`
	Params json.RawMessage `json:"params"`
}

type pluginSettingParams struct {
	Source string `json:"source"`
	Key    string `json:"key"`
	Value  string `json:"value"`
}

func decodeParams[T any](raw json.RawMessage) (T, error) {
	var v T
	if len(raw) == 0 || string(raw) == "null" {
		return v, nil
	}
	if err := json.Unmarshal(raw, &v); err != nil {
		return v, ipc.Errorf(ipc.CodeInvalid, "bad params: %v", err)
	}
	return v, nil
}

// dispatch answers a request: hello and quit at once, the rest on the loop,
// where the player and the source may be touched.
func (a *App) dispatch(method string, raw json.RawMessage) (any, error) {
	switch method {
	case "hello":
		p, err := decodeParams[helloParams](raw)
		if err != nil {
			return nil, err
		}
		if p.Protocol != 0 && p.Protocol != ipc.Protocol {
			return nil, ipc.Errorf(ipc.CodeUnsupported, "protocol %d; this core speaks %d", p.Protocol, ipc.Protocol)
		}
		return map[string]any{
			"app":      "Zero Music",
			"version":  Version,
			"protocol": ipc.Protocol,
			"capabilities": map[string]bool{
				"webSignIn": false,
				"spawn":     false,
				"remote":    false,
				"http":      true,
				"lyrics":    true,
				"qr":        true,
			},
			"screen": map[string]int{"width": 320, "height": 170},
		}, nil
	case "quit":
		time.AfterFunc(50*time.Millisecond, a.Quit)
		return struct{}{}, nil
	}

	var result any
	var err error
	a.Do(func() { result, err = a.onLoop(method, raw) })
	if err != nil {
		return nil, rpcError(err)
	}
	return result, nil
}

func (a *App) onLoop(method string, raw json.RawMessage) (any, error) {
	switch method {
	case "sources.list":
		return a.sources(), nil
	case "sources.setEnabled":
		p, err := decodeParams[enabledParams](raw)
		if err != nil {
			return nil, err
		}
		if err := a.setSourceEnabled(p.ID, p.On); err != nil {
			return nil, err
		}
		return struct{}{}, nil

	case "source.call":
		p, err := decodeParams[sourceCallParams](raw)
		if err != nil {
			return nil, err
		}
		src := a.sourceOf(p.Source)
		if src == nil {
			return nil, ipc.Errorf(ipc.CodeUnavailable, "no such source: %s", p.Source)
		}
		return sourceCall(src, p.Method, p.Params)

	case "library.folders":
		return a.local.Folders(), nil
	case "library.addFolder":
		p, err := decodeParams[folderParams](raw)
		if err != nil {
			return nil, err
		}
		if p.Path == "" {
			return nil, ipc.Errorf(ipc.CodeInvalid, "no folder given")
		}
		a.local.AddFolders(p.Path)
		return a.local.Folders(), nil
	case "library.removeFolder":
		p, err := decodeParams[folderParams](raw)
		if err != nil {
			return nil, err
		}
		a.local.RemoveFolder(p.Path)
		return a.local.Folders(), nil
	case "library.rescan":
		a.local.Rescan()
		return map[string]bool{"started": true}, nil

	case "player.playTracks":
		p, err := decodeParams[playTracksParams](raw)
		if err != nil {
			return nil, err
		}
		if len(p.Tracks) == 0 {
			return nil, ipc.Errorf(ipc.CodeInvalid, "no tracks given")
		}
		source := p.Source
		if source == "" {
			source = p.Tracks[0].Source
		}
		if a.sourceOf(source) == nil {
			return nil, ipc.Errorf(ipc.CodeUnavailable, "no such source: %s", source)
		}
		tracks := make([]music.Track, len(p.Tracks))
		for i, t := range p.Tracks {
			if t.Source == "" {
				t.Source = source
			}
			tracks[i] = t
		}
		start := min(max(p.Start, 0), len(tracks)-1)
		a.pl.PlayTrack(tracks, start)
		return struct{}{}, nil
	case "player.toggle":
		a.pl.TogglePlay()
		return struct{}{}, nil
	case "player.next":
		a.pl.Next()
		return struct{}{}, nil
	case "player.prev":
		a.pl.Prev()
		return struct{}{}, nil
	case "player.seek":
		p, err := decodeParams[seekParams](raw)
		if err != nil {
			return nil, err
		}
		if p.Position < 0 {
			return nil, ipc.Errorf(ipc.CodeInvalid, "negative position")
		}
		a.pl.Seek(time.Duration(p.Position) * time.Millisecond)
		return struct{}{}, nil
	case "player.volume":
		p, err := decodeParams[volumeParams](raw)
		if err != nil {
			return nil, err
		}
		if p.Volume < 0 || p.Volume > 1 {
			return nil, ipc.Errorf(ipc.CodeInvalid, "volume %v is outside 0..1", p.Volume)
		}
		a.settings.Volume = p.Volume
		a.pl.SetVolume(p.Volume)
		a.saveSettingsSoon()
		return struct{}{}, nil
	case "player.shuffle":
		p, err := decodeParams[shuffleParams](raw)
		if err != nil {
			return nil, err
		}
		if p.On == nil {
			a.pl.ToggleShuffle()
		} else {
			a.pl.SetShuffle(*p.On)
		}
		a.settings.Shuffle = a.pl.Shuffle()
		a.saveSettingsSoon()
		return struct{}{}, nil
	case "player.repeat":
		p, err := decodeParams[repeatParams](raw)
		if err != nil {
			return nil, err
		}
		mode, ok := repeatMode(p.Mode)
		if !ok {
			return nil, ipc.Errorf(ipc.CodeInvalid, "repeat %q is not off, all or one", p.Mode)
		}
		a.pl.SetRepeat(mode)
		a.settings.Repeat = int(mode)
		a.saveSettingsSoon()
		return struct{}{}, nil
	case "player.jump":
		p, err := decodeParams[jumpParams](raw)
		if err != nil {
			return nil, err
		}
		if p.Index < 0 || p.Index >= len(a.pl.Queue()) {
			return nil, ipc.Errorf(ipc.CodeInvalid, "no track %d in the queue", p.Index)
		}
		a.pl.Jump(p.Index)
		return struct{}{}, nil
	case "player.remove":
		p, err := decodeParams[jumpParams](raw)
		if err != nil {
			return nil, err
		}
		return nil, ipc.Errorf(ipc.CodeUnsupported, "removing track %d is not implemented yet", p.Index)
	case "player.state":
		return a.state(true), nil

	case "settings.get":
		return a.settings, nil
	case "settings.set":
		patch, err := decodeParams[settingsPatch](raw)
		if err != nil {
			return nil, err
		}
		next, err := a.settings.apply(patch)
		if err != nil {
			return nil, ipc.Errorf(ipc.CodeInvalid, "%v", err)
		}
		a.settings = next
		a.applySettings()
		if err := a.settings.Save(a.settingsPath); err != nil {
			a.log.Printf("settings: %v", err)
		}
		return a.settings, nil

	case "session.get":
		s, ok := a.pl.Session()
		if !ok {
			return nil, nil
		}
		return s, nil
	case "session.clear":
		a.clearSession()
		return struct{}{}, nil

	case "plugin.settings":
		return a.pluginSettings(raw)
	case "plugin.setSetting":
		p, err := decodeParams[pluginSettingParams](raw)
		if err != nil {
			return nil, err
		}
		src := a.sourceOf(p.Source)
		if src == nil {
			return nil, ipc.Errorf(ipc.CodeUnavailable, "no such source: %s", p.Source)
		}
		if err := src.SetSetting(p.Key, p.Value); err != nil {
			return nil, err
		}
		return struct{}{}, nil
	case "plugin.signIn":
		p, err := decodeParams[idParams](raw)
		if err != nil {
			return nil, err
		}
		src := a.sourceOf(p.ID)
		if src == nil {
			return nil, ipc.Errorf(ipc.CodeUnavailable, "no such source: %s", p.ID)
		}
		acct, err := src.SignIn()
		if err != nil {
			return nil, err
		}
		a.publishEvent("sources.changed", a.sources())
		return acct, nil
	case "plugin.signOut":
		p, err := decodeParams[idParams](raw)
		if err != nil {
			return nil, err
		}
		src := a.sourceOf(p.ID)
		if src == nil {
			return nil, ipc.Errorf(ipc.CodeUnavailable, "no such source: %s", p.ID)
		}
		if err := src.SignOut(); err != nil {
			return nil, err
		}
		a.publishEvent("sources.changed", a.sources())
		return struct{}{}, nil
	case "plugin.account":
		p, err := decodeParams[idParams](raw)
		if err != nil {
			return nil, err
		}
		src := a.sourceOf(p.ID)
		if src == nil {
			return nil, ipc.Errorf(ipc.CodeUnavailable, "no such source: %s", p.ID)
		}
		return src.Account()
	case "plugins.list":
		return a.pluginsInfo(), nil
	case "plugin.grant":
		p, err := decodeParams[enabledParams](raw)
		if err != nil {
			return nil, err
		}
		pl := a.pluginsByID[p.ID]
		if pl == nil {
			return nil, ipc.Errorf(ipc.CodeUnavailable, "no such plugin: %s", p.ID)
		}
		if err := pl.Grant(p.On); err != nil {
			return nil, err
		}
		a.publishEvent("sources.changed", a.sources())
		return a.pluginsInfo(), nil
	case "plugin.install", "plugin.update", "plugin.remove":
		return nil, ipc.Errorf(ipc.CodeUnsupported, "plugins are not installed yet")

	case "ping":
		return map[string]bool{"ok": true}, nil
	default:
		return nil, ipc.Errorf(ipc.CodeUnsupported, "no such method: %s", method)
	}
}

func (a *App) saveSettingsSoon() {
	if err := a.settings.Save(a.settingsPath); err != nil {
		a.log.Printf("settings: %v", err)
	}
}

func (a *App) pluginSettings(raw json.RawMessage) (any, error) {
	p, err := decodeParams[idParams](raw)
	if err != nil {
		return nil, err
	}
	src := a.sourceOf(p.ID)
	if src == nil {
		return nil, ipc.Errorf(ipc.CodeUnavailable, "no such source: %s", p.ID)
	}
	info := src.Info()
	values := make(map[string]string, len(info.Settings))
	for _, s := range info.Settings {
		values[s.Key] = src.Setting(s.Key)
	}
	return map[string]any{"settings": info.Settings, "values": values}, nil
}

// sourceCall runs one of a source's methods. It keeps the plugin and local
// APIs the same, as MyGo Music's plugins expect them.
func sourceCall(src music.Source, method string, raw json.RawMessage) (any, error) {
	switch method {
	case "home":
		return src.Home()
	case "browse":
		p, err := decodeParams[struct {
			Page string `json:"page"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return src.Browse(p.Page)
	case "search":
		p, err := decodeParams[struct {
			Query  string           `json:"query"`
			Kind   music.SearchKind `json:"kind"`
			Offset int              `json:"offset"`
		}](raw)
		if err != nil {
			return nil, err
		}
		if p.Query == "" {
			return nil, ipc.Errorf(ipc.CodeInvalid, "empty query")
		}
		if p.Kind == "" {
			p.Kind = music.SearchTracks
		}
		return src.Search(p.Query, p.Kind, p.Offset)
	case "suggest":
		p, err := decodeParams[struct {
			Query string `json:"query"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return src.Suggest(p.Query)
	case "playlists":
		return src.Playlists()
	case "playlist":
		p, err := decodeParams[idParams](raw)
		if err != nil {
			return nil, err
		}
		return src.Playlist(p.ID)
	case "album":
		p, err := decodeParams[idParams](raw)
		if err != nil {
			return nil, err
		}
		return src.Album(p.ID)
	case "artist":
		p, err := decodeParams[idParams](raw)
		if err != nil {
			return nil, err
		}
		return src.Artist(p.ID)
	case "liked":
		p, err := decodeParams[struct {
			Offset int `json:"offset"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return src.Liked(p.Offset)
	case "recent":
		return src.Recent()
	case "isLiked":
		p, err := decodeParams[struct {
			IDs []string `json:"ids"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return src.IsLiked(p.IDs)
	case "like":
		p, err := decodeParams[struct {
			ID string `json:"id"`
			On bool   `json:"on"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return struct{}{}, src.Like(p.ID, p.On)
	case "lyrics":
		p, err := decodeParams[idParams](raw)
		if err != nil {
			return nil, err
		}
		return src.Lyrics(p.ID)
	case "stream":
		p, err := decodeParams[struct {
			ID      string `json:"id"`
			Quality string `json:"quality"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return src.Stream(p.ID, p.Quality)
	case "createPlaylist":
		p, err := decodeParams[struct {
			Name string `json:"name"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return src.CreatePlaylist(p.Name)
	case "renamePlaylist":
		p, err := decodeParams[struct {
			ID   string `json:"id"`
			Name string `json:"name"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return struct{}{}, src.RenamePlaylist(p.ID, p.Name)
	case "deletePlaylist":
		p, err := decodeParams[idParams](raw)
		if err != nil {
			return nil, err
		}
		return struct{}{}, src.DeletePlaylist(p.ID)
	case "addToPlaylist":
		p, err := decodeParams[struct {
			ID       string   `json:"id"`
			TrackIDs []string `json:"trackIds"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return struct{}{}, src.AddToPlaylist(p.ID, p.TrackIDs)
	case "removeFromPlaylist":
		p, err := decodeParams[struct {
			ID       string   `json:"id"`
			TrackIDs []string `json:"trackIds"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return struct{}{}, src.RemoveFromPlaylist(p.ID, p.TrackIDs)
	case "movePlaylistTrack":
		p, err := decodeParams[struct {
			ID   string `json:"id"`
			From int    `json:"from"`
			To   int    `json:"to"`
		}](raw)
		if err != nil {
			return nil, err
		}
		return struct{}{}, src.MovePlaylistTrack(p.ID, p.From, p.To)
	case "signIn":
		return src.SignIn()
	case "signOut":
		return struct{}{}, src.SignOut()
	case "account":
		return src.Account()
	default:
		return nil, ipc.Errorf(ipc.CodeUnsupported, "source %s does not do %s", src.Info().ID, method)
	}
}

// rpcError gives err the code the UI acts on.
func rpcError(err error) error {
	if err == nil {
		return nil
	}
	var rpc *ipc.Error
	if errors.As(err, &rpc) {
		return rpc
	}
	switch {
	case errors.Is(err, music.ErrSignIn):
		return &ipc.Error{Code: ipc.CodeSignIn, Message: err.Error()}
	case errors.Is(err, music.ErrSetup):
		return &ipc.Error{Code: ipc.CodeSetup, Message: err.Error()}
	case errors.Is(err, music.ErrUnavailable):
		return &ipc.Error{Code: ipc.CodeUnavailable, Message: err.Error()}
	case errors.Is(err, music.ErrUnsupported):
		return &ipc.Error{Code: ipc.CodeUnsupported, Message: err.Error()}
	}
	return &ipc.Error{Code: ipc.CodeInternal, Message: err.Error()}
}
