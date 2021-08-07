#!/bin/bash
set -e; set -o pipefail

gcc_maj=$(gcc -v 2>&1 | grep 'gcc version' | awk '{print $3}' | awk -F '.' '{print $1}')
if [[ ${gcc_maj} -lt 9 ]]; then
	echo "GCC major version is ${gcc_maj}, minimum required is 9."
	exit 1
fi
