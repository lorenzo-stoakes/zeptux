package main

import (
	"os"
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
