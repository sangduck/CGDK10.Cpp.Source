#!/bin/sh
# CGCII 웹사이트 (HARMATTAN 192.168.0.92). 실행: ./run.sh   / 백그라운드: ./run.sh start  / 중지: ./run.sh stop
# sudo 없이 사용자 권한으로 8080 포트에 정적 서버를 띄운다. 80 포트는 nginx 가 사용 중이므로
# 80 포트의 nginx 는 Docker 컨테이너라 sudo + docker cp 가 필요하다 (cgcii.nginx.conf 참고).
cd "$(dirname "$0")"
PORT=${PORT:-8080}
case "$1" in
start)
    nohup "$0" > cgcii_web.log 2>&1 &
    echo $! > cgcii_web.pid; echo "started pid $(cat cgcii_web.pid) on :$PORT (log: cgcii_web.log)"; exit 0 ;;
stop)
    [ -f cgcii_web.pid ] && kill "$(cat cgcii_web.pid)" 2>/dev/null && rm -f cgcii_web.pid && echo stopped; exit 0 ;;
esac
exec python3 -m http.server "$PORT" --bind 0.0.0.0 --directory .
