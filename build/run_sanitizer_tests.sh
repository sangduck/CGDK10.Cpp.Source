#!/bin/bash
#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                      Ver 10.0 / release 2019.12.11                        *
#*                                                                           *
#*                run unit tests with sanitizers (linux)                     *
#*                                                                           *
#*****************************************************************************
#
# usage) build/run_sanitizer_tests.sh [result directory]
#
#  - builds the core (build/) with the sanitizers (AddressSanitizer + UndefinedBehaviorSanitizer / ThreadSanitizer),
#    runs the unit tests and the admin e2e test (admin_server, host_agent, the service : the sanitizer build)
#    * build/ and include/ are copied to a work directory (SANITIZER_WORK_DIR, default ~/.cache/cgdk_sanitizer/<sanitizer>)
#      (not /tmp : it is a memory file system on some systems and the build is large (about 20GB))
#      : the libraries and the executables of the normal build are not overwritten
#    * 'alignment' is not checked (the buffers read unaligned integers by design : x86/arm64)
#    * any sanitizer error fails the test executable (-fno-sanitize-recover=all)
#  - results : <result directory>/<name>.xml (JUnit) / .log (default <root>/test-results-sanitizer)
#  - SANITIZER     : sanitizers (default 'address,undefined'. 'thread' : data races (ThreadSanitizer, not with address))
#  - SANITIZER_E2E : 0 : the admin e2e test (run_admin_e2e.sh) is not run with the sanitizer build (default 1)
#  - SANITIZER_CXX : compiler (default g++)
#  - UNIT_TEST_EXCLUDE : tests not to run (default "system.object.dump" : it attaches gdb to itself, which hangs with the sanitizers)
#  - exit code : 0 all passed, 1 failed (build or test), 2 no unit test executable
#
#-----------------------------------------------------------------------------
set -u

ROOT_DIR=$(cd "$(dirname "$(realpath "$0")")/.." && pwd)
RESULT_DIR=$(realpath -m "${1:-$ROOT_DIR/test-results-sanitizer}")
SANITIZER=${SANITIZER:-address,undefined}
WORK_DIR=$(realpath -m "${SANITIZER_WORK_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/cgdk_sanitizer/${SANITIZER//,/_}}")
SANITIZER_CXX=${SANITIZER_CXX:-g++}
CORES=$(nproc)

# check) the work directory must not be the source
if [ "$WORK_DIR" = "$ROOT_DIR" ] || [ -e "$WORK_DIR/.svn" ]; then
	echo "invalid work directory : $WORK_DIR"
	exit 1
fi

echo "=== sanitizer : $SANITIZER ($SANITIZER_CXX)"
echo "    source : $ROOT_DIR"
echo "    work   : $WORK_DIR"

# 1) copy the sources (without the outputs of the normal build)
mkdir -p "$WORK_DIR"
for DIR in build include; do
	rsync -a --delete \
		--exclude=.svn --exclude=.vs --exclude=/execute --exclude=/src/unit_test --exclude=output/ \
		--exclude='*.obj' --exclude='*.pdb' --exclude='*.ilk' --exclude='*.pch' --exclude='*.ipch' --exclude='*.idb' \
		--exclude='*.lib' --exclude='*.exe' --exclude='*.dll' \
		"$ROOT_DIR/$DIR/" "$WORK_DIR/$DIR/" || exit 1
done

# 2) compiler wrapper (the flags are added to compile and link)
#    - alignment : only with undefined
FLAG_NO_ALIGNMENT=""
[[ "$SANITIZER" == *undefined* ]] && FLAG_NO_ALIGNMENT="-fno-sanitize=alignment"
cat > "$WORK_DIR/.sanitizer_cxx" <<EOF
#!/bin/sh
exec $SANITIZER_CXX -fsanitize=$SANITIZER $FLAG_NO_ALIGNMENT -fno-sanitize-recover=all -fno-omit-frame-pointer "\$@"
EOF
chmod +x "$WORK_DIR/.sanitizer_cxx"

# 3) build (Debug)
cmake -S "$WORK_DIR/build" -B "$WORK_DIR/_sanitizer_build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="$WORK_DIR/.sanitizer_cxx" > "$WORK_DIR/cmake.log" 2>&1 \
	|| { tail -20 "$WORK_DIR/cmake.log"; exit 1; }
if ! make -C "$WORK_DIR/_sanitizer_build" -j"$CORES" > "$WORK_DIR/make.log" 2>&1; then
	grep -E "error" "$WORK_DIR/make.log" | head -20
	echo "build failed : $WORK_DIR/make.log"
	exit 1
fi

# 4) run the unit tests (build/run_unit_tests.sh of the work directory)
#    - detect_leaks=0 : the singletons are not released at the exit (not leaks of the tests)
#    - thread : a data race fails the executable (exit code 66 at the exit)
export ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0:abort_on_error=1}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1}
export TSAN_OPTIONS=${TSAN_OPTIONS:-second_deadlock_stack=1}
export UNIT_TEST_EXCLUDE=${UNIT_TEST_EXCLUDE-system.object.dump}
bash "$WORK_DIR/build/run_unit_tests.sh" Debug "$RESULT_DIR"
result=$?

# - the patterns of the sanitizer reports
REPORT_PATTERN="ERROR: AddressSanitizer|runtime error:|WARNING: ThreadSanitizer"

reports=$(grep -lE "$REPORT_PATTERN" "$RESULT_DIR"/*.log 2>/dev/null)
if [ -n "$reports" ]; then
	echo "=== sanitizer reports (unit tests)"
	for file in $reports; do
		echo "--- $(basename "$file")"
		grep -m 5 -E "$REPORT_PATTERN" "$file"
	done
	[ $result -eq 0 ] && result=1
fi

# 5) e2e : admin_server / host_agent / the service built with the sanitizers (SANITIZER_E2E=0 : skip)
#    - the reports of every process (also the service started by host_agent) : <result directory>/e2e/reports/<sanitizer>.<pid>
#    - ports : 21887.. + CGDK_E2E_PORT_OFFSET (default 500)
if [ "${SANITIZER_E2E:-1}" != "0" ]; then
	DIR_E2E="$RESULT_DIR/e2e"
	rm -rf "$DIR_E2E"
	mkdir -p "$DIR_E2E/reports"

	echo "=== e2e ($SANITIZER)"
	(
		export ASAN_OPTIONS="$ASAN_OPTIONS:log_path=$DIR_E2E/reports/asan"
		export UBSAN_OPTIONS="$UBSAN_OPTIONS:log_path=$DIR_E2E/reports/ubsan"
		export TSAN_OPTIONS="$TSAN_OPTIONS:log_path=$DIR_E2E/reports/tsan"
		export TMPDIR="$DIR_E2E"
		export CGDK_E2E_PORT_OFFSET=${CGDK_E2E_PORT_OFFSET:-500}
		bash "$WORK_DIR/build/src/server.admin/unit_test/e2e/run_admin_e2e.sh" Debug
	) > "$DIR_E2E/run_admin_e2e.log" 2>&1
	result_e2e=$?

	grep -E "FAIL|^e2e:" "$DIR_E2E/run_admin_e2e.log" | sed 's/^/    /'
	if [ $result_e2e -ne 0 ]; then
		echo "    e2e failed (exit $result_e2e) : $DIR_E2E/run_admin_e2e.log"
		result=1
	fi

	reports=$(ls "$DIR_E2E/reports/" 2>/dev/null)
	if [ -n "$reports" ]; then
		echo "=== sanitizer reports (e2e)"
		for file in $reports; do
			echo "--- $file"
			grep -m 5 -E "$REPORT_PATTERN|SUMMARY" "$DIR_E2E/reports/$file"
		done
		result=1
	fi
fi

exit $result
