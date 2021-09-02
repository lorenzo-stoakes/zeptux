package main

import (
	"fmt"
	"os"
	"reflect"
	"strings"
)

const (
	DEFAULT_BUILD_FILE         = "zeptux.zbuild"
	EMBEDDED_SHELL_PREFIX      = "$$shell{"
	EMBEDDED_SHELL_SUFFIX_CHAR = '}'
)

// 'Special' variables, e.g. ones that have an impact further than allowing
// $varname substitution.
var SPECIAL_VARS = []string{
	"build_dir", "default", "includes", "default_cflags",
}

// Reserved to avoid conflicts.
var RESERVED_VARS = []string{
	"source", "output",
}

// The different 'operations' that can be performed.
type operation int

const (
	// Top-level operations.
	SET operation = iota
	PREHOOK
	OPTION
	BUILD
	COMMAND
	// Nested operations.
	CC
	CPP
	LD
	FOREACH
	SHELL
	CALL
)

// Represents a dependency or target.
type depget_kind int

const (
	FILE depget_kind = iota
	GLOB
	RECURSIVE_GLOB
	RULE
)

type depget struct {
	kind depget_kind
	name string
}

func (d depget) String() string {
	if d.kind == RECURSIVE_GLOB {
		return "*" + d.name
	}

	return d.name
}

// Represents a set of dependencies/targets, possibly labelled.
type depgetset struct {
	label   string
	depgets []depget
}

func (d depgetset) String() string {
	ret := ""

	if len(d.label) > 0 {
		ret = d.label + "="
	}

	if len(d.depgets) == 1 {
		ret += d.depgets[0].String()
	} else {
		ret += "["
		var dg_strs []string
		for _, dg := range d.depgets {
			dg_strs = append(dg_strs, dg.String())
		}
		ret += strings.Join(dg_strs, ",")
		ret += "]"
	}
	return ret
}
func (d *depgetset) empty() bool {
	return len(d.depgets) == 0
}

// The different options that can be set.
type option int

const (
	COMPUTE_DEPENDENCIES_OPTION option = iota
)

func (o option) String() string {
	switch o {
	case COMPUTE_DEPENDENCIES_OPTION:
		return "computer_dependencies"
	}

	return "??UNKNOWN OPTION??"
}

// Represents a SET statement.
type set_statement struct {
	is_special, is_append bool
	key, val              string
}

// Represents an OPTION statement.
type option_statement struct {
	opt option
}

// The 'when' of prehooks.
type prehook_when int

const (
	ALWAYS_PREHOOK prehook_when = iota
	ON_CHANGE_PREHOOK
)

func (pw prehook_when) String() string {
	switch pw {
	case ALWAYS_PREHOOK:
		return "always"
	case ON_CHANGE_PREHOOK:
		return "on change"
	}

	return "??UNKNOWN PREHOOK WHEN??"
}

// Represents a PREHOOK statement.
type prehook_statement struct {
	dependencies depgetset
	when         prehook_when
	statements   statements
}

// Represents a parameterised string, e.g. any expression that can contain
// variables and/or embedded shell code.
type param_string_element_kind int

const (
	STRING_ELEM param_string_element_kind = iota
	VARIABLE_ELEM
	EMBEDDED_SHELL
)

type param_string_element struct {
	kind param_string_element_kind
	str  string
}
type parameterised_string []param_string_element

func (s parameterised_string) String() string {
	parts := make([]string, 0, len(s))

	for _, elem := range s {
		switch elem.kind {
		case STRING_ELEM:
			parts = append(parts, elem.str)
		case VARIABLE_ELEM:
			parts = append(parts, "$("+elem.str+")")
		case EMBEDDED_SHELL:
			parts = append(parts, "$$shell{"+elem.str+"}")
		}
	}

	return strings.Join(parts, "")
}

// Represents a (nested) SHELL statement.
type shell_statement struct {
	expression parameterised_string
}

// Represents a (nested) CALL statement.
type call_statement string

// Represents a (nested) CC statement.
type cc_statement parameterised_string

// Represents a (nested) CPP statement.
type cpp_statement parameterised_string

// Represents a (nested) LD statement.
type ld_statement parameterised_string

// Represents a (nested) FOREACH statement.
type foreach_statement struct {
	from, to   string
	excluding  depgetset
	statements statements
}

// Represents a BUILD statement.
type build_statement struct {
	target, dependencies depgetset
	local_dir, alias     string
	statements           statements
}

// Represents a COMMAND statement.
type command_statement struct {
	name             string
	local_dir        string
	has_nested_calls bool
	dependencies     depgetset
	statements       statements
}

// Represents generic list of statements. TODO: interface{} abuse.
type statements []interface{}

// Represents current parse state. We are essentially using a state machine.
// TODO: interface{} abuse.
type parse_state struct {
	line_num int
	line     string
	// Each stack entry points at the current operation we are in the midst
	// of parsing.
	currop_stack []operation
	// Each stack entry points at the current statement array we are
	// appending to.
	statements_stack []*statements

	// Set of rule names.
	rule_set map[string]bool

	// List of all statements.
	statements statements
}

// Set our current operation and pointer to statements being appended to.
func (s *parse_state) push_currop(op operation, statements *statements) {
	s.currop_stack = append(s.currop_stack, op)
	s.statements_stack = append(s.statements_stack, statements)
}

// Pop our current operation. We don't care what we _were_ constructing
// (everything exists as a tree from the top-level statements list down) so
// there's no need to return anything.
func (s *parse_state) pop_currop() {
	if len(s.currop_stack) == 0 || len(s.statements_stack) == 0 {
		s.fatal_line_error("Mismatched CLOSING brace")
	}

	// Sanity check...
	if len(s.currop_stack) != len(s.statements_stack) {
		panic("Impossible")
	}

	s.currop_stack = s.currop_stack[:len(s.currop_stack)-1]
	s.statements_stack = s.statements_stack[:len(s.statements_stack)-1]
}

// Output a fatal error referring to a given line/line number to STDERR, then exit.
func (s *parse_state) fatal_line_error(msg string, args ...interface{}) {
	line_error("FATAL", s.line_num, s.line, msg, args...)
	os.Exit(1)
}

// Output a fatal syntax error, then exit.
func (s *parse_state) syntax_error() {
	s.fatal_line_error("Syntax error")
}

// Append a statement to the statement array currently being parsed.
func (s *parse_state) append_statement(statement interface{}) {
	// We have to do this to avoid copy-by-value and our whole recursion
	// collapsing as a result of our use of the screwy interface{} type.
	assert_statement_is_pointer(statement)

	// Top-level statements get added to the top-level parse_state array.
	if len(s.statements_stack) == 0 {
		s.statements = append(s.statements, statement)
		return
	}

	// We also keep a stack of pointers to statement arrays so we can append
	// to the current statement's list of nested statements.
	arrptr := s.statements_stack[len(s.statements_stack)-1]
	*arrptr = append(*arrptr, statement)
}

// Extracts operation name, returns false if comment, empty line + should be skipped.
func (s *parse_state) parse_operation_name() (string, bool) {
	// Ignore comments and empty lines.
	if len(s.line) == 0 || strings.HasPrefix(s.line, "#") {
		return "", false
	}

	var opstr string
	first_space := strings.Index(s.line, " ")
	if first_space == -1 {
		opstr = s.line
	} else {
		opstr = s.line[:first_space]
	}

	return opstr, true
}

// Parse SET statement.
func (s *parse_state) parse_set_line() {
	var statement set_statement

	// Squashes whitespace.
	parts := strings.Fields(s.line)

	if len(parts) < 5 {
		s.syntax_error()
	}
	// set [special|var] [key] [+]= ...
	mode := parts[1]
	statement.key = parts[2]
	operator := parts[3]

	if operator != "=" && operator != "+=" {
		s.syntax_error()
	}
	statement.is_append = operator == "+="

	statement.is_special = mode == "special"
	if !statement.is_special && mode != "var" {
		s.syntax_error()
	}

	if is_reserved_var(statement.key) {
		s.fatal_line_error("'%s' is a reserved variable name", statement.key)
	}

	if statement.is_special && !is_special_var(statement.key) {
		s.fatal_line_error("'%s' is not a special variable", statement.key)
	}

	statement.val = strings.TrimSpace(strings.SplitN(s.line, operator, 2)[1])

	s.append_statement(&statement)
}

// Parse OPTION statement.
func (s *parse_state) parse_option_line() {
	parts := strings.Fields(s.line)
	if len(parts) != 2 {
		s.syntax_error()
	}

	optstr := parts[1]
	var statement option_statement
	switch optstr {
	case "compute_dependencies":
		statement.opt = COMPUTE_DEPENDENCIES_OPTION
	default:
		s.fatal_line_error("Unrecognised option '%s'", optstr)
	}

	s.append_statement(&statement)
}

// Parse PREHOOK statement, first line.
func (s *parse_state) parse_prehook_first_line() {
	// prehook [deps] [always|on change] {

	line := s.line

	// Prehook statements always end with a opening brace.
	if !strings.HasSuffix(line, "{") {
		s.syntax_error()
	}
	// Chop off {.
	line = strings.TrimSpace(line[:len(line)-1])

	var statement prehook_statement

	if strings.HasSuffix(line, "on change") {
		statement.when = ON_CHANGE_PREHOOK
		line = strings.TrimSpace(line[:len(line)-len("on change")])
	} else if strings.HasSuffix(line, "always") {
		statement.when = ALWAYS_PREHOOK
		line = strings.TrimSpace(line[:len(line)-len("always")])
	} else {
		s.syntax_error()
	}

	parts := strings.Fields(s.line)
	if len(parts) < 2 {
		s.syntax_error()
	}

	depstr := parts[1]
	if depstr != "*" {
		if dependencies := parse_depgets(depstr); dependencies == nil {
			s.syntax_error()
		} else {
			statement.dependencies = *dependencies
		}
	}

	// We are now entering into a state where we are parsing nested prehook
	// statements.
	s.append_statement(&statement)
	s.push_currop(PREHOOK, &statement.statements)
}

// Parse (nested) SHELL statement.
func (s *parse_state) parse_shell_line() {
	line := strings.TrimSpace(s.line[len("shell "):])

	if ptr := parse_parameterised_string(line); ptr == nil {
		s.syntax_error()
	} else {
		s.append_statement(&shell_statement{*ptr})
	}
}

// Parse (nested) CALL statement.
func (s *parse_state) parse_call_line() {
	line := strings.TrimSpace(s.line[len("call "):])

	statement := call_statement(line)
	s.append_statement(&statement)
}

// Parse (nested) CC statement.
func (s *parse_state) parse_cc_line() {
	line := strings.TrimSpace(s.line[len("cc "):])

	pstr := parse_parameterised_string(line)
	if pstr == nil {
		s.syntax_error()
	}

	statement := cc_statement(*pstr)
	s.append_statement(&statement)
}

// Parse (nested) CPP statement.
func (s *parse_state) parse_cpp_line() {
	line := strings.TrimSpace(s.line[len("cpp "):])

	pstr := parse_parameterised_string(line)
	if pstr == nil {
		s.syntax_error()
	}

	statement := cpp_statement(*pstr)
	s.append_statement(&statement)
}

// Parse (nested) LD statement.
func (s *parse_state) parse_ld_line() {
	line := strings.TrimSpace(s.line[len("ld "):])

	pstr := parse_parameterised_string(line)
	if pstr == nil {
		s.syntax_error()
	}

	statement := ld_statement(*pstr)
	s.append_statement(&statement)
}

// Parse first line of (nested) FOREACH statement.
func (s *parse_state) parse_first_foreach_line() {
	line := strings.TrimSpace(s.line[len("foreach "):])

	// foreach lines always end with an opening brace.
	if !strings.HasSuffix(line, "{") {
		s.syntax_error()
	}
	// Chop off {.
	line = strings.TrimSpace(line[:len(line)-1])

	var statement foreach_statement

	// Extract any excluding filter.
	if words := strings.Fields(line); len(words) > 2 && words[len(words)-2] == "excluding" {
		excluding_str := words[len(words)-1]
		excluding := parse_depgets(excluding_str)
		if excluding == nil {
			s.syntax_error()
		}
		statement.excluding = *excluding
		line = strings.Join(words[:len(words)-2], " ")
	}

	parts := strings.Split(line, " to ")
	if len(parts) != 2 {
		s.syntax_error()
	}

	statement.from = strings.TrimSpace(parts[0])
	statement.to = strings.TrimSpace(parts[1])

	// We are now entering into a state where we are parsing nested prehook
	// statements.
	s.append_statement(&statement)
	s.push_currop(FOREACH, &statement.statements)
}

// Parse FOREACH lines other than the first.
func (s *parse_state) parse_foreach_line() {
	opstr, nonblank := s.parse_operation_name()
	if !nonblank {
		return
	}

	switch opstr {
	case "shell":
		s.parse_shell_line()
	case "call":
		s.parse_call_line()
	case "cc":
		s.parse_cc_line()
	case "c++":
		s.parse_cpp_line()
	case "ld":
		s.parse_ld_line()
	case "}":
		s.pop_currop()
	default:
		s.syntax_error()
	}
}

// Parse PREHOOK lines other than the first.
func (s *parse_state) parse_prehook_line() {
	opstr, nonblank := s.parse_operation_name()
	if !nonblank {
		return
	}

	switch opstr {
	case "shell":
		s.parse_shell_line()
	case "call":
		s.parse_call_line()
	case "}":
		s.pop_currop()
	default:
		s.syntax_error()
	}
}

// Parse BUILD statement, first line.
func (s *parse_state) parse_build_first_line() {
	line := strings.TrimSpace(s.line[len("build "):])

	// build <targets> [ in "local dir"] from <dependencies> [as "alias"]

	// Build statements always end with a opening brace.
	if !strings.HasSuffix(line, "{") {
		s.syntax_error()
	}
	// Strip {.
	line = strings.TrimSpace(line[:len(line)-1])

	var statement build_statement

	// Extract any alias.
	if words := strings.Fields(line); len(words) > 2 && words[len(words)-2] == "as" {
		statement.alias = words[len(words)-1]
		line = strings.Join(words[:len(words)-2], " ")
	}

	// Now extract dependencies, which MUST be present.
	if parts := strings.SplitN(line, " from ", 2); len(parts) != 2 {
		s.syntax_error()
	} else {
		dep := parse_depgets(strings.TrimSpace(parts[1]))
		if dep == nil {
			s.syntax_error()
		}
		statement.dependencies = *dep

		// Chop dependencies from line.
		line = strings.TrimSpace(parts[0])
	}

	// Check for local dir.
	if parts := strings.SplitN(line, " in ", 2); len(parts) == 2 {
		statement.local_dir = strings.TrimSpace(parts[1])
		line = strings.TrimSpace(parts[0])
	}

	// Now finally we are left only with the target.
	target := parse_depgets(line)
	if target == nil {
		s.syntax_error()
	}
	statement.target = *target

	// We are now entering into a state where we are parsing nested build
	// statements.
	s.append_statement(&statement)
	s.push_currop(BUILD, &statement.statements)
}

// Parse BUILD lines other than the first.
func (s *parse_state) parse_build_line() {
	opstr, nonblank := s.parse_operation_name()
	if !nonblank {
		return
	}

	switch opstr {
	case "shell":
		s.parse_shell_line()
	case "call":
		s.parse_call_line()
	case "cc":
		s.parse_cc_line()
	case "c++":
		s.parse_cpp_line()
	case "ld":
		s.parse_ld_line()
	case "foreach":
		s.parse_first_foreach_line()
	case "}":
		s.pop_currop()
	default:
		s.syntax_error()
	}
}

// Parse COMMAND statement, first line.
func (s *parse_state) parse_command_first_line() {
	line := strings.TrimSpace(s.line[len("command "):])

	// Command statements always end with a opening brace.
	if !strings.HasSuffix(line, "{") {
		s.syntax_error()
	}
	// Chop off {.
	line = strings.TrimSpace(line[:len(line)-1])

	var statement command_statement

	// Check if dependencies specified.
	if parts := strings.SplitN(line, " needs ", 2); len(parts) == 2 {
		needs_str := strings.TrimSpace(parts[1])
		line = strings.TrimSpace(parts[0])

		dg := parse_depgets(needs_str)
		if dg == nil {
			s.syntax_error()
		}
		statement.dependencies = *dg
	}

	// Check for local dir.
	if parts := strings.SplitN(line, " in ", 2); len(parts) == 2 {
		statement.local_dir = strings.TrimSpace(parts[1])
		line = strings.TrimSpace(parts[0])
	}

	// Rest of line is just the command name.
	statement.name = line

	// We are now entering into a state where we are parsing nested command
	// statements.
	s.append_statement(&statement)
	s.push_currop(COMMAND, &statement.statements)
}

// Parse COMMAND lines other than the first.
func (s *parse_state) parse_command_line() {
	opstr, nonblank := s.parse_operation_name()
	if !nonblank {
		return
	}

	switch opstr {
	case "shell":
		s.parse_shell_line()
	case "call":
		// Ass we are parsing a command we know this exists.
		cmd := s.statements[len(s.statements)-1].(*command_statement)
		cmd.has_nested_calls = true
		s.parse_call_line()
	case "cc":
		s.parse_cc_line()
	case "c++":
		s.parse_cpp_line()
	case "ld":
		s.parse_ld_line()
	case "}":
		s.pop_currop()
	default:
		s.syntax_error()
	}
}

func (s *parse_state) parse_top_line() {
	opstr, nonblank := s.parse_operation_name()
	if !nonblank {
		return
	}

	switch opstr {
	case "set":
		s.parse_set_line()
	case "option":
		s.parse_option_line()
	case "prehook":
		s.parse_prehook_first_line()
	case "build":
		s.parse_build_first_line()
	case "command":
		s.parse_command_first_line()
	case "}":
		s.fatal_line_error("Mismatched brace")
	default:
		s.syntax_error()
	}
}

func (s *parse_state) parse_line() {
	if len(s.currop_stack) == 0 {
		s.parse_top_line()
		return
	}

	currop := s.currop_stack[len(s.currop_stack)-1]

	// Commands that actually alters the state machine state.
	switch currop {
	case PREHOOK:
		s.parse_prehook_line()
	case BUILD:
		s.parse_build_line()
	case COMMAND:
		s.parse_command_line()
	case FOREACH:
		s.parse_foreach_line()
	default:
		panic("Impossible")
	}
}

func assert_statement_is_pointer(statement interface{}) {
	if reflect.ValueOf(statement).Kind() != reflect.Ptr {
		panic("append_statement() passed a non-pointer?")
	}
}

// Is the specified variable name 'special', e.g. subject to `set special xxx`?
func is_special_var(name string) bool {
	for _, v := range SPECIAL_VARS {
		if name == v {
			return true
		}
	}

	return false
}

// Is the specified variable name reserved to avoid conflicts?
func is_reserved_var(name string) bool {
	for _, v := range RESERVED_VARS {
		if name == v {
			return true
		}
	}

	return false
}

// Output an error referring to a given line/line number to STDERR.
func line_error(prefix string, line_num int, line string, msg string, args ...interface{}) {
	prerr(prefix, "%d: %s", line_num, line)
	prerr(prefix, msg, args...)
}

// Parse depget. Note that we do not figure out what might be referring to
// rules, that gets handled after initial parse complete.
func parse_depget(str string) *depget {
	if strings.HasPrefix(str, "**") {
		// We strip the first * as the ** denotes recursive GLOB and we
		// label it as such so this is redundant.
		return &depget{RECURSIVE_GLOB, str[1:]}
	} else if strings.Contains(str, "*") {
		return &depget{GLOB, str}
	} else {
		// We will process 'files' looking for entries which are in fact
		// rules.
		return &depget{FILE, str}
	}

	return nil
}

// General function for parsing possibly labelled dependency/target sets.
func parse_depgets(str string) *depgetset {
	var ret depgetset

	// Do we have a label? (We avoid hell by disallowing odd placement of
	// '=' chars otherwise.
	if strings.Contains(str, "=") {
		parts := strings.Split(str, "=")
		if len(parts) != 2 {
			return nil
		}

		ret.label = strings.TrimSpace(parts[0])
		str = strings.TrimSpace(parts[1])
	}

	if strings.HasPrefix(str, "[") {
		// Missing terminating ].
		if !strings.HasSuffix(str, "]") {
			return nil
		}

		// Chop off [, ] chars.
		str = str[1:]
		str = str[:len(str)-1]

		for _, elem := range strings.Split(str, ",") {
			dg := parse_depget(strings.TrimSpace(elem))
			if dg == nil {
				return nil
			}

			ret.depgets = append(ret.depgets, *dg)
		}

	} else if strings.HasSuffix(str, "]") {
		// Stray ].
		return nil
	} else {
		// Without outer [] this is a singular depget.
		dg := parse_depget(str)
		if dg == nil {
			return nil
		}
		ret.depgets = []depget{*dg}
	}

	return &ret
}

// Parse a parameterised string, e.g. one potentially containing variables and
// embedded shell, e.g. shell scripts/bar.py $(foo) $$shell{stat -c%s kernel.elf}
func parse_parameterised_string(str string) *parameterised_string {
	var ret parameterised_string

	in_var := false
	in_var_paren := false
	in_embedded := false
	escaped := false
	curr := ""

	append_string_elem := func() {
		if len(curr) > 0 {
			ret = append(ret, param_string_element{STRING_ELEM, curr})
		}
		curr = ""
	}

	terminate_in_var := func() {
		if len(curr) > 0 {
			ret = append(ret, param_string_element{VARIABLE_ELEM, curr})
		}
		curr = ""
		in_var = false
	}

	for i := 0; i < len(str); i++ {
		chr := str[i]

		switch chr {
		case '\\':
			if escaped {
				curr += string(chr)
				escaped = false
			} else {
				escaped = true
			}
		case '$':
			if escaped || in_embedded {
				escaped = false
				// If we're escaping a char like '$foo\$' we're
				// into a silly situation and just have to abort.
				if in_var {
					return nil
				}

				curr += string(chr)
				continue
			}

			// Account for $foo$bar$baz.
			if in_var {
				terminate_in_var()
				// Keep in_var set for the next variable.
				continue
			}

			// Start embedded shell expression.
			if strings.HasPrefix(str[i:], EMBEDDED_SHELL_PREFIX) {
				i += len(EMBEDDED_SHELL_PREFIX) - 1
				in_embedded = true

				append_string_elem()
				continue
			}

			// Handle $(foo) variables.
			if i < len(str)-1 && str[i+1] == '(' {
				i++
				in_var_paren = true
			}

			append_string_elem()
			in_var = true
		case ' ', '\t':
			// $(foo bar) is not permitted!
			if in_var_paren {
				return nil
			}

			if in_var {
				terminate_in_var()
			}

			escaped = false
			curr += string(chr)
		case ')':
			// Reasonable to terminate here.
			if in_var {
				terminate_in_var()
			}

			if in_var_paren {
				in_var_paren = false
				continue
			}

			curr += string(chr)
		case EMBEDDED_SHELL_SUFFIX_CHAR:
			if !in_embedded {
				curr += string(chr)
				continue
			}

			ret = append(ret, param_string_element{EMBEDDED_SHELL, curr})
			curr = ""
			in_embedded = false
		default:
			escaped = false
			curr += string(chr)
		}
	}

	// Incomplete expressions.
	if in_embedded || in_var_paren {
		return nil
	}

	// Terminate any remaining variables/strings.
	if in_var {
		terminate_in_var()
	} else {
		append_string_elem()
	}

	return &ret
}

func dump_statement(indent int, statement interface{}) {
	print_indent := func() {
		for i := 0; i < indent; i++ {
			fmt.Printf("\t")
		}
	}

	print_indent()
	switch val := statement.(type) {
	case *set_statement:
		fmt.Printf("set ")
		if val.is_special {
			fmt.Printf("special ")
		} else {
			fmt.Printf("var ")
		}
		fmt.Printf("%s", val.key)
		if val.is_append {
			fmt.Printf(" += ")
		} else {
			fmt.Printf(" = ")
		}
		fmt.Printf("%s\n", val.val)
	case *option_statement:
		fmt.Printf("option %s\n", val.opt)
	case *prehook_statement:
		fmt.Printf("prehook %s %s {\n", val.dependencies, val.when)
		dump_statements(indent+1, val.statements)
		print_indent()
		fmt.Printf("}\n")
	case *build_statement:
		fmt.Printf("build %s", val.target)
		if len(val.local_dir) > 0 {
			fmt.Printf(" in %s", val.local_dir)
		}
		fmt.Printf(" from %s", val.dependencies)
		if len(val.alias) > 0 {
			fmt.Printf(" as %s", val.alias)
		}
		fmt.Printf(" {\n")
		dump_statements(indent+1, val.statements)
		print_indent()
		fmt.Printf("}\n")
	case *command_statement:
		fmt.Printf("command %s", val.name)
		if !val.dependencies.empty() {
			fmt.Printf(" needs %s", val.dependencies)
		}
		fmt.Printf(" {\n")
		dump_statements(indent+1, val.statements)
		print_indent()
		fmt.Printf("}\n")
	case *shell_statement:
		fmt.Printf("shell %s\n", val.expression)
	case *foreach_statement:
		fmt.Printf("foreach %s to %s", val.from, val.to)
		if !val.excluding.empty() {
			fmt.Printf(" excluding %s", val.excluding)
		}
		fmt.Printf(" {\n")
		dump_statements(indent+1, val.statements)
		print_indent()
		fmt.Printf("}\n")
	case *cc_statement:
		fmt.Printf("cc %s\n", parameterised_string(*val))
	case *cpp_statement:
		fmt.Printf("c++ %s\n", parameterised_string(*val))
	case *ld_statement:
		fmt.Printf("ld %s\n", parameterised_string(*val))
	case *call_statement:
		fmt.Printf("call %s\n", string(*val))
	default:
		fmt.Println("??UNKNOWN??")
	}
}

func dump_statements(indent int, statements statements) {
	for _, s := range statements {
		dump_statement(indent, s)
	}
}

func (s *parse_state) dump() {
	dump_statements(0, s.statements)
}

// Extract all rule names from parse state.
func (s *parse_state) extract_rule_names() []string {
	var ret []string

	for _, statement := range s.statements {
		switch s := statement.(type) {
		case *build_statement:
			if len(s.alias) > 0 {
				ret = append(ret, s.alias)
				continue
			}

			// Ignore broken (will error out at next stage) or multi build targets
			if len(s.target.depgets) != 1 || s.target.depgets[0].kind != FILE {
				continue
			}
			ret = append(ret, s.target.depgets[0].name)
		case *command_statement:
			ret = append(ret, s.name)
		}
	}

	return ret
}

// Set depgets which refer to rules as such now we know the names of all rules.
func (s *parse_state) fixup_depgets(dgs *depgetset) {
	for i, dg := range dgs.depgets {
		if dg.kind != FILE {
			continue
		}

		if _, ok := s.rule_set[dg.name]; !ok {
			continue
		}

		dgs.depgets[i].kind = RULE
	}
}

// Fixup dependencies that might refer to commands rather than files.
func (s *parse_state) fixup_dependencies() {

	for _, statement := range s.statements {
		switch st := statement.(type) {
		case *prehook_statement:
			s.fixup_depgets(&st.dependencies)
		case *build_statement:
			s.fixup_depgets(&st.dependencies)
		case *command_statement:
			s.fixup_depgets(&st.dependencies)
		}
	}
}

func do_parse() *parse_state {
	var state parse_state

	for i, line := range read_all(DEFAULT_BUILD_FILE) {
		line = strings.TrimSpace(line)
		state.line = line
		state.line_num = i + 1

		state.parse_line()
	}

	if len(state.currop_stack) != 0 {
		state.fatal_line_error("Mismatched brace")
	}

	if len(state.statements_stack) != 0 {
		panic("Impossible!")
	}

	// We can now extract all rule names and store in parse state.
	rules := state.extract_rule_names()
	rule_set := make(map[string]bool)
	for _, rule := range rules {
		rule_set[rule] = true
	}
	state.rule_set = rule_set

	// Mark dependencies that rely on a rule as relying on such rather than
	// a file.
	state.fixup_dependencies()

	return &state
}
