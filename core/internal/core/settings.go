package core

import (
	"fmt"
	"slices"
)

// Settings are what the listener chose, as settings.json keeps them.
type Settings struct {
	Language   string  `json:"language"`
	Quality    string  `json:"quality"`
	Normalize  bool    `json:"normalize"`
	Volume     float64 `json:"volume"`
	Shuffle    bool    `json:"shuffle"`
	Repeat     int     `json:"repeat"`
	CacheMB    int     `json:"cacheMB"`
	LastSource string  `json:"lastSource,omitempty"`
	// DisabledSources are the plugins the listener turned off.
	DisabledSources []string `json:"disabledSources,omitempty"`
}

// DefaultSettings is what a fresh install has.
func DefaultSettings() Settings {
	return Settings{
		Language:  "system",
		Quality:   "high",
		Normalize: false,
		Volume:    0.8,
		Repeat:    0,
		CacheMB:   3072,
	}
}

// normal returns s with every field in range, so a hand-edited file cannot
// ask for a volume of 9 or a quality no source has.
func (s Settings) normal() Settings {
	switch s.Language {
	case "", "system":
		s.Language = "system"
	case "en", "zh-CN":
	default:
		s.Language = "system"
	}
	if !slices.Contains([]string{"standard", "high", "lossless"}, s.Quality) {
		s.Quality = "high"
	}
	if s.Volume < 0 {
		s.Volume = 0
	}
	if s.Volume > 1 {
		s.Volume = 1
	}
	s.Repeat = min(max(s.Repeat, 0), 2)
	if s.CacheMB < 0 {
		s.CacheMB = 0
	}
	if s.CacheMB > 4096 {
		s.CacheMB = 4096
	}
	s.DisabledSources = uniqueStrings(s.DisabledSources)
	return s
}

// LoadSettings reads the settings, or returns the defaults when there are
// none.
func LoadSettings(path string) (Settings, error) {
	s := DefaultSettings()
	if _, err := readJSON(path, &s); err != nil {
		return s.normal(), err
	}
	return s.normal(), nil
}

// Save writes the settings to path.
func (s Settings) Save(path string) error {
	return writeJSON(path, s.normal())
}

func uniqueStrings(in []string) []string {
	if len(in) == 0 {
		return nil
	}
	seen := make(map[string]bool, len(in))
	out := make([]string, 0, len(in))
	for _, s := range in {
		if s == "" || seen[s] {
			continue
		}
		seen[s] = true
		out = append(out, s)
	}
	return out
}

// settingsPatch is what settings.set carried: only the fields given change.
type settingsPatch struct {
	Language   *string  `json:"language"`
	Quality    *string  `json:"quality"`
	Normalize  *bool    `json:"normalize"`
	Volume     *float64 `json:"volume"`
	Shuffle    *bool    `json:"shuffle"`
	Repeat     *int     `json:"repeat"`
	CacheMB    *int     `json:"cacheMB"`
	LastSource *string  `json:"lastSource"`
}

// apply changes the fields the patch has, and reports the result.
func (s Settings) apply(p settingsPatch) (Settings, error) {
	if p.Language != nil {
		if !slices.Contains([]string{"system", "en", "zh-CN"}, *p.Language) {
			return s, fmt.Errorf("unknown language %q", *p.Language)
		}
		s.Language = *p.Language
	}
	if p.Quality != nil {
		if !slices.Contains([]string{"standard", "high", "lossless"}, *p.Quality) {
			return s, fmt.Errorf("unknown quality %q", *p.Quality)
		}
		s.Quality = *p.Quality
	}
	if p.Normalize != nil {
		s.Normalize = *p.Normalize
	}
	if p.Volume != nil {
		if *p.Volume < 0 || *p.Volume > 1 {
			return s, fmt.Errorf("volume %v is outside 0..1", *p.Volume)
		}
		s.Volume = *p.Volume
	}
	if p.Shuffle != nil {
		s.Shuffle = *p.Shuffle
	}
	if p.Repeat != nil {
		if *p.Repeat < 0 || *p.Repeat > 2 {
			return s, fmt.Errorf("repeat %d is not off, all or one", *p.Repeat)
		}
		s.Repeat = *p.Repeat
	}
	if p.CacheMB != nil {
		if *p.CacheMB < 0 {
			return s, fmt.Errorf("cacheMB %d is negative", *p.CacheMB)
		}
		s.CacheMB = *p.CacheMB
	}
	if p.LastSource != nil {
		s.LastSource = *p.LastSource
	}
	return s.normal(), nil
}
