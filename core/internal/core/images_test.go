package core

import (
	"encoding/base64"
	"image"
	"image/color"
	"image/png"
	"os"
	"path/filepath"
	"testing"

	"zeromusic/core/internal/music"
)

// A cover is fetched from a file URL, cropped square and answered as RGB565:
// two bytes a pixel, the size asked for.
func TestImageGet(t *testing.T) {
	app := testApp(t)
	dir := t.TempDir()
	path := filepath.Join(dir, "cover.png")
	f, err := os.Create(path)
	if err != nil {
		t.Fatal(err)
	}
	img := image.NewRGBA(image.Rect(0, 0, 40, 20))
	for y := 0; y < 20; y++ {
		for x := 0; x < 40; x++ {
			img.Set(x, y, color.RGBA{R: uint8(x * 6), G: 100, B: uint8(y * 12), A: 255})
		}
	}
	if err := png.Encode(f, img); err != nil {
		t.Fatal(err)
	}
	f.Close()

	got, err := call(t, app, "image.get", map[string]any{"url": music.FileURL(path), "size": 32})
	if err != nil {
		t.Fatal(err)
	}
	answer, ok := got.(imageAnswer)
	if !ok {
		t.Fatalf("answer: %T", got)
	}
	data, err := base64.StdEncoding.DecodeString(answer.Data)
	if err != nil {
		t.Fatal(err)
	}
	if answer.Width != 32 || answer.Height != 32 || len(data) != 32*32*2 {
		t.Fatalf("answer: %dx%d, %d bytes", answer.Width, answer.Height, len(data))
	}
}
