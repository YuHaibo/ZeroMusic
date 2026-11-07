package ipc

import (
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"path/filepath"
	"sync"
	"time"
)

// Server listens on a Unix socket and answers Handler requests on it. Events
// go to every connection; a UI that falls behind loses events rather than
// holding the core up.
type Server struct {
	path   string
	ln     net.Listener
	handle Handler

	mu     sync.Mutex
	conns  map[*conn]struct{}
	closed bool
	log    *log.Logger
}

// conn is one UI: a reader for its requests and a writer for what goes back.
type conn struct {
	nc   net.Conn
	out  chan []byte
	done chan struct{}
	once sync.Once
}

func (c *conn) close() {
	c.once.Do(func() {
		close(c.done)
		c.nc.Close()
	})
}

// Listen opens the socket at path, replacing a stale one. It fails if another
// core answers there already.
func Listen(path string, h Handler, logger *log.Logger) (*Server, error) {
	if err := os.MkdirAll(filepath.Dir(path), 0o700); err != nil {
		return nil, fmt.Errorf("ipc: socket directory: %w", err)
	}
	if c, err := net.DialTimeout("unix", path, 200*time.Millisecond); err == nil {
		c.Close()
		return nil, fmt.Errorf("ipc: another core is listening on %s", path)
	}
	if err := os.Remove(path); err != nil && !errors.Is(err, os.ErrNotExist) {
		return nil, fmt.Errorf("ipc: removing stale socket: %w", err)
	}
	ln, err := net.Listen("unix", path)
	if err != nil {
		return nil, fmt.Errorf("ipc: listen: %w", err)
	}
	if err := os.Chmod(path, 0o600); err != nil {
		ln.Close()
		return nil, fmt.Errorf("ipc: chmod socket: %w", err)
	}
	if logger == nil {
		logger = log.Default()
	}
	return &Server{path: path, ln: ln, handle: h, conns: map[*conn]struct{}{}, log: logger}, nil
}

// Path is the socket's path.
func (s *Server) Path() string { return s.path }

// Serve accepts connections until Close. It returns nil after Close.
func (s *Server) Serve() error {
	for {
		nc, err := s.ln.Accept()
		if err != nil {
			s.mu.Lock()
			closed := s.closed
			s.mu.Unlock()
			if closed {
				return nil
			}
			if errors.Is(err, net.ErrClosed) {
				return nil
			}
			return err
		}
		c := &conn{nc: nc, out: make(chan []byte, 64), done: make(chan struct{})}
		s.mu.Lock()
		if s.closed {
			s.mu.Unlock()
			c.close()
			return nil
		}
		s.conns[c] = struct{}{}
		s.mu.Unlock()
		go s.write(c)
		go s.read(c)
	}
}

// Close stops the listener and disconnects every UI.
func (s *Server) Close() error {
	s.mu.Lock()
	if s.closed {
		s.mu.Unlock()
		return nil
	}
	s.closed = true
	conns := make([]*conn, 0, len(s.conns))
	for c := range s.conns {
		conns = append(conns, c)
	}
	s.conns = map[*conn]struct{}{}
	s.mu.Unlock()

	err := s.ln.Close()
	for _, c := range conns {
		c.close()
	}
	os.Remove(s.path)
	return err
}

// Broadcast sends an event to every UI. A UI whose queue is full misses it.
func (s *Server) Broadcast(event string, data any) {
	b, err := json.Marshal(Event{Event: event, Data: data})
	if err != nil {
		s.log.Printf("ipc: marshaling %s event: %v", event, err)
		return
	}
	frame := frame(b)
	s.mu.Lock()
	conns := make([]*conn, 0, len(s.conns))
	for c := range s.conns {
		conns = append(conns, c)
	}
	s.mu.Unlock()
	for _, c := range conns {
		select {
		case c.out <- frame:
		case <-c.done:
		default:
			// The UI is not keeping up; a state event follows.
		}
	}
}

func (s *Server) read(c *conn) {
	defer func() {
		s.mu.Lock()
		delete(s.conns, c)
		s.mu.Unlock()
		c.close()
	}()
	for {
		var hdr [4]byte
		if _, err := io.ReadFull(c.nc, hdr[:]); err != nil {
			return
		}
		n := binary.BigEndian.Uint32(hdr[:])
		if n == 0 || n > MaxFrame {
			s.log.Printf("ipc: frame of %d bytes refused", n)
			return
		}
		buf := make([]byte, n)
		if _, err := io.ReadFull(c.nc, buf); err != nil {
			return
		}
		var req Request
		if err := json.Unmarshal(buf, &req); err != nil {
			s.log.Printf("ipc: bad request: %v", err)
			continue
		}
		if req.ID == nil {
			continue // a notification, of which there are none yet
		}
		id := *req.ID
		go s.answer(c, id, req)
	}
}

func (s *Server) answer(c *conn, id int64, req Request) {
	result, err := s.handle.Handle(req.Method, req.Params)
	resp := Response{ID: id, Result: result}
	if err != nil {
		resp.Result = nil
		var rpc *Error
		if errors.As(err, &rpc) {
			resp.Error = rpc
		} else {
			resp.Error = &Error{Code: CodeInternal, Message: err.Error()}
		}
	}
	b, merr := json.Marshal(resp)
	if merr != nil {
		b, _ = json.Marshal(Response{ID: id, Error: &Error{Code: CodeInternal, Message: "cannot encode result"}})
	}
	select {
	case c.out <- frame(b):
	case <-c.done:
	}
}

func (s *Server) write(c *conn) {
	for {
		select {
		case b := <-c.out:
			if _, err := c.nc.Write(b); err != nil {
				c.close()
				return
			}
		case <-c.done:
			return
		}
	}
}

// frame prefixes b with its length, as everything on the socket is.
func frame(b []byte) []byte {
	if len(b) > MaxFrame {
		b = nil
	}
	out := make([]byte, 4+len(b))
	binary.BigEndian.PutUint32(out, uint32(len(b)))
	copy(out[4:], b)
	return out
}
