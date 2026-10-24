#!/usr/bin/env bash
#-----------------------------------------------------------------------------
# cgdk_control.sh : admin server control api with a request signature (HMAC-SHA256)
#
# usage)
#   cgdk_control.sh <METHOD> <path> [curl options...]
#     ex) cgdk_control.sh GET /api/instances
#         cgdk_control.sh POST /api/instances/21101/start
#         cgdk_control.sh GET /metrics -o /dev/null -w '%{http_code}'
#
# environment)
#   CGDK_CONTROL_URL       : http://127.0.0.1:21888 (default)
#   CGDK_CONTROL_KEY       : "certify_key" of "acceptor_control" (required)
#   CGDK_CONTROL_BODY      : request body (default: empty)
#   CGDK_CONTROL_TIMESTAMP : unix time (default: now. for test only)
#   CGDK_CONTROL_STATUS_ONLY=1 : prints the http status only
#
# signature)
#   X-CGDK-Timestamp : unix time (seconds)
#   X-CGDK-Signature : hex(HMAC-SHA256(key, METHOD + "\n" + path + "\n" + timestamp + "\n" + hex(SHA256(body))))
#   (the key itself is not sent. needs openssl)
#-----------------------------------------------------------------------------
set -u

if [ $# -lt 2 ]; then
	echo "usage: $0 <METHOD> <path> [curl options...]" >&2
	exit 2
fi
if [ -z "${CGDK_CONTROL_KEY:-}" ]; then
	echo "CGDK_CONTROL_KEY is not set" >&2
	exit 2
fi

METHOD="$1"
URL_PATH="$2"
shift 2

URL="${CGDK_CONTROL_URL:-http://127.0.0.1:21888}"
BODY="${CGDK_CONTROL_BODY:-}"
TIMESTAMP="${CGDK_CONTROL_TIMESTAMP:-$(date +%s)}"

BODY_HASH=$(printf '%s' "$BODY" | openssl dgst -sha256 -r | cut -d' ' -f1)
SIGNATURE=$(printf '%s\n%s\n%s\n%s' "$METHOD" "$URL_PATH" "$TIMESTAMP" "$BODY_HASH" | openssl dgst -sha256 -hmac "$CGDK_CONTROL_KEY" -r | cut -d' ' -f1)

BODY_ARGS=()
[ "${CGDK_CONTROL_STATUS_ONLY:-}" = "1" ] && BODY_ARGS+=(-o /dev/null -w '%{http_code}')
[ -n "$BODY" ] && BODY_ARGS+=(--data-binary "$BODY")

exec curl -s -m 10 -X "$METHOD" -H "X-CGDK-Timestamp: $TIMESTAMP" -H "X-CGDK-Signature: $SIGNATURE" "${BODY_ARGS[@]}" "$@" "$URL$URL_PATH"
