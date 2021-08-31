package main

import (
	"fmt"
	"os"
)

// Output an error to STDERR.
func prerr(prefix, msg string, args ...interface{}) {
	fmt.Fprintf(os.Stderr, "%s: ", prefix)
	fmt.Fprintf(os.Stderr, msg, args...)
	fmt.Fprintf(os.Stderr, "\n")
}

// Output a fatal error to STDERR, then exit.
func fatal(msg string, args ...interface{}) {
	prerr("FATAL", msg, args...)
	os.Exit(1)
}
