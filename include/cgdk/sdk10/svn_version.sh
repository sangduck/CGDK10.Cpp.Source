#!/usr/bin/env bash

set -e

WORK_DIR="$1"
TEMPLATE_FILE="$2"
OUTPUT_FILE="$3"

if [ $# -ne 3 ]; then
    echo "usage: sub_version.sh <working copy path> <template file> <output file>"
    exit 1
fi

DATE=$(date "+%Y/%m/%d %H:%M:%S")
URL=$(svn info --show-item url "$WORK_DIR" 2>/dev/null || echo "unknown")

# same values as SubWCRev.exe (windows). 'svnversion' reads the working copy only (no server access)
#   - $WCRANGE$  update revision range        ex) "11234:11237" (svnversion)
#   - $WCREV$    highest committed revision   ex) "11237"       (svnversion -c)
#   - $WCMODS$   1 if there are local modifications ('M' of svnversion. unversioned files are not counted)
#   - LC_ALL=C : 'svnversion' prints a localized message for a non working copy (ex. korean locale)
SVNVERSION=$(LC_ALL=C svnversion "$WORK_DIR" 2>/dev/null || true)
SVNVERSION_COMMITTED=$(LC_ALL=C svnversion -c "$WORK_DIR" 2>/dev/null || true)

case "$SVNVERSION" in
    [0-9]*)
        RANGE=$(echo "$SVNVERSION" | tr -d 'MSP')
        REVISION=$(echo "$SVNVERSION_COMMITTED" | tr -d 'MSP' | sed 's/.*://')
        case "$SVNVERSION" in
            *M*) MODIFIED=1 ;;
            *)   MODIFIED=0 ;;
        esac
        ;;
    *)
        # not a working copy ("Unversioned directory", "exported", no svn, ...)
        RANGE="unknown"
        REVISION=0          # $WCREV$ is used as a number
        MODIFIED=0
        ;;
esac

# check) revision must be a number
case "$REVISION" in
    ''|*[!0-9]*) REVISION=0 ;;
esac

sed \
    -e 's|\$WCREV\$|'"${REVISION}"'|g' \
    -e 's|\$WCNOW\$|'"${DATE}"'|g' \
    -e 's|\$WCURL\$|'"${URL}"'|g' \
    -e 's|\$WCRANGE\$|'"${RANGE}"'|g' \
    -e 's|\$WCMODS?1:0\$|'"${MODIFIED}"'|g' \
    -e 's|&amp;|\&|g' \
    "$TEMPLATE_FILE" > "$OUTPUT_FILE"

echo "[ gen] Generated $OUTPUT_FILE"