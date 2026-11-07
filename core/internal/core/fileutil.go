package core

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
)

// readJSON reads path into v. A missing file is not an error: v stays as it
// was.
func readJSON(path string, v any) (bool, error) {
	b, err := os.ReadFile(path)
	if err != nil {
		if os.IsNotExist(err) {
			return false, nil
		}
		return false, fmt.Errorf("core: reading %s: %w", path, err)
	}
	if err := json.Unmarshal(b, v); err != nil {
		return false, fmt.Errorf("core: reading %s: %w", path, err)
	}
	return true, nil
}

// writeJSON writes v to path atomically, private to the user: a .part file
// renamed over the old one, so a power cut leaves one of the two, never half.
func writeJSON(path string, v any) error {
	b, err := json.MarshalIndent(v, "", "  ")
	if err != nil {
		return fmt.Errorf("core: encoding %s: %w", path, err)
	}
	b = append(b, '\n')
	if err := os.MkdirAll(filepath.Dir(path), 0o700); err != nil {
		return fmt.Errorf("core: making %s: %w", filepath.Dir(path), err)
	}
	part := path + ".part"
	f, err := os.OpenFile(part, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0o600)
	if err != nil {
		return fmt.Errorf("core: writing %s: %w", part, err)
	}
	if _, err := f.Write(b); err != nil {
		f.Close()
		os.Remove(part)
		return fmt.Errorf("core: writing %s: %w", part, err)
	}
	if err := f.Sync(); err != nil {
		f.Close()
		os.Remove(part)
		return fmt.Errorf("core: writing %s: %w", part, err)
	}
	if err := f.Close(); err != nil {
		os.Remove(part)
		return fmt.Errorf("core: writing %s: %w", part, err)
	}
	if err := os.Rename(part, path); err != nil {
		os.Remove(part)
		return fmt.Errorf("core: replacing %s: %w", path, err)
	}
	return nil
}
