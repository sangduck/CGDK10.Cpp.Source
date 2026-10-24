#!/bin/bash
#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                         run unit tests (linux)                            *
#*                                                                           *
#*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
#*  Best for Game Developement and Optimized for Game Developement.          *
#*                                                                           *
#*                (c) 2016 Cho Sanghyun All right reserved.                  *
#*                          http://www.CGCII.co.kr                           *
#*                                                                           *
#*****************************************************************************
#
# usage) build/run_unit_tests.sh [Debug|Release] [result directory]
#
#  - runs every unit test built by CMake (build/src/unit_test/CGDK10.unit_test.*.<build type>)
#    (unit tests are built only when GoogleTest is found : 'apt install libgtest-dev')
#  - results are written as JUnit XML (gtest xml) : <result directory>/<name>.xml
#  - a crashed / timed out executable is written as a failed test case : <result directory>/<name>.process.xml
#  - output of each executable : <result directory>/<name>.log
#  - exit code : 0 all passed, 1 failed, 2 no unit test executable or invalid result directory
#  - UNIT_TEST_TIMEOUT : timeout of each executable (seconds, default 600)
#  - UNIT_TEST_EXCLUDE : names not to run (extended regex. ex. "system.object.dump|server_query")
#
#-----------------------------------------------------------------------------

BUILD_TYPE=${1:-Release}
ROOT_DIR=$(cd "$(dirname "$(realpath "$0")")/.." && pwd)
BIN_DIR="$ROOT_DIR/build/src/unit_test"
RESULT_DIR=$(realpath -m "${2:-$ROOT_DIR/test-results}")
TIMEOUT=${UNIT_TEST_TIMEOUT:-600}

# 1) result directory
#    - removes only the results of the previous run (*.xml, *.log). the directory itself is not removed
#      (a wrong directory - ex. a build directory - must not be lost)
#    - a build/source directory (CMakeCache.txt, CMakeLists.txt, .svn) is refused
for MARK in CMakeCache.txt CMakeLists.txt .svn; do
	if [ -e "$RESULT_DIR/$MARK" ]; then
		echo "invalid result directory (has $MARK): $RESULT_DIR"
		exit 2
	fi
done
mkdir -p "$RESULT_DIR"
rm -f "$RESULT_DIR"/*.xml "$RESULT_DIR"/*.log

# 2) unit test executables
shopt -s nullglob
EXECUTABLES=("$BIN_DIR"/CGDK10.unit_test.*."$BUILD_TYPE")

if [ ${#EXECUTABLES[@]} -eq 0 ]; then
	echo "(err ) no unit test executable in '$BIN_DIR' ($BUILD_TYPE) : is GoogleTest installed? ('apt install libgtest-dev')"
	exit 2
fi

# - xml escape (for the failure message)
xml_escape()
{
	sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' -e 's/"/\&quot;/g' | tr -cd '\11\12\15\40-\176'
}

# 3) run
count_failed=0

for exe in "${EXECUTABLES[@]}"; do
	name=$(basename "$exe" ".$BUILD_TYPE")
	name=${name#CGDK10.unit_test.}

	# - excluded (UNIT_TEST_EXCLUDE : extended regex of the names)
	if [ -n "${UNIT_TEST_EXCLUDE:-}" ] && [[ "$name" =~ $UNIT_TEST_EXCLUDE ]]; then
		echo "=== [$name] skipped (UNIT_TEST_EXCLUDE)"
		continue
	fi

	xml="$RESULT_DIR/$name.xml"
	log="$RESULT_DIR/$name.log"
	work="$RESULT_DIR/work/$name"
	mkdir -p "$work"

	echo "=== [$name] $(date '+%H:%M:%S')"

	# - run in its own working directory
	time_begin=$(date +%s)
	( cd "$work" && timeout --kill-after=10 "$TIMEOUT" "$exe" --gtest_brief=1 --gtest_output="xml:$xml" ) > "$log" 2>&1
	result=$?
	time_elapsed=$(( $(date +%s) - time_begin ))

	grep -E '^\[  (PASSED|FAILED|SKIPPED) |^\[  FAILED  \] .*\(|^ *YOU HAVE' "$log" | sed 's/^/    /'

	# - process result : 1 is 'test failed' (written in the gtest xml)
	if [ $result -eq 0 ]; then
		echo "    exit=0 (${time_elapsed}s)"
		continue
	fi

	count_failed=$((count_failed + 1))

	if [ $result -eq 1 ] && [ -s "$xml" ]; then
		echo "    exit=1 : test failed (${time_elapsed}s)"
		continue
	fi

	# - crashed / timed out / no result : a failed test case
	if [ $result -eq 124 ] || [ $result -eq 137 ]; then
		reason="timed out (${TIMEOUT}s)"
	elif [ $result -gt 128 ]; then
		reason="crashed (signal $((result - 128)))"
	else
		reason="exit code $result"
	fi
	echo "    exit=$result : $reason (${time_elapsed}s)"

	{
		echo '<?xml version="1.0" encoding="UTF-8"?>'
		echo "<testsuites tests=\"1\" failures=\"1\" errors=\"0\" name=\"$name\">"
		echo "  <testsuite name=\"$name.process\" tests=\"1\" failures=\"1\" errors=\"0\" time=\"$time_elapsed\">"
		echo "    <testcase name=\"process_exit\" classname=\"$name.process\" time=\"$time_elapsed\">"
		echo "      <failure message=\"$reason\">"
		tail -n 80 "$log" | xml_escape
		echo "      </failure>"
		echo "    </testcase>"
		echo "  </testsuite>"
		echo "</testsuites>"
	} > "$RESULT_DIR/$name.process.xml"
done

# 4) clean up working directories
rm -rf "$RESULT_DIR/work"

# return)
echo "=== ${#EXECUTABLES[@]} unit test executables : $count_failed failed"
[ $count_failed -eq 0 ]
