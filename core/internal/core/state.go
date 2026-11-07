package core

import (
	"zeromusic/core/internal/music"
	"zeromusic/core/internal/player"
)

// Version is what hello reports, and what plugins are told the app is: MyGo
// Music 0.2.0 is what their manifests ask for.
const Version = "0.2.0"

// PlaybackState is what Now Playing, the queue and the status bar show.
type PlaybackState struct {
	Playing  bool          `json:"playing"`
	Loading  bool          `json:"loading"`
	Ready    bool          `json:"ready"`
	Track    *music.Track  `json:"track,omitempty"`
	Position int64         `json:"position"` // milliseconds
	Duration int64         `json:"duration"` // milliseconds
	Queue    []music.Track `json:"queue,omitempty"`
	At       int           `json:"at"`
	Order    []int         `json:"order,omitempty"`
	Shuffle  bool          `json:"shuffle"`
	Repeat   string        `json:"repeat"`
	Volume   float64       `json:"volume"`
	Error    string        `json:"error,omitempty"`
	Lyrics   []lyricsLine  `json:"lyrics,omitempty"`
}

type lyricsLine struct {
	Time float64 `json:"time"` // seconds, as an LRC has it
	Text string  `json:"text"`
}

// state builds what the UI shows. full adds the queue, which is big enough to
// leave out of the half-second position events.
func (a *App) state(full bool) PlaybackState {
	s := PlaybackState{
		Playing:  a.pl.Playing(),
		Loading:  a.pl.Loading(),
		Ready:    a.pl.Ready(),
		Track:    a.pl.Current(),
		Position: a.pl.Position().Milliseconds(),
		Duration: a.pl.Duration().Milliseconds(),
		Shuffle:  a.pl.Shuffle(),
		Repeat:   repeatName(a.pl.Repeat()),
		Volume:   a.pl.Volume(),
		Error:    a.pl.Err(),
	}
	if full {
		s.Queue = a.pl.Queue()
		order, at := a.pl.PlayOrder()
		s.Order, s.At = order, at
		if lines, ok := a.pl.Lyrics(); ok {
			s.Lyrics = make([]lyricsLine, len(lines))
			for i, line := range lines {
				s.Lyrics[i] = lyricsLine{Time: line.At.Seconds(), Text: line.Text}
			}
		}
	}
	return s
}

func (a *App) emitState(full bool) {
	a.publishEvent("player.state", a.state(full))
}

func repeatName(m player.RepeatMode) string {
	switch m {
	case player.RepeatAll:
		return "all"
	case player.RepeatOne:
		return "one"
	default:
		return "off"
	}
}

func repeatMode(name string) (player.RepeatMode, bool) {
	switch name {
	case "off", "":
		return player.RepeatOff, true
	case "all":
		return player.RepeatAll, true
	case "one":
		return player.RepeatOne, true
	}
	return player.RepeatOff, false
}
