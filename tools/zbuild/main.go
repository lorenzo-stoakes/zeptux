package main

import (
	"os"
)

// TODO: Make configurable.
const (
	CC_BINARY             = "gcc"
	CPP_BINARY            = "g++"
	LD_BINARY             = "ld"
	DEBUG                 = false
	UNQUIET               = true
	ZBUILD_TMPFILE_PREFIX = ".zbuild."
)

func main() {
	state := do_parse()

	// Will use default if not specified
	rule_name := ""
	if len(os.Args) > 1 {
		rule_name = os.Args[1]
	}

	do_build(state, rule_name)
}
