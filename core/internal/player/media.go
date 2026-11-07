package player

import "time"

// MediaSession is the system integration that shows what plays and takes
// commands back. The device has none; a desktop can plug in MPRIS or the
// platform's controls. A nil Media is the same as NoopMedia.
type MediaSession interface {
	// Set says what plays now, with its cover once loaded.
	Set(MediaInfo)
	// SetPlayback says whether it plays and where it is.
	SetPlayback(playing bool, position time.Duration)
	// Clear says nothing plays.
	Clear()
	// OnCommand hears the listener's commands from the system.
	OnCommand(func(cmd MediaCommand, pos time.Duration))
}

// MediaCommand is what a MediaSession asks the player to do.
type MediaCommand int

const (
	MediaPlay MediaCommand = iota
	MediaPause
	MediaNext
	MediaPrevious
	MediaSeek
)

// MediaInfo is what a MediaSession shows.
type MediaInfo struct {
	Title    string
	Artist   string
	Album    string
	Artwork  []byte
	Duration time.Duration
	Position time.Duration
	Playing  bool
}

// NoopMedia is a MediaSession for a device with no system integration.
type NoopMedia struct{}

func (NoopMedia) Set(MediaInfo)                               {}
func (NoopMedia) SetPlayback(bool, time.Duration)             {}
func (NoopMedia) Clear()                                      {}
func (NoopMedia) OnCommand(func(MediaCommand, time.Duration)) {}

func (p *Player) media() MediaSession {
	if p.Media == nil {
		return NoopMedia{}
	}
	return p.Media
}
