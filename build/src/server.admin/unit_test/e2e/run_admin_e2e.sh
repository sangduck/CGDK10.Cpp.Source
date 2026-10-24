#!/usr/bin/env bash
#*****************************************************************************
#*                                                                           *
#*                      Cho sanghyun's Game Classes II                       *
#*                       Ver 10.0 / Release 2019.12.11                       *
#*                                                                           *
#*            e2e test : service -> host_agent -> admin_server               *
#*                                 (linux. same scenarios as run_admin_e2e.ps1)
#*                                                                           *
#*****************************************************************************
#
# usage)
#   bash run_admin_e2e.sh [Debug|Release] [test executable]
#
# scenarios)
#   1) basic          : desired 'running' -> service(unit test) initialized -> admin sends START -> RUNNING
#   2) certify_key    : wrong key is rejected, right key is accepted
#   3) host_restart   : host_agent restarts -> admin unregisters and registers again
#   4) crash_loop     : service exits repeatedly -> auto restart is suspended
#   5) control_api    : http control api (key, list, desired, metrics) and admin setting reload
#   6) host_reload    : host_agent reloads host_agent_server_list.json / host_agent_setting.json (web ui policy)
#   7) security       : websocket Origin check, control api auth lockout (429), host status (/api/hosts, /metrics)
#   8) web_login      : host_agent web ui login ("login_key"), session cookie
#   9) admin_restart  : admin_server restarts -> host_agent reconnects and registers again, host status restored
#
# notes)
#   - ports 21887/21888(admin) / 22044/22052/22053(host_agent) must be free.
#     (environment variable CGDK_E2E_PORT_OFFSET is added to all ports. ex) 100 -> 21987/21988/22144/22152/22153)
#   - needs curl.
#   - admin_server/host_agent run in a temporary directory with their own settings.
#   - the service is 'service_admin.e2e_through_host_agent' of unit_test.server_system.service
#     (rank 0.0.10.1 = iid 21101 of host_agent_server_list.json).
#   - exit code: 0 (all passed) / 1 (failed)
#-----------------------------------------------------------------------------
set -u

CONFIGURATION="${1:-Debug}"
TEST_EXE="${2:-}"

#-----------------------------------------------------------------------------
# ports (CGDK_E2E_PORT_OFFSET is added. several runs can be done at the same time on a node)
#-----------------------------------------------------------------------------
PORT_OFFSET="${CGDK_E2E_PORT_OFFSET:-0}"
PORT_ADMIN=$((21887 + PORT_OFFSET))
PORT_CONTROL=$((21888 + PORT_OFFSET))
PORT_HOST=$((22044 + PORT_OFFSET))
PORT_WEB=$((22052 + PORT_OFFSET))
PORT_WEBSOCKET=$((22053 + PORT_OFFSET))

#-----------------------------------------------------------------------------
# paths
#-----------------------------------------------------------------------------
ROOT_BUILD="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"		# build/
DIR_EXECUTE="$ROOT_BUILD/execute"

SUFFIX=".${CONFIGURATION}"		# - linux executables : <name>.Debug / <name>.Release
EXE_HOST_AGENT="$DIR_EXECUTE/CGDK10.server.admin.host_agent_linux$SUFFIX"
EXE_ADMIN="$DIR_EXECUTE/CGDK10.server_admin.admin_server.console/CGDK10.server_admin.admin_server.console_linux$SUFFIX"
[ -z "$TEST_EXE" ] && TEST_EXE="$ROOT_BUILD/src/unit_test/CGDK10.unit_test.server_system.service$SUFFIX"

for file in "$EXE_HOST_AGENT" "$EXE_ADMIN" "$TEST_EXE"; do
	[ -x "$file" ] || { echo "not found: '$file'"; exit 1; }
done
command -v curl > /dev/null || { echo "curl is not found"; exit 1; }

DIR_WORK="${TMPDIR:-/tmp}/cgdk_admin_e2e_${PORT_OFFSET}"
rm -rf "$DIR_WORK"
mkdir -p "$DIR_WORK"

#-----------------------------------------------------------------------------
# helpers
#-----------------------------------------------------------------------------
COUNT_CHECKS=0
COUNT_FAILED=0
PID_ADMIN=""
PID_HOST=""
CONTROL="http://127.0.0.1:${PORT_CONTROL}"

# - new_admin_dir <name> <manager> <acceptor_extra> <desired> [control_extra] [root_extra]
new_admin_dir()
{
	local dir="$DIR_WORK/$1/admin"
	mkdir -p "$dir/config"
	cat > "$dir/config/admin_server.setting.json" <<EOF
{
  "service": {
    "setting": { "name": "admin_server" },
    "logger": { "logger_file": { "path": "log", "filename": "admin_server.log" } }
  },
  "service_instance_manager": { $2 },
  ${6:-}
  "acceptor_control": { "local_endpoint": { "address": "127.0.0.1", "port": ${PORT_CONTROL} }, ${5:-} "prepare_on_start": 2, "must_prepare": 2 },
  "acceptor_host_agent": {
    "local_endpoint": { "port": ${PORT_ADMIN} },
    $3
    "prepare_on_start": 2,
    "must_prepare": 2
  }
}
EOF
	[ -n "$4" ] && printf '%s\n' "$4" > "$dir/config/admin_server.desired_state.json"
	echo "$dir"
}

# - new_host_agent_dir <name> <acceptor_server_extra> [web_extra]
new_host_agent_dir()
{
	local dir="$DIR_WORK/$1/host_agent"
	mkdir -p "$dir"
	cat > "$dir/host_agent_setting.json" <<EOF
{
  "service": {},
  "system_executor": { "thread_count": 2 },
  "socket_to_admin_server": { "remote_endpoint": { "address": "127.0.0.1", "port": ${PORT_ADMIN} } },
  "acceptor_server": { "local_endpoint": { "Port": ${PORT_HOST} }, $2 "prepare_on_start": 2, "must_prepare": 2 },
  "acceptor_web": { "local_endpoint": { "Port": ${PORT_WEB} }, ${3:-} "prepare_on_start": 2, "must_prepare": 2 },
  "acceptor_websocket": { "local_endpoint": { "Port": ${PORT_WEBSOCKET} }, ${4:-} "prepare_on_start": 2, "must_prepare": 2 }
}
EOF
	cat > "$dir/host_agent_server_list.json" <<EOF
{
  "e2e service": {
    "iid": 21101,
    "rank": { "group": 0, "field": 0, "role": 10, "index": 1 },
    "execute_file": "not_exist_e2e_service",
    "path": "./",
    "working_path": "./"
  }
}
EOF
	echo "$dir"
}

# - start_server <exe> <dir> [env...]  (prints pid)
start_server()
{
	local exe="$1" dir="$2"
	shift 2
	( cd "$dir" && exec env "$@" "$exe" < /dev/null > "$dir/stdout.log" 2>&1 ) &
	echo $!
}

stop_server()
{
	local pid="$1"
	[ -z "$pid" ] && return
	kill -TERM "$pid" 2> /dev/null
	for _ in $(seq 1 50); do
		kill -0 "$pid" 2> /dev/null || break
		sleep 0.1
	done
	kill -KILL "$pid" 2> /dev/null
	wait "$pid" 2> /dev/null
}

stop_all()
{
	stop_server "$PID_HOST"; PID_HOST=""
	stop_server "$PID_ADMIN"; PID_ADMIN=""
}
trap stop_all EXIT

# - wait_admin_log <dir> <regex> [timeout] [count]
wait_admin_log()
{
	local dir="$1" pattern="$2" timeout="${3:-15}" count="${4:-1}"
	local deadline=$((SECONDS + timeout))
	while [ $SECONDS -lt $deadline ]; do
		local found
		found=$(cat "$dir"/log/*.log 2> /dev/null | grep -cE "$pattern")
		[ "${found:-0}" -ge "$count" ] && return 0
		sleep 0.2
	done
	return 1
}

# - http_code <method> <url> [key] [host]  -> http status (000: connection failed)
http_code()
{
	local key_args=()
	[ -n "${3:-}" ] && key_args+=(-H "X-CGDK-Key: $3")
	[ -n "${4:-}" ] && key_args+=(-H "Host: $4")
	curl -s -m 5 -X "$1" -o /dev/null -w "%{http_code}" "${key_args[@]}" "$2"
}

# - websocket_code <origin> [cookie] -> http status of the websocket handshake (101: accepted)
websocket_code()
{
	local cookie_args=()
	[ -n "${2:-}" ] && cookie_args=(-H "Cookie: $2")
	curl -s -m 3 -o /dev/null -w "%{http_code}" -H "Connection: Upgrade" -H "Upgrade: websocket" -H "Sec-WebSocket-Version: 13" \
		-H "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==" -H "Origin: $1" "${cookie_args[@]}" "http://127.0.0.1:${PORT_WEBSOCKET}/log"
}

http_body()
{
	local key_args=()
	[ -n "${3:-}" ] && key_args=(-H "X-CGDK-Key: $3")
	curl -s -m 5 -X "$1" "${key_args[@]}" "$2"
}

# - wait_http <method> <url> <key> <code> [regex] [timeout]
wait_http()
{
	local deadline=$((SECONDS + ${6:-15}))
	while [ $SECONDS -lt $deadline ]; do
		if [ "$(http_code "$1" "$2" "$3")" = "$4" ]; then
			[ -z "${5:-}" ] && return 0
			http_body "$1" "$2" "$3" | grep -qE "$5" && return 0
		fi
		sleep 0.5
	done
	return 1
}

invoke_service()
{
	env CGDK_TEST_HOST_AGENT=1 CGDK_TEST_HOST_AGENT_PORT="$PORT_HOST" "$@" "$TEST_EXE" "--gtest_filter=service_admin.e2e_through_host_agent" 2>&1 | grep -q "\[  PASSED  \] 1 test"
}

# - add_result <scenario> <check> <command...>
add_result()
{
	local scenario="$1" check="$2"
	shift 2
	COUNT_CHECKS=$((COUNT_CHECKS + 1))
	if "$@"; then
		echo "  [PASS] $scenario - $check"
	else
		echo "  [FAIL] $scenario - $check"
		COUNT_FAILED=$((COUNT_FAILED + 1))
	fi
}

expect_code() { [ "$(http_code "$1" "$2" "${4:-}" "${5:-}")" = "$3" ]; }
expect_websocket() { [ "$(websocket_code "$1" "${3:-}")" = "$2" ]; }
# - slow_request_closed <port> : sends a part of a request. closed by the server in time -> 0
slow_request_closed()
{
	exec 3<> "/dev/tcp/127.0.0.1/$1" || return 1
	printf 'GET /metrics HTTP/1.1\r\nX-Slow: ' >&3
	local line rc
	read -t 10 -u 3 line
	rc=$?
	exec 3>&- 3<&-
	# - 1 : EOF (closed) / > 128 : timeout
	[ $rc -ne 0 ] && [ $rc -lt 128 ]
}

# - websocket_keepalive_closed <origin> : websocket (ping_interval 1s). the server sends PING, no PONG -> closed in time -> 0
websocket_keepalive_closed()
{
	exec 3<> "/dev/tcp/127.0.0.1/${PORT_WEBSOCKET}" || return 1
	printf 'GET /log HTTP/1.1\r\nHost: 127.0.0.1:%s\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: %s\r\n\r\n' "$PORT_WEBSOCKET" "$1" >&3
	local out="$DIR_WORK/websocket_keepalive.bin" start=$SECONDS rc
	timeout 15 cat <&3 > "$out"
	rc=$?
	exec 3>&- 3<&-
	local elapsed=$((SECONDS - start))
	# - 101 / PING (0x89 0x00) / closed (not the timeout)
	head -c 12 "$out" | grep -q "HTTP/1.1 101" && od -An -tx1 -v "$out" | tr -d '\n' | grep -q " 89 00" && [ $rc -ne 124 ] && [ $elapsed -lt 12 ]
}

expect_text() { [ "$1" = "$2" ]; }
expect_not_text() { [ "$1" != "$2" ]; }
expect_match() { printf '%s' "$1" | grep -qE "$2"; }
expect_not_websocket() { [ "$(websocket_code "$1")" != "$2" ]; }

#-----------------------------------------------------------------------------
# scenarios
#-----------------------------------------------------------------------------
DESIRED_RUNNING='{ "21101": "running" }'
REGISTERED="> HOST\['.*'\] registered"

# 1) basic
echo "1) basic"
DIR_ADMIN=$(new_admin_dir basic '"auto_start": false' '' "$DESIRED_RUNNING")
DIR_HOST=$(new_host_agent_dir basic '')
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN"); sleep 2
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST")
add_result basic "host registered" wait_admin_log "$DIR_ADMIN" "HOST\['.*'\] registered"
add_result basic "service reached RUNNING" invoke_service
add_result basic "START result with request id" wait_admin_log "$DIR_ADMIN" "request #[0-9]+ START result: SUCCESS"
add_result basic "INIT of missing executable fails" wait_admin_log "$DIR_ADMIN" "request #[0-9]+ INIT result: FAIL"
stop_all

# 2) certify key
echo "2) certify_key"
DIR_ADMIN=$(new_admin_dir certify_key '"auto_start": false' '"certify_key": "e2e-key",' '')
DIR_HOST=$(new_host_agent_dir certify_key '')
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN"); sleep 2
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST" CGDK_ADMIN_CERTIFY_KEY=wrong-key)
add_result certify_key "wrong key rejected" wait_admin_log "$DIR_ADMIN" "certify key mismatch"
sleep 1
sed -i 's/"socket_to_admin_server": {/"socket_to_admin_server": { "certify_key": "e2e-key",/' "$DIR_HOST/host_agent_setting.json"
add_result certify_key "right key accepted (setting reload)" wait_admin_log "$DIR_ADMIN" "HOST\['.*'\] registered" 30
stop_all

# 3) host_agent restart
echo "3) host_restart"
DIR_ADMIN=$(new_admin_dir host_restart '"auto_start": false' '' '')
DIR_HOST=$(new_host_agent_dir host_restart '')
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN"); sleep 2
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST")
add_result host_restart "first registration" wait_admin_log "$DIR_ADMIN" "$REGISTERED"
stop_server "$PID_HOST"
add_result host_restart "unregistered on disconnect" wait_admin_log "$DIR_ADMIN" "< HOST\['.*'\] unregistered"
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST")
add_result host_restart "registered again" wait_admin_log "$DIR_ADMIN" "$REGISTERED" 15 2
stop_all

# 4) crash loop
echo "4) crash_loop"
DIR_ADMIN=$(new_admin_dir crash_loop '"auto_start": false, "restart_interval": 0, "restart_max_count": 2, "restart_window": 60' '' "$DESIRED_RUNNING")
DIR_HOST=$(new_host_agent_dir crash_loop '')
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN"); sleep 2
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST")
wait_admin_log "$DIR_ADMIN" "HOST\['.*'\] registered"
for _ in 1 2 3; do invoke_service; sleep 0.5; done
add_result crash_loop "auto restart suspended" wait_admin_log "$DIR_ADMIN" "auto restart is suspended"
stop_all

# 5) control api / admin setting reload
echo "5) control_api"
DIR_ADMIN=$(new_admin_dir control_api '"auto_start": false' '' '' '"certify_key": "ctl-key",')
DIR_HOST=$(new_host_agent_dir control_api '')
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN"); sleep 2
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST")
wait_admin_log "$DIR_ADMIN" "HOST\['.*'\] registered"
add_result control_api "no key -> 401" expect_code GET "$CONTROL/api/instances" 401
add_result control_api "list instances" wait_http GET "$CONTROL/api/instances" ctl-key 200 '"iid":21101'
add_result control_api "get instance (status)" wait_http GET "$CONTROL/api/instances/21101" ctl-key 200 '"status":\{'
add_result control_api "unknown instance -> 404" expect_code GET "$CONTROL/api/instances/999" 404 ctl-key
add_result control_api "set desired" expect_code PUT "$CONTROL/api/instances/21101/desired/stopped" 200 ctl-key
add_result control_api "desired is applied" wait_http GET "$CONTROL/api/instances/21101" ctl-key 200 '"desired":"stopped"'
add_result control_api "wrong method -> 405" expect_code GET "$CONTROL/api/reload" 405 ctl-key
add_result control_api "metrics" wait_http GET "$CONTROL/metrics" ctl-key 200 "cgdk_admin_hosts 1"
head -c $((100 * 1024)) /dev/zero | tr '\0' 'x' > "$DIR_ADMIN/big_body.txt"
CODE_BIG_BODY=$(curl -s -m 5 -o /dev/null -w "%{http_code}" -X POST -H "X-CGDK-Key: ctl-key" --data-binary "@$DIR_ADMIN/big_body.txt" "$CONTROL/api/reload")
add_result control_api "too large body is refused" expect_not_text "$CODE_BIG_BODY" 200
BIG_HEADER=$(head -c $((20 * 1024)) /dev/zero | tr '\0' 'y')
CODE_BIG_HEADER=$(curl -s -m 5 -o /dev/null -w "%{http_code}" -H "X-CGDK-Key: ctl-key" -H "X-Big: $BIG_HEADER" "$CONTROL/api/instances")
add_result control_api "too large header is refused" expect_not_text "$CODE_BIG_HEADER" 200
add_result control_api "server alive after large requests" expect_code GET "$CONTROL/api/instances" 200 ctl-key
TOOL="$ROOT_BUILD/src/server.admin/deploy/tools/cgdk_control.sh"
TIMESTAMP_SIGNED=$(date +%s)
CODE_SIGNED=$(CGDK_CONTROL_KEY=ctl-key CGDK_CONTROL_URL="$CONTROL" CGDK_CONTROL_STATUS_ONLY=1 CGDK_CONTROL_TIMESTAMP=$TIMESTAMP_SIGNED bash "$TOOL" GET /api/instances)
CODE_REPLAY=$(CGDK_CONTROL_KEY=ctl-key CGDK_CONTROL_URL="$CONTROL" CGDK_CONTROL_STATUS_ONLY=1 CGDK_CONTROL_TIMESTAMP=$TIMESTAMP_SIGNED bash "$TOOL" GET /api/instances)
add_result control_api "signed request (hmac)" expect_text "$CODE_SIGNED" 200
add_result control_api "replayed signature rejected" expect_text "$CODE_REPLAY" 401
sed -i 's/ctl-key/ctl-key-2/' "$DIR_ADMIN/config/admin_server.setting.json"
add_result control_api "reload" expect_code POST "$CONTROL/api/reload" 200 ctl-key
add_result control_api "old key rejected after reload" expect_code GET "$CONTROL/metrics" 401 ctl-key
add_result control_api "new key accepted after reload" expect_code GET "$CONTROL/metrics" 200 ctl-key-2
add_result control_api "audit log (json lines)" expect_match "$(cat "$DIR_ADMIN/log/admin_audit.jsonl" 2> /dev/null)" '"source":"control","address":"[^"]+","action":"POST /api/reload","result":200'
stop_all

# 6) host_agent reload (server list / web ui policy)
echo "6) host_reload"
DIR_ADMIN=$(new_admin_dir host_reload '"auto_start": false' '' '')
DIR_HOST=$(new_host_agent_dir host_reload '' '"allowed_addresses": [ "10.255.255.1" ],')
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN"); sleep 2
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST")
wait_admin_log "$DIR_ADMIN" "HOST\['.*'\] registered"
add_result host_reload "web ui rejects not allowed address" expect_code GET "http://127.0.0.1:${PORT_WEB}/" 000
sleep 1
cat > "$DIR_HOST/host_agent_server_list.json" <<EOF
{
  "e2e service": { "iid": 21101, "rank": { "group": 0, "field": 0, "role": 10, "index": 1 }, "execute_file": "not_exist_e2e_service", "path": "./", "working_path": "./" },
  "e2e service 2": { "iid": 21102, "rank": { "group": 0, "field": 0, "role": 10, "index": 2 }, "execute_file": "not_exist_e2e_service", "path": "./", "working_path": "./" }
}
EOF
add_result host_reload "added service is registered to admin" wait_http GET "$CONTROL/api/instances" "" 200 '"iid":21102' 20
sed -i 's/10\.255\.255\.1/127.0.0.1/' "$DIR_HOST/host_agent_setting.json"
add_result host_reload "web ui allowed after reload" wait_http GET "http://127.0.0.1:${PORT_WEB}/" "" 200 "" 20
stop_all

# 7) security (websocket origin / dns rebinding / auth lockout / host status)
echo "7) security"
DIR_ADMIN=$(new_admin_dir security '"auto_start": false' '' '' '"certify_key_file": "config/control.key", "auth_fail_max": 3, "auth_fail_window": 60, "auth_block_time": 60, "request_timeout": 2,')
printf 'sec-key\n' > "$DIR_ADMIN/config/control.key"		# - key file (certify_key_file)
chmod 600 "$DIR_ADMIN/config/control.key"
DIR_HOST=$(new_host_agent_dir security '' '"request_timeout": 2,' '"ping_interval": 1,')
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN"); sleep 2
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST")
wait_admin_log "$DIR_ADMIN" "HOST\['.*'\] registered"
add_result security "websocket same origin accepted" expect_websocket "http://127.0.0.1:${PORT_WEB}" 101
add_result security "websocket other origin rejected" expect_not_websocket "http://evil.example.com" 101
add_result security "web ui rejects dns rebinding host" expect_code GET "http://127.0.0.1:${PORT_WEB}/" 403 "" "evil.example.com:${PORT_WEB}"
add_result security "slow request closed (control)" slow_request_closed "$PORT_CONTROL"
add_result security "slow request closed (web ui)" slow_request_closed "$PORT_WEB"
add_result security "websocket keepalive (ping, no pong -> closed)" websocket_keepalive_closed "http://127.0.0.1:${PORT_WEB}"
add_result security "host status (/api/hosts)" wait_http GET "$CONTROL/api/hosts" sec-key 200 '"status":\{"cpu_usage"' 20
add_result security "host metrics" wait_http GET "$CONTROL/metrics" sec-key 200 'cgdk_host_memory_used_bytes\{'
add_result security "control timeout metrics" wait_http GET "$CONTROL/metrics" sec-key 200 'cgdk_admin_control_timeouts_total\{reason="request"\} [1-9]'
add_result security "host web metrics" wait_http GET "$CONTROL/metrics" sec-key 200 'cgdk_host_web_timeouts_total\{[^}]*acceptor="web",reason="request"\} [1-9]' 20
for _ in 1 2 3; do http_code GET "$CONTROL/metrics" wrong-key > /dev/null; done
add_result security "auth lockout (429)" expect_code GET "$CONTROL/metrics" 429 sec-key
add_result security "auth lockout is logged" wait_admin_log "$DIR_ADMIN" "blocked: too many failures"
stop_all

# 8) web ui login (host_agent "login_key")
echo "8) web_login"
EVENT_DIR="$DIR_WORK/web_login/events"
mkdir -p "$EVENT_DIR"
printf '#!/bin/sh\necho "$@" >> "%s"\n' "$EVENT_DIR/events.txt" > "$EVENT_DIR/event.sh"
chmod +x "$EVENT_DIR/event.sh"
DIR_ADMIN=$(new_admin_dir web_login '"auto_start": false' '' '' '' "\"event_command\": \"$EVENT_DIR/event.sh\",")
DIR_HOST=$(new_host_agent_dir web_login '' '"login_key": "web-key",')
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN"); sleep 2
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST")
wait_admin_log "$DIR_ADMIN" "HOST\['.*'\] registered"
WEB="http://127.0.0.1:${PORT_WEB}"
add_result web_login "login page without session" wait_http GET "$WEB/" "" 200 "LOGIN KEY"
add_result web_login "page needs session (401)" expect_code GET "$WEB/webpage_host_info.htm" 401
add_result web_login "websocket needs session" expect_not_websocket "$WEB" 101
HEADERS=$(curl -s -m 5 -o /dev/null -D - -X POST --data "key=wrong" "$WEB/login")
add_result web_login "wrong key -> error" expect_match "$HEADERS" "Location: /\?error=1"
HEADERS=$(curl -s -m 5 -o /dev/null -D - -X POST --data "key=web-key" "$WEB/login")
COOKIE=$(printf '%s' "$HEADERS" | grep -oE "cgdk_session=[0-9a-f]{64}" | head -1)
add_result web_login "login sets session cookie" expect_match "$COOKIE" "cgdk_session="
PAGE=$(curl -s -m 5 -H "Cookie: $COOKIE" "$WEB/")
add_result web_login "frame page with session" expect_match "$PAGE" "wsUri"
AUDIT_HOST=$(cat "$DIR_HOST/log/host_agent_audit.jsonl" 2> /dev/null)
add_result web_login "host agent audit log (login 401)" expect_match "$AUDIT_HOST" '"action":"login","result":401'
add_result web_login "host agent audit log (login 200)" expect_match "$AUDIT_HOST" '"action":"login","result":200'
add_result web_login "websocket with session" expect_websocket "$WEB" 101 "$COOKIE"
wait_file_match()
{
	local deadline=$((SECONDS + 15))
	while [ $SECONDS -lt $deadline ]; do
		grep -q "$2" "$1" 2> /dev/null && return 0
		sleep 0.2
	done
	return 1
}
add_result web_login "event command (host_connected)" wait_file_match "$EVENT_DIR/events.txt" host_connected
stop_all

# 9) admin_server restart (host_agent reconnects and registers again, the host status is restored)
echo "9) admin_restart"
DIR_ADMIN=$(new_admin_dir admin_restart '"auto_start": false' '' '')
DIR_HOST=$(new_host_agent_dir admin_restart '')
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN"); sleep 2
PID_HOST=$(start_server "$EXE_HOST_AGENT" "$DIR_HOST")
add_result admin_restart "first registration" wait_admin_log "$DIR_ADMIN" "$REGISTERED"
stop_server "$PID_ADMIN"
sleep 3
add_result admin_restart "host agent keeps running" kill -0 "$PID_HOST"
PID_ADMIN=$(start_server "$EXE_ADMIN" "$DIR_ADMIN")
add_result admin_restart "registered again (reconnect)" wait_admin_log "$DIR_ADMIN" "$REGISTERED" 40 2
add_result admin_restart "host status after restart" wait_http GET "$CONTROL/api/hosts" "" 200 '"status":\{"cpu_usage"' 20
stop_all

#-----------------------------------------------------------------------------
# result
#-----------------------------------------------------------------------------
echo ""
echo "e2e: $COUNT_CHECKS checks, $COUNT_FAILED failed (logs: $DIR_WORK)"
[ $COUNT_FAILED -eq 0 ]
