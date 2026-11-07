package core

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"image"
	_ "image/gif"
	_ "image/jpeg"
	_ "image/png"
	"sync"

	"golang.org/x/image/draw"

	"zeromusic/core/internal/ipc"
	"zeromusic/core/internal/music"
)

// imageBudget is how much RGB565 the service keeps in memory: covers are
// small, and the device has 512 MB for everything.
const imageBudget = 16 << 20

// imageService fetches, decodes and downscales covers to RGB565, keeping the
// last ones so a page that comes back doesn't fetch them again.
type imageService struct {
	mu      sync.Mutex
	entries map[string][]byte
	order   []string
	bytes   int
}

func newImageService() *imageService {
	return &imageService{entries: map[string][]byte{}}
}

func (s *imageService) get(key string) ([]byte, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()
	b, ok := s.entries[key]
	return b, ok
}

func (s *imageService) put(key string, b []byte) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, ok := s.entries[key]; !ok {
		s.order = append(s.order, key)
	}
	s.entries[key] = b
	s.bytes += len(b)
	for s.bytes > imageBudget && len(s.order) > 0 {
		old := s.order[0]
		s.order = s.order[1:]
		if b, ok := s.entries[old]; ok {
			delete(s.entries, old)
			s.bytes -= len(b)
		}
	}
}

type imageParams struct {
	URL  string `json:"url"`
	Size int    `json:"size"`
}

type imageAnswer struct {
	Width  int    `json:"width"`
	Height int    `json:"height"`
	Format string `json:"format"`
	Data   string `json:"data"` // base64 of RGB565, little-endian
}

// imageGet returns a cover as RGB565: fetched once, decoded, cropped square
// and scaled to size.
func (a *App) imageGet(raw json.RawMessage) (any, error) {
	p, err := decodeParams[imageParams](raw)
	if err != nil {
		return nil, err
	}
	if p.URL == "" {
		return nil, ipc.Errorf(ipc.CodeInvalid, "no image URL")
	}
	size := min(max(p.Size, 32), 320)
	key := fmt.Sprintf("%d|%s", size, p.URL)
	if b, ok := a.images.get(key); ok {
		return imageAnswer{Width: size, Height: size, Format: "rgb565", Data: base64.StdEncoding.EncodeToString(b)}, nil
	}

	data, err := music.Fetch(music.SizedImage(p.URL, size))
	if err != nil {
		return nil, &ipc.Error{Code: ipc.CodeNetwork, Message: err.Error()}
	}
	src, _, err := image.Decode(bytes.NewReader(data))
	if err != nil {
		return nil, &ipc.Error{Code: ipc.CodeUnavailable, Message: err.Error()}
	}
	pixels, err := coverPixels(src, size)
	if err != nil {
		return nil, err
	}
	a.images.put(key, pixels)
	return imageAnswer{Width: size, Height: size, Format: "rgb565", Data: base64.StdEncoding.EncodeToString(pixels)}, nil
}

// coverPixels crops the picture square in the middle and scales it to size,
// as RGB565 little-endian.
func coverPixels(src image.Image, size int) ([]byte, error) {
	b := src.Bounds()
	if b.Dx()*b.Dy() > 12_000_000 {
		return nil, fmt.Errorf("image is %dx%d, too large", b.Dx(), b.Dy())
	}
	side := min(b.Dx(), b.Dy())
	if side <= 0 {
		return nil, fmt.Errorf("image has no pixels")
	}
	crop := image.Rect(0, 0, side, side).Add(image.Pt(b.Min.X+(b.Dx()-side)/2, b.Min.Y+(b.Dy()-side)/2))
	dst := image.NewRGBA(image.Rect(0, 0, size, size))
	draw.ApproxBiLinear.Scale(dst, dst.Bounds(), src, crop, draw.Over, nil)
	out := make([]byte, size*size*2)
	for y := 0; y < size; y++ {
		for x := 0; x < size; x++ {
			r, g, bl, _ := dst.At(x, y).RGBA()
			v := uint16(r>>11)<<11 | uint16(g>>10)<<5 | uint16(bl>>11)
			out[(y*size+x)*2] = byte(v)
			out[(y*size+x)*2+1] = byte(v >> 8)
		}
	}
	return out, nil
}
