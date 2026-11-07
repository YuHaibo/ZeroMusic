// Command zero-music-core is the head of Zero Music: the library, playback,
// plugins and settings, answering the LVGL UI over a Unix socket.
package main

import (
	"context"
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"os/signal"
	"runtime/debug"
	"syscall"

	"zeromusic/core/internal/core"
	"zeromusic/core/internal/ipc"
)

func main() {
	var (
		socket  = flag.String("socket", "", "Unix socket to listen on (default $XDG_RUNTIME_DIR/zero-music.sock)")
		headless = flag.Bool("headless", false, "run without a UI (the core always does; accepted for scripts)")
		configDir = flag.String("config-dir", "", "override the config directory")
		dataDir   = flag.String("data-dir", "", "override the data directory")
		cacheDir  = flag.String("cache-dir", "", "override the cache directory")
		stateDir  = flag.String("state-dir", "", "override the state directory")
		logPath   = flag.String("log", "", "write the log to this file as well as stderr")
		version   = flag.Bool("version", false, "print the version and exit")
	)
	flag.Parse()
	if *version {
		fmt.Println("zero-music-core", core.Version)
		return
	}
	_ = *headless

	debug.SetMemoryLimit(180 << 20)
	debug.SetGCPercent(40)

	logger := log.New(os.Stderr, "zero-music-core: ", log.LstdFlags|log.Lmicroseconds)
	if *logPath != "" {
		f, err := os.OpenFile(*logPath, os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0o600)
		if err != nil {
			logger.Printf("log: %v", err)
		} else {
			defer f.Close()
			logger.SetOutput(io.MultiWriter(os.Stderr, f))
		}
	}

	dirs := core.DefaultDirs()
	if *configDir != "" {
		dirs.Config = *configDir
	}
	if *dataDir != "" {
		dirs.Data = *dataDir
	}
	if *cacheDir != "" {
		dirs.Cache = *cacheDir
	}
	if *stateDir != "" {
		dirs.State = *stateDir
	}
	path := core.SocketPath(*socket, dirs)

	app, err := core.New(core.Options{Dirs: dirs, Logger: logger})
	if err != nil {
		logger.Fatalf("%v", err)
	}
	server, err := ipc.Listen(path, app, logger)
	if err != nil {
		logger.Fatalf("%v", err)
	}
	app.SetPublisher(server.Broadcast)

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	runDone := make(chan struct{})
	go func() {
		defer close(runDone)
		app.Run(ctx)
	}()
	serveDone := make(chan struct{})
	go func() {
		defer close(serveDone)
		if err := server.Serve(); err != nil {
			logger.Printf("ipc: %v", err)
		}
	}()

	logger.Printf("listening on %s", path)
	select {
	case <-ctx.Done():
	case <-app.Done():
	}
	stop()
	server.Close()
	<-runDone
	<-serveDone
	logger.Printf("stopped")
}
