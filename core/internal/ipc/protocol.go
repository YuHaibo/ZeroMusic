// Package ipc carries the JSON-RPC between the UI and the core over a Unix
// socket: length-prefixed JSON requests and responses, and events pushed the
// other way.
package ipc

import (
	"encoding/json"
	"errors"
	"fmt"
)

// Protocol is the version of the framing and methods below. The UI checks it
// in the hello answer.
const Protocol = 1

// MaxFrame is the largest request or response, events included.
const MaxFrame = 8 << 20

// ErrorCode names what went wrong, so the UI can show it as it should.
type ErrorCode string

const (
	CodeSignIn      ErrorCode = "signIn"
	CodeSetup       ErrorCode = "setup"
	CodeUnavailable ErrorCode = "unavailable"
	CodeUnsupported ErrorCode = "unsupported"
	CodeNetwork     ErrorCode = "network"
	CodeInternal    ErrorCode = "internal"
	CodeCanceled    ErrorCode = "canceled"
	CodeInvalid     ErrorCode = "invalid"
)

// Error is the error object of a failed response.
type Error struct {
	Code    ErrorCode `json:"code"`
	Message string    `json:"message"`
	Detail  string    `json:"detail,omitempty"`
}

func (e *Error) Error() string { return string(e.Code) + ": " + e.Message }

// Errorf returns an Error with code.
func Errorf(code ErrorCode, format string, args ...any) *Error {
	return &Error{Code: code, Message: fmt.Sprintf(format, args...)}
}

// CodeOf returns the code for err: the one an *Error carries, or internal.
func CodeOf(err error) ErrorCode {
	var e *Error
	if errors.As(err, &e) {
		return e.Code
	}
	return CodeInternal
}

// Request is what the UI sends. ID is absent on notifications.
type Request struct {
	ID     *int64          `json:"id,omitempty"`
	Method string          `json:"method"`
	Params json.RawMessage `json:"params,omitempty"`
}

// Response answers a Request with the same ID, or says why not.
type Response struct {
	ID     int64  `json:"id"`
	Result any    `json:"result"`
	Error  *Error `json:"error,omitempty"`
}

// Event is pushed to every connected UI without an ID.
type Event struct {
	Event string `json:"event"`
	Data  any    `json:"data,omitempty"`
}

// Handler answers a request. It may be called from any goroutine, and may
// block: the server runs each request on its own.
type Handler interface {
	Handle(method string, params json.RawMessage) (any, error)
}
