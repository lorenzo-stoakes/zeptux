package main

import (
	"fmt"
	"os"
	"path"
	"strings"
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

// Remove the extension from a filename, if present.
func remove_ext(name string) string {
	var bare string
	if strings.Contains(name, ".") {
		parts := strings.Split(name, ".")
		bare = strings.TrimSpace(parts[0])
	} else {
		bare = name
	}

	return bare
}

// Replace file extension with that specified.
func replace_ext(filename, ext string) string {
	bare := remove_ext(filename)

	if ext[0] != '.' {
		ext = "." + ext
	}

	return bare + ext
}

// Determine if file `candidate` is newer than `comparator`.
func is_file_newer(candidate_dir, candidate, comparator string) (bool, error) {
	candidate = path.Join(candidate_dir, candidate)

	info1, err1 := os.Stat(candidate)
	if err1 != nil {
		return false, err1
	}

	info2, err2 := os.Stat(comparator)
	// If the comparator doesn't exist, then we consider the candidate to be
	// newer.
	if os.IsNotExist(err2) {
		return true, nil
	}

	if err2 != nil {
		return false, err2
	}

	return info1.ModTime().After(info2.ModTime()), nil
}
