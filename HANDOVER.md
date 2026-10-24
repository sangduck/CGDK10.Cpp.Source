# Handover — 2026-10-21

## Request and repository
- User requested submission of current changes and saved handover information.
- Working copy: C:\project\CGDK10.C++ (SVN).
- Repository: svn://cgcii.co.kr/ProjectCGDK/CGDK10.C++.public
- This submission includes all three existing tracked modifications plus this document.

## Submitted changes
- build/src/net.update/update/net.update.Nreceiver.cpp
- include/cgdk/sdk10/net.update/update/net.update.Nreceiver.h
  - Receive handlers parse and enqueue work; a serial system-executor worker performs MD5 checks, decompression, file I/O and update processing.
  - Queue byte limit defaults to 32 MiB, with configurable limit and receive-side backpressure.
  - Stop enqueues cancellation and waits for draining (up to 30 seconds); worker callbacks avoid waiting for themselves.
  - Failed executor posting falls back to draining on the calling thread.
  - UPDATE_BEGIN error responses adjust sender/receiver argument order.
- include/cgdk/sdk10/system/object/Iexecutor.h
  - Alignment-only changes to coroutine post/execute declarations.

## Validation and follow-up
- SVN status and diffs inspected. No build or runtime tests were run during this submission task; earlier test results are not available in this chat.
- Follow-up validation: build net.update Debug/Release, run end-to-end update transfer with MD5/decompression, exercise queue backpressure, cancellation, stop from callback, and executor-post failure.
- New unversioned files were not automatically added: this checkout contains many generated binaries, build directories, dependencies, local settings, and older source/document files. They remain local and require separate classification before submission.
- This chat also discussed naming a server-specific independent account/data unit. Candidates included SubAccount, ServerAccount, ServerProfile, Shard, Depot and Context; no final name or code change was selected.

## Finding the submission
- Use `svn log -l 1 HANDOVER.md` to retrieve this document's submission revision.

## Submission integration
- Updated the three modified source files to r11390 before submission.
- Resolved overlapping upstream comment corrections with local receiver restructuring using mine-conflict; non-conflicting upstream edits were retained.


## Handover refresh — 2026-10-22
- Confirmed that all previously tracked code changes were submitted in r11391 (2026-10-22).
- No additional tracked code modifications or conflicts were present at the repeat submission request.
- SVN reports 407 unversioned entries, including build artifacts, dependencies, local settings and pre-existing source/document files. These have not been added or submitted.
- Build/runtime validation remains pending; this refresh only verifies SVN submission state.

## Handover — 2026-10-22 (r11394 : TCP socket option + linux socket 성능 기능)
- r11394 (2026-10-22 15:37) 에 두 세션의 작업이 한 커밋으로 들어갔고 그 로그 메시지가 비어 있다. 서버가 revprop 변경(pre-revprop-change hook)을 허용하지 않아 메시지를 고칠 수 없어, 이 문서의 커밋 로그와 아래에 내용을 적는다.
- TCP socket option (sSOCKET_OPTION_TCP, Isocket::socket_option() / apply_socket_option()) : TCP_NODELAY / TCP_QUICKACK / SO_BUSY_POLL / TCP_NOTSENT_LOWAT / TCP Fast Open.
  - 기본값은 아무것도 켜지 않는다(TCP_NODELAY 만 예전과 같이 true). acceptor "socket_option"(accept 완료 직후 on_accept() 직전 복사) / "tcp_fastopen"(listen socket), connect_requestable·connector "socket_option".
  - 상세 : document/handover/CGDK10.handover.2026-10.md 1.10절, release_note.txt 2026.10.22, 헤더 주석(net.definitions.parameters.h, net.io.connective.acceptor_tcp.h, net.io.connect_requestable.tcp.h).
  - 단위 테스트 build/src/net.socket/unit_test/unit_test.net_socket.http/test_socket_option.cpp, tutorial tutorials/network.1.socket/socket.ex15.socket_option.
  - 검증 : Windows Debug x64 (r11394 이후 system.object/net.socket/crypt/http 재빌드) 단위 테스트 socket_option.* 11개 모두 통과 (Linux 전용 UDS skip 테스트 1개는 제외)
  - tutorial 실행 : Windows Debug 에서 4개 절(START_PARAMETER / context / 코드 / connector) 모두 정상 (listen socket TCP_FASTOPEN=1, 설정한 socket 의 TCP_NODELAY 가 getsockopt 로 확인됨, echo 수신)
  - Linux 는 컴파일·실행 미검증 (epoll 쪽 변경 : TCP_QUICKACK 을 recv 마다 재설정, TFO connect(TCP_FASTOPEN_CONNECT)의 첫 send() EINPROGRESS 를 sendable 5종에서 EWOULDBLOCK 과 같이 처리, UDS skip). SAKPUNG/POKPUNG 에서 test_socket_option 실행 필요.
  - 남은 일 : Windows TFO 클라이언트가 SYN 에 데이터를 실으려면 ConnectEx 에 송신 버퍼를 넘기는 접속+첫 send API 가 필요(미구현). Linux TFO connect 의 거절이 on_disconnect 로 오는 것과 Ntcp_reconnect 재접속 예약의 관계 미확인. epoll_wait 자체의 busy poll(net.core.busy_poll / 6.9+ EPIOCSPARAMS)은 io_context 쪽 과제.
- 같은 커밋의 linux socket 성능 기능(병행 세션) : SOCK_NONBLOCK 생성/accept4, socket buffer 명시 설정 스위치(net::io::socket::Ntcp::enable_explicit_socket_buffer_size, linux 기본은 커널 autotuning), 수신 버퍼 축소 hysteresis, MSG_ZEROCOPY(io_uring 백엔드), UDP recvmmsg/sendmmsg, acceptor "cpu_affinity"(SO_INCOMING_CPU) / "tcp_defer_accept", test_linux_io.cpp, bench_linux_io.cpp. r11393 의 system.object io_uring 백엔드와 함께 본다.
- START_PARAMETER(sSOCKET_ACCEPT_PARAMETER / sSOCKET_CONNECT_REQUEST_PARAMETER) 구조체 크기가 바뀌었다 : 사용하는 쪽(gruma 등) 전체 재빌드 필요.

## Handover refresh - 2026-10-22 (evening, r11402~r11418)
- r11394 의 Linux 미검증 항목 : POKPUNG(g++ 15.2 Debug) 에서 test_socket_option 12개와 net_socket.http 전체 스위트를 epoll / io_uring 으로 통과.
- 인수인계 문서의 버그성 남은 일(1~7, 11, 12)을 모두 처리하고, connector 의 connect/attach 순서·bind(any:0) EADDRINUSE 묻힘, acceptor stop 수명, io_uring 의 -ECANCELED poll 과 SYNC_CANCEL 을 새로 찾아 고쳤다 : document/handover/CGDK10.handover.2026-10.md 1.12절. object pool/factory TLS, 그룹 헤더, acceptor 헤더가 바뀌어 gruma 전체 재빌드 필요.
- epoll_wait busy poll 은 r11429(1.13). io_uring completion 1단계(백엔드) r11437(1.14), 2단계(소켓 op 전환)는 WIP r11441(1.15 : completion 모드 2개 실패 남음), 3단계 남음. 그 밖에 Windows TFO SYN 데이터와 _buffer_unreal.h 의 실제 UE 빌드 확인.

## Handover refresh - 2026-10-23 (r11444 : io_uring completion 2단계 완료)
- 1.15 의 completion 모드 실패 2개(connector_many_connects, io_uring_executor)는 백엔드의 제출 소유 문제였다 : owner 가 아닌 thread 의 enter 가 owner 의 지연 SQE 까지 제출해 짧게 사는 thread 소유가 됐다. r11444 : poll/op 는 ring 의 staging 에 쓰고 owner worker 만 제출(1.16). 소켓 코드 변경 없음.
- 검증 : WSL2(kernel 6.6) net_socket.http 전체 completion 3회 / epoll / readiness io_uring 모두 114 통과, io_context_uring_cq 9개(새 테스트 ops_from_short_lived_threads 포함). POKPUNG(kernel 7.0), ASan 미실행.
- 남은 일 : 3단계(multishot recv + provided buffer), POKPUNG 벤치(epoll 대 completion), Windows TFO SYN 데이터, _buffer_unreal.h UE 빌드.

## Handover refresh - 2026-10-23 (r11451 : io_uring completion, kernel 7.0 에서 드러난 경쟁)
- r11444 는 WSL(8 CPU)에서만 검증됐고 mistral(kernel 7.0, 32 CPU)에서 completion 모드 connector_many_connects 3회 모두 실패 등이 났다. r11451 : owner 가 아닌 thread 는 어떤 ring 에도 enter 하지 않음(MSG_RING wake, poll remove staging, 취소는 SYNC_CANCEL), owner 의 flush+submit 을 한 락 안에서, per handle 배열을 chunk 표로(인수인계 문서 1.17). r11448(다른 세션)의 transient worker / acceptor 대기와 함께 동작.
- 검증 : mistral 과 WSL 에서 io_context_uring_cq 10개 x3, completion 3회, epoll, readiness 전체 스위트(1.17 표). 남은 간헐 실패 : completion 1회차 uds stop 지연 3개(r11448 부터, acceptor 가 전달까지 기다리면 해결될 것으로 분석), zerocopy 통지 1개(readiness 경로).
- 리눅스 검증 장비 : hukpung/monsoon/mistral(ssh alias, 문서 3.2). completion 백엔드 변경은 mistral 전체 스위트 3회를 기본으로. hukpung 은 OpenSSL 1.1 이라 현재 빌드 불가.

## Handover refresh - 2026-10-23 (r11453 : io_uring completion 3단계 multishot recv)
- multishot RECV + provided buffer ring : 코드는 다른 세션의 일괄 커밋 r11453(이 작업 세션의 파일 8개)에 들어갔고, 설명과 검증은 인수인계 문서 1.18 과 이 문서 커밋에 있다.
- 검증 : r11453 을 WSL(kernel 6.6) 과 mistral(kernel 7.0, 32 CPU) 에서 io_context_uring_cq 11개 x3, net_socket.http completion 3회, epoll, readiness io_uring 전체 스위트 모두 통과(WSL 114 / mistral 116, skip 5 / 3).
- 남은 것 : 64KB 대용량은 단일 RECV 와 같거나 느림(복사 + 16KB 버퍼), Release·POKPUNG 벤치, ASan. receivable::Nstream 헤더가 바뀌어 gruma 재빌드 필요.

## Handover refresh - 2026-10-23 (r11459 : io_uring completion 수신 모드 전환)
- 큰 트래픽은 multishot 을 취소하고 단일 RECV(수신 버퍼가 IOCP 규칙으로 1KB -> 8KB -> 64KB -> 메시지 크기), 작은 수신 16번이면 multishot 으로 복귀. 상세는 인수인계 문서 1.19.
- 검증 : WSL 과 mistral(completion x3)에서 emergency.* 기존 실패 외 통과. mistral 의 epoll/readiness 와 Release 벤치는 사용자 요청으로 중지 : 성능 효과는 아직 재지 않았다.
- 그 사이 다른 세션의 4단계 r11457(무복사 수신)이 있다. 작업 사본에는 다른 세션의 미커밋 api.io_context.uring_cq.cpp / acceptor_tcp*.cpp 변경이 남아 있다(이 커밋들에 포함하지 않음).
- 벤치는 앞으로 모두 Release 로 한다(사용자 지시).

## Handover refresh - 2026-10-23 (r11464, r11465 : emergency.* linux 실패 + io_uring post 정체)
- 1.19 에서 기존 실패로 남긴 emergency.* 3개 : 2개는 테스트 전제(linux 는 accept 를 미리 준비하지 않음, executor 가 post 를 배치로 가져감), 1개는 테스트 포트가 linux 임시 포트 범위 안이었다(r11465).
- 그 jam 테스트가 io_uring 두 백엔드의 실제 버그를 드러냈다 : 막히는 executable 하나가 executor 의 모든 post 를 멈춤(readiness : post eventfd 를 배치 실행 뒤에 다시 poll, completion : wake 가 바쁜 ring 이나 자기 ring 에 묶임). r11464, 인수인계 문서 1.20.
- 검증 : mistral net_socket.http 전체 2회 epoll 125 x2 / completion 125 x2 / readiness 125 + 1회차 기존 간헐 실패 2개, windows emergency.* 8/8.
- 남은 일 13 : readiness 의 emergency.manager_signals 간헐 실패(원본도 7/24). 수신 처리 중 모은 송신이 다음 수신 때까지 나가지 않는 증상.

## Handover refresh - 2026-10-23 (r11468 : MSG_ZEROCOPY 통지 경쟁)
- 1.17 에 "조사 필요"로 남은 linux_io.tcp_zerocopy_large_messages_io_uring 간헐 실패(pending 1) : readiness 경로는 send(MSG_ZEROCOPY) 뒤에 소유자를 큐에 넣는데, 그 사이 다른 worker 가 EPOLLERR 로 그 송신의 통지를 먼저 읽으면 큐에 없는 id 라 지나치고 노드가 영영 남았다. 아직 보관 안 된 id 까지 닿는 통지를 기억해 두고 보관 시 바로 놓는다(인수인계 문서 1.21). completion(SEND_ZC)은 해당 없음.
- 검증 : mistral Debug 에서 수정 전 readiness 1900회 중 5회 재현, 수정 후 2500회 pending 실패 0, completion 498/498. 수정 후 첫 500회에 "received 0"(에코 없음) 3회가 몰려 났고 원인 미확인(이후 2000회 재발 없음, 남은 일 13 과 같은 증상일 수 있음).
- test_linux_io.cpp 는 다른 세션(send cork)이 수정 중이라 진단 출력은 넣지 않았다.

## Handover refresh - 2026-10-24 (r11470~r11472 : executor 깨우기·scheduler 락·정지 지연, r11471 : send cork)
- r11470 : `executor::queue_list` 가 몰린 post 를 thread 하나로 처리하던 문제(post 는 queue 가 비어 있다가 찰 때만 깨움 -> 나머지는 1초 timeout 까지 잠듦, odbc querier 의 blocking query 가 하나씩 실행됨). 꺼낸 뒤 남아 있으면 다음 thread 를 깨운다(chain). idle 4 thread 에 4개 : 950ms -> 약 1.1ms. 같은 커밋에 scheduler 가 실행 시각이 된 것을 힙 락 안에서 post(syscall)하던 것을 락 밖으로.
- r11472 : `queue_list_threads::stop()` 이 기다리는 thread 를 깨우지 않아 최대 1초 늦던 문제(`queue_list::cancel_wait()`). **헤더 멤버 추가 : gruma 등 재빌드.**
- r11471(다른 세션) : epoll send cork 나이 확인을 적응형 간격(1~32 송신)으로, `SEND_CORK_DURATION` 20us -> 200us. r11371·r11377(수신 버퍼)과 함께 r11345 이후의 큰 묶음 push 회귀(r11344 대비 -22%)가 회복됐다(SAKPUNG N=320만까지 끊김 0, us/msg 0.040/0.013 vs r11344 0.039/0.010).
- 인수인계 문서 1.3(send cork), 1.22(실행기), 2. 남은 일 14(실행기·IOCP 쪽 남은 후보), 3.1 테스트 표.
- 검증 : Windows execute Release·Debug 61/61, Linux FOEHN execute 61/61 + executor/schedulable x5 + time_event·service·log·server_query. 새 테스트 3개는 모두 수정 전 코드에서 실패 확인.
- gruma 재배포는 아직 안 됨 : Jenkins gruma.BuildServers 마지막 빌드 #44 = CGDK r11351(위 회귀가 들어 있고 그 뒤 변경은 빠짐). #44 와 같은 파라미터(REBUILD, 배포 포함)로 다시 돌리면 된다. Claude 의 Jenkins triggerBuild(배포 포함)는 자동 권한 검사에서 막혀 사용자가 실행해야 한다. 재배포 뒤 MISTRAL·MONSOON 의 gruma 컨테이너도 새 이미지로 재시작할 것(그 재시작 예약 작업은 2026-10-24 에 끔).

## Handover refresh - 2026-10-24 (r11474, r11475 : io_uring 의 EINTR 와 completion post 분배)
- 남은 일 13(readiness emergency.manager_signals)의 진짜 원인 : 응답 지연이 아니라 테스트 클라이언트의 SO_RCVTIMEO recv 가 EINTR. io_uring 은 요청을 제출한 thread(완료 때), ring 을 만들거나 ring 에 제출한 thread(ring 해제 때)에 task work 를 보내 그 thread 의 타임아웃 있는 blocking 호출을 끊는다(POKPUNG kernel 7.0 C 프로브 표 : 인수인계 문서 1.23).
- r11474 : 두 백엔드의 ring 은 곧 끝나는 임시 thread 가 생성. readiness 는 executor worker 만 enter(다른 thread 는 SQ 에 두고 post eventfd 로 깨움, pop 안 함, detach/destroy 는 미제출 SQE 를 NOP 로), completion 은 다른 thread 의 깨우기를 MSG_RING 대신 ring 별 eventfd 로. 새 테스트 linux_io.*requests_of_the_workers_only. mistral readiness emergency.* x60 실패 0(수정 전 4/24).
- r11475 : completion 의 post 깨우기가 바쁜 ring 으로 가면 쉬게 된 worker 가 큐를 가져가지 않던 문제(jam 테스트 간헐 실패, POKPUNG r11473 3/8). owner 가 기다리기 직전 큐 확인. 새 테스트 io_context_uring_cq.post_taken_by_a_worker_going_idle(수정 전 3/3 실패).
- r11459 Release 벤치(POKPUNG) : 수신 모드 전환 on/off 차이 없음. completion 은 epoll 대비 64B x32 -32%, uds -37%, 16B x256 +17%, 64KB +15%, udp +40%(인수인계 문서 2.남은 일 8).
- 남은 것 : completion 의 send_cork 테스트 간헐 실패(mistral 만, 남은 일 16 : 진단 32회 재현 안 됨). MONSOON 루트 fs 읽기 전용(I/O 오류, sudo 필요). 헤더 변경 : gruma 재빌드 필요.

## Handover refresh - 2026-10-24 (r11477 : 남은 일 16 send cork 테스트)
- completion 의 send_cork 테스트 간헐 실패는 cork 결함이 아니었다 : 클라이언트 socket 이 서버 socket 과 같은 ring(같은 worker thread)에 묶여 서버의 2.4ms handler 가 끝난 뒤 실행됐다(mistral 진단 16회 중 3회 실패, 모두 same_ring 1·같은 thread, 서버는 배치 중 정상 송신). 테스트는 서버가 보낸 바이트로 판정하도록 바꾸고 1-thread 변형을 추가(인수인계 문서 1.25).
- 드러난 설계 특성 = 남은 일 17 : completion 은 같은 ring 의 socket 을 owner 하나가 차례로 실행하므로 긴/blocking handler 가 같은 ring 의 다른 socket 을 붙잡는다(extra thread 는 ring 이 없어 대신 못 함). 사용자 문서(socket.io_uring.html)에 적을 것 - 그 파일은 다른 세션의 미커밋 변경이 있어 이번에는 손대지 않았다.

## Handover refresh - 2026-10-24 (r11481 : 남은 일 17 의 블로킹 경우)
- completion 에서 handler 가 scoped_thread_block 안에서 블로킹하면 같은 ring 의 socket 이 모두 멈췄다(200 접속에서 다른 접속 최대 270ms). 블로킹하는 동안 그 ring 을 다른 worker 가 대신 실행한다(STATE_LENT, 막힌 executable 의 결과는 owner 에게 미룸, 배치 나머지는 먼저). 인수인계 문서 1.26.
- CPU 를 오래 쓰는 handler 는 범위 밖(사용자 결정, COOP_TASKRUN 이라 CQE 가 올라오지 않음).
- 검증 : mistral net_socket.http 전체 3 백엔드 x2 131, system.object.execute x2 73, 새 테스트 blocking_handler 는 수정 전 completion 4/4 실패.
- 새 남은 일 18 : completion 에서 한 acceptor 가 받은 socket 이 모두 listen ring(thread 하나)에 묶인다(200/200). 처리량 영향은 벤치로 확인 필요.

## Handover refresh - 2026-10-24 (r11487 : 남은 일 18 accept 된 socket 의 ring)
- completion 에서 accept 된 socket 은 accept handler 의 첫 제출로 listen ring 에 묶였다(한 acceptor 의 접속 전부가 thread 하나). 이제 accept CQE 에서 새 fd 를 owned ring 에 round robin 으로 바인딩한다(`_bind_accepted`, 인수인계 문서 1.27).
- 벤치가 틀려 있었다 : `bench_linux_io` 의 서버 socket 은 system executor(epoll)에서 돌았다(linux acceptor 의 기본 io_context, 남은 일 19). 서버도 case 의 executor 에 올려 재니 수정 전 completion tcp 64B x32 는 epoll 의 1/5(2.48M, 서버 1 thread) -> 수정 후 10.58M. uds 13.9M -> 25.1M, 16B x256 878K -> 1.18M.
- 검증 : mistral Debug net_socket.http 3 백엔드 x2 132, system.object.execute x2 74 통과. 새 테스트 2개는 수정 전 실패 확인.
- 남은 일 19(linux acceptor 가 acceptor 의 io_context 대신 system executor 를 줌 : 동작 변경이라 결정 필요), 20(이전 completion / epoll 비교 수치 재측정, 클라이언트 CONNECT 쪽 몰림 검토).

## Handover refresh - 2026-10-24 (r11489 : 남은 일 19 accept 된 socket 의 io_context)
- linux acceptor 가 accept 한 socket 에 hook -> acceptor 의 io_context -> system executor 순서로 io_context 를 준다(windows·헤더 주석과 같음). 전에는 acceptor 에 executor 를 줘도 서버 socket 이 system executor 에서 돌았다.
- 동작 변경은 acceptor 에 io_context 를 준 linux 서버만 : gruma 는 해당 없음(acceptor 에 io_context 를 주지 않음), tutorial ex15 5번과 executor 를 준 테스트·벤치의 서버 socket 은 이제 그 executor.
- 테스트 test_acceptor_io_context.cpp : 수정 전 `the_executor_of_the_acceptor` 실패 확인. 상세는 인수인계 문서 2. 남은 일 19.

## Handover refresh - 2026-10-24 (r11490 : zerocopy ENOBUFS = 사용자 단위 memlock)
- r11468 때 원인 미확인으로 남긴 "received 0" : MSG_ZEROCOPY / SEND_ZC 가 고정하는 페이지는 사용자 단위 RLIMIT_MEMLOCK(mistral 8MB, 같은 계정의 모든 프로세스가 나눠 씀)에 합산되고, 넘으면 ENOBUFS 로 실패해 CGDK 가 연결을 닫았다. 닫힌 쪽의 RST 가 상대 수신 버퍼까지 버려 메시지가 하나도 완성되지 않음. 다른 세션 테스트와 겹칠 때만 나던 이유. 8개 동시 실행으로 재현(errno 105).
- 수정 : ENOBUFS 면 복사로 다시 보내고 100ms 동안 zerocopy 쉼(readiness 송신 4곳, completion SEND_ZC 재제출). 새 테스트 tcp_zerocopy_falls_back_to_copy_on_enobufs(RLIMIT_MEMLOCK 0). 인수인계 문서 1.28.
- 검증 : mistral 8개 동시 readiness / completion 실패 0, 단독 3모드 150/150, net_socket.http 전체 3모드 135 통과.
- 공유 작업 사본의 송신 cpp 3개와 test_linux_io.cpp 는 다른 세션의 미커밋 수정이 있어 별도 checkout 에서 커밋했다 : 그 세션이 svn update 로 합칠 것.
