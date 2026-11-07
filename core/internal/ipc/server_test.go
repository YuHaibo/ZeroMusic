package ipc

import (
	"encoding/binary"
	"encoding/json"
	"io"
	"net"
	"path/filepath"
	"testing"
	"time"
)

type echo struct{}

func (echo) Handle(method string, params json.RawMessage) (any, error) {
	if method == "fail" {
		return nil, Errorf(CodeSignIn, "sign in first")
	}
	return map[string]any{"method": method, "params": string(params)}, nil
}

func dial(t *testing.T, path string) net.Conn {
	t.Helper()
	c, err := net.Dial("unix", path)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { c.Close() })
	return c
}

func writeFrame(t *testing.T, c net.Conn, req Request) {
	t.Helper()
	b, err := json.Marshal(req)
	if err != nil {
		t.Fatal(err)
	}
	if _, err := c.Write(frame(b)); err != nil {
		t.Fatal(err)
	}
}

func readFrame(t *testing.T, c net.Conn) []byte {
	t.Helper()
	c.SetReadDeadline(time.Now().Add(5 * time.Second))
	var hdr [4]byte
	if _, err := io.ReadFull(c, hdr[:]); err != nil {
		t.Fatal(err)
	}
	n := binary.BigEndian.Uint32(hdr[:])
	b := make([]byte, n)
	if _, err := io.ReadFull(c, b); err != nil {
		t.Fatal(err)
	}
	return b
}

// A request gets its own answer, with the same ID and its result.
func TestServerAnswers(t *testing.T) {
	path := filepath.Join(t.TempDir(), "test.sock")
	s, err := Listen(path, echo{}, nil)
	if err != nil {
		t.Fatal(err)
	}
	defer s.Close()
	go s.Serve()

	c := dial(t, path)
	id := int64(7)
	writeFrame(t, c, Request{ID: &id, Method: "hi", Params: json.RawMessage(`{"a":1}`)})
	var resp Response
	if err := json.Unmarshal(readFrame(t, c), &resp); err != nil {
		t.Fatal(err)
	}
	if resp.ID != id || resp.Error != nil {
		t.Fatalf("answer: %+v", resp)
	}
	result, _ := resp.Result.(map[string]any)
	if result["method"] != "hi" {
		t.Errorf("result: %+v", resp.Result)
	}
}

// A failed request says which kind of failure it was, for the UI to act on.
func TestServerErrorCode(t *testing.T) {
	path := filepath.Join(t.TempDir(), "test.sock")
	s, err := Listen(path, echo{}, nil)
	if err != nil {
		t.Fatal(err)
	}
	defer s.Close()
	go s.Serve()

	c := dial(t, path)
	id := int64(1)
	writeFrame(t, c, Request{ID: &id, Method: "fail"})
	var resp Response
	if err := json.Unmarshal(readFrame(t, c), &resp); err != nil {
		t.Fatal(err)
	}
	if resp.Error == nil || resp.Error.Code != CodeSignIn {
		t.Fatalf("error: %+v", resp.Error)
	}
}

// An event reaches a connected UI without an ID.
func TestServerBroadcast(t *testing.T) {
	path := filepath.Join(t.TempDir(), "test.sock")
	s, err := Listen(path, echo{}, nil)
	if err != nil {
		t.Fatal(err)
	}
	defer s.Close()
	go s.Serve()

	c := dial(t, path)
	// Let the accept land before the event, or the UI would not hear it.
	deadline := time.Now().Add(2 * time.Second)
	for {
		s.mu.Lock()
		n := len(s.conns)
		s.mu.Unlock()
		if n > 0 {
			break
		}
		if time.Now().After(deadline) {
			t.Fatal("the UI was not accepted")
		}
		time.Sleep(5 * time.Millisecond)
	}
	s.Broadcast("toast", map[string]string{"text": "hi"})
	var ev Event
	if err := json.Unmarshal(readFrame(t, c), &ev); err != nil {
		t.Fatal(err)
	}
	if ev.Event != "toast" {
		t.Errorf("event: %+v", ev)
	}
}
