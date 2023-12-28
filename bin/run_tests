#!/usr/bin/env bash

BIN_DIR="$(dirname "$0")"
BIN_TEST_DIR="$BIN_DIR/tst"
TEST_UTILS_DIR="$BIN_DIR/utils/" #TODO: Inject these relative paths from Makefile?

for directory in "$BIN_TEST_DIR" "$TEST_UTILS_DIR"
do
	[ ! -d "$directory" ] && { >&2 echo "$directory not found. Did you \`make test\`?"; exit 1; }
done

# On a per-test suite, not per-test case basis
PICTRL_EXIT_ON_FAIL="${PICTRL_EXIT_ON_FAIL:-false}"
FAILED_TESTS=0
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$TEST_UTILS_DIR"
while read -d $'\0' file
do
	echo "[TEST] Running $file"
	$file
	status=$?

	if [ $status -eq 0 ]
	then
		echo "[PASS] $file"
		echo
	else
		>&2 echo "[FAIL] $file"
		>&2 echo
		((FAILED_TESTS++)) || true
		if [ "$PICTRL_EXIT_ON_FAIL" = true ]
		then
			exit 1
		fi
	fi
done < <(find "$BIN_TEST_DIR" -type f -executable -print0) || exit

exit $FAILED_TESTS
