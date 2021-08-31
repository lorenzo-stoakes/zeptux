package main

import (
	"fmt"
	"os"
	"path"
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

// Does the file under the specified diretory exist?
func file_exists(dir, name string) bool {
	// We treat any error as a failure. We should be able to stat the file!
	if _, err := os.Stat(path.Join(dir, name)); err != nil {
		return false
	}

	return true
}

// Does the specified file exist and is it a directory?
func dir_exists(path string) bool {
	// We treat any error as a failure. We should be able to stat the dir!
	if info, err := os.Stat(path); err != nil || !info.IsDir() {
		return false
	}

	return true
}
