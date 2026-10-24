#!/usr/bin/env bash
#-----------------------------------------------------------------------------
# notify_slack.sh : example of "event_command" of the admin server
#
#  - admin server runs : notify_slack.sh <event> <iid> <name> <message>
#    event : host_connected / host_disconnected / service_down / crash_loop / request_failed
#  - posts to a slack incoming webhook (SLACK_WEBHOOK_URL, ex. in the EnvironmentFile of systemd)
#  - setting) "event_command": "/opt/cgdk/tools/notify_slack.sh"
#-----------------------------------------------------------------------------
set -u

EVENT="${1:-}"
IID="${2:-}"
NAME="${3:-}"
MESSAGE="${4:-}"

[ -z "${SLACK_WEBHOOK_URL:-}" ] && exit 0

# - host_connected is not posted (too many)
[ "$EVENT" = "host_connected" ] && exit 0

# - json escape
json_escape() { printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/\t/\\t/g' | tr -d '\r\n'; }

TEXT="[$(hostname)] *$(json_escape "$EVENT")* $(json_escape "$NAME") (iid $(json_escape "$IID")) : $(json_escape "$MESSAGE")"

curl -s -m 10 -X POST -H "Content-Type: application/json" --data "{\"text\":\"$TEXT\"}" "$SLACK_WEBHOOK_URL" > /dev/null
