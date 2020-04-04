#!/bin/sh
set -e

wait_for_sleep() {
	# Wait until the given input is in sleep state. Assuming that it
	# is in sleep state because it called read(3) on stdin.

	awkscript="{ if (\$1 == ${1}) print(substr(\$2, 0, 1)) }"
	while [ "$(ps -A -o pid,stat | awk "${awkscript}" )" != "S" ]; do
		 true
	done
}

session="input-test"
testdir="/tmp/input-test"
outfile="${testdir}/output"

INPUT="${INPUT:-$(pwd)/../input}"
if [ ! -x "${INPUT}" ]; then
	echo "Couldn't find input executable '${INPUT}'" 1>&2
	exit 1
fi

# Don't pickup user configuration file
INPUT="env INPUTRC=/dev/null '${INPUT}'"

mkdir "${testdir}"
trap "rm -rf '${testdir}' ; tmux kill-session -t '${session}' 2>/dev/null || true" INT EXIT

for test in *; do
	[ -d "${test}" ] || continue

	name=${test##*/}
	printf "Running test case '%s': " "${name}"

	cmd="${INPUT} > '${outfile}'"
	if [ -s "${test}/opts" ]; then
		set -- $(cat "${test}/opts")
		cmd="${INPUT} $@ > '${outfile}'"
	fi
	tmux new-session -d -s "${session}" "${cmd}"

	pid="$(tmux list-panes -t "${session}" -F "#{pane_pid}")"
	while read -r line; do
		wait_for_sleep "${pid}"
		tmux send-keys -t "${session}" "${line}"
	done < "${test}/input"

	if [ ! -e "${test}/exits" ]; then
		wait_for_sleep "${pid}"
		tmux send-keys -t "${session}" C-d

		# Can't use wait(1) because proc was started in different env.
		while tmux has-session -t "${session}" 2>/dev/null; do
			true
		done
	fi

	if ! cmp -s "${outfile}" "${test}/output"; then
		printf "FAIL: Output didn't match.\n\n"
		diff -u "${outfile}" "${test}/output"
		exit 1
	fi

	printf "OK.\n"
done
