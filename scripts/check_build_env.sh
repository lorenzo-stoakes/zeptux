#!/bin/bash
set -e; set -o pipefail

function get_maj_ver()
{
	awk -F '.' '{print $1}'
}

function get_min_ver()
{
	awk -F '.' '{print $2}'
}

# Check go >= 1.16
go_ver=$(go version | awk '{print $3}' | sed 's/go//')
go_maj=$(echo $go_ver | get_maj_ver)
if [[ ${go_maj} -lt 1 ]]; then
	echo "go major version is ${go_maj}, minimum required is 1."
	exit 1
fi
go_min=$(echo $go_ver | get_min_ver)
if [[ ${go_min} -lt 16 ]]; then
	echo "go minor version is ${go_min}, minimum required is 16."
	exit 1
fi

# Check gcc >= 11
gcc_maj=$(gcc -v 2>&1 | grep 'gcc version' | awk '{print $3}' | get_maj_ver)
if [[ ${gcc_maj} -lt 11 ]]; then
	echo "GCC major version is ${gcc_maj}, minimum required is 11."
	exit 1
fi
