-----------------------------------------------------------------------------
 CGDK10 server admin - deploy examples
-----------------------------------------------------------------------------
 service ──> host_agent(22044) ──> admin_server(21887)
                 └ web ui(22052/22053)       └ control api(http, 21888)

 1) systemd (linux)
    - systemd/cgdk-host-agent.service   : host agent (host마다 1개)
    - systemd/cgdk-admin-server.service : admin server
    - 두 프로그램 모두 foreground로 실행된다. (--daemon을 주지 않는다.)
      systemd stop은 SIGTERM을 보내고 정상 종료(destroy)된다.
    - 인증 키는 EnvironmentFile(/etc/cgdk/*.env)의 CGDK_ADMIN_CERTIFY_KEY로 넘긴다.
      (설정 파일의 "certify_key"가 있으면 그것을 쓴다.)

 2) docker (admin server)
    - docker/Dockerfile.admin_server
    - host agent는 host의 service process를 시작/정지하므로 image로 제공하지 않는다.
    - control api는 기본이 127.0.0.1:21888이므로 container 밖에서는 접속할 수 없다.
      밖에서 쓰려면 "acceptor_control"의 address를 0.0.0.0으로 하고
      "certify_key"나 "allowed_addresses"를 반드시 설정한 후 -p 21888:21888로 연다.

 3) 설정
    - key는 파일로 줄 수 있다. ("<이름>_file" : certify_key_file, login_key_file)
      파일의 앞뒤 공백/줄바꿈은 뺀다. linux에서 다른 사용자가 읽을 수 있으면 경고한다. (chmod 600)
      파일을 읽지 못하면 아무도 인증할 수 없는 key로 동작한다. (key가 없는 것(=검사 안 함)으로 동작하지 않는다.)
      우선순위 : "<이름>_file" > "<이름>" > 환경변수 (certify_key : CGDK_ADMIN_CERTIFY_KEY, login_key : CGDK_HOST_AGENT_WEB_KEY)
    - admin server : config/admin_server.setting.json
        "acceptor_host_agent"      : local_endpoint, certify_key, allowed_addresses
        "acceptor_control"         : local_endpoint, certify_key, allowed_addresses
                                     (없으면 127.0.0.1:21888. key도 주소 목록도 없으면 loopback만 허용한다.)
                                     auth_fail_max(기본 5, 0: 제한 없음), auth_fail_window(초, 60), auth_block_time(초, 300)
                                     : 같은 주소에서 key를 auth_fail_max번 틀리면 auth_block_time 동안 429로 막는다.
                                     require_signature(기본 false) : true면 key 헤더는 받지 않고 HMAC 서명만 받는다.
                                     metrics_require_signature(기본 false) : true면 GET /metrics도 서명만 받는다.
                                     signature_skew(초, 기본 60) : 서명 시각의 허용 차이
                                     request_timeout(초, 기본 10) : 접속 후 이 시간 안에 첫 요청이 끝나지 않으면 끊는다.
                                     idle_timeout(초, 기본 60, 0: 끊지 않음) : 이 시간 동안 받은 것이 없으면 끊는다.
                                     max_connections(기본 64, 0: 제한 없음) : 동시 접속 수 (넘으면 끝날 때까지 받지 않는다.)
        "event_command"            : 사건이 생기면 실행할 명령 (<command> <event> <iid> <name> <message>)
                                     event : host_connected / host_disconnected / service_down / crash_loop / request_failed
                                     예) tools/notify_slack.sh (SLACK_WEBHOOK_URL)
        "audit_log"                : 감사 log 파일 (json lines. 기본 "log/admin_audit.jsonl", ""이면 남기지 않는다.)
                                     control 요청(POST/PUT, 인증 실패/차단)과 console 명령을 남긴다.
        "service_instance_manager" : auto_start, desired_file, restart_interval,
                                     restart_max_count, restart_window, request_timeout
    - host agent   : host_agent_setting.json
        "socket_to_admin_server"   : remote_endpoint, certify_key
        "acceptor_server"          : local_endpoint, certify_key, allowed_addresses
        "acceptor_web"             : local_endpoint, allowed_addresses (web ui. 없으면 loopback만 허용한다.)
                                     login_key : 있으면 web ui/websocket은 login해야 쓸 수 있다. (없으면 환경변수 CGDK_HOST_AGENT_WEB_KEY)
                                     login_session_time(초, 기본 43200) : login session(cookie 'cgdk_session') 유지 시간
                                     login_idle_time(초, 기본 1800) : 이 시간 동안 쓰지 않으면 session을 끊는다. (0: 끊지 않음)
                                     session은 login한 주소에서만 쓸 수 있다. (cookie를 가로채도 다른 주소에서는 쓸 수 없다.)
                                     화면 오른쪽 위 사용자 menu의 'logout' (/logout)
                                     (allowed_addresses로 다른 host에 열 때는 login_key를 반드시 설정한다. login 5번 실패 시 300초 차단)
                                     request_timeout(초, 기본 10), idle_timeout(초, 기본 60), max_connections(기본 64)
                                     : acceptor_control과 같다.
        "acceptor_websocket"       : local_endpoint, allowed_addresses (web ui. 없으면 loopback만 허용한다.)
                                     allowed_origins : [ "http://admin.example.com:22052" ]
                                     (websocket은 Origin을 검사한다. 기본은 같은 host의 web ui(ip 주소/localhost)만 허용.
                                      host 이름으로 접속하려면 그 origin을 넣는다. DNS rebinding 방지)
                                     max_messages_per_second(기본 50), max_messages_burst(기본 100) : 접속마다 메시지 수 제한
                                     (넘으면 끊는다. 0: 제한 없음)
                                     request_timeout(초, 기본 10), idle_timeout(초, 기본 0: 끊지 않음), max_connections(기본 64)
                                     (websocket은 page가 열려 있는 동안 받을 것이 없을 수 있으므로 idle_timeout 기본값이 0이다.)
                                     ping_interval(초, 기본 30, 0: 보내지 않음) : 이 시간 동안 받은 것이 없으면 PING을 보낸다.
                                     두 배의 시간 동안 PONG(받은 것)이 없으면 끊는다. (browser는 PING에 자동으로 답한다.)
        "audit_log"                : 감사 log 파일 (json lines. 기본 "log/host_agent_audit.jsonl", ""이면 남기지 않는다.)
                                     web login(성공/실패/차단), websocket 제어 메시지(SVxx/EVxx), 메시지 제한 초과를 남긴다.
        "service"."logger"         : "logger_file": { "path", "filename" } (log 파일)
      host_agent_server_list.json : 관리할 service 목록
      host_agent_info.json        : "host": { "iid", "name" } (처음 실행할 때 만들어진다.)

 4) 설정 다시 읽기 (재시작하지 않는다.)
    - admin server : console 'reload' 또는 'POST /api/reload'
        접속 정책(acceptor_host_agent/acceptor_control)과 service_instance_manager 설정, desired state를 다시 읽는다.
        (local_endpoint 같은 acceptor 설정은 재시작해야 바뀐다.)
    - host agent   : 파일이 바뀌면 5초 안에 자동으로 다시 읽는다.
        host_agent_server_list.json : 있는 service는 설정을 바꾸고(다음 실행부터 적용), 새 service는 등록한다.
                                      목록에서 빠진 service는 멈춰 있을 때만 지운다. (실행 중이면 남겨둔다.)
                                      등록/삭제가 있으면 admin server와 다시 인증해서 목록을 맞춘다.
        host_agent_setting.json     : 접속 정책(acceptor_server/acceptor_web/acceptor_websocket)을 다시 읽는다.
                                      socket_to_admin_server의 remote_endpoint/certify_key가 바뀌면 다시 접속한다.
        목록에서 빠졌지만 실행 중이던 service는 멈추면 지운다.
    - 이미 접속된 연결은 그대로 둔다. (새 정책은 새 접속부터 적용된다.)

 5) control api (admin server, http. 'X-CGDK-Key: <key>' 또는 'Authorization: Bearer <key>')
      GET  /api/hosts                              : host 목록 (cpu/memory/threads/processes/handles/network)
      GET  /api/instances                          : service 목록
      GET  /api/instances/<iid>                    : service 상세 (status)
      POST /api/instances/<iid>/<request>          : start|stop|init|destroy|kill|pause|continue
      PUT  /api/instances/<iid>/desired/<state>    : running|stopped|none
      POST /api/reload                             : 설정 다시 읽기
      GET  /metrics                                : prometheus text format
    ex) curl -H "X-CGDK-Key: $KEY" http://127.0.0.1:21888/api/instances
        curl -X POST -H "X-CGDK-Key: $KEY" http://127.0.0.1:21888/api/instances/21101/start
    - 서명 (key를 보내지 않는다. 다른 host에서 쓸 때 권장. "require_signature": true)
        X-CGDK-Timestamp : unix time(초)
        X-CGDK-Signature : hex(HMAC-SHA256(key, METHOD + "\n" + path + "\n" + timestamp + "\n" + hex(SHA256(body))))
        같은 서명은 한 번만 받는다. (replay 방지)
        tools/cgdk_control.sh (linux, openssl) / tools/cgdk_control.ps1 (windows)
        ex) CGDK_CONTROL_KEY=$KEY tools/cgdk_control.sh POST /api/instances/21101/start
    - TLS를 쓰지 않는다. 다른 host에서 쓸 때는 내부망/VPN으로 제한하고 "allowed_addresses"를 설정한다.
    - POST/PUT 요청은 admin log에 '(audit) control [주소] METHOD path -> status'로 남는다. (audit_log 파일에도 남는다.)
    - 요청 크기 제한 : header 16KB, body 64KB (넘으면 끊는다. host agent web ui는 header 16KB, body 4KB, websocket frame 64KB)
      (CGDK Nhttp_server::set_http_limits. 기본 header 64KB, body 16MB)
    - websocket 메시지는 나뉘어(fragment) 와도 합쳐서 처리한다. (합친 크기도 websocket frame 제한을 넘을 수 없다.)
    - /metrics : cgdk_admin_*, cgdk_service_*, cgdk_host_* (host_agent가 5초마다 보내는 host 상태)
      cgdk_admin_control_connections / cgdk_admin_control_max_connections : control api 접속 수 / max_connections
      cgdk_admin_control_timeouts_total{reason="request"|"idle"} : 시간 제한(request_timeout/idle_timeout)으로 끊은 수
      cgdk_host_web_connections{host,name,acceptor="web"|"websocket"} : host agent web ui 접속 수
      cgdk_host_web_timeouts_total{host,name,acceptor,reason="request"|"idle"} : host agent web ui를 시간 제한으로 끊은 수
      (host agent가 5초마다 host 상태와 함께 보낸다. /api/hosts의 "web". 이전 host agent는 보내지 않는다.)

 6) monitoring (monitoring/)
    - prometheus.yml               : scrape 예 (Authorization: Bearer <key>)
    - cgdk_alerts.yml              : alert rules (admin down, host 끊김, service 미실행, crash loop, pending request, host cpu)
    - grafana_cgdk_dashboard.json  : grafana dashboard (import)

 7) e2e test
    - windows : unit_test/e2e/run_admin_e2e.ps1
    - linux   : unit_test/e2e/run_admin_e2e.sh [Debug|Release] (curl, openssl 필요. Jenkinsfile의 'Admin E2E Test' stage)
    - Jenkinsfile의 'Windows E2E Test' stage : WINDOWS_NODE를 설정하면 windows node에서 빌드하고 run_admin_e2e.ps1을 실행한다.
    - 환경변수 CGDK_E2E_PORT_OFFSET을 주면 모든 port에 더한다. (같은 node에서 동시에 돌릴 때. Jenkins는 EXECUTOR_NUMBER*100)
