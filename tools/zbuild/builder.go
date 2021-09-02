package main

import (
	"fmt"
	"path"
	"path/filepath"
	"strings"
)

type rule struct {
	name, dir, target, multi_glob string
	is_multi, is_phony            bool
	rule_deps                     []string
	file_deps                     []string
	shell_commands                []string
}

type unconditional_prehook struct {
	shell_commands []string
}

type conditional_prehook struct {
	exts                    []string
	deferred_shell_commands statements
	seen_files              map[string]bool
}

type build_graph struct {
	vars                                     map[string]string
	build_dir, def, includes, default_cflags string
	rules                                    map[string]*rule
	rule_is_done                             map[string]bool
	options                                  map[string]bool
	unconditional_prehooks                   []unconditional_prehook
	conditional_prehooks                     []conditional_prehook
}

func (b *build_graph) dump() {
	fmt.Printf("-- VARS: --\n")
	for key, value := range b.vars {
		fmt.Printf("%s -> %s\n", key, value)
	}
	fmt.Printf("(special) build_dir = %s\n", b.build_dir)
	fmt.Printf("(special) default = %s\n", b.def)
	fmt.Printf("(special) includes = %s\n", b.includes)
	fmt.Printf("(special) default_cflags = %s\n", b.default_cflags)

	fmt.Printf("\n-- RULES: --\n")
	for name, rule := range b.rules {
		fmt.Printf("Rule %s:", name)
		if b.rule_is_done[rule.name] {
			fmt.Printf(" (done)")
		}
		fmt.Printf("\n")

		fmt.Printf("dir = '%s', target='%s', multi_glob='%s'",
			rule.dir, rule.target, rule.multi_glob)

		if rule.is_multi {
			fmt.Printf(", MULTITARGET")
		}

		if rule.is_phony {
			fmt.Printf(", COMMAND")
		}

		fmt.Printf("\n")

		fmt.Printf("Rule dependencies: [")
		for _, dep := range rule.rule_deps {
			fmt.Printf("%s, ", dep)
		}
		fmt.Printf("]\n")

		fmt.Printf("File dependencies: [")
		for _, dep := range rule.file_deps {
			fmt.Printf("%s, ", dep)
		}
		fmt.Printf("]\n")

		fmt.Printf("Shell commands:\n")
		for _, shell := range rule.shell_commands {
			fmt.Printf("%s\n", shell)
		}

		fmt.Printf("---\n")
	}
}

// Prefixes each include with -I
func prefix_includes(str string) string {
	var ret []string

	for _, incl := range strings.Fields(str) {
		ret = append(ret, "-I"+incl)
	}

	return strings.Join(ret, " ")
}

func (b *build_graph) init_extract_special_var(key, val string) {
	switch key {
	case "build_dir":
		b.build_dir = val
	case "default":
		b.def = val
	case "includes":
		b.includes = prefix_includes(val)
	case "default_cflags":
		b.default_cflags = val
	default:
		panic("Impossible!")
	}
}

func (b *build_graph) init_extract_vars(state *parse_state) {
	for _, statement := range state.statements {
		switch s := statement.(type) {
		case *set_statement:
			if s.is_special {
				b.init_extract_special_var(s.key, s.val)
			} else if s.is_append {
				if _, ok := b.vars[s.key]; !ok {
					fatal("Trying to append missing variable '%s'",
						s.key)
				} else {
					b.vars[s.key] += strings.TrimSpace(s.val)
				}
			} else {
				b.vars[s.key] = strings.TrimSpace(s.val)
			}
		}
	}
}

func (b *build_graph) combine_vars(additional_vars map[string]string) map[string]string {
	// TODO: Inefficient!
	vars := make(map[string]string)
	for k, v := range b.vars {
		vars[k] = v
	}
	for k, v := range additional_vars {
		vars[k] = v
	}

	vars["build_dir"] = b.build_dir
	vars["default"] = b.def
	vars["includes"] = b.includes
	vars["default_cflags"] = b.default_cflags

	return vars
}

// Substitute variables contained in parameterised string with actual string
// values.
func (b *build_graph) substitute_vars(rule string, additional_vars map[string]string,
	str *parameterised_string) string {
	var ret []string

	vars := b.combine_vars(additional_vars)

	for _, elem := range *str {
		switch elem.kind {
		case STRING_ELEM:
			ret = append(ret, elem.str)
		case VARIABLE_ELEM:
			if val, ok := vars[elem.str]; !ok {
				fatal("Rule '%s': Unrecognised variable $(%s)", rule,
					elem.str)
			} else {
				ret = append(ret, val)
			}
		case EMBEDDED_SHELL:
			// TODO: This will only function for 'shell' commands.
			ret = append(ret, "$("+elem.str+")")
		}
	}

	subst := strings.Join(ret, "")

	return subst
}

func (b *build_graph) expand_foreach_statement(rule *rule, additional_vars map[string]string,
	foreach *foreach_statement) []string {
	// Commands cannot do foreach, only multi build statements.
	if rule.is_phony {
		panic("Impossible!")
	}
	if !rule.is_multi {
		fatal("Rule '%s' is using a foreach but isn't a multitarget rule",
			rule.name)
	}

	// For now we only support "foreach source to output".
	if foreach.from != "source" || foreach.to != "output" {
		fatal("Rule '%s' specifies foreach %s to %s however only 'source to output' supported",
			rule.name, foreach.from, foreach.to)
	}

	// Extract 'excluding' filter.
	excluding := make(map[string]bool)
	if !foreach.excluding.empty() {
		exclude_files := extract_file_dependencies(rule.name, rule.dir, &foreach.excluding)
		for _, f := range exclude_files {
			excluding[f] = true
		}
	}

	// Figure out our target file extension.
	target_ext := path.Ext(rule.multi_glob)

	var ret []string

	// Determine which files have actually changed - these are the only ones
	// we loop over.
	for _, source := range rule.file_deps {
		if excluding[source] {
			continue
		}

		// We always target base directory.
		output := path.Base(replace_ext(source, target_ext))

		if newer, err := is_file_newer(rule.dir, source, output); err != nil {
			fatal("Rule '%s': Error when comparing '%s' to '%s' (dir '%s'): %s",
				rule.name, source, output, rule.dir, err)
		} else if !newer {
			continue
		}

		full_source := source
		if len(rule.dir) > 0 {
			full_source = path.Join(rule.dir, source)
		}

		// Now generate the shell commands for each foreach iteration.
		additional_vars["source"] = full_source
		additional_vars["output"] = output
		ss := b.extract_shell_commands(rule, additional_vars, foreach.statements)
		ret = append(ret, ss...)
	}

	return ret
}

// Generates special variable params for CC and CPP statements, e.g. the
// 'includes' and 'default_cflags' params.
func (b *build_graph) gen_special_cc_params() string {
	ret := ""

	if len(b.includes) > 0 {
		ret += " " + b.includes
	}

	if len(b.default_cflags) > 0 {
		ret += " " + b.default_cflags
	}

	return ret
}

func (b *build_graph) extract_shell_commands(rule *rule,
	additional_vars map[string]string, statements statements) []string {
	var ret []string

	var name string
	if rule == nil {
		// TODO: This is an assumption!
		name = "prehook"
	} else {
		name = rule.name
	}

	// Note that call statements should already have been expanded prior to
	// this so we don't deal with them here.
	for _, statement := range statements {
		switch s := statement.(type) {
		case *shell_statement:
			ret = append(ret,
				b.substitute_vars(name, additional_vars, &s.expression))
		case *cc_statement:
			suffix := b.substitute_vars(name, additional_vars, (*parameterised_string)(s))
			str := CC_BINARY + b.gen_special_cc_params() + " " + suffix
			ret = append(ret, str)
		case *cpp_statement:
			suffix := b.substitute_vars(name, additional_vars, (*parameterised_string)(s))
			str := CPP_BINARY + b.gen_special_cc_params() + " " + suffix
			ret = append(ret, str)
		case *ld_statement:
			suffix := b.substitute_vars(name, additional_vars, (*parameterised_string)(s))
			str := LD_BINARY + " " + suffix
			ret = append(ret, str)
		case *foreach_statement:
			if rule == nil {
				fatal("Invalid foreach command")
			}

			ret = append(ret, b.expand_foreach_statement(rule, additional_vars, s)...)
		default:
			panic("Impossible!")
		}
	}

	return ret
}

func extract_rule_dependencies(dgs *depgetset) []string {
	var ret []string

	for _, dg := range dgs.depgets {
		if dg.kind != RULE {
			continue
		}
		ret = append(ret, dg.name)
	}

	// We don't need to check if these rules exist, because if they don't
	// they will be FILE dependencies and if the files don't exist we'll
	// error out elsewhere.
	return ret
}

// Read the ".d" dependencies file generated by the compiler associated with the
// specified source file and retrieve listed dependency files.
func parse_dependencies(file string) []string {
	// Remove any relative path as we are currently (rather unwisely!)
	// generating object/dependency files in root and not differentiating on
	// relative subdir.
	file = replace_ext(path.Base(file), "d")

	// If no dependency file nothing to read.
	if !file_exists("", file) {
		return []string{}
	}

	// Format of file is source.o: xxxx \ [newline] xxxx \ [newline] etc.
	str := strings.Join(read_all(file), " ")
	// Strip out the "\"'s.
	str = strings.ReplaceAll(str, "\\", "")
	// Remove the object file name.
	parts := strings.Split(str, ":")
	if len(parts) != 2 {
		fatal("Invalid dependencies file %s", file)
	}

	// Strips all whitespace.
	return strings.Fields(parts[1])
}

func extract_file_dependencies(rule, dir string, dgs *depgetset) []string {
	hash := make(map[string]bool)

	for _, dg := range dgs.depgets {
		switch dg.kind {
		case RULE:
			continue
		case FILE:
			if !file_exists(dir, dg.name) {
				fatal("Rule '%s': Dependency '%s' does not exist",
					rule, dg.name)
			}
			hash[dg.name] = true
		case GLOB:
			var glob_path string
			if len(dir) > 0 {
				glob_path = path.Join(dir, dg.name)
			} else {
				glob_path = dg.name
			}
			if matches, err := filepath.Glob(glob_path); err != nil {
				fatal("Rule '%s': Unable to read GLOB '%s': %s",
					rule, glob_path, err)
			} else {
				// We set the `dir` param later.
				for _, match := range matches {
					dir := path.Dir(dg.name)
					name := path.Join(dir, path.Base(match))
					hash[name] = true
				}
			}
		case RECURSIVE_GLOB:
			fatal("Rule '%s': Recursive GLOBs are not current supported!",
				rule)
		default:
			panic("Impossible!")
		}
	}

	var ret []string
	for k, _ := range hash {
		ret = append(ret, k)
	}

	return ret
}

func (b *build_graph) init_command(cmd *command_statement) {
	if _, ok := b.rules[cmd.name]; ok {
		fatal("Duplicate command '%s'", cmd.name)
	}

	if len(cmd.local_dir) > 0 && !dir_exists(cmd.local_dir) {
		fatal("Rule '%s': Local directory '%s' does not exist or not a directory",
			cmd.name, cmd.local_dir)
	}

	file_deps := extract_file_dependencies(cmd.name, cmd.local_dir, &cmd.dependencies)
	if len(file_deps) > 0 {
		fatal("Rule '%s': Commands cannot have file dependencies",
			cmd.name)
	}

	r := rule{
		name:      cmd.name,
		dir:       cmd.local_dir,
		target:    cmd.name,
		is_phony:  true,
		rule_deps: extract_rule_dependencies(&cmd.dependencies),
	}

	additional_vars := make(map[string]string)

	label := cmd.dependencies.label
	if len(label) > 0 {
		if len(r.rule_deps) > 0 {
			fatal("Rule '%s' specifies dependencies labelled '%s' but they contain rules?",
				r.name, label)
		}

		additional_vars[label] = strings.Join(r.file_deps, " ")
	}

	r.shell_commands = b.extract_shell_commands(&r, additional_vars,
		cmd.statements)

	b.rules[r.name] = &r
}

// Check that rule dependencies actually exist.
func (b *build_graph) check_rule_deps() {
	for _, r := range b.rules {
		for _, dep := range r.rule_deps {
			if _, ok := b.rules[dep]; !ok {
				fatal("Unable to resolve dependency '%s' for rule '%s'",
					dep, r.name)
			}
		}
	}
}

func (b *build_graph) fixup_nested_command_calls(cmd *command_statement) {
	var new_statements statements

	// To implement calls we simply copy generated commands from referenced
	// commands in place. TODO: This only supports 1 level of nesting.
	for _, statement := range cmd.statements {
		switch s := statement.(type) {
		case *call_statement:
			name := string(*s)
			if rule, ok := b.rules[name]; !ok {
				fatal("Unable to lookup called rule '%s'", name)
			} else {
				for _, shell := range rule.shell_commands {
					// We fake things out by reinsterted
					// parameterised strings to replace the
					// calls. TODO: Hacky.
					paramstr := parameterised_string{param_string_element{STRING_ELEM, shell}}
					fake_statement := shell_statement{paramstr}
					new_statements = append(new_statements, &fake_statement)
				}
			}
		default:
			new_statements = append(new_statements, statement)
		}
	}
	cmd.statements = new_statements
}

func (b *build_graph) init_commands(state *parse_state) {
	// Process non-nested commands (ones that invoke 'call') first.
	for _, statement := range state.statements {
		switch s := statement.(type) {
		case *command_statement:
			if s.has_nested_calls {
				continue
			}
			b.init_command(s)
		}
	}

	// Now process nested commands.
	for _, statement := range state.statements {
		switch s := statement.(type) {
		case *command_statement:
			if !s.has_nested_calls {
				continue
			}
			b.fixup_nested_command_calls(s)
			b.init_command(s)
		}
	}
}

func (b *build_graph) init_build(build *build_statement) {
	if len(build.target.depgets) != 1 {
		fatal("Build statements currently only support a single target (multi glob or target file)")
	}

	is_multi := false
	name := ""
	target := build.target.depgets[0]

	switch target.kind {
	case FILE:
		name = target.name
	case GLOB:
		is_multi = true
	case RECURSIVE_GLOB:
		fatal("Build targeting '%s': Recursive glob not permited in build target", target.name)
	case RULE:
		fatal("Build targeting '%s': Target cannot be a rule", target.name)
	}

	multi_glob := ""
	if is_multi {
		name = build.alias

		multi_glob = target.name
		if multi_glob[0] != '*' {
			fatal("Rule '%s': Multi-target glob '%s' cannot be prefixed",
				name, multi_glob)
		}

		if len(build.alias) == 0 {
			fatal("Rule '%s': Multi-target builds must specify an alias",
				name)
		}

	} else if len(build.alias) > 0 {
		fatal("Biuld targeting '%s': Non-multi builds cannot have aliases",
			target.name)
	}

	if _, ok := b.rules[name]; ok {
		fatal("Duplicate build '%s'", name)
	}

	if len(build.local_dir) > 0 && !dir_exists(build.local_dir) {
		fatal("Rule '%s': Local directory '%s' does not exist or not a directory",
			name, build.local_dir)
	}

	r := rule{
		name:       name,
		dir:        build.local_dir,
		target:     name,
		is_multi:   is_multi,
		multi_glob: multi_glob,
		is_phony:   false,
		rule_deps:  extract_rule_dependencies(&build.dependencies),
		file_deps:  extract_file_dependencies(name, build.local_dir, &build.dependencies),
	}

	// We need to insert the alias as a global (!) variable containing the
	// targets. This makes aliases being set order-dependent.
	if is_multi {
		var outputs []string

		output_ext := path.Ext(multi_glob)

		for _, name := range r.file_deps {
			output := path.Base(replace_ext(name, output_ext))
			outputs = append(outputs, output)
		}

		b.vars[build.alias] = strings.Join(outputs, " ")
	}

	// Add labelled variable.
	additional_vars := make(map[string]string)
	label := build.dependencies.label
	if len(label) > 0 {
		if len(r.rule_deps) > 0 {
			fatal("Rule '%s' specifies dependencies labelled '%s' but they contain rules?",
				r.name, label)
		}

		additional_vars[label] = strings.Join(r.file_deps, " ")
	}

	r.shell_commands = b.extract_shell_commands(&r, additional_vars, build.statements)

	b.rules[r.name] = &r
}

func (b *build_graph) init_builds(state *parse_state) {
	for _, statement := range state.statements {
		switch s := statement.(type) {
		case *build_statement:
			b.init_build(s)
		}
	}
}

func (b *build_graph) init_options(state *parse_state) {
	for _, statement := range state.statements {
		switch s := statement.(type) {
		case *option_statement:
			switch s.opt {
			case COMPUTE_DEPENDENCIES_OPTION:
				if DEBUG {
					fmt.Println("compute_dependencies option SET")
				}

				b.options["compute_dependencies"] = true
				// Generates dependency (suffix .d) files.
				b.default_cflags += "-MD"
			default:
				panic("Impossible!")
			}
		}
	}
}

func (b *build_graph) init_unconditional_prehook(statements statements) {
	// Used in place of additional variables as none defined for prehooks
	// (we don't yet support labelled dependencies).
	dummy := make(map[string]string)

	shells := b.extract_shell_commands(nil, dummy, statements)
	pre := unconditional_prehook{shell_commands: shells}
	b.unconditional_prehooks = append(b.unconditional_prehooks, pre)
}

func (b *build_graph) init_conditional_prehook(presrc *prehook_statement) {
	if len(presrc.dependencies.label) > 0 {
		fatal("Labels are not supported in prehook statements")
	}

	pre := conditional_prehook{deferred_shell_commands: presrc.statements}
	pre.seen_files = make(map[string]bool)

	for _, dg := range presrc.dependencies.depgets {
		switch dg.kind {
		case RECURSIVE_GLOB:
			ext := dg.name[2:]
			pre.exts = append(pre.exts, ext)
		default:
			fatal("Conditional prehooks only supported for file ext only")
		}
	}

	b.conditional_prehooks = append(b.conditional_prehooks, pre)
}

func (b *build_graph) init_prehooks(state *parse_state) {
	for _, statement := range state.statements {
		switch s := statement.(type) {
		case *prehook_statement:
			switch s.when {
			case ALWAYS_PREHOOK:
				if !s.dependencies.empty() {
					fatal("Unconditional prehooks cannot have dependencies")
				}

				b.init_unconditional_prehook(s.statements)
			case ON_CHANGE_PREHOOK:
				b.init_conditional_prehook(s)
			default:
				panic("Impossible!")
			}
		}
	}
}

func (b *build_graph) init(state *parse_state) {
	b.vars = make(map[string]string)
	b.rules = make(map[string]*rule)
	b.rule_is_done = make(map[string]bool)
	b.options = make(map[string]bool)

	b.init_extract_vars(state) // On the first pass we pull out the variables.
	b.init_options(state)
	b.init_commands(state)
	b.init_builds(state)
	b.init_prehooks(state)
	b.check_rule_deps()

	// We currently REQUIRE a default rule.
	if len(b.def) == 0 {
		fatal("Default rule not specified")
	}
}

// Actually execute build steps, recursion etc. handled elsewhere.
func (b *build_graph) exec_build(rule *rule, target string) {
	if DEBUG {
		fmt.Printf("Executing rule '%s'...\n", rule.name)
	}

	for _, shell := range rule.shell_commands {
		if !shell_exec(shell) {
			fatal("Rule '%s': Command failed with non-zero exit code",
				rule.name)
		}
	}

	// If a multi target rule, we touch a temporary file to keep track of
	// whether sources have been updated.
	if rule.is_multi {
		touch(target)
	}
}

// Expand file dependency list to include dependencies obtained from the .d
// dependency file. Deduplicates output.
func add_depfile_deps(rule *rule, file_deps []string) []string {
	var ret []string

	seen := make(map[string]bool)

	// We normalise all file dependencies to project root to avoid issues
	// with rule execution in a particular directory but dependencies
	// being relative to root.
	// TODO: Do this better, the relative directory stuff would likely be
	// handled fair more cleanly and consistently by being set on file
	// dependency generation and no longer referenced.
	dir := rule.dir
	rule.dir = ""

	for _, filename := range file_deps {
		// Normalise.
		filename = path.Join(dir, filename)

		if seen[filename] {
			continue
		}
		seen[filename] = true
		ret = append(ret, filename)

		for _, dep := range parse_dependencies(filename) {
			if seen[dep] {
				continue
			}
			seen[dep] = true

			ret = append(ret, dep)
		}
	}

	return ret
}

func (b *build_graph) exec_conditional_prehook(pre *conditional_prehook, filename string) {
	// Insert the filename as variable $source.
	additional_vars := make(map[string]string)
	additional_vars["source"] = filename

	shell_commands := b.extract_shell_commands(nil, additional_vars,
		pre.deferred_shell_commands)

	for _, shell := range shell_commands {
		if !shell_exec(shell) {
			fatal("Conditional prehook command failed with non-zero exit code")
		}
	}
}

func should_exec_conditional_prehook(pre *conditional_prehook, filename string) bool {
	if pre.seen_files[filename] {
		return false
	}

	ext := path.Ext(filename)
	if len(ext) > 0 && ext[0] == '.' {
		ext = ext[1:]
	}

	for _, target_ext := range pre.exts {
		if ext == target_ext {
			pre.seen_files[filename] = true
			return true
		}
	}

	return false
}

func (b *build_graph) exec_conditional_prehooks(filename string) {
	for _, pre := range b.conditional_prehooks {
		if should_exec_conditional_prehook(&pre, filename) {
			b.exec_conditional_prehook(&pre, filename)
		}
	}
}

// Determine whether file dependencies indicate a rule need be run.
func (b *build_graph) check_file_deps(rule *rule, target string) bool {
	var file_deps []string

	if b.options["compute_dependencies"] {
		file_deps = add_depfile_deps(rule, rule.file_deps)
	} else {
		file_deps = rule.file_deps
	}

	ret := false

	for _, filename := range file_deps {
		if newer, err := is_file_newer(rule.dir, filename, target); err != nil {
			panic(err)
		} else if newer {
			if DEBUG {
				fmt.Printf("%s: '%s' is newer than '%s' so will run!\n",
					rule.name, filename, target)
			}

			// We don't exit early as we may need to execute prehooks.
			ret = true

			b.exec_conditional_prehooks(filename)
		}
	}

	return ret
}

// Returns true if the rule had to run.
func (b *build_graph) run_build(rule_name string) bool {
	rule, ok := b.rules[rule_name]
	if !ok {
		fatal("Cannot find rule '%s'", rule_name)
	}

	if b.rule_is_done[rule_name] {
		return false
	}

	// File we are comparing file dependencies too.
	target := ""
	if rule.is_multi {
		// In the case of multi-target, we generate a temporary file
		// named after the alias to compare to.
		target = ZBUILD_TMPFILE_PREFIX + rule.name
	} else {
		// Otherwise the target name will be the same as the rule name
		// which will be a file.
		target = rule.name
	}

	// Check if we need to run the rule by file dependencies...
	should_exec := b.check_file_deps(rule, target)

	// Now, recurse :) we do this even if the file dependencies have already
	// confirmed we need to rebuild because the rules above us may also need
	// to be rebuilt themselves as additional dependencies.
	b.rule_is_done[rule_name] = true // Guard against cycles.
	for _, ruledep := range rule.rule_deps {
		if b.run_build(ruledep) {
			should_exec = true
		}
	}

	// Commands should always run.
	if rule.is_phony {
		should_exec = true
	}

	// If target doesn't exist we should definitely run!
	if !file_exists("", target) {
		should_exec = true
	}

	if should_exec {
		b.exec_build(rule, target)
	}

	return should_exec
}

func (b *build_graph) run_unconditional_prehooks() {
	for _, prehook := range b.unconditional_prehooks {
		for _, shell := range prehook.shell_commands {
			if !shell_exec(shell) {
				fatal("Prehook command failed with non-zero exit code")
			}
		}
	}
}

func do_build(state *parse_state, rule_name string) {
	var graph build_graph
	graph.init(state)

	graph.run_unconditional_prehooks()

	if rule_name == "" {
		graph.run_build(graph.def)
	} else {
		graph.run_build(rule_name)
	}
}
