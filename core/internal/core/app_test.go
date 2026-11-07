package core

import (
	"context"
	"encoding/json"
	"io"
	"log"
	"testing"
	"time"

	"zeromusic/core/internal/ipc"
)

func testApp(t *testing.T) *App {
	t.Helper()
	dirs := Dirs{
		Config: t.TempDir(),
		Data:   t.TempDir(),
		Cache:  t.TempDir(),
		State:  t.TempDir(),
	}
	app, err := New(Options{Dirs: dirs, Logger: log.New(io.Discard, "", 0), SkipDefaultFolders: true})
	if err != nil {
		t.Fatal(err)
	}
	ctx, cancel := context.WithCancel(context.Background())
	done := make(chan struct{})
	go func() {
		app.Run(ctx)
		close(done)
	}()
	t.Cleanup(func() {
		cancel()
		select {
		case <-done:
		case <-time.After(5 * time.Second):
			t.Error("the core did not stop")
		}
	})
	return app
}

func call(t *testing.T, app *App, method string, params any) (any, error) {
	t.Helper()
	var raw json.RawMessage
	if params != nil {
		b, err := json.Marshal(params)
		if err != nil {
			t.Fatal(err)
		}
		raw = b
	}
	return app.Handle(method, raw)
}

// The UI learns what it speaks to from hello.
func TestHello(t *testing.T) {
	app := testApp(t)
	got, err := call(t, app, "hello", map[string]any{"protocol": 1, "uiVersion": "0.1.0"})
	if err != nil {
		t.Fatal(err)
	}
	m := got.(map[string]any)
	if m["app"] != "Zero Music" || m["protocol"] != ipc.Protocol {
		t.Errorf("hello: %+v", m)
	}
}

// A protocol the core doesn't speak is refused, rather than half understood.
func TestHelloProtocol(t *testing.T) {
	app := testApp(t)
	_, err := call(t, app, "hello", map[string]any{"protocol": 99})
	if ipc.CodeOf(err) != ipc.CodeUnsupported {
		t.Errorf("error: %v", err)
	}
}

// Settings set takes only what it knows, keeps the file, and reports what it
// did not accept.
func TestSettings(t *testing.T) {
	app := testApp(t)
	got, err := call(t, app, "settings.set", map[string]any{"volume": 0.3, "quality": "lossless"})
	if err != nil {
		t.Fatal(err)
	}
	s := got.(Settings)
	if s.Volume != 0.3 || s.Quality != "lossless" {
		t.Errorf("settings: %+v", s)
	}
	if _, err := call(t, app, "settings.set", map[string]any{"volume": 2.0}); ipc.CodeOf(err) != ipc.CodeInvalid {
		t.Errorf("loud volume: %v", err)
	}
	loaded, err := LoadSettings(app.settingsPath)
	if err != nil {
		t.Fatal(err)
	}
	if loaded.Volume != 0.3 || loaded.Quality != "lossless" {
		t.Errorf("on disk: %+v", loaded)
	}
}

// Folders are added and removed as the listener chose them, and the answer is
// what is there now.
func TestLibraryFolders(t *testing.T) {
	app := testApp(t)
	folder := t.TempDir()
	got, err := call(t, app, "library.addFolder", map[string]any{"path": folder})
	if err != nil {
		t.Fatal(err)
	}
	folders := got.([]string)
	if len(folders) != 1 || folders[0] != folder {
		t.Fatalf("folders: %q", folders)
	}
	got, err = call(t, app, "library.removeFolder", map[string]any{"path": folder})
	if err != nil {
		t.Fatal(err)
	}
	if folders := got.([]string); len(folders) != 0 {
		t.Errorf("after removing: %q", folders)
	}
}

// A source that cannot do what was asked says so with the code the UI knows.
func TestSourceUnsupported(t *testing.T) {
	app := testApp(t)
	_, err := call(t, app, "source.call", map[string]any{"source": "local", "method": "signIn"})
	if ipc.CodeOf(err) != ipc.CodeUnsupported {
		t.Errorf("error: %v", err)
	}
}

// Playing nothing, or from a source that is gone, is refused before the
// player is touched.
func TestPlayTracksRefused(t *testing.T) {
	app := testApp(t)
	if _, err := call(t, app, "player.playTracks", map[string]any{"source": "local"}); ipc.CodeOf(err) != ipc.CodeInvalid {
		t.Errorf("no tracks: %v", err)
	}
	if _, err := call(t, app, "player.playTracks", map[string]any{
		"source": "nowhere",
		"tracks": []map[string]any{{"id": "1", "name": "one"}},
	}); ipc.CodeOf(err) != ipc.CodeUnavailable {
		t.Errorf("no source: %v", err)
	}
}

// The player's changes reach the UI as player.state, with the queue when the
// queue is what changed.
func TestPlayerStateEvent(t *testing.T) {
	app := testApp(t)
	events := make(chan map[string]any, 8)
	app.SetPublisher(func(name string, data any) {
		if name != "player.state" {
			return
		}
		b, _ := json.Marshal(data)
		var m map[string]any
		json.Unmarshal(b, &m)
		select {
		case events <- m:
		default:
		}
	})
	if _, err := call(t, app, "player.shuffle", map[string]any{"on": true}); err != nil {
		t.Fatal(err)
	}
	select {
	case m := <-events:
		if m["shuffle"] != true {
			t.Errorf("state: %+v", m)
		}
	case <-time.After(2 * time.Second):
		t.Fatal("no player.state")
	}
}

// Quit stops the core so the wrapper can reap it.
func TestQuit(t *testing.T) {
	app := testApp(t)
	if _, err := call(t, app, "quit", nil); err != nil {
		t.Fatal(err)
	}
	select {
	case <-app.Done():
	case <-time.After(2 * time.Second):
		t.Fatal("quit did not stop the core")
	}
}
