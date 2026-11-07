package core

import (
	"context"
	"io"
	"log"
	"os"
	"path/filepath"
	"testing"
	"time"

	"zeromusic/core/internal/ipc"
	"zeromusic/core/internal/music"
)

// pluginApp starts a core with the demo plugin installed, as an install would
// leave it.
func pluginApp(t *testing.T) *App {
	t.Helper()
	dirs := Dirs{
		Config: t.TempDir(),
		Data:   t.TempDir(),
		Cache:  t.TempDir(),
		State:  t.TempDir(),
	}
	if err := os.MkdirAll(dirs.PluginsDir(), 0o700); err != nil {
		t.Fatal(err)
	}
	if err := os.CopyFS(filepath.Join(dirs.PluginsDir(), "demo"), os.DirFS("../../examples/demo-plugin")); err != nil {
		t.Fatal(err)
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

// A plugin in the plugins directory is a source, and its methods run over
// source.call like the local one's.
func TestPluginSource(t *testing.T) {
	app := pluginApp(t)
	got, err := call(t, app, "sources.list", nil)
	if err != nil {
		t.Fatal(err)
	}
	var found bool
	for _, s := range got.([]sourceInfo) {
		if s.ID == "demo" {
			found = true
			if s.Kind != "plugin" || !s.Enabled {
				t.Errorf("demo: %+v", s)
			}
		}
	}
	if !found {
		t.Fatal("demo is not a source")
	}

	got, err = call(t, app, "source.call", map[string]any{
		"source": "demo",
		"method": "search",
		"params": map[string]any{"query": "song", "kind": "tracks"},
	})
	if err != nil {
		t.Fatal(err)
	}
	tracks := got.(*music.SearchResult).Tracks
	if len(tracks) == 0 {
		t.Fatal("the demo found no songs")
	}
}

// Turning a plugin off hides it and refuses its methods until it is on again.
func TestDisablePlugin(t *testing.T) {
	app := pluginApp(t)
	if _, err := call(t, app, "sources.setEnabled", map[string]any{"id": "demo", "on": false}); err != nil {
		t.Fatal(err)
	}
	if _, err := call(t, app, "source.call", map[string]any{
		"source": "demo",
		"method": "search",
		"params": map[string]any{"query": "song"},
	}); ipc.CodeOf(err) != ipc.CodeUnavailable {
		t.Errorf("disabled plugin: %v", err)
	}
	if _, err := call(t, app, "sources.setEnabled", map[string]any{"id": "demo", "on": true}); err != nil {
		t.Fatal(err)
	}
	if _, err := call(t, app, "source.call", map[string]any{
		"source": "demo",
		"method": "search",
		"params": map[string]any{"query": "song"},
	}); err != nil {
		t.Errorf("enabled plugin: %v", err)
	}
}
