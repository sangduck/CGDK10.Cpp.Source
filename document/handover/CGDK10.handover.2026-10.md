# CGDK10.Cpp 인수인계 (2026-10)

2026-10 동안 CGDK10.Cpp 에서 한 작업과, 이어서 작업할 사람이 알아야 할 규칙·결정·남은 일·검증 방법을 정리한 문서다.
세부 변경은 각 리비전의 커밋 메시지에 있고, 여기에는 **코드만 봐서는 알기 어려운 것**을 적는다.

- 저장소: `ProjectCGDK/CGDK10.C++.public` (svn)
- 작성 시점 HEAD: r11387 + 이 문서가 들어간 커밋 (이후 갱신: r11401, r11418 까지 1.12 에 반영, r11472 까지 1.22 에 반영, r11477 까지 1.25 에 반영, r11487 까지 1.27 에 반영, r11505 까지 1.31 에 반영, r11512 까지 1.32 에 반영)

---

## 1. 영역별 작업 요약

### 1.1 net.socket - Linux UDS(같은 host 접속) (r11386, r11387, 이 커밋)

Windows 의 `SIO_LOOPBACK_FAST_PATH` 에 해당하는 Linux 경로.

- **acceptor** `sSOCKET_ACCEPT_PARAMETER::enable_uds` (컨텍스트 `"enable_uds"`)
  - TCP 와 함께 추상 이름 UDS `"\0cgdk.tcp.<실제 listen port>"` 로도 listen 한다. 이름 규칙은 `api::socket::make_uds_address()` 한 곳.
  - listen 주소가 any / loopback 일 때만 UDS 로도 받는다. UDS 실패 시 경고 로그 후 TCP 만.
  - UDS 로 받은 socket 도 같은 `process_alloc_connectable` 클래스, 같은 on_connect / on_message / on_disconnect.
- **acceptor** `sSOCKET_ACCEPT_PARAMETER::uds_only` (컨텍스트 `"uds_only"`)
  - TCP socket 은 bind 만 하고(SO_REUSEADDR 끔 = 포트 독점, 겹치면 기존 포트 재할당 로직) listen 하지 않는다 => TCP 접속은 거절.
  - UDS 실패 시 socket 을 닫고 예외(시작 실패). listen 주소 제한 없음. Windows 는 경고 후 무시(TCP).
- **connect_requestable** (클라이언트)
  - loopback(127.x, ::1) + port 로 접속하면 먼저 UDS 로 접속해 보고, 없으면 TCP.
  - bind_endpoint 를 지정하면(any:0 제외) 시도하지 않는다. 전역 스위치 `connect_requestable::Ntcp::enable_uds_loopback(bool)`. **기본값은 false(TCP)** : UDS 를 쓰려면 클라이언트 프로세스에서 한 번 `enable_uds_loopback(true)` 를 부른다(2026-10-23, 처음 들어갈 때는 true 였음).
  - UDS 로 바꿀 때 **detach / closesocket / address_family / set_epoll_handle 을 모두** 바꿔야 한다(epoll 핸들은 native handle 과 따로 저장됨).
  - `connective::connector`(다중 접속기)도 같은 규칙·같은 스위치로 UDS 를 쓴다(r11406, 1.12).
- **UDS 여부 확인**: `net::sockaddr::is_unix()` (사용자 요구로 유지). `is_loopback()` 은 UDS 도 true, 대입/비교가 AF_UNIX 를 유지, `to_string()` = `"unix"`. 주소 조회는 AF_UNIX 면 family 만 남긴다(경로 미저장).
- 파라미터 구조체 크기가 바뀌었다 => CGDK 를 쓰는 쪽(gruma 등) 전체 재빌드 필요.
- 성능(POKPUNG): UDS 저부하 지연 p50 16.7 -> 9.3us, pipeline +26%, 64KB +17%. 접속(raw 1스레드) TCP 18us vs UDS 3.6us, CGDK 클라이언트 8 worker TCP 2.2만/s vs UDS 17.5만/s. 일반 TCP acceptor 성능 변화 없음(accept 40만/s, pipeline 동일). UDS 가 없는 서버로 loopback 접속하면 UDS 시도 비용 ~2.5us/접속.
- tutorial: `tutorials/network.1.socket/socket.ex14.uds` (이 커밋).

### 1.2 net.socket - epoll 핸들 수명 버그 (r11387, 이 커밋)

접속 벤치에서 발견. **UDS 와 무관하게 TCP 에도 있던 버그**들이다.

1. `process_complete_connecting` 이 on_connect **뒤에** `set_event(get_epoll_handle(), ..., m_pexecutable_socket)` 를 했다. 그 사이 다른 스레드가 close 하면 닫힌 fd 번호를 받은 다른 socket 의 connect executable 을 바꿔치기해 그 socket 이 SYN 에서 영원히 멈췄다.
   => `_process_begin_complete_connect()` / `_process_end_complete_connect()` 로 완료 처리 동안 `m_epoll_event_processing_now` 를 세운다(close 는 EPOLLHUP 만 큐잉하고 fd 를 열어 둠). Ntcp, Ntcp_reconnect 모두.
2. `socket::Ntcp::process_close_socket()` 4.B(SYN) / 4.C(BINDED) 가 fd 를 닫고 `m_epoll_handle` 을 남겨, 소멸자가 재사용된 남의 fd 를 detach + close 했다 => 함께 reset.
3. connect_requestable 이 any:0 도 명시적으로 `bind(0)` 했다. TIME_WAIT 가 많으면 0.8ms/회, 포트 고갈 시 실패 => Linux 는 bind 생략(connect 가 포트 선택).
4. (이 커밋) `acceptor::executable_accept` 가 acceptor 를 raw 포인터로 들고 있어, `stop()` 직후 acceptor 를 해제하면 처리 중인 accept 이벤트가 해제된 acceptor 의 가상 함수를 불렀다(pure virtual call). => `object_ptr` 로 잡고 풀 반환 시(on_final_release) 놓는다. 순환 참조는 stop 때 `m_pexecutable_accept(_uds)` reset + epoll detach 로 끊긴다.

진단법(재발 시): `/proc/<pid>/fdinfo/<epoll fd>` 의 `events: 40000000` = ONESHOT 이 이미 소진됨. 이벤트 조회(`_process_get_executable`)·`detach`·`set_event` 호출부에 임시 `fprintf + backtrace()` (`-rdynamic`) 를 넣어 "닫힌 fd 를 만지는 호출"을 찾는다.

### 1.3 net.socket - 송신 경로 (r11345, r11349, r11352, r11358, r11361, r11362, r11471)

- epoll 송신 모으기(send cork) -> 그룹 락 2단계에서 `send_defer` 로 일원화, 옛 단일 슬롯 API 는 r11362 에서 삭제. `send_cork` 는 `send_defer::begin_cork/end_cork` 만 부른다.
- 송신 큐 한도(r11361, 사용자 결정 (a)): epoll gather/buffered 도 iocp 처럼 "보내는 스레드가 가져간 묶음(EPOLLOUT 대기 잔량 포함)은 한도에 안 셈".
- **성능 회귀(1.11 에서 수정)**: r11345 이후 수신 경로(`net.io.receivable.stream.epoll.cpp` `_process_prepare_receive_buffer`)가 1KB 버퍼를 되감기/memmove 로 재사용하고 8배 확장 규칙이 새로 할당할 때만 동작해 버퍼가 안 커진다 -> recv 당 ~700B. 같은 데이터에 recv 횟수가 수백 배, 메시지당 CPU 서버 2배·클라 9배. Linux 큰 묶음 push 처리량이 r11344 대비 약 -22% (2억/s -> 1.55억/s 포화). Windows IOCP 는 해당 없음. -> 1.11 : 줄이기에 히스테리시스(연속 64회 작은 수신 뒤에만)를 넣어 수정(r11371, r11377). POKPUNG 재측정은 안 했다.
- **송신 쪽 원인(r11471 에서 수정)**: 회귀의 원인은 수신 버퍼만이 아니었다. send cork 가 송신마다 `chrono::tick::now()` 로 가장 오래 모은 송신의 나이를 확인했고(8B echo 서버 perf 14.6%), `SEND_CORK_DURATION` 20us 마다 flush 해 큰 수신 묶음을 처리하며 ~3.6KB 씩 잘게 보냈다. 수신만 고치면 N=320만에서 오히려 나빠졌다(echo 가 빨라진 만큼 클라가 잘게 받음).
  - r11471 : 나이 확인 간격을 적응형으로(cork 시작 때 1, 확인마다 걸린 시간으로 다음 간격을 정함, 최대 `SEND_CORK_CHECK_MAX` 32 송신), `SEND_CORK_DURATION` 20us -> 200us(epoll 블록만, Windows 무관). 50us 는 큰 묶음에서 클라 CPU 3배·N=320만 끊김이라 기각.
  - 상한 의미 : "가장 오래된 송신이 200us 를 넘기면 다음 송신 때 보낸다"는 송신 간격이 고를 때 그대로다. 빠르던 송신이 갑자기 느려지면 그 한 간격(최대 32 송신)만큼 늦을 수 있다.
  - 큰 묶음 push(SAKPUNG, tcp_echo 100소켓·8B, us/msg 서버/클라) : r11461 0.067/0.069(N=240만 27~39, 320만 72~106 소켓 끊김) -> r11471 0.039~0.041/0.013~0.014(N=320만까지 끊김 0, 3.2억/s). 기준선 r11344 0.039/0.010. 지연(POKPUNG ping-pong·pipeline)은 같거나 좋아짐(message 마다 2us 일하는 pipeline +9%). 상세 표는 r11471 커밋 메시지.
  - 테스트 : `linux_io.send_cork_sends_old_responses_{gather,buffered}`(test_linux_io.cpp) - 나이 flush 를 끈 빌드에서 실패 확인.

### 1.4 server.system.group - 그룹 락 개선 1~3단계 (r11343~r11360)

- **락 순서 규칙**: 멤버 락(`Igroupable::m_lockable_group`, 둘 이상이면 std::lock) -> 입장 락(`m_lockable_enter`) -> 그룹 락(`Igroup::m_lockable_group`). 그룹 락 안에서 다른 멤버 락을 잡지 말 것.
- leave_all / sender 는 락 안 스냅샷, 락 밖 처리. `member->leave_group()` 대신 `Igroup<base>::leave(member, param)` (스냅샷 뒤 다른 그룹으로 옮긴 멤버를 빼지 않도록).
- array_seat 좌석은 0번부터(사용자 결정).
- 2단계(r11352): 그룹·멤버 락 타입이 `group_lock_object`(recursive_mutex 파생) - 가장 바깥 unlock 뒤 `send_defer` flush. 락 안에서 응답을 기다리는 동기 쿼리는 `scoped_thread_block` 또는 `send_defer::flush()` 필요. `std::unique_lock<lock_object>` 처럼 타입을 박지 말고 CTAD/decltype.
- 3단계: list / Nmap / array / observable / container 에 **lazy** copy-on-write 멤버 스냅샷(`server.group.member_snapshot.h`). 입장/퇴장 구간 **시작**에서 게시본을 버리고, 읽는 쪽이 없으면 다시 만든다. eager COW 는 2만 명 그룹에서 코어 0.5~2.6개라 금지.
- 남은 것: 그룹 락 안 사용자 코드(for_each_member / 콜백)가 다른 멤버 락을 잡으면 여전히 교착 가능, `array::member(nullptr, seat)` null 역참조, container map/set 의 일부 시그니처·raw 포인터 콜백 UAF 가능성.
- 테스트: `unit_test.server_system.service/test_group_leave_all.cpp`, `test_group_send_defer*.cpp`.

### 1.5 system.object

- object pool(r11336, r11348): TLS<->중앙 stack 을 최대 32개 chain 단위로 이동(교차 스레드 alloc/free 73.5 -> 18.3ns). `LOCKFREE_SELF_STACK` 을 다른 구현으로 바꾸지 말 것(배치 API 의존). 16-shard 방식은 기각(`sharded_self_stack.h` 는 include 되지 않음). 종료 후 반환 UAF 수정(r11348). 남은 것: close 뒤 TLS 스택 잔여 객체 누수.
- object_ptr final release 함정: 해제 중인 객체의 raw 포인터로 object_ptr 을 만들면 0->1->0 이중 해제. `std::set<object_ptr<T>>::erase(raw)` 도 해당(r11346 에서 재접속 관리자 수정).
- io_context(epoll, r11341): 적응형 event 배치, fd generation 태그(닫힌 handle 의 이벤트를 새 등록에 전달하지 않음).
- circular_list 이동 대입(r11335): 이제 원본을 비운다(예전엔 복사). `erase` 는 노드를 예비로 남기므로 락 밖에서 begin()/end() 를 읽지 말 것(r11339).

### 1.6 log (r11332, r11338 외)

- logger_hub_archive 큐는 lock-free MPSC, 자식은 `Nlogger_async` 자체 노드 큐. children 스냅샷은 lock-free load + 옛 스냅샷 보존.
- `Ilogger::set_filter()` 는 옛 필터를 logger 수명 동안 모두 보존(reader 보호 없이 해제하면 UAF, r11338). 해제 로직을 다시 넣지 말 것.
- logger_file 은 파일을 직접 쓴다(사용자 결정). 종료 시에만 무손실 보장(`reset_logger()` 가 hub 를 destroy).
- logger_slack 은 `server.log.slack.h` 를 직접 include 해야 등록된다(http/OpenSSL 링크 분리).

### 1.7 server.query

- ODBC querier(r11329, r11330): 큐에 들어간 요청의 결과는 정확히 한 번(실행 / `on_cancel` / 예외). querier 밖 코드는 `m_lockable_query` 밖에서 실행. 통계 게이지(processing/using/exist)는 `reset_statistics()` 가 지우지 않는다(의도된 동작 변경).
- redis querier(r11335, r11346): 공유/전용 세션 소유, 대기열 FIFO(공유 동기 대기 100ms 후 공유), 세션 참조 기반 통계, push+send 를 세션 큐 락 안에서(응답 뒤섞임 방지), stop/unregister 정리 순서. 규칙은 코드 주석에도 있다.

### 1.8 server.service.doss (r11333, r11337, r11342)

- 트랜잭션은 단일 코루틴(`co_request_transaction`). 헤드리스 gtest `unit_test.server_service.doss`.
- `session::destroy()` UAF 수정: 분리 코루틴이 self 참조를 완료까지 유지. entity id 는 `make_entity_id(prepository->get_repository_id(), ...)` (하드코딩 금지).

### 1.9 정리 작업 (r11372~r11385)

- 불필요한 지역 복사 제거, 흐름 제어 매크로(`RETURN_IF` 등)를 sendable 파일에서 if 문으로 풀어 씀, `if(x, false)` 형태 오용 수정.
- `CGASSERT(size != 0, false)` -> `CGASSERT_FATAL(...)`, data==nullptr 은 false 반환으로 통일.
- 영어 오타(주석·문자열 320파일, 파일 이름 svn mv, 식별자: sended->sent, occure->occurred, dump_manager API 이름 + deprecated 별칭), 한국어 주석 맞춤법(361파일). gruma 쪽 이름 변경 반영은 gruma r1193/r1194.

---

### 1.10 net.socket - TCP socket option (이 커밋)

`sSOCKET_OPTION_TCP`(net.definitions.parameters.h)를 모든 socket 이 `Isocket::socket_option()` 으로 가지며, connect/accept 완료 시 `Isocket::apply_socket_option()`(connectable::Ntcp::process_complete_connect)이 handle 에 적용한다. UDS 는 건너뛴다. **기본값은 아무것도 켜지 않는다**(TCP_NODELAY 만 예전처럼 true. `connectable::Ntcp::m_enable_tcp_nodelay` 는 `socket_option().tcp_nodelay` 로 대체).

- 필드: `tcp_nodelay`, `tcp_quickack`, `busy_poll`(us), `tcp_notsent_lowat`(bytes), `tcp_fastopen`(접속 socket 용).
- **acceptor**: `"socket_option"`(accept 완료 직후, on_accept() 직전에 socket 에 복사. `flag_socket_option` 이 있을 때만 => 없으면 코드로 정한 socket 의 값을 덮지 않는다. **1.11 이후** : 할당 직후 socket 의 옵션은 기본값으로 되돌려지므로 '코드로 정한 값' 은 `on_request_accept()`/`on_alloc_connectable()` 에서 정한 것만 뜻한다. 풀 재사용 socket 이 옛 옵션을 가지던 문제), `"tcp_fastopen"`(listen socket. true 면 `DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH`=256, 숫자면 그 값). 코드: `pacceptor->socket_option(option)`(이후 accept 되는 socket 에 **on_accept() 직전** 복사. alloc 시점이 아닌 이유: Windows 는 accept socket 을 prepare_on_start 만큼 미리 걸어 두므로 alloc 복사면 그 socket 들이 옛 값을 유지), `on_accept()` 에서 `_pconnectable->socket_option()` 으로 socket 별 변경. `on_request_accept()` 는 acceptor 에 socket_option 이 없을 때만 유효.
  - Linux 서버 TFO 는 `sysctl net.ipv4.tcp_fastopen` 에 2 비트가 필요. 없으면 시작 로그에 경고(`api::socket::get_sysctl_tcp_fastopen()`).
- **connect_requestable / connector**: `"socket_option"`(connector 는 start 컨텍스트 = 할당하는 socket 의 기본값, 접속 컨텍스트 = 그 socket). `tcp_fastopen` 은 `socket_option` 안. START_PARAMETER 는 `socket_option` + `flag_socket_option = 1`(0 이면 socket 자신의 값 유지).
  - Windows: ConnectEx 전에 `TCP_FASTOPEN`. ConnectEx 에 데이터를 넘기지 않으므로 cookie 요청/캐시만 되고 SYN 에 데이터는 안 실린다(접속+첫 send API 가 있어야 함).
  - Linux: `TCP_FASTOPEN_CONNECT`. connect() 가 즉시 0, 첫 send() 가 SYN 을 보낸다. cookie 가 없으면 첫 send() 가 `EINPROGRESS` => epoll sendable 5종(10곳)에서 `EWOULDBLOCK` 과 같이 EPOLLOUT 대기. on_connect() 가 handshake 전에 불리므로 그 안의 `remote_endpoint()` 는 실패하고, 거절된 접속은 on_fail_connect 가 아니라 on_disconnect 로 온다(Ntcp_reconnect 의 재접속 경로와 맞물리는지는 미확인).
- Windows 의 `tcp_quickack` 은 `SIO_TCP_SET_ACK_FREQUENCY = 1` 한 번(영구). Linux 의 `TCP_QUICKACK` 은 영구가 아니라 receivable 이 recv 마다 다시 건다(stream.epoll.cpp). `busy_poll` / `tcp_notsent_lowat` 은 Windows 에서 파서가 경고 후 무시.
- `busy_poll` 은 socket 별 값이라 epoll_wait 자체의 busy poll 과 무관(그건 `net.core.busy_poll` 또는 6.9+ `EPIOCSPARAMS`, io_context 쪽 과제). nonblocking recv 가 NIC 큐를 한 번 폴링하는 효과. 올리려면 CAP_NET_ADMIN(EPERM 은 한 번만 경고).
- 파라미터 구조체 크기가 바뀌었다 => 사용하는 쪽 전체 재빌드.
- 검증: Windows Debug x64 단위 테스트(`test_socket_option.cpp`) 통과. Linux(POKPUNG, g++ 15.2 Debug)도 12개 모두 통과(2026-10-22, epoll / io_uring. net_socket.http 전체 스위트 108 통과).
- tutorial: `tutorials/network.1.socket/socket.ex15.socket_option`.

### 1.11 net.socket / system.object - Linux 소켓 성능 요소 (r11393~r11398, socket_option 풀 재사용 수정 커밋)

변경 전 트리(r11392)와 비교해 Linux 소켓 경로에 빠져 있던 성능 요소를 넣고, 그 과정에서 찾은 버그를 고쳤다. 모두 Linux 전용(`_SYS_EPOLL_H`)이며 Windows 는 동작이 바뀌지 않는다. **리비전 주의**: r11393(io_uring 백엔드 + CPU 고정 executor)만 설명이 붙은 커밋이다. r11394, r11395, r11396(모두 메시지 없음, 약 18분 간격)은 다른 세션이 공유 작업 사본 전체를 한 번에 올린 것이라 이 절의 net.socket/UDP/테스트/io_uring 수정(중간 상태 포함)과 1.10 의 TCP socket option 작업이 섞여 들어갔고, r11395 는 `api.io_context.Nepoll.cpp` 의 `eRESULT::FAILURE`(없는 멤버) 때문에 빌드가 깨진 상태였다(r11396 에 수정이 섞여 들어감). r11397(접속 중 close 경쟁 마무리)만 설명이 붙어 있다. 변경 내용별 설명은 이 절이 유일하다.

- **수신 버퍼 회귀(1.3 의 미해결 항목) 수정** `net.io.receivable.stream.epoll.cpp`: 작은 수신 1번에 최소 크기로 줄이고 다음 폭주에 8배로 늘리던 요동(이벤트마다 재할당, recv 당 ~700B)이 원인. 줄이기는 연속 `COUNT_SMALL_RECEIVES_TO_SHRINK`(64)회 작은 수신 뒤에만(히스테리시스). 창을 채운 수신이나 큰 수신이 카운트를 리셋.
- **접속당 syscall 정리**: `socket()/accept4()` 에 `SOCK_NONBLOCK`(fcntl 2회 제거, `set_nonblock` 은 ioctl 1회), Linux 에서 `SO_LINGER(0)` 생략(기본값), `SO_SNDBUF/SO_RCVBUF` 는 Linux 기본 미설정(커널 autotuning 유지. `net::io::socket::Ntcp::enable_explicit_socket_buffer_size(true)` 로 이전 동작. Windows 는 true). acceptor `tcp_defer_accept`(초, 컨텍스트 `"tcp_defer_accept"`, 기본 0 : 서버가 먼저 보내는 프로토콜은 끈 채로).
- **UDP**: `recvmmsg`(배치 적응 1~32, 데이터그램마다 풀 버퍼 1개) / `sendmmsg`(다른 thread 송신·send_defer·소켓 버퍼 가득 대기분을 최대 64개씩. EWOULDBLOCK 이면 EPOLLOUT 을 기다려 이어 보냄 : 이전엔 버림). **Linux UDP 는 수신 자체가 안 되고 있었다**: `executable_socket` 에 socket 이 연결되지 않아(`set_socket` 미호출, TCP 는 `process_complete_connect` 에서 함) EPOLLIN 이 와도 `process_execute_epoll` 이 불리지 않았다(r11392 에서 echo 테스트로 재현, server received 0). `Ndatagram::prepare_receive()` 에서 연결, close 때 해제, 이벤트마다 `add_ref` 하던 미반환 참조 제거.
- **MSG_ZEROCOPY** (`net.io.sendable.Itcp_gather` : `Ntcp_async_gather`(기본 sendable)·`Ntcp_async_gather_buffered` 공통): **io_uring 백엔드의 io_context 에서만**(사용자 결정. epoll 에서는 통지 1건당 epoll_wait/epoll_ctl 이 더 들어 손해). 단일 버퍼 >= `zerocopy_threshold()`(16KB) 송신은 페이지 고정, 소유자는 커널 통지(error queue, EPOLLERR)까지 보관. `socket::Ntcp::process_execute_epoll` 이 `Isendable::process_complete_error_queue()` 를 먼저 불러 통지만이면 EPOLLERR 를 지운다. AF_INET/AF_INET6 만(UDS 제외). `enable_zerocopy(false)` 로 끔. loopback 에서는 커널이 복사하고 "copied" 로 통지한다(이득 없음, 동작 확인용).
- **acceptor CPU 지역성** `enable_cpu_affinity`(컨텍스트 `"cpu_affinity"`): accept 된 socket 을 `SO_INCOMING_CPU` 의 CPU 에 고정된 io_context(CPU 당 1 thread, 요청 시 생성, acceptor 들이 공유, `system_executor` destroy handler 로 정지)에 배정. `process_get_connectable_io_context()` 기본 구현이 담당(emergency acceptor 의 override 는 그대로). `executor::io_context_threads::START_PARAMETER::cpu_affinity`, `make_io_context_threads(name, count, cpu)`, `api::set_current_thread_affinity()`.
- **접속 중 close 경쟁 수정** `net.io.connect_requestable.tcp.cpp`: 다른 thread 가 접속 완료(EPOLLOUT)를 처리하는 중에 SYN 소켓을 닫으면(`process_close_socket` 4.B -> `process_complete_connecting(EPOLLHUP)`) 가드(`m_result_connecting == PROCESS`)에서 그냥 돌아가 아무도 disconnect 를 끝내지 않았고 `wait_complete_closed()` 가 30초 timeout 까지 기다렸다. 이제 그 close 는 EPOLLHUP 으로 큐에 넣고 처리 중인 thread 의 `_process_end_complete_connect()` 가 disconnect 를 완료한다. 처리 플래그는 가드 안에서 함께 세운다(`_process_begin_complete_connect` 제거. `Ntcp_reconnect::process_complete_connecting` 도 같은 가드). epoll 에서도 이론상 같은 경쟁인데(2번 남은 일의 fd 재사용 창과 같은 부류), io_uring 은 접속 완료 CQE 가 수 us 안에 와서 `linux_io.io_uring_executor`(접속 직후 close 200회)가 1.4초~250초로 흔들리며 드러났다. 이 테스트의 소요 시간이 지연 버그의 지표다 : 여러 번 돌려 1~2초가 아니면 의심할 것.
- **io_uring 백엔드** `api::io_context_uring`(system.object): epoll 과 같은 준비 이벤트 모델(`IORING_OP_POLL_ADD`)로 소켓 코드는 그대로. `api::io_context_linux` 가 create() 때 `executor::default_io_backend()`(`set_default_io_backend()`, env `CGDK_IO_URING=1`)에 따라 epoll/io_uring 을 고르고, 불가(5.13 미만, io_uring 비활성)면 epoll 로 fallback. raw syscall(liburing 불필요).
  - **모든 poll 은 1회성**(POLL_ADD). EPOLLONESHOT 등록(TCP)은 소켓 코드의 set_event 가 재무장(poll 이 살아 있으면 POLL_REMOVE + POLL_ADD, 끝났으면 POLL_ADD. `IORING_POLL_UPDATE_EVENTS` 는 6.6 커널이 multishot 과 함께 EINVAL). EPOLLONESHOT 없는 등록(UDP, listen, post eventfd)은 **그 CQE 를 꺼낸 thread 가 이벤트 처리를 끝낸 뒤 `flush_pending()` 에서 같은 마스크로 다시 POLL_ADD**(등록 NODE 에 마스크 보관). EPOLLET 과 같은 동작 : 처리 중 들어온 것은 drain 으로 소비되고 이후 도착이 CQE 하나를 만든다.
    - 처음엔 multishot(`IORING_POLL_ADD_MULTI`)이었는데 깨어날 때마다 CQE 가 하나씩 쌓여(합쳐지지 않음) UDP 가 epoll 의 1/13 이었다. 다음엔 CQE 를 꺼낼 때 바로 재무장 SQE 를 썼는데 SQ 는 공유라 다른 thread 의 submit 에 끌려 들어가 아직 drain 중인 소켓의 poll 이 즉시 다시 완료되어 두 thread 가 같은 UDP 소켓을 동시에 처리했다(데이터그램 유실, post eventfd 정지). 그래서 처리 후로 옮겼다.
  - worker 가 처리 중 낸 재무장 SQE 는 다음 `pop_events()`/`flush_pending()` 에서 한 번의 `io_uring_enter` 로 묶어 제출(epoll 은 이벤트마다 epoll_ctl). 다른 thread 의 SQE 는 즉시 제출.
  - CQ 는 한 thread 만 커널에서 대기하고(`lockable_cq`, timed_mutex) 나머지는 **자기 대기 시간만큼만** lock 을 기다린다. 처음엔 plain mutex 였는데 커널 대기 thread 가 1초(executor 의 대기) 동안 lock 을 잡고 있어 `wait_complete_connecting`/`wait_complete_closed` 처럼 `execute(2ms)` 로 executor 를 펌핑하는 thread 가 worker 들과 mutex 를 경쟁하며 수 초씩 굶었다(접속 직후 `stop()` 200회가 1.4초~250초). 못 잡으면 빈손으로 돌아오고 이벤트는 커널에 있는 thread 가 처리한다. **일반 원칙** : epoll 은 모든 thread 가 동시에 epoll_wait 할 수 있지만 io_uring 의 CQ 는 한 ring 에 대기자 하나가 정상이다. thread 마다 ring 을 두는 구조가 io_uring 의 본래 모델이고, 지금 백엔드는 epoll 과 같은 '공유 ring + 다중 thread' 모델을 억지로 맞춘 것이라 이런 종류의 지연이 더 있을 수 있다.
  - 수명: `RING_GUARD` 가 ring 사용 thread 를 세고 destroy() 는 handle 무효화 -> nop 으로 대기자 깨움 -> 사용자 0 까지 대기 -> munmap. 디스패처는 destroy 뒤에도 백엔드 객체를 들고 있다(정지 중 socket close 가 set_event/detach 를 부른다 : 이게 unique_ptr reset 과 겹쳐 SIGSEGV 였음).
  - 미구현: completion 모드(IORING_OP_RECV/SEND, provided buffer). IOCP 처럼 송수신 계층을 다시 써야 하는 별도 작업. 기대 이득은 syscall 수 자체의 제거.
  - **요청은 제출한 thread 소유** (r11417): io_uring 은 SQE 를 제출한 thread 가 끝나면 그 poll 을 `-ECANCELED` 로 끝낸다. 이 백엔드는 아무 thread 나 남의 미제출 SQE 까지 한꺼번에 제출하므로 임시 std::thread 가 소유자가 되기 쉽다. 전에는 `pop_events` 가 `-ECANCELED` 를 버려 그 등록(system executor 의 post eventfd 포함)이 영원히 이벤트를 못 받았다 => 그 프로세스에서 post 가 더는 실행되지 않아 재접속 관리자·접속 정리가 멈췄다. 이제 poll 마다 시퀀스(user_data 24~30비트)를 두고, 지금 걸린 poll 이 커널에 의해 취소되면 CQE 를 꺼낸 thread 가 다시 건다(modify/detach 가 직접 취소한 것은 버림). 옛 CQE 가 새 poll 의 armed 표시를 지우던 문제도 같은 시퀀스로 막는다.
  - **poll 이 파일을 잡는다**: `close(fd)` 해도 그 fd 의 poll 이 끝날 때까지 소켓이 실제로 닫히지 않는다(FIN 없음, listen·UDS 이름 유지). detach 는 미제출 SQE 를 바로 제출한 뒤 `IORING_REGISTER_SYNC_CANCEL`(6.0+) 을 ENOENT 까지 반복한다(EINVAL 이면 옛 POLL_REMOVE). `_submit_poll_add` 는 `lockable_sq` 안에서 registry 가 같은 등록인지 확인한 뒤에만 건다(detach 뒤 re-arm 경쟁).
- 테스트: `test_linux_io.cpp`(11개, CMake 만. `CGDK_IO_URING=1` 로 전체 스위트를 io_uring 으로도 돌린다), 벤치 `bench_linux_io.cpp`(`CGDK_BENCH=<초>`). 두 파일은 r11392 트리에서도 빌드되어(io_uring 부분은 `__has_include`) 기준선 비교에 쓴다. UDP echo 테스트는 창(64B 는 64개, 8000B 는 8개) 단위로 보낸다 : 한 번에 다 보내면 `rmem_default`(212KB) 를 넘는 분량을 커널이 버려 느린 소비자에서 간헐적으로 실패했다(UDP 정상 동작).
- 벤치(WSL2 Ubuntu 24.04, kernel 6.6, 8 vCPU, gcc 13 Debug, 전용 executor 4 thread, 같은 프로세스 안의 echo ping-pong, 3초 x 교대 5라운드 중앙값. WSL2 는 라운드마다 ±30% 흔들리니 ±30% 안은 같은 값으로 볼 것): 아래 표.

| 케이스 (echo ping-pong, 3초 x 5라운드 중앙값) | r11392 epoll | 변경 후 epoll | 변경 후 io_uring |
|---|---|---|---|
| tcp 64B x32 clients, window 16 | 2,642,590 msg/s | 2,710,414 msg/s (+3%) | 1,704,861 msg/s (-36%) |
| tcp 64KB x4 clients, window 2 | 64,406 msg/s (4,025 MB/s) | 54,083 msg/s (3,380 MB/s. 라운드 최소 33K : 노이즈, 앞선 두 벤치는 63K/66K) | 32,654 msg/s (2,041 MB/s) |
| udp 64B x16 sockets, window 32 | 0 (수신 불가 버그) | 577,105 msg/s | 452,985 msg/s (multishot poll 때 50,119) |
| uds 64B x32 clients, window 16 | 2,682,470 msg/s | 2,639,527 msg/s | 1,833,855 msg/s |

  - 해석 : **epoll 경로는 노이즈 안에서 변화 없음**. 접속당 syscall 절감·버퍼 autotuning·히스테리시스는 loopback ping-pong 처리량에는 보이지 않는다(커널 복사가 지배). 수신 버퍼 회귀는 큰 묶음 push 패턴(1.3 의 -22%)에서 재야 하는데 POKPUNG 에서 못 했다.
  - **io_uring(준비 모드)은 WSL2 loopback 에서 epoll 보다 느리다**(TCP -36%, UDP -22%). 공유 ring 에 CQ 대기자 하나(나머지는 lock 대기)인 구조 비용으로 본다. syscall 수 자체는 epoll 과 같다(이벤트당 poll 재무장 1건). 이득이 나려면 completion 모드(IORING_OP_RECV/SEND, provided buffer) 또는 thread 당 ring 이 필요하다(2. 남은 일).
  - **UDP 는 r11392 에서 Linux 수신이 전혀 안 됐다**(0). 지금 epoll 577K msg/s. io_uring 은 multishot poll 을 걷어내 50K -> 453K.
  - MSG_ZEROCOPY 는 loopback 에서 커널이 복사하고 "copied" 로 통지하므로 이 표에 이득이 없다(동작 확인용). 실 NIC 에서 64KB 이상 송신으로 다시 잴 것.

- **실 장비 검증·벤치(2026-10-23, HEAD r11445, mistral : Ubuntu 26.04, kernel 7.0, 32 CPU, gcc 15.2)**: Debug 전체 스위트 119개가 epoll 과 io_uring(`CGDK_IO_URING=1`) 모두 116 통과·3 skip·실패 0. `linux_io.io_uring_executor` 5회 1.39~1.44초, `socket_option.*` 3회 통과. Windows x64 Debug 단위 테스트 75개 통과. 벤치는 Release, 기준선 r11392(bench_linux_io.cpp 만 넣어 빌드)와 교대 5라운드 x 3초 중앙값. 실 장비는 라운드 편차가 ±5% 안이다.

| 케이스 | thread | r11392 epoll | HEAD epoll | HEAD io_uring |
|---|---|---|---|---|
| tcp 16B x256 clients, window 1 | 4 | 455,772 | 439,690 (-4%) | 316,378 (-31%) |
| tcp 16B x256 clients, window 1 | 8 | 558,754 | 575,728 (+3%) | 280,418 (-50%) |
| tcp 64B x32 clients, window 16 | 4 | 6,604,313 | 6,685,047 (+1%) | 6,018,015 (-9%) |
| tcp 64B x32 clients, window 16 | 8 | 8,370,415 | 8,659,203 (+3%) | 4,779,445 (-43%) |
| tcp 64KB x4 clients, window 2 | 4 | 141,464 (8,842 MB/s) | 144,752 (+2%) | 146,122 (+3%) |
| tcp 64KB x4 clients, window 2 | 8 | 136,574 (8,536 MB/s) | 139,843 (+2%) | 119,187 (-13%) |
| udp 64B x16 sockets, window 32 | 4 | 0 (수신 불가) | 624,245 | 552,514 (-11%) |
| udp 64B x16 sockets, window 32 | 8 | 0 | 391,310 | 508,245 (+30%) |
| uds 64B x32 clients, window 16 | 4 | 7,077,473 | 7,160,978 (+1%) | 4,953,902 (-31%) |
| uds 64B x32 clients, window 16 | 8 | 8,667,352 | 8,538,468 (-1%) | 4,693,396 (-46%) |

  - 해석 : **epoll 경로는 실 장비에서도 변화 없음(±4%)**. 수신 버퍼 히스테리시스·syscall 절감은 echo ping-pong 에서는 안 보인다(1.3 의 큰 묶음 push 패턴은 아직 못 잼). **io_uring 준비 모드는 thread 가 늘수록 더 뒤처진다**(4 thread -9%, 8 thread -43%) : CQ 대기자 하나 + lock 경쟁이라는 구조 비용이 그대로 드러난다. UDP 8 thread 만 io_uring 이 빠른데(+30%) epoll 쪽이 4->8 thread 에서 오히려 떨어진 것(624K->391K : EPOLLET 소켓 16개를 8 thread 가 다투는 비용)이 원인이다. 결론 : 지금 io_uring 백엔드는 **옵션으로만 두고 기본은 epoll**. completion 모드(2. 남은 일)가 없으면 켤 이유가 없다.
  - 벤치 바이너리 : `~/cgdk_vb/{head,base}/build/src/unit_test/CGDK10.unit_test.net_socket.http.Release`(mistral), 스크립트 `linux_verify_bench.sh` / `linux_bench_only.sh`(export -> tar -> scp -> nohup, 결과 `~/cgdk_vb/verify.log`, `bench.log`, `bench_rounds.log`).

- 작업 환경 메모(1.11): WSL 의 gcc 13 Debug 는 `extern template class _shared_buffer<buffer>` 때문에 shared_buffer 의 constexpr 기본 생성자 심볼이 어디서도 안 나와 링크가 실패한다(gcc 15 는 정상). 저장소는 건드리지 않고 WSL 사본(`~/cgdk`)에서만 그 줄을 주석 처리해 빌드했다(`~/cgdk/sync.sh`).

### 1.12 인수인계 후속 처리 (r11402~r11418, 2026-10-22 저녁~밤)

이 문서의 "2. 남은 일" 을 이어받아 처리한 것. 각 커밋 메시지에 원인·검증 수치가 있다.

- **r11404 접속 즉시 실패** (남은 일 1): Linux `connect_requestable::Ntcp::process_request_connecting()` 이 connect() 즉시 실패(ENETUNREACH, 임시 포트 고갈의 EADDRNOTAVAIL 등)·epoll attach 실패 때 false 만 돌려주고 socket 을 **SYN 에 남겼다** => on_fail_connect 없음, 이후 close 가 접속된 socket 처럼 `process_complete_disconnect` 를 탐, `stop()` 의 `wait_complete_closed()` 가 timeout 까지 대기. 이제 iocp 처럼 `process_complete_connect(1)`. `Ntcp_reconnect::process_request_connecting()` 은 즉시 실패면(재접속 켜짐 + CLOSED) 여기서 재접속을 예약하고 true(대기 중)를 돌려준다. loopback 의 ECONNREFUSED 는 Linux 도 **비동기**(EPOLLERR)로 오므로 원래 재접속됐다 : 즉시 실패는 브로드캐스트 주소처럼 라우팅 단계에서 실패하는 경우.
  - 테스트 `test_connect_requestable.cpp`(새 파일). Windows 의 거절 접속은 RST 뒤 SYN 재시도로 **회당 약 2초**(테스트 대기 시간에 반영). 테스트 포트 33900~ 는 Linux 임시 포트 범위라, 리스너 전에 재접속 시도 중인 클라이언트 socket 이 같은 포트를 잠깐 가질 수 있다(acceptor start 재시도).
- **r11405 connector 의 connect/attach 순서** (새로 발견): Linux `connective::connector::Ntcp::process_connecting()` 이 **connect() 전에** epoll 에 붙였다. 접속 전 TCP socket 은 붙이는 즉시 `EPOLLOUT | EPOLLHUP`(0x14)로 보고되므로 worker 가 connect() 전에 이벤트를 가져가면 거짓 on_fail_connect + ONESHOT 소진, connect() 즉시 성공(0)은 이중 완료. 이제 connect 먼저, attach 나중(connect_requestable 과 같음). 테스트 `connect_requestable.connector_many_connects`(8 thread x 250).
  - **일반 원칙**: 접속 전 socket 을 epoll 에 먼저 붙이지 말 것(1.2 의 "즉시 성공도 attach 후 EPOLLOUT 으로 완료" 와 같은 규칙).
- **r11406 connector UDS** (남은 일 3): connector 도 bind 없음/any:0 + loopback 이면 UDS 를 먼저 시도. 스위치 공유. 테스트 `uds.connector`(풀 socket 이 UDS/TCP 를 오가며 재사용).
- **r11402 group::array::member(nullptr, seat)** (남은 일 4 의 null 역참조): 첫 줄 `member_group_lock(nullptr)` 역참조. 이제 `leave_by_seat()` 로 자리 비우기, 범위 밖 seat 는 아무것도 안 함(전엔 on_member_entering 뒤 `.at()` 예외). 테스트 `group_lock_order.array_member_null_empties_seat`.
- **r11403 svn_version 경합** (남은 일 7): 생성 파일을 빌드 폴더(`svn_version.h.generated`)로, 최종 svn_version.h 는 임시 이름 + rename. SAKPUNG 에서 bld/ bld_debug/ 동시 빌드 40회 : 원본 37/80 실패 -> 0/80.
- 남은 일 10 확인: gruma 서버 코드는 UDP socket 을 쓰지 않는다(영향 없음).
- **r11407 TFO 거절과 재접속**: Linux TFO 접속의 거절은 on_disconnect 로 오며 Ntcp_reconnect 는 그 경로로도 재접속한다(코드 수정 없음, 테스트로 고정).
- **r11408 `_buffer_unreal.h`** (남은 일 6): TSparseArray / TList / TLinkedListBase 직렬화를 UE5 API 로 다시 씀, TStaticArray extract 의 버퍼 미전진·null 검사 순서, sizeof 기반 사전 bound check 오류, TMap 키/값 타입. r11155 에서 사라진 한글 주석을 r11129 에서 복원(UTF-8). **실제 UE 로는 빌드하지 않았다**(모의 타입으로 MSVC/g++ 검증). FString 경로의 `TStringViewImpl`/생성자 인자 순서, examples 안 옛 사본은 그대로.
- **r11409 / r11410 그룹** (남은 일 4): for_each_member / find_member / container·observable 의 for_each·find 콜백을 lazy 스냅샷 위에서 **락 밖**에서 호출(이벤트 안에서 부르면 live). container·observable unregister/unsubscribe 의 erase 뒤 raw 포인터 콜백 UAF·임시 object_ptr 이중 해제, map/set unregister_object 시그니처, map unregister_object_all 이 새 객체를 지우던 것, 멤버가 있을 때 capacity 변경(이제 INVALID_STATE 예외), leave_by_seat 범위 재확인. on_member_* 이벤트가 어떤 락 안에서 불리는지는 .h 주석.
- **r11411 / r11412 object pool·factory TLS** (남은 일 5): sTLS_DATA 를 thread 가 소유(thread_local owner, thread 종료 때 정리), 풀/팩토리 소멸 시 분리, `_get_tls_data()` 가 소유 풀을 확인. close 뒤 TLS 잔여 객체 누수와 같은 T 재생성 시 해제된 data 사용 수정. close 는 다른 thread 의 TLS 캐시를 즉시 지우지 않는다(그 thread 의 다음 free/GC 또는 종료 때). Linux 같은 thread alloc/free 약 +0.4ns. **CGDK lib 전체 + gruma 재빌드 필요.**
- **r11413 connector bind** (남은 일 11 의 원인): Linux `bind(any:0)` 는 TIME_WAIT 포트도 피해서 대량 접속 반복 시 EADDRINUSE, 그 실패가 **통지 없이** 묻혔다(정체처럼 보임). any:0 이면 bind 생략, bind 실패는 on_fail_connect. io_uring/acceptor 문제가 아니었음(ring 상태·listen Recv-Q 로 확인). 테스트 포트를 28400~(임시 포트 범위 밖)로.
- **r11414 accept 후 attach 창** (남은 일 2): accept 된 socket 의 on_connect~attach 동안 `m_epoll_event_processing_now` 를 세워 close 를 큐잉(fd 를 닫지 않음), attach 는 epoll handle 로 락 안에서, 큐잉된 close 는 attach 없이 이 thread 가 완료.
- **r11417 / r11418 io_uring 간헐 실패와 acceptor 수명**: (1) 위 1.11 의 "요청은 제출한 thread 소유" - 재접속 관리자 정지와 `linux_io.io_uring_executor` 의 정리 지연은 같은 원인(post eventfd 의 poll 이 죽음). 회귀 테스트 `linux_io.io_uring_poll_of_an_exited_thread`. (2) poll 이 닫은 소켓을 잡고 있어 `uds.uds_only`/`listen_name_conflict` 가 부하 시 ~10% 실패 -> SYNC_CANCEL. (3) **epoll 에도 있던 acceptor 수명 버그**: `acceptor::stop()` 이 `count()==0` 을 확인한 뒤 다른 thread 의 처리 중인 accept 가 소켓을 목록에 넣을 수 있어, 그 소켓이 acceptor 파괴 뒤 끊기며 죽은 acceptor 의 `process_connective_disconnect` 를 불렀다("pure virtual method called"). `m_count_accepting` 으로 진행 중 accept 를 세고 `process_close_socket` 이 listen 을 닫기 전에 기다린다(`_process_wait_accepting`, 자기 thread 는 제외, 30초 뒤 포기 로그). acceptor 객체 크기가 바뀌어 **gruma 재빌드**. (4) `uds.reconnect` 간헐 실패는 다른 테스트 프로세스와의 포트 충돌(31400+pid%500) - 빈 포트를 골라 쓰도록. (5) `linux_io.io_uring_executor` 단독 실행 실패는 system executor 링이 나중에 생겨 fd 수에 섞인 것 - 세기 전에 `system_executor::executor()`.

---

### 1.13 epoll_wait 자체의 busy poll (r11429, 2026-10-23)

1.10 에서 "io_context 쪽 과제" 로 남겼던 것. socket 의 `busy_poll`(SO_BUSY_POLL) 은 recv 가 NIC 큐를 한 번 보는 것이고, 이것은 **epoll_wait() 가 잠들기 전에 NIC 큐를 usecs 동안 돌며 보는 것**(지연 감소, CPU 사용). 커널 6.9+ 의 `EPIOCSPARAMS`(epoll 인스턴스별)를 쓰므로 전역 sysctl `net.core.busy_poll` 이 필요 없다.

- API (`executor.Iio_context.h`): `executor::set_default_busy_poll(usecs, budget, prefer)` (이후 만드는 모든 epoll io_context. system_executor 는 처음 쓸 때 만들어지므로 **그 전에**. acceptor 의 cpu_affinity io_context 도 받는다), `make_io_context_threads(name, count, cpu, busy_poll_usecs)`, `make_io_context_threads(name, context)`(`"thread_count"`, `"cpu_affinity"`, `"busy_poll"`, `"busy_poll_budget"`, `"prefer_busy_poll"`) 와 `io_context_threads::START_PARAMETER` 의 같은 필드, `Iio_context::set_busy_poll()/get_busy_poll()`(시작 후 변경·끄기·읽기), `is_busy_poll_available()`.
- 구현: `api::io_context_Nepoll::set_busy_poll()` 이 `ioctl(EPIOCSPARAMS)`(구조체·ioctl 번호는 옛 헤더를 위해 Nepoll.cpp 에 직접 정의). `io_context::_process_start()` 가 기본값을 적용하고 `io_context_threads` 의 자기 값이 덮는다. 기본값은 atomic 하나에 packed.
- **제약**: 같은 NAPI ID(한 NIC 큐)의 socket 들이 한 epoll 에 있어야 커널이 돈다. loopback/UDS 는 NAPI ID 가 없어 아무 효과가 없다 => **POKPUNG 단위 테스트는 설정·읽기·동작 무해성만 확인**, 실제 지연 이득은 실 NIC 에서 acceptor `"cpu_affinity"`(SO_INCOMING_CPU 로 CPU 별 io_context) 와 함께 재야 한다(미측정). budget 64 초과는 CAP_NET_ADMIN(EPERM 경고). 6.9 미만 커널은 ENOTTY/EINVAL 경고 후 false(sysctl 만 가능). io_uring 백엔드와 Windows 는 경고 후 무시. `prefer_busy_poll` 은 epoll_wait 가 제때 불리는 동안 NAPI 를 폴링 모드로 유지한다(인터럽트 억제).
- `io_context_threads::START_PARAMETER` 에 필드 3개 추가(크기 변경) : 이 구조체를 직접 쓰는 곳(저장소 안에는 make_io_context_threads 뿐, gruma 없음) 재빌드.
- 테스트 `test_linux_io.cpp` `linux_io.epoll_busy_poll_parameter / epoll_busy_poll_default / io_uring_has_no_busy_poll`(EPIOCGPARAMS 로 커널 값 직접 확인), tutorial ex15 5절. POKPUNG(kernel 7.0) net_socket.http 전체 epoll/io_uring 각 3회 115 통과. Windows x64 Debug system.object/net.socket/테스트/ex15 빌드, ex15 는 "not available".
- 관찰: 이 작업 중 epoll 기본 전체 스위트 1회에서 `linux_io.io_uring_executor` 가 "connected yet 1" 로 실패(r11417 이후 9회 중 1회). 재현 안 됨, 주시할 것.


### 1.14 io_uring completion 백엔드 1단계 (r11437, 2026-10-23)

남은 일 8 의 1단계. **worker thread 마다 ring 하나 + op 별 executable** 의 새 백엔드 `api::io_context_uring_cq`(`api.io_context.uring_cq.h/.cpp`, `CGDK_IO_URING=completion|cq|2`, `io_backend::io_uring_completion`). 소켓 코드는 아직 readiness 경로 그대로이고(2단계에서 op 로 전환), 새 백엔드가 attach/set_event/detach 도 서비스하므로 전체 스위트가 completion 모드로도 통과한다.

- **ring/worker**: `io_context_threads::process_thread_run()` 이 시작 때 `bind_worker()`, 끝에 `unbind_worker()`. ring 상태 ORPHAN/OWNED/DRAINING. 고아 ring(종료한 extra thread 것, worker 가 붙기 전에 만든 ring 0)은 다른 worker 가 입양하거나 pop 마다 드레인(그동안 대기 10ms 클램프). fd 는 첫 attach/제출 때 ring 에 묶인다(`m_binding[fd]`, 현재 thread 가 worker 면 그 ring, 아니면 라운드로빈). 고아 ring 의 fd 는 다음 제출 때 옮겨 묶는다. poll seq 는 io_context 전역.
- **제출**: owner 는 SQE 를 써 두고 `pop_results()` 의 `io_uring_enter(to_submit, GETEVENTS)` 하나로 제출+대기. 다른 thread 는 SQ 락 아래서 즉시 enter(**그 요청의 소유자는 제출한 thread** : 종료하면 -ECANCELED. readiness poll 은 재무장, op 는 -ECANCELED 전달). 깨우기는 eventfd 가 아니라 **NOP SQE**(fd 절약, 재무장 불필요). post 는 공유 deque + ring 하나 깨우기(size/8 배치, 남으면 다음 ring). ring 없는 thread(wait_complete_* 펌핑)는 post 큐만 cv 로 처리.
- **op API**(`Iio_context` 가상, 기본 false): `submit_recv/recvmsg/send(zc)/sendmsg(zc)/accept(multishot)/connect/cancel/cancel_fd/wait_cancelled_fd`, 완료는 `process_execute(res, cqe_flags)`(bytes 또는 -errno, F_MORE/F_NOTIF). 제출마다 executable 참조 1, F_MORE CQE 는 전달 전 add_ref 1 추가(마지막 CQE 가 제출 참조 소비). OP 레코드(풀, mutex) + `user_data = bit63 | OP*`, fd 별 in-flight 카운트. registry generation 은 31비트로(태그 자리).
- **opcode 주의**: `IORING_OP_SEND_ZC=47, SENDMSG_ZC=48`(56/57 은 BIND/LISTEN). 상수는 커널 헤더로 확인할 것.
- **RLIMIT_MEMLOCK**: ring 메모리가 사용자별 memlock(POKPUNG 8MB)에 계상된다. SQ4096/CQ16384 는 ~540KB 라 15개뿐 => ring 당 SQ256/CQ1024(~36KB)로 줄이고 ENOMEM 이면 32까지 반으로. **서버에서는 `ulimit -l` 을 올릴 것.** 한도를 넘으면 그 worker 는 post 큐만 처리하고, ring 이 하나도 없으면 attach/submit 이 false.
- `is_available()`: 6.0+ 기능 + `IORING_REGISTER_PROBE` 로 필요한 opcode 확인. 불가하면 epoll 로 fallback(경고).
- 벤치(Debug, 4 thread, loopback echo, 소켓은 아직 readiness): tcp 64B x32 epoll 6.57M / 공유 ring io_uring 5.88M / **uring_cq 6.71M** msg/s, tcp 64KB x4 150K / 179K / 180K, uds 6.81M / 6.65M / 6.77M, udp 681K / 500K / 510K. 공유 ring 이 잃던 TCP 성능은 회복(epoll 수준), UDP 는 io_uring 계열 -25% 남음. 실제 이득은 2단계(recv/send 를 op 로) 뒤에 잰다.
- 테스트 `unit_test.object.execute/test_io_context_uring_cq.cpp`(8개, CMake 만: post 폭주, readiness 호환, recv/send, ZC+gather, multishot accept+cancel_fd, connect, cancel/wait_cancelled_fd, 진행 중 op 와 stop). ASan 통과. net_socket.http 전체 3 모드 통과.
- 제약: COOP_TASKRUN 이라 syscall 없이 오래 도는 소유 thread 의 완료는 다음 커널 진입까지 지연. owner 의 지연 제출은 다음 pop 에서 나간다(executable 안에서 오래 블록하면 그동안 제출 안 됨). `wait_cancelled_fd` 를 worker 자신이 부르면 자기 ring 의 전달은 기다리지 않는다. OP 풀은 mutex(2단계 핫패스면 TLS 캐시로).
- 참고: 기존 readiness 백엔드의 SYNC_CANCEL 루프는 kernel 7.0 에서도 첫 호출 0, 다음 ENOENT 로 끝난다(확인).


### 1.15 io_uring completion 2단계 - 소켓 op 전환 (r11441, **미완성 WIP**, 2026-10-23)

**r11441 의 svn 로그 메시지는 비어 있다**(커밋 도구 문제). 그 커밋의 설명은 이 절과 아래 요약이 대신한다 : "net.socket(linux): io_uring completion 2단계 WIP - 소켓 수신·송신·close·accept·connect 를 op 로 (completion 모드 2개 테스트 실패 남음, epoll/readiness 경로 무변경)".

남은 일 8 의 2단계. 구현 에이전트가 사용량 한도로 중단된 상태를 **인수인계용으로 그대로 커밋**한 것이다. epoll 과 readiness io_uring 경로는 건드리지 않았고(전체 스위트 116 통과), completion 모드(`CGDK_IO_URING=completion`)는 114/116 으로 2개가 실패한다. Windows 빌드·75개 통과.

- **구조** (스펙 `uring_cq_phase2_spec.md` 의 요지, 코드 주석에도 있음): 선택 지점은 `Isocket::process_decide_io_completion()`(접속 완료 처리 `connectable::Ntcp::process_complete_connect` 에서 on_connect 전에 결정 : io_context 가 completion 이고 receivable/sendable 이 `is_io_completion_supported()`) 과 `process_start_io(pio_context, is_attached)`(on_connect 뒤, acceptor/connector/connect_requestable 세 곳 : completion 이면 첫 RECV op, 아니면 attach/set_event). `m_is_io_completion`, `m_count_io_inflight`(op executable 이 소켓을 잡는다 : 풀 재사용 방지), `m_is_receiving_op`(수신 락 아래).
  - 수신 `net.io.receivable.stream.uring.cpp`: 소켓당 RECV 1개 상시(`executable_receiving`, 수신 락 `m_lockable_receiving` 을 Linux completion 에도 사용), 완료 0 바이트/오류 => disconnect, 버퍼 성장/축소는 epoll 코드 재사용.
  - 송신 `net.io.sendable.tcp_async_gather.uring.cpp`: 직접 send 는 그대로, EAGAIN/부분/EINPROGRESS 만 SENDMSG op, 큰 단일 버퍼는 SEND_ZC op(통지 CQE 로 owner 해제, error queue 미사용).
  - close `net.io.socket.tcp.cpp`: completion 소켓은 handle 교체 + shutdown 뒤 `submit_cancel_fd` 후 close. RECV op 가 있으면 그 완료가 disconnect 를 끝내고, 없으면 close 가 직접.
  - accept `net.io.connective.acceptor_tcp.uring.cpp`: listen(TCP, UDS)마다 multishot ACCEPT op, 각 CQE 는 `_process_accepted()`(accept 루프와 공유). stop 은 `m_count_accepting` 대기 뒤 cancel_fd.
  - connect: connect_requestable `process_submit_connect_op()` / connector 가 `submit_connect` op, 완료는 기존 `process_complete_connecting` 으로.
- **남은 실패 2개 (completion 모드)**:
  1. `connect_requestable.connector_many_connects`: 2000 중 983 이 on_fail_connect. 원인(추정, 1.14 의 규칙): **요청은 제출한 thread 소유** - 테스트의 8개 임시 thread 가 CONNECT op 를 제출하고 바로 종료 => 그 op 들이 -ECANCELED. 짧게 사는 로직 thread 가 흔하므로 **다른 thread 의 제출은 ring 소유 worker 가 대신 SQE 를 쓰도록(ring 별 MPSC 큐 + NOP wake)** 백엔드를 고쳐야 한다(1단계 스펙의 "later phase" 항목). 소켓 쪽은 바꿀 것이 없다.
  2. `linux_io.io_uring_executor`: accept 된 소켓 정리가 10초 안에 끝나지 않음("connected yet 1", 41초). readiness 모드에서도 드물게 보이던 증상이지만 completion 에서는 1회 실행에 바로 났으므로 close 경로(RECV op 취소 완료가 disconnect 를 끝내는 흐름) 를 의심할 것.
- 미검증: ASan, uds.* gdb 반복, 벤치(epoll 대 completion msg/s, `strace -c` syscall 비율), `attached_count()==0` 확인. 2단계를 이어받는 사람은 위 1번(백엔드)부터 고치고 completion 모드로 전체 스위트 3회, 그 다음 벤치.


### 1.16 io_uring completion 2단계 - 다른 thread 의 제출을 ring 소유 worker 가 대신 쓴다 (r11444, 2026-10-23)

1.15 의 남은 실패 2개를 백엔드 수정 하나로 해결했다. 소켓 코드는 바꾸지 않았다.

- **원인 확정**: io_uring 요청은 `io_uring_enter` 를 부른 thread 의 소유다. owner 가 아닌 thread 가 SQ 에 직접 쓰고 즉시 enter 하면, owner 가 지연 제출용으로 SQ 에 써 둔 SQE(CONNECT/RECV op, POLL_ADD)까지 **함께 제출**되어 그 요청들이 짧게 사는 thread 소유가 된다. 테스트의 임시 thread 가 `request_connect` 를 하면 post 의 NOP wake 가 바로 이 경로였다(`connect_requestable` 이 CONNECT 를 worker 에 post 하도록 r11441 에서 바꿨어도 소용이 없었던 이유). thread 가 끝나면 op 가 -ECANCELED => `connector_many_connects` 983 실패, `io_uring_executor` 의 close 정리 지연.
- **구조** (`api.io_context.uring_cq.cpp` 머리 주석): 
  - `RING::staged` : POLL_ADD 와 op 는 어느 thread 가 쓰든 ring 의 staging 벡터에 쓴다(SQ 락). **ring 의 owner 만** `pop_results()` 에서 `flush_staged()` 로 SQ 에 옮겨 자기 enter 로 제출한다(배치당 syscall 하나 유지). owner 가 아닌 thread 는 staging 뒤 NOP(UD_IGNORE) 으로 owner 를 깨운다(`_wake_staged`, `RING::is_wake_pending` : owner 가 flush 전에 지우므로 flush 까지 한 번만).
  - 소유자가 의미 없는 SQE(NOP, POLL_REMOVE, ASYNC_CANCEL : 제출 즉시 완료)만 쓰는 thread 가 바로 enter 한다(`RING::write_now`). 그래서 SQ 에는 소유가 중요한 SQE 가 owner 의 flush 밖에 남지 않는다.
  - staging 중인 poll 의 취소(modify / detach) 는 staging 에서 지운다(`_purge_staged_poll`. 제출됐다면 닫힌 fd 번호를 poll 할 뻔). staging 중인 op 의 취소(`submit_cancel` / `submit_cancel_fd` / `wait_cancelled_fd`) 는 `RING::synthetic` 으로 옮기고 owner 가 `pop_results()` 첫머리에서 -ECANCELED 로 전달한다(`OP::state` STAGED / SUBMITTED / CANCELLED, `_deliver_synthetic`). `_submit_poll_cancel` 의 "submit now" 는 없앴다.
  - 고아 ring 의 staged / synthetic 은 드레인하는 worker 의 ring 으로 옮긴다(`_adopt_staged`, 두 ring 락을 `std::scoped_lock`). `unbind_worker` 는 남은 SQE 를 제출하지 않는다(제출하면 종료와 함께 취소). ring 없는 thread 의 handle 이 고아 ring 에 묶여 있으면 owned ring 으로 다시 묶는다(`_ring_of_handle`).
  - 최종 CQE / synthetic 완료의 공통 정리 `_complete_op_final`. `destroy()` 는 staged / synthetic 도 비운다.
- **테스트** `test_io_context_uring_cq.cpp` `ops_from_short_lived_threads` : 8 thread x 50 CONNECT 를 임시 thread 가 제출하고 즉시 종료 => 전부 0 으로 완료(이전 코드면 -ECANCELED), recv / attach 도 임시 thread 에서, 제출 직후 `submit_cancel` / `wait_cancelled_fd` (synthetic 경로).
- **검증** (WSL2 kernel 6.6, gcc 13, Debug) : `io_context_uring_cq.*` 9개 3회 통과. net_socket.http 전체 `CGDK_IO_URING=completion` 3회, epoll 1회, readiness io_uring 1회 모두 114 통과(skip 5 : busy_poll 2, bench 3). `connector_many_connects` 387~907ms, `io_uring_executor` 1.4s.
- **벤치** (WSL2, Debug, `CGDK_BENCH=3`, 두 번 실행) : tcp 64B x32 epoll 503K / 819K, uring_cq 592K / 678K msg/s. uds 64B x32 epoll 494K / 994K, uring_cq 950K / 720K. tcp 64KB x4 epoll 13K / 31K, uring_cq 8.4K / 20.6K. 같은 바이너리가 실행마다 1.6~2배 흔들려(WSL 노이즈) 비교 불가 : **POKPUNG 에서 다시 잴 것**. 64KB 에서 uring_cq 가 epoll 보다 낮게 나오는 경향은 두 번 다 같았다(SEND_ZC 통지 2회 + RECV 재발행 비용 : 3단계 provided buffer 의 대상).
- **미검증** : POKPUNG(kernel 7.0, g++ 15) 실행, ASan, `strace -c` syscall 비율. Windows 는 linux 전용 파일이라 영향 없음(미빌드). `connect_requestable::Ntcp::process_submit_connect_op`(CONNECT 를 worker 에 post) 는 이제 필요 없지만 두었다 : 없애면 post 한 번이 준다.
- **다음** : 3단계(multishot recv + provided buffer). 완료 통지와 staging 의 wake 비용은 `linux_io.io_uring_completion_sockets` 가 아직 readiness 와 같은 수준인지 POKPUNG 벤치로 볼 것.
- **후속(1.17)** : 1.16 의 WSL 검증만으로는 부족했다. kernel 7.0 / 32 CPU 에서 남은 경쟁 두 개와 handle 표 크기 문제가 드러났다(r11451).


### 1.17 io_uring completion - owner 가 아닌 thread 는 어떤 ring 에도 enter 하지 않는다 (r11451, 2026-10-23)

1.16(r11444)을 mistral(Ubuntu 26.04, kernel 7.0, 32 CPU)에서 돌리자 completion 모드 `connector_many_connects` 가 3회 모두 실패(2000 중 ~1000)하고 `ops_from_short_lived_threads` 도 1/3 실패했다. WSL(6.6, 8 CPU)에서는 보이지 않았다. 원인은 세 가지였다.

- **요청 소유권이 남은 경로** : `io_uring_enter` 는 SQ 의 head~tail 사이 SQE 를 **전부** 제출한다. owner 가 `pop_results()` 에서 staged SQE 를 SQ 로 옮기고 tail 을 공개한 뒤 자기 enter 전에, 다른 thread 가 깨우기 NOP(`_wake_staged`, post 의 `_wake_ring`, dispose/destroy) 이나 POLL_REMOVE / ASYNC_CANCEL 을 쓰고 enter 하면 owner 의 SQE 가 그 thread 소유가 된다(1.16 의 "소유가 의미 없는 SQE 만 직접 제출" 규칙이 틀렸다 : 그 SQE 자체가 아니라 함께 제출되는 남의 SQE 가 문제).
  - 깨우기 : 전용 wake ring(`m_wake_ring`, `m_lockable_wake`)에서 `IORING_OP_MSG_RING`(=40, `IORING_MSG_DATA`)으로 대상 ring 의 CQ 에 CQE 를 직접 넣는다(`_send_wake`). 대상 ring 에 enter 하지 않고, msg 는 제출 즉시 inline 으로 끝나 thread 가 종료해도 남는 요청이 없다(대상이 IOPOLL / DEFER_TASKRUN 이 아니므로). owner 자신은 SQ 에 NOP(`RING::write_wake()`). `RING::write_nop / write_now` 삭제, `is_available()` 이 MSG_RING 을 probe.
  - 다른 thread 의 POLL_REMOVE 는 staging. 커널에 있는 op 의 취소 : owner 면 ASYNC_CANCEL 제출, 아니면 **ring 의 SQ 락 안에서** `IORING_REGISTER_SYNC_CANCEL`(timeout 0, `_cancel_now`) - 그 thread 에서 즉시 취소되고 아무것도 제출하지 않는다. 락 안인 이유 : 완료·해제된 OP 레코드가 다음 op 에 재사용된 뒤 그 주소로 엉뚱한 op 를 취소하는 ABA 를 막는다(`_complete_op_final` 이 같은 락을 잡는다). owner 는 이 락을 잡은 채 커널에서 기다리지 않으므로 교착 없음.
- **flush 와 submit 사이의 틈** : owner 가 op 를 SQ 로 옮기며 `OP_SUBMITTED` 로 표시하고 락을 푼 뒤 enter 전에 다른 thread 가 취소하면, sync cancel 이 커널에서 그 op 를 못 찾고 지나가고 op 는 그 뒤 제출되어 **영영 취소되지 않는다**(WSL : 임시 thread 테스트의 recv 취소 6초 무응답, `socket_sustain.idle_timeout` 의 close 무응답 뒤 abort). `pop_results()` 가 `flush_staged()` 와 제출 enter 를 같은 SQ 락 안에서 하고, 기다림은 락 밖의 두 번째 enter(CQ 가 비었을 때만)로 바꿨다. 이제 `OP_SUBMITTED` 는 항상 커널에 있다.
- **handle 표 크기** : 1.14 의 per handle 배열은 create() 시점의 soft RLIMIT_NOFILE(Ubuntu 1024)로 잡혀 fd >= 1024 의 op 를 거절했다(CGDK 는 net 초기화 때 soft 를 hard 로 올린다 : `connector_many_connects` 의 2000+2000 fd 중 절반 실패. 실패 errno 는 남은 ETIME/ENOTCONN 이라 원인을 숨겼다). r11448 은 hard limit(무한이면 1M, ~7MB)으로 키웠고, r11451 은 `HANDLE_ENTRY{armed, binding, inflight}` 를 16384 개 chunk 로 나눠 그 범위의 fd 를 처음 쓸 때 할당한다(`_entry` CAS 할당 / `_find_entry` 조회만. chunk 없음 = 쓴 적 없음). io_context 마다 고정 할당이 없다(acceptor `cpu_affinity` 의 CPU 별 io_context 도).
- r11448 의 `bind_worker(_is_transient)`, `wait_cancelled_fd(_is_wait_delivery)` 는 그대로.
- **테스트** `fd_above_the_limit_at_create` : soft limit 256 에서 만든 executor 가, 한도를 올린 뒤 fd 700 이상의 recv / `wait_cancelled_fd` / attach 를 처리.
- **검증** (r11449 + 이 수정, Debug) :

| 장비 | io_context_uring_cq x3 | completion x3 | epoll | readiness io_uring |
|---|---|---|---|---|
| mistral (kernel 7.0, 32 CPU, gcc 15.2) | 10/10 x3 | 116, 116, 113(+uds 3) | 116 | 116 |
| WSL2 (kernel 6.6, 8 CPU, gcc 13) | 10/10 x3 | 114, 113(+zerocopy 1), 114 | 114 | 114 |

  - `connector_many_connects` : completion 3회 모두 통과(수정 전 3회 모두 실패).
  - uds 3개(`listen_name_and_release`, `listen_stop_start_repeated`, `listen_ignored_for_non_loopback_address`) : r11448 커밋 메시지의 "stop 직후 UDS 이름 해제 지연" 과 같은 증상(1회차에 3개 한꺼번에). 이 수정 이전부터 있다. 분석 : acceptor stop 은 `wait_cancelled_fd(fd, 1s, false)` 로 커널의 취소만 기다리는데, 반복 SYNC_CANCEL 의 ENOENT 는 owner 의 task work 가 요청을 poll hash 에서 뺀 시점이고 파일 참조는 같은 task work 의 뒤쪽(완료 batch flush 후 free)에서 놓인다. 그 틈에 close + 재접속이 끼면 listen socket 이 아직 살아 있다. **`_is_wait_delivery=true`(전달까지 대기)면 CQE 를 사용자 공간에서 받은 뒤이므로 해제가 끝나 있다** - acceptor 쪽(r11448 작업)에서 확인할 것. stop 을 worker 가 부르는 경우의 자기 대기는 `_find_worker_ring()` 이 이미 건너뛴다.
  - `linux_io.tcp_zerocopy_large_messages_io_uring` : readiness 백엔드 + MSG_ZEROCOPY 통지 경로의 간헐 실패(pending 1 이 10초 안에 안 끝남). mistral epoll 실행 1회, WSL completion 실행 1회. 이 수정은 그 경로를 건드리지 않는다. => 해결 r11468(1.21 : 송신 thread 가 hold 하기 전에 다른 thread 가 통지를 읽음).
  - hukpung(Ubuntu 24.04, kernel 6.8) : OpenSSL 1.1 이라 util.crypt 빌드 실패(`EVP_MD_get_size`, `SSL_get1_peer_certificate`). 이 수정과 무관, 미검증.
- **벤치**(mistral Debug, 참고용) : 이 수정 전후 uring_cq 수치 차이는 실행 간 흔들림 안(tcp 64B x32 3.9~5.4M, uds 4.9~6.1M msg/s. r11444 은 4.3 / 5.0M, 5.5 / 4.9M). 비교는 Release 로.
- **교훈** : WSL 8 CPU 의 통과는 completion 백엔드의 경쟁을 거의 드러내지 못한다. completion 백엔드를 고치면 mistral(32 CPU)에서 전체 스위트 3회를 기본으로 돌릴 것.


### 1.18 io_uring completion 3단계 - multishot RECV + provided buffer ring (r11453, 2026-10-23)

남은 일 8 의 3단계. 코드는 다른 세션의 일괄 커밋 **r11453** 에 들어갔다(이 작업 세션의 파일 8개. 그 커밋 메시지의 "linux 빌드/테스트는 이 커밋에서 확인하지 않음" 은 아래 검증으로 보완한다). 1.17(r11451) 위에서 동작한다.

- **백엔드** (`api.io_context.uring_cq.h/.cpp`) : `submit_recv_multishot(fd, executable)` / `get_provided_buffer(param)` / `return_buffer(param)` / `provided_buffer_out_count()`. `Iio_context` 가상(기본 false / nullptr)과 `executor::io_context`, `api::io_context_uring` 이 전달한다.
  - ring 마다 provided buffer ring(`IORING_REGISTER_PBUF_RING`, 그룹 0, `PROVIDED_BUFFER_COUNT` 256 x `PROVIDED_BUFFER_SIZE` 16KB = ring 당 4MB 가상 메모리, 링 자체는 4KB)을 **그 ring 의 첫 multishot recv 때** 등록한다(`_prepare_pbuf` : `_submit_op_ex` 의 prepare 단계). 등록은 요청이 아니므로 어느 thread 든 할 수 있고, 실패하면 -1 로 기억해 다시 시도하지 않는다(false). 버퍼 그룹 id 는 io_uring 인스턴스별이라 모든 ring 이 같은 0 을 쓴다. SQE 는 opcode RECV + `IOSQE_BUFFER_SELECT` + ioprio `IORING_RECV_MULTISHOT`, len 0.
  - 완료는 기존과 같이 `process_execute(res, param)`. `IORING_CQE_F_BUFFER` 가 있으면 param 의 bit 16~31 이 buffer id 이고, **bit 32~47 에 그 CQ 의 ring index + 1** 을 넣는다(소비자가 어느 ring 의 버퍼인지 알아야 돌려줄 수 있다. 고아 ring 을 drain 한 CQE 도 그 ring 의 것). F_MORE 의 add_ref 규칙은 1.14 그대로.
  - **반환(다른 ring 의 worker 포함)** : buffer ring 은 커널과 공유하는 메모리일 뿐 요청이 아니므로 1.16/1.17 의 소유 규칙 대상이 아니다. 생산자(tail 쓰기)는 ring 별 `RING::lockable_pbuf` 아래 한 thread 씩, 커널은 head 를 소비한다. 엔트리 수 = 버퍼 수라 tail 칸은 항상 커널이 다 쓴 칸이다. 같은 버퍼를 두 번 돌려주면 커널이 같은 버퍼를 두 번 내주므로 `pbuf_out[bid]`(CQE 처리 때 1, 반환 때 exchange 0)로 두 번째 반환을 무시한다. `RING_GUARD` 로 destroy 와 겹치지 않고, 메모리는 ring 을 닫은 뒤 `RING::destroy_pbuf()` 가 푼다.
  - **CQE 실행 순서** : multishot 은 한 op 의 CQE 가 여럿이다. 고아 ring 을 여러 worker 가 번갈아 drain 하면 같은 소켓의 CQE 두 개가 두 thread 에서 동시에(순서가 바뀌어) 실행될 수 있었다. 이제 drain 한 worker 는 그 ring 을 **자기가 실행을 마칠 때까지 `STATE_DRAINING` 으로 붙든다**(`t_draining`, `_release_draining()` : `flush_pending()` / 다음 `pop_results()` / `unbind_worker()`). 한 ring 의 CQE 는 한 번에 한 thread 에서 CQ 순서대로 실행된다(owner 는 원래 그렇다).
  - `CGDK_IO_URING_MULTISHOT=0`(또는 off) 이면 `submit_recv_multishot()` 이 false 를 돌려 소켓이 2단계의 RECV 경로로 간다(A/B 비교용).
- **소켓** (`net.io.receivable.stream.uring.cpp`, `.h`) : `receive(bool _is_multishot = true)` 가 multishot 을 먼저 시도하고, 안 되면 `submit_recv`. 소켓 참조(add_ref / `m_count_io_inflight`)는 제출 때 1, **F_MORE 없는 마지막 완료에서만** 놓는다(통계도 같이).
  - 데이터는 `m_buffer_received` 의 남은 데이터 뒤로 복사(`_reserve_receive_buffer` : 자리가 모자라면 앞으로 당기거나 2배씩 키움)하고 **곧바로 반환**한다. 반환 대상은 op 를 제출한 io_context(`executable_receiving::m_pio_context`) : 소켓은 disconnect 때 `m_pio_context` 를 놓기 때문이다. 수신 처리(process_packet, 버퍼 크기 조정)는 2단계와 같고, provided buffer 데이터는 "창을 다 채움" 판정에서 뺀다.
  - F_MORE 가 없는 완료 : 데이터 → 다시 multishot. **-ENOBUFS** → RECV 하나를 `m_buffer_received` 로(버퍼 없이도 진행), 그 완료 뒤 다시 multishot. 0 / 에러 → disconnect. -ECANCELED 인데 handle 이 열려 있으면 재제출(기존).
  - close 흐름(1.15)은 그대로다. multishot 이 도는 동안 `m_is_receiving_op` 는 계속 true 라 `process_close_socket()` 은 `submit_cancel_fd` 만 하고, op 의 마지막 완료(-ECANCELED, 또는 그 전에 커널이 끝낸 데이터 / 0)가 disconnect 를 끝낸다. 처리 중 예외로 닫혀도 F_MORE 면 그냥 돌아가 마지막 완료를 기다린다.
- **-ENOBUFS 는 기다리는 데이터 없이도 온다** : 커널은 recv **전에** 버퍼를 고른다. multishot 이 마지막 버퍼를 쓴 직후의 재시도에서 바로 -ENOBUFS 로 끝나므로 "그 바이트가 소켓에 남아 있다" 고 가정하면 안 된다(테스트가 처음에 그렇게 가정했다가 틀렸다).
- **테스트** `io_context_uring_cq.op_recv_multishot_provided_buffer` : worker 가 아닌 thread 의 제출 → 도착별 CQE(F_BUFFER + F_MORE), 512KB 순서·내용, 버퍼를 쥐고 고갈 → -ENOBUFS → post 한 executable(다른 ring 의 worker 포함)과 worker 가 아닌 thread 에서 반환, 두 번째 반환 거부 → 재제출 뒤 유실 없음, cancel_fd → -ECANCELED 와 참조 해제, 짧게 사는 thread 의 제출이 살아남음, peer close → 0, 16 소켓 x 1MB 동시(버퍼 공유, -ENOBUFS 재제출, 실행 겹침 0). `linux_io.io_uring_completion_sockets` 한 번을 gdb 로 센 호출 수 : multishot 제출 124 / 단일 RECV 0 / 버퍼 반환 3285(fallback 없음).
- **검증** (r11453 = r11451 + 이 작업, Debug) :

| 장비 | io_context_uring_cq x3 | completion x3 | epoll | readiness io_uring |
|---|---|---|---|---|
| mistral (kernel 7.0, 32 CPU, gcc 15.2) | 11/11 x3 | 116, 116, 116 | 116 | 116 |
| WSL2 (kernel 6.6, 8 CPU, gcc 13) | 11/11 x3 | 114, 114, 114 | 114 | 114 |

  - 16 소켓 테스트의 -ENOBUFS 는 실행마다 5~20회 일어났고 모두 회복했다.
  - 1.17 이전 트리(r11448 작업 중 상태)에서 한 비교 : completion 전체 스위트 교대 10회 중 multishot 9회 통과(나머지 1회는 zerocopy 통지) / multishot 없는 기준 4회 통과(uds 간헐 실패, abort·segv 각 1회). 그 트리의 첫 검증에서 multishot 쪽에 `uds.connector` echo 정지 뒤 종료 중 힙 손상이 1회 있었다. 같은 계열(uds stop 지연, 종료 시 예외)이 기준 쪽에도 있었고, r11453 검증 6회(WSL 3, mistral 3)에서는 나오지 않았다.
- **벤치** (참고용, Debug, `CGDK_BENCH=3`, multishot / `CGDK_IO_URING_MULTISHOT=0` 교대 2회. 실행 간 흔들림이 차이보다 클 수 있다. **서버 socket 은 system executor(epoll)에서 돌았다 : uring_cq 는 클라이언트만**(1.27). 서버도 같은 backend 로 잰 표는 1.31) :

| uring_cq msg/s | WSL multishot | WSL 단일 RECV | mistral multishot | mistral 단일 RECV |
|---|---|---|---|---|
| tcp 64B x32 | 2.28M / 1.98M | 2.01M / 1.85M | 5.73M / 5.47M | 5.48M / 5.33M |
| uds 64B x32 | 2.67M / 2.42M | 2.13M / 1.76M | 5.63M / 5.80M | 5.82M / 4.91M |
| tcp 16B x256 | 303K / 234K | 161K / 171K | 363K / 379K | 334K / 307K |
| tcp 64KB x4 | 38.5K / 41.3K | 45.7K / 49.5K | 162K / 134K | 160K / 155K |

  같은 실행의 epoll : WSL tcp 64B 2.59M / 2.33M, mistral 6.24M / 5.85M. 작은 메시지는 multishot 쪽이 같거나 빠르고(WSL 에서 뚜렷), **64KB 는 같거나 느리다** : 64KB 메시지가 16KB 버퍼 4개 이상의 CQE 와 복사로 온다(2단계 RECV 는 큰 `m_buffer_received` 로 한 번에 받는다).
- **남은 것** : (1) 대용량 : 기다리는 메시지가 버퍼보다 크면(`required_bytes > PROVIDED_BUFFER_SIZE`) 그 메시지 동안 RECV 로 직접 받거나, 버퍼 크기·수를 설정으로. (2) 버퍼 메모리 : ring 당 4MB 가상, 쓰는 만큼 RSS. (3) worker 가 executable 안에서 같은 io_context 를 펌핑하면(중첩 pop) 같은 소켓의 다음 CQE 가 중첩 실행될 수 있다 : 지원하지 않는다. (4) Release·POKPUNG 벤치, ASan, `strace -c` syscall 비율은 돌리지 않았다. (5) 송신은 2단계 그대로(SEND bundle, zc 통지 비용).
- `receivable::Nstream` 헤더(`receive(bool)`, `executable_receiving::m_pio_context`)가 바뀌었다 : gruma 등 사용하는 쪽 재빌드.


### 1.19 io_uring completion - 수신 모드 전환 : 큰 트래픽은 IOCP 식 가변 RECV 버퍼, 작은 메시지는 multishot (r11459, 2026-10-23)

그 사이 다른 세션의 4단계 r11457(provided buffer 무복사 수신 기본 on, ring 소유 worker 송신 op 옵션)이 들어왔다 : 이 문서에 절이 없으니 그 커밋 메시지와 `document/Web/site/socket.io_uring.html` 을 볼 것. 이 절은 그 위의 r11459 다.

- **문제** : multishot RECV 는 16KB 고정 provided buffer 라 큰 메시지가 여러 CQE 로 조각나고 꼬리 복사가 든다(64KB 메시지 = 4개 이상, 1.18 벤치의 64KB 열세). IOCP 는 받은 버퍼를 트래픽에 맞춰 키워(창을 다 채우면 x8) 한 번에 받는다.
- **동작** (`net.io.receivable.stream.uring.cpp` 의 `_process_select_receive_mode()`) :
  - 기본은 multishot. 한 완료가 provided buffer 를 **다 채웠거나**(뒤에 더 있음) provided buffer 보다 **큰 메시지를 기다리면** RECV 모드로 바꾼다. 진행 중인 multishot 은 `submit_cancel(executable)` 로 한 번 취소하고(`m_is_receive_mode_switching`), 그 마지막 완료(-ECANCELED 인데 handle 이 열림, 또는 그 전에 커널이 끝낸 데이터)가 기존 재제출 경로로 RECV 를 낸다. 그 사이 도착한 F_MORE 완료는 순서대로 처리된다(1.18 의 실행 순서 보장).
  - RECV 모드의 버퍼는 **기존 단일 RECV 규칙 그대로**(`_process_prepare_receive_buffer` : 기본 1KB, 창을 다 채우면 x8 로 1KB -> 8KB -> 64KB, 메시지가 더 크면 그 크기, 작은 수신 64번이면 축소). 전환 때 미리 키우지 않는다(사용자 결정).
  - RECV 모드에서 provided buffer 절반보다 작은 수신이 16번 이어지면(`COUNT_SMALL_RECEIVES_TO_MULTISHOT`) 다시 multishot. 새 접속은 multishot 으로 시작(풀 재사용 socket 은 `process_start_receive()` 에서 초기화).
  - `Iio_context::provided_buffer_size()` 추가(기본 0 = 전환 없음). `CGDK_IO_URING_RECV_ADAPTIVE=0` 이면 전환을 끈다(항상 multishot, A/B 용). 테스트용 `is_receive_multishot_mode()`.
- **테스트** `linux_io.io_uring_completion_receive_mode` : 64B 왕복 20번 multishot 유지 -> 64KB/256KB 왕복에서 RECV 모드 -> 64B 왕복 40번 뒤 multishot 복귀 -> 큰·작은 메시지를 섞은 파이프라인 -> 모든 메시지 순서·내용 비교. `CGDK_IO_URING_RECV_ADAPTIVE=0` 으로 돌리면 이 테스트가 RECV 모드 기대에서 실패함을 확인했다(전환을 실제로 검사한다).
- **검증** (r11458 + 이 변경, Debug. 작업 사본에 있던 다른 세션의 미커밋 `api.io_context.uring_cq.cpp` / `acceptor_tcp*.cpp` 변경은 커밋판으로 바꿔서 검증) :

| 장비 | io_context_uring_cq x3 | completion x3 | epoll | readiness io_uring |
|---|---|---|---|---|
| WSL2 (kernel 6.6, 8 CPU) | 11/11 x3 | 122 x3 (+emergency 1) | 121 (+emergency 2) | 121 (+emergency 2) |
| mistral (kernel 7.0, 32 CPU) | 11/11 x3 | 122, 123, 123 (+emergency 2~3) | 중지 | 중지 |

  - 실패는 모두 `emergency.acceptor_own_executors` / `acceptor_works_while_system_executor_is_jammed` / `acceptor_stuck_session_and_exception` 이다. r11457 메시지에 따르면 그 변경이 없는 HEAD(r11455)에서도 세 모드 모두 실패하던 기존 문제이고, 이 변경과 관계없다(남은 일).
  - mistral 의 epoll / readiness 실행과 **Release 벤치(전환 on/off 교대)는 사용자 요청으로 중지했다 : 이 변경의 성능 효과는 아직 재지 않았다.**
- **다음** : mistral Release 벤치로 `CGDK_IO_URING_RECV_ADAPTIVE=1/0` 교대(tcp 64KB x4 가 기대 효과, 64B x32 / uds / 16B x256 은 변화 없어야 함). 스크립트는 scratchpad 의 `ad_verify.sh` 와 같은 흐름(Debug 검증 뒤 `CGDK10.unit_test.net_socket.http.Release` 로 `CGDK_BENCH=3 CGDK_BENCH_THREADS=8`, 5라운드). 이후 벤치는 모두 Release 로 한다(사용자 지시).
- `receivable::Nstream` 헤더(필드)와 `Iio_context`(가상 함수)가 바뀌었다 : gruma 등 재빌드.

### 1.20 emergency.* linux 실패 정리 + io_uring 의 post 정체 (r11464, r11465, 2026-10-23)

1.19 에서 "기존 실패"로 남긴 `emergency.*` 3개를 정리했다. 2개는 테스트 전제 문제였고, 그 테스트가 **io_uring 두 백엔드의 실제 버그**를 드러냈다.

- **io_uring 백엔드 (r11464)** : 막히는 executable 하나가 executor 의 모든 post 를 멈췄다.
  - readiness(`api::io_context_uring`) : post eventfd 의 poll 을 CQE 를 받은 thread 가 **배치를 다 실행한 뒤**(flush_pending) 다시 걸었다. `executable_post` 는 큐가 남으면 eventfd 에 다시 신호를 주어 다른 thread 가 나머지를 가져가게 되어 있었지만 poll 이 없어 아무도 받지 못했다(66 thread 중 1개만 post 처리). => post eventfd 는 `pop_events()` 가 CQE 를 받는 즉시 다시 걸고 바로 제출(소켓은 그대로 처리 뒤 re-arm : 같은 소켓을 두 thread 가 처리하지 않기 위해).
  - completion(`api::io_context_uring_cq`) : post 큐는 ring 하나에 wake 를 보내고 그 ring 이 가져갈 때까지 신호를 켜 둔다. ① `_wake_ring()` 이 **자기 ring** 을 고를 수 있었다 : 자기 ring 의 wake 는 다음 `pop_results()` 에서야 제출되는데 이 thread 는 막히는 post 를 실행하려는 참 => wake 가 SQ 에 남고 이후 post 는 아무 ring 도 깨우지 않음. ② 늦게 ring 을 만든 worker(`bind_worker`)는 신호가 켜져 있으면 큐를 가져가지 않았다. => 커널에서 기다리는 ring(`RING::is_waiting`)을 먼저, 자기 ring 은 다른 ring 이 없을 때만 / 큐가 비어 있지 않으면 새 ring 이 가져감.
- **테스트 (r11465)** :
  - `acceptor_own_executors` : 준비된 socket executor 수는 windows 만 2/1(prepare_on_start/must_prepare). linux 는 accept 를 미리 준비하지 않아 접속한 socket 만 executor 를 갖는다.
  - `acceptor_works_while_system_executor_is_jammed` : linux executor 는 한 thread 가 post 여러 개를 한 배치로 가져가므로(적응형 event 배치) "thread 수만큼 post = 모두 막힘" 이 아니다 => 모든 thread 가 막힐 때까지 더 post.
  - 포트 33400~ 은 linux 임시 포트 범위(32768~) 안이라 다른 테스트의 client socket 과 겹치면 acceptor 가 조용히 port+1 로 옮겼다 => 27400 + (pid % 100) * 8, 재할당 끔. **새 테스트 포트는 32768 아래, 다른 테스트 범위(28400·29400·30400~31899·32400~)와 겹치지 않게.**
- **검증** (mistral kernel 7.0, 32 CPU, Debug) : jam 테스트가 원본은 readiness 5/5 실패·completion 실패, 수정 후 세 백엔드 통과. net_socket.http 전체 2회 : epoll 125/125 x2, completion 125/125 x2, readiness 125/125 + 1회차 기존 간헐 실패 2개(아래 남은 일 13, 12). system.object.execute `*uring*` 11/11 x2. windows Release emergency.* 8/8.
- **조사했으나 남긴 것** : readiness 의 `emergency.manager_signals` 간헐 실패(원본 7/24, 수정 후 9/24 : 이 변경과 무관). 남은 일 13.

### 1.21 MSG_ZEROCOPY(readiness io_uring) - hold 전에 읽힌 통지 (r11468, 2026-10-23)

1.17 에 "조사 필요"로 남긴 `linux_io.tcp_zerocopy_large_messages_io_uring` 간헐 실패(pending 1 이 안 끝남)의 원인과 수정.

- **개념** : `SO_ZEROCOPY` 를 켠 socket 에 `send(MSG_ZEROCOPY)` 하면 커널은 버퍼를 복사하지 않고 페이지를 고정한다. 커널은 zerocopy 송신을 0, 1, 2 ... 로 세고, 다 쓴 범위 [ee_info, ee_data] 를 error queue 에 통지한다(`recvmsg(MSG_ERRQUEUE)`, epoll 에서는 EPOLLERR). 그때까지 버퍼 소유자를 잡아 두어야 한다(`Itcp_gather::m_queue_zerocopy`, id = 그 송신의 카운터 값). loopback 은 커널이 송신 안에서 복사하고 곧바로 "copied" 로 통지한다. completion 백엔드의 `IORING_OP_SEND_ZC` 는 error queue 대신 F_NOTIF CQE 로 통지하고, 소유자는 op 제출 **전에** executable 에 넣는다(이 문제 없음).
- **원인** : readiness 경로(`tcp_async_gather.epoll.cpp`, `tcp_async_gather_buffered.epoll.cpp`)는 `send()` 가 돌아온 **뒤에** `_zerocopy_hold()` 로 id 와 소유자를 큐에 넣는다. 그 사이 다른 worker 가 EPOLLERR 로 `process_complete_error_queue()` 를 먼저 돌면 통지된 id 가 아직 큐에 없어 아무것도 지우지 않고, 곧 들어간 노드는 다시 통지되지 않아 socket 이 닫힐 때까지 남는다(pending 1, 여러 번 겹치면 13 도 봤다). 송신 결과(부분 송신 = 카운트됨, EAGAIN = 안 됨)를 알아야 id 가 정해지므로 send 전에 넣는 방식은 되돌리기가 복잡하다.
- **수정** (`net.io.sendable.Itcp_gather.{h,cpp}`) : 통지 범위가 아직 hold 되지 않은 id(`m_zerocopy_id_next` 이상)까지 닿으면 그 끝을 `m_zerocopy_id_notified_end` 에 기억하고, `_zerocopy_hold()` 는 자기 id 가 그 범위 안이면 큐에 넣지 않고 소유자를 바로 놓는다. 송신 thread 는 한 번에 하나(m_flag_sending)이므로 hold 안 된 id 는 많아야 하나지만 범위 끝으로 일반화했다. `_zerocopy_reset()` 이 함께 지운다. 헤더 멤버 추가 : gruma 등 재빌드.
- **검증** (mistral kernel 7.0, 32 CPU, Debug, r11461 + 수정, `--gtest_filter=linux_io.tcp_zerocopy*`) :

| | readiness 1차 | readiness 2차 | CPU 부하(busy loop 24) 교대 | completion |
|---|---|---|---|---|
| 수정 전 | 400회 중 4 실패(pending 1, 13) | 500회 0 실패 | 1000회 중 1 실패(pending) | 200/200 |
| 수정 후 | 500회 : pending 실패 0, "received 0" 3 | 1000/1000 | 1000/1000 | 498/498 |

  - 수정 후 1차의 **"received 0"**(30초 동안 에코가 하나도 안 옴) 3건은 한 구간(약 240~250번째)에 몰려 났다 => 원인은 zerocopy 송신의 ENOBUFS(사용자 단위 memlock 한도)로 연결이 닫힌 것, 해결 r11490(1.28). 아래는 당시 기록. 진단 출력(접속·끊김 사유·서버 수신 바이트)을 넣은 2000회와 수정 전 1900회에서는 다시 나오지 않았다. 남은 일 13(readiness 에서 수신 처리 중 모은 송신이 다음 수신까지 안 나감)과 같은 증상일 수 있다 : 서버가 에코를 모아 두고 flush 하지 않으면 클라이언트는 아무것도 못 받는다.
  - 같은 장비에서 테스트 바이너리를 여러 개 동시에 돌리면 이 테스트는 서로 간섭해 대량 실패한다(`lio_send` false) : 반복은 직렬로.
- **코드 읽기로 본 것(미확인)** : `socket::Ntcp::process_execute_epoll()` 은 첫 이벤트의 EPOLLERR 만 `process_complete_error_queue()` 로 거른다. 처리 중에 큐잉된 이벤트(`m_epoll_event_flag_queued`)에 EPOLLERR 가 섞여 들어오면 걸러지지 않고 5-2 의 끊기(DISCONNECT_REASON_FAIL)로 간다. 처리 중에는 re-arm 을 하지 않으므로(ONESHOT, `post_epoll_event_EPOLLOUT` 도 플래그만) 실제로 생기는지는 확인 못 했다.

### 1.22 system.object - executor 깨우기·scheduler 락·정지 지연 (r11470, r11472, 2026-10-24)

2026-10-20 "최적화가 가장 필요한 부분" 분석에서 남은 실행기 쪽 후보 3개. Windows·Linux 공통.

- **queue_list 깨우기 (r11470, `executor.queue_list.cpp`)**
  - `process_post()` 는 queue 가 비어 있다가 찰 때만 thread 하나를 깨운다. 그래서 한꺼번에 들어온 일을 깨어난 thread 하나가 혼자 처리하고, 나머지는 wait 의 timeout(`queue_list_threads` 는 1초)까지 잠들어 있었다. **`queue_list_threads` 를 쓰는 odbc querier 에서 blocking query 가 몰리면 사실상 하나씩 실행됐다.**
  - `pop_queued_executable()` 이 꺼낸 뒤에도 queue 에 남아 있으면 기다리는 thread 를 하나 더 깨운다(chain : 남은 일 수만큼 차례로 깸, 기다리는 thread 가 없으면 notify 는 아무 일도 안 함). post 쪽 규칙은 그대로.
  - 측정(FOEHN, idle thread 4개에 blocking executable 4개, 4개가 모두 실행을 시작할 때까지) : 950ms -> 약 1.1ms.
- **scheduler 의 락 안 post (r11470, `executor.scheduler.cpp`)**
  - 실행 시각이 된 executable 을 `m_lockable_priorityqueue` 를 잡은 채 하나씩 post(PostQueuedCompletionStatus / eventfd syscall) 해서, 그동안 다른 thread 의 `post_at()` / `cancel()` 이 그 락을 기다렸다.
  - 락 안에서는 꺼내 모으기만 하고(object_ptr 이동) 락 밖에서 실행 시각 순서대로 post. 다음 실행 시각까지 기다려야 하는데 꺼낸 것이 있으면 기다리기 전에 먼저 post. post 하나가 예외를 던져도 나머지를 post 한 뒤 첫 예외를 다시 던진다.
  - `m_pexecutor` 를 `m_lockable_pexecutor` 없이 읽던 것을 `has_executor()` / `executor()` 로.
  - `cancel()` 의미는 같다 : 꺼낸(= 실행이 확정된) 뒤의 `cancel()` 은 false(전에는 post 가 끝날 때까지 기다렸다가 false).
- **queue_list_threads::stop() 지연 (r11472, `executor.queue_list.{h,cpp}`, `executor.queue_list_threads.cpp`)**
  - `stop()` 은 `m_is_active` 만 false 로 바꾸고 기다렸다. wait 조건이 'queue 가 비어 있지 않음' 뿐이라 깨워도 다시 잠들어, 각 thread 가 1초 timeout 뒤에야 끝났다(odbc querier 종료, 실행기를 만드는 테스트마다 약 1초).
  - `queue_list` 에 `m_is_wait_cancelled`(`m_lockable_complete` 안에서 읽고 씀) + `cancel_wait(bool)`. true 면 wait 조건이 바로 참(꺼낼 것이 없으면 false 반환)이고 기다리던 thread 를 모두 깨운다. `queue_list::start()` 가 false 로 되돌린다(stop 뒤 다시 start 가능).
  - `queue_list_threads::stop()` 은 `m_is_active = false` 직후, `start()` 의 thread 생성 실패 경로도 `cancel_wait(true)`.
  - **헤더 멤버 추가 : CGDK 를 쓰는 쪽(gruma 등) 재빌드 필요.**
- **테스트** (`unit_test.object.execute/test_executor.cpp`, 모두 수정 전 코드에서 실패 확인) : `queue_list_threads_burst_parallel`(수정 전 950ms), `queue_list_threads_stop_wakes_waiting_threads`(수정 전 stop 950ms / 999ms), `scheduler_many_due_with_concurrent_cancel`(같은 시각 1000개 + 50ms 에 걸친 1000개가 한 번씩, 다른 thread 의 post_at+cancel 반복은 모두 성공·미실행). `message_transmitter_detach_while_transmitting` 은 송신 thread 가 한 번 돌기 전에 등록/해제 2000번이 끝나면 count 0 으로 실패하던 타이밍 결함을 고침.
- **검증** : Windows VS18 x64 execute Release·Debug 61/61. Linux FOEHN(gcc 15.2.1, r11462 export + 변경 파일) execute Release·Debug 61/61, `CGDK10_executor.*` + `CGDK10_schedulable.*` x5, server_system.time_event 17 / service 174 / log 24, server_query 14 통과. 그때 본 net_socket.http 의 `emergency.*` 2개 실패는 r11462 트리라서다(HEAD 라이브러리로 되돌려도 같음, r11464·r11465 에서 해결 : 1.20).

### 1.23 io_uring - 남은 일 13 의 진짜 원인 : io_uring 이 요청·ring 을 가진 thread 의 blocking 호출을 EINTR 로 끊는다 (r11474, 2026-10-24)

- **증상 재해석** : readiness `emergency.manager_signals`(mistral `emergency.*` 반복 4/24) 는 "응답이 다음 수신 때 나감"이 아니었다. 테스트 클라이언트(OS blocking socket, `SO_RCVTIMEO` 3초)의 `recv` 가 **-1 EINTR** 로 바로 돌아왔다(errno 를 찍어 확인, 테스트 전체 440ms). 서버는 응답을 보냈고 다음 `ask` 가 그 응답을 받아 하나씩 밀린 것처럼 보였다. `acceptor_own_executors` 의 같은 모양 실패(70ms)도 같은 원인.
- **커널 규칙** (POKPUNG kernel 7.0, C 프로브 `tw_probe*.c` 각 20회. ring 을 **실제로 해제**할 때(munmap + close) 잰 값이다 : mmap 이 남아 있으면 해제되지 않아 잘못 잰다) :

| thread 가 한 일 | 그 thread 의 타임아웃 있는 blocking 호출 |
|---|---|
| 제출한 poll 이 그 thread 가 막혀 있는 동안 완료 | 20/20 EINTR (완료가 그 thread 의 task_work) |
| ring 생성 | ring 해제 때 20/20 EINTR |
| NOP 하나라도 제출, MSG_RING 제출 | 그 ring 해제 때 20/20 EINTR |
| 제출한 poll 이 해제 때 걸려 있음 | 40/20 (취소 완료 + 해제) |
| GETEVENTS 대기만, SYNC_CANCEL register, 다른 thread 가 poll 하는 eventfd 쓰기 | 0 |

  `TIF_NOTIFY_SIGNAL` 은 잠든 thread 를 깨운다(COOP_TASKRUN 이어도). 잰 것은 `SO_RCVTIMEO` recv 뿐이다 : 커널 소스로는 -EINTR 을 그대로 돌려주는 호출(타임아웃 있는 socket 호출, `epoll_wait`)이 사용자에게 보이고, -ERESTARTSYS 계열(타임아웃 없는 recv, nanosleep, futex)은 다시 시작된다(미측정).
- **CGDK 에서 걸린 경로** : readiness 는 ① `create()` 가 post eventfd poll 을 만든 thread 에서 제출(그 executor 의 post 가 그 thread 를 끊음) ② worker 가 아닌 thread(main, emergency manager, 다른 executor 의 worker)가 attach/modify/detach 때 바로 enter(남의 지연 SQE 까지 제출) ③ `destroy()` 의 깨우기 NOP ④ 생성 thread ⑤ `is_available()` 의 시험 ring(프로세스에서 처음 부른 thread). emergency.* 에서는 **앞 테스트에서 main 이 만든 acceptor executor 가 해제될 때 다음 테스트의 main recv 가 끊겼다**(그래서 단독 실행은 통과). completion 은 비소유 thread 가 ring 에 제출하지 않지만(1.16/1.17) wake ring 을 만든 thread, 깨우려고 MSG_RING 을 제출한 모든 thread(post 한 thread, stage 한 thread, stop 한 thread)가 executor 해제 때 끊겼다.
- **수정** (`api.io_context.uring{,_cq}.{h,cpp}`, `executor.io_context_threads.cpp`) :
  - 두 백엔드의 `_io_uring_setup()` 은 **곧 끝나는 임시 thread** 에서 ring 을 만든다(errno 전달). 끝난 thread 는 커널이 붙들지 않는다.
  - readiness : `io_context_threads::start()` 가 thread 를 띄우기 전에 `expect_workers()`, 각 worker 는 `bind_worker()/unbind_worker()`(transient 도). 그 뒤 **worker 가 아닌 thread 는 ring 에 enter 하지 않는다** : SQE 는 SQ 에 두고 post eventfd 에 직접 써서(`executable_post::wake()`, `m_is_wake_submit` 로 worker 가 제출할 때까지 한 번) worker 를 깨운다. eventfd CQE 를 받은 worker 는 eventfd poll 을 다시 걸며 SQ 를 바로 제출한다. `pop_events()` 도 아무것도 가져가지 않고 2ms 까지 기다린다(wait_complete_* 펌핑은 worker 가 처리할 때까지 기다림). `create()` 는 제출하지 않는다(첫 worker 가 제출). `destroy()` 는 미제출 SQE 를 NOP 로 바꾸고, worker 가 아니면 NOP 대신 eventfd 로 깨운다. `detach()` 는 미제출 SQE 중 그 등록의 POLL_ADD 를 NOP 로, POLL_REMOVE 도 NOP 로 바꾸고 그 대상(커널에 있는 옛 poll)은 sync cancel(`RING::neutralize_registration`). SQ 가 차면 worker 가 아닌 thread 는 100ms 까지 worker 를 기다린 뒤 직접 제출(최후 수단). expect_workers() 없는 io_context(직접 펌핑)는 예전처럼 아무 thread 나.
  - readiness 의 지연 제출 플래그 `t_defer_submit`(전역 bool) -> `t_pdefer_context`(io_context 별) : 한 executor 의 worker 가 다른 executor 의 SQE 를 자기 배치 끝까지 미루던 문제도 사라졌다(그 executor 의 worker 를 깨운다 : 예전 메모의 "t_defer_submit 이 다른 io_context 의 SQE 까지 미룸").
  - completion : MSG_RING wake ring 을 없애고 **ring 마다 wake eventfd**(`RING::wake_fd`)를 owner 가 multishot POLL_ADD 로 감시한다(`write_wake_poll`, bind_worker 때 무장, 끝난 poll 은 같은 세대일 때만 owner 가 다시 무장 : `UD_WAKE_POLL | 세대 << 16 | index`). 다른 thread 는 eventfd 에 쓰기만 한다(`_send_wake(RING*)`). post / stage / dispose / destroy 의 깨우기가 모두 이것. `is_available()` 은 MSG_RING 을 더 보지 않는다.
- **테스트** `linux_io.io_uring_requests_of_the_workers_only` / `io_uring_completion_requests_of_the_workers_only` : 테스트 thread 가 executor 를 만들고 acceptor 를 시작한 뒤 `SO_RCVTIMEO` 20ms recv 를 반복하는 동안 다른 thread 가 post 2000 + 접속 20, 그 다음 테스트 thread 가 acceptor·executor 를 멈춘 뒤 recv 15번. 두 단계 모두 EINTR 0 이어야 한다(실패 메시지에 끊긴 시각).
- **검증** (mistral kernel 7.0, 32 CPU, Debug, r11473 + 변경) :
  - 수정 전 백엔드 + 새 테스트 x10 : readiness 10/10 실패(EINTR 2~4회 : post·접속 중, 해제 때), completion 1/10(시각이 맞을 때만 : wake ring 해제는 대개 thread 가 막히기 전).
  - 중간 단계에서 배운 것 : worker 만 enter 로는 readiness 가 20/20 그대로(생성 thread), 생성 thread 까지 고치고 `destroy()` 의 NOP 를 남기면 20/20(stop 한 thread), 둘 다 고쳐야 0.
  - 수정 후 : 새 테스트 2개 x20 통과. readiness `emergency.*` x60 : **실패 0** (수정 전 4/24, 중간 단계 7/60·10/60). system.object.execute 전체 72 x2. net_socket.http 전체 Debug readiness x2 / epoll / completion x3 129 통과, Release readiness 129 통과. Release completion 1회에 send_cork 2개 실패 : 수정 전 r11473 에서도 mistral completion 에서 나는 간헐 실패(남은 일 16).
- **동작 변경** : readiness io_uring executor 를 worker 가 아닌 thread 가 `execute()` 로 펌핑하면 이벤트를 처리하지 않는다(기다리기만 함). 다른 executor 의 thread 가 attach/send(EAGAIN) 하면 eventfd 쓰기 하나가 더 든다. 헤더 멤버·함수 변경(`io_context_uring`, `io_context_uring_cq`, `io_context_linux`) : gruma 등 재빌드.
- 이 조사 중 본 것 : MONSOON 은 2026-10-24 루트 파일시스템이 I/O 오류로 `emergency_ro`(읽기 전용)가 되어 쓸 수 없었다(sudo 필요, 손대지 않음. 재부팅 뒤 다시 쓸 수 있다). r11471 메시지의 MONSOON completion 실패 3개 중 `uds.stop_while_connecting` 과 같은 프로세스에서 뒤이은 `uds.many_tcp_connects_without_bind` 는 r11462 트리였다.
  - 정정 (2026-10-24, MONSOON Debug 교대 6회) : 이 둘을 고친 것은 r11464 가 아니라 **이 수정(r11474)** 이다. `stop_while_connecting` 은 r11462·r11473 6/6 실패(r11464 도 2회 중 1회 실패), r11474·r11475·r11478 6/6 통과. `many_tcp_connects_without_bind` 는 단독으로는 r11462 도 통과한다(앞 테스트가 남긴 post 적체에 걸린 것).
  - 원인 (r11473 계측) : post 큐 적체. 테스트 thread 의 loopback 접속은 UDS 로 즉시 성공(400/400)하고 완료만 post 된다. 같은 때 closer thread 의 접속 start/stop 폭주가 post 를 쌓는데, worker 들은 깨우기 1회당 post 1개(executor 배치 `t_count_event_batch` 1 -> `_pop_posted` 의 남은 칸 1, 큐 신호는 ring 하나씩)로만 가져가(깨우기 176만 ~= 실행 post 185만) 큐가 60만~160만으로 자랐고 완료 post 가 3초 넘게 기다렸다. MSG_RING 깨우기 실패 0, `m_lockable_wake` 대기 평균 ~250ns(경합 아님). r11474 는 같은 폭주에서 큐 ~20 이고 테스트가 1~2초에 끝난다. eventfd 깨우기 한 칸이 왜 그만큼 빠른지는 재지 않았다("깨우기 1회당 post 1개" 구조는 HEAD 에도 남아 있다).

### 1.24 io_uring completion - post 의 깨우기가 바쁜 ring 으로 가면 다른 worker 가 쉬어도 기다리던 문제 (r11475, 2026-10-24)

- **증상** : `emergency.acceptor_works_while_system_executor_is_jammed`(system executor 의 모든 thread 가 jam 을 하나씩 받아야 함)가 POKPUNG r11473 completion 전체 스위트 8회 중 3회 실패(`wait_jammed()` false : 5초 안에 다 막히지 않음). 단독 x20 은 통과.
- **원인** : post 큐는 ring 하나를 깨우고 그 ring 이 가져갈 때까지 신호(`m_is_signaled`)를 켜 둔다(1.20). `_wake_ring()` 은 커널에서 기다리는 owner 를 먼저 고르지만 그 순간 하나도 없으면 **바쁜 ring** 을 깨운다. 그 owner 가 긴 executable(jam : 최대 10초) 안이면 큐의 post 는 그 owner 가 돌아올 때까지 실행되지 않고, 뒤이어 쉬게 된 worker 들은 자기 ring 만 기다린다(신호가 켜져 있으니 새 post 도 아무도 안 깨움). readiness 는 ring 하나를 모든 worker 가 기다리므로 해당 없음.
- **수정** (`api.io_context.uring_cq.{h,cpp}`) : owner 가 커널에서 기다리기 직전 `is_waiting` 을 먼저 켜고(seq_cst) `m_count_post`(큐 크기, 큐 lock 안에서 갱신)를 본다. 있으면 기다리지 않고 한 배치를 가져간다(`_pop_posted(..., true)` : 깨운 ring 과 같이, 남으면 다음 ring 을 깨움). post 쪽은 큐에 넣은 뒤 `is_waiting` 을 seq_cst 로 읽으므로(`_wake_ring` 첫 바퀴) 확인 뒤에 온 post 는 이 ring 을 기다리는 ring 으로 보고 깨운다(Dekker 식 : 놓침 없음). 기다리지 않을 때(제출할 것·task work 없음)는 enter 도 건너뛴다.
- **테스트** `io_context_uring_cq.post_taken_by_a_worker_going_idle` : worker 2개, W1 은 막히고 W2 는 50ms 짧은 일 중일 때 온 post 가 1초 안에 시작해야 한다(20라운드). 수정 전(r11474) 3회 모두 실패(19/20 라운드 지연, 2·3회차는 앞의 짧은 일조차 바쁜 W1 로 가서 시작 못 함). 이 테스트가 결함을 결정적으로 재현한다.
- **검증** (POKPUNG kernel 7.0, 32 CPU, Debug, r11474 + 변경) : 새 테스트 x5, execute 전체 73 x2, `emergency.*` completion x10, jam 단독 x20, net_socket.http 전체 completion x4 / epoll / readiness 129 통과. 참고 : 수정 전 r11474 의 completion 전체 x6 은 jam 실패 0(r11474 의 eventfd 깨우기 뒤로 그 테스트 맥락의 빈도가 줄었다 : r11473 은 3/8).
- 헤더 멤버 추가(`m_count_post`) : gruma 등 재빌드.

### 1.25 send cork 테스트의 completion 간헐 실패 = 클라이언트가 서버와 같은 ring (r11477, 2026-10-24)

- **증상** (남은 일 16) : completion 에서 `linux_io.send_cork_sends_old_responses_{gather,buffered}` 가 가끔 **둘이 연달아** 실패(24개 응답이 배치 끝에 한꺼번에 도착 : spread 13~105us, 기대 >= 1ms). mistral 에서만, 전체 스위트 맥락에서만(단독 x30 은 통과).
- **진단** (테스트 사본 : 서버 handler 가 echo 마다 `statistics_get_sent_bytes()`·클라이언트가 그때까지 처리한 바이트·`is_ring_owner(client_fd)`·thread id·`sched_getcpu()` 기록) : mistral completion 전체 16회(Debug 8·Release 8) 중 3회 실패(Debug 2, Release 1), 세 번 모두 **`same_ring 1`, 서버 thread = 클라이언트 thread**(그리고 `same_ring 1` 인 회차는 모두 실패). 서버는 배치 중에 3메시지(96B)마다 정상적으로 보냈고(0 -> 96 -> 192 ... 672), 클라이언트가 처리한 바이트는 배치 내내 0 이었다. 통과한 23회(mistral 13·POKPUNG 10)는 모두 `same_ring 0` 이고 클라이언트가 서버 송신 직후 처리했다.
- **원인** : cork 결함이 아니다. 테스트가 서버 송신 시각 대신 **클라이언트의 `on_message` 시각**을 쟀고, completion 은 ring 의 CQE 를 owner thread 하나만 실행하므로 서버와 같은 ring 에 묶인 클라이언트는 서버의 2.4ms handler 가 끝날 때까지 실행되지 못했다. 배치는 socket 이 처음 제출한 worker 의 ring(accept 된 socket = listen ring, 클라이언트 = CONNECT post 를 받은 worker 의 ring)으로 정해지고 round robin 카운터에 따르므로 한 프로세스의 두 테스트가 함께 걸린다(통과 회차에서도 두 테스트가 같은 두 worker 를 역할만 바꿔 썼다). 함께 본 다른 가설 : 스케줄러가 클라이언트 thread 를 서버 CPU 에 올림(wake-affine) - C 재현 300회에 0, 실패 회차는 아예 같은 thread 였다. Nagle / 지연 ACK - TCP_NODELAY 가 기본으로 켜지고 실패·통과의 소요 시간이 같다(20~31ms). 거친 시계 - `chrono::tick` 은 steady_clock.
- **수정** (`test_linux_io.cpp`) : 서버 socket 이 echo 마다 보낸 바이트를 기록하고 "마지막 echo 전에 4번 이상 보냄 + 배치 중간(12번째)까지 3개 이상 보냄"으로 판정한다(클라이언트 도착 시각은 보지 않음). `send_cork_sends_old_responses_one_thread` : 서버·클라이언트를 1-thread executor 에 같이 올린 결정적 변형(클라이언트는 반드시 배치 뒤에 실행되지만 서버는 배치 중에 보냈어야 한다).
- **검증** : POKPUNG Debug 새 테스트 3개 x20 (epoll / readiness / completion) 통과, **cork 만료를 끈 빌드(`SEND_CORK_DURATION` 1시간)에서는 3개 모두 실패**(echo 마다 보낸 바이트 0 ... 0 : r11471 이 잡으려던 결함을 그대로 잡는다). 전체 스위트 POKPUNG Debug completion x3 / epoll / readiness 130 통과. mistral(옛 테스트가 진단 16회 중 3회 실패하던 곳) completion Debug x6 / Release x2 130 통과.
- **드러난 설계 특성** : 남은 일 17.

### 1.26 completion - 블로킹하는 worker 의 ring 을 다른 worker 가 대신 실행한다 (r11481, 2026-10-24)

남은 일 17 중 **블로킹** 경우를 고쳤다(사용자 결정 : CPU 를 오래 쓰는 handler 는 범위 밖).

- **측정** (mistral, 200 접속, 한 접속 handler 가 `scoped_thread_block` 안에서 300ms) : 수정 전 completion 은 다른 접속 최대 **270ms**(4/4), 수정 후 0.6~2ms. 프로브(17 접속, 200ms)에서 같은 ring 에 걸린 접속은 CPU 사용 때 178ms, 블로킹 때 수정 전 176ms -> 수정 후 0.6ms.
- **동작** (`api.io_context.uring_cq.cpp`) :
  - `io_context_threads::process_control_thread_blocked/unblocked` -> `io_context_linux::thread_blocked/unblocked` -> `io_context_uring_cq::thread_blocked/unblocked`. 가장 바깥 블록에서 owner 의 ring 을 `STATE_LENT` 로 빌려준다(되찾기는 가장 바깥 unblock).
  - 블로킹 중에도 task work 가 잠든 owner 를 깨우므로(1.23) CQE 는 CQ 에 올라온다. 다른 worker 가 고아 ring 과 같은 경로(`_drain_orphans` -> `_take_lent`, `STATE_DRAINING_LENT`, `t_draining` 로 실행 끝까지 잡음)로 실행한다. 빌린 ring 이 있으면 idle worker 대기는 1ms(`LENT_WAIT`), 빌려줄 때 하나를 바로 깨운다. staged SQE 는 그 worker 의 ring 으로 옮긴다(`_adopt_staged`).
  - **순서** : owner 가 막힌 executable(`RING::pblocked`)의 결과는 대신 실행하지 않고 owner 에게 미룬다(`RING::deferred`). owner 가 막히기 전에 받은 배치의 나머지(`RING::carry`)는 대신 실행하는 worker 가 CQ 보다 먼저 실행한다 - executor 가 실행 중인 배치를 `api::io_context_uring_cq::current_batch()`(`BATCH`)로 보이게 하고 `thread_blocked()` 가 나머지를 옮기며 배치 수를 줄인다. 한 executable 의 결과는 동시에 실행되지 않는다(IOCP 와 같은 규칙).
  - owner 는 풀리면 ring 을 되찾고(대신 실행하던 worker 가 있으면 그 배치가 끝날 때 : `is_reclaim`), 다음 `pop_results()` 가 carry·deferred 를 CQ 보다 먼저 실행한다(다 못 내면 CQ 는 다음 번). 빌려준 동안 owner 의 pump(`wait_complete_*`)는 post 큐만, `is_ring_owner()` 는 false(송신은 op 대신 바로 send).
- **CPU 를 오래 쓰는 handler 는 그대로** => 1.34 에서 해결. 아래 "CQE 자체가 올라오지 않는다" 는 틀린 설명이었다(바쁜 thread 도 타이머 tick 에 task work 를 실행 : 1.34). 원래 문장 : `COOP_TASKRUN` 이라 owner 가 사용자 공간에서 바쁘면 CQE 자체가 올라오지 않는다(대신 실행할 것이 없음). 끄면 IPI 비용이 생긴다. completion 서버의 handler 는 짧게, 긴 계산은 다른 executor 로.
- **테스트** `test_blocking_handler.cpp` (`blocking_handler.does_not_hold_the_other_sockets`, 프로세스의 backend 로 돈다) : 200 접속 중 하나가 300ms 블로킹하는 동안 나머지 199개가 100ms 안에 응답(같은 thread 의 하나는 8개 파이프라인 순서), 막힌 접속에 그 사이 보낸 4개가 풀린 뒤 순서대로. 수정 전 라이브러리 completion 4/4 실패.
- **검증** (mistral kernel 7.0, Debug, r11480 + 변경) : net_socket.http 전체 epoll / io_uring / completion x2 131 통과, system.object.execute epoll / completion x2 73 통과, windows system.object 컴파일. 미실행 : ASan / valgrind, Release 벤치.
- **함께 드러난 것** : completion 에서 accept 된 socket 200개가 **모두 listen socket 의 ring(thread 하나)** 에 있었다(남은 일 18).
- 헤더 변경(`io_context_uring_cq` 멤버·함수, `io_context_linux` 함수) : gruma 등 재빌드.

### 1.27 completion - accept 된 socket 을 accept CQE 에서 round robin ring 에 바인딩 (r11487, 2026-10-24)

남은 일 18 을 고쳤다.

- **원인** : completion 은 handle 을 **처음 제출한 worker 의 ring** 에 묶는다(`HANDLE_ENTRY::binding`, `_ring_of_handle` : 지역성). accept 완료는 listen ring 의 owner 가 실행하고 그 handler(`process_connective_accept_cq` -> `process_start_io` -> 첫 RECV)가 제출하므로 한 acceptor 의 socket 이 모두 listen ring 에 묶였다. 완료 모드의 close 는 바인딩을 지우지 않으므로(readiness 의 detach 만 지움) **fd 번호에 옛 바인딩이 남아** 번호를 다시 받은 socket 은 옛 ring 으로 간다 : 수정 전 같은 프로세스의 뒤쪽 case 가 조금 퍼져 보인 것은 이것(벤치의 "server 5 thr top 56%" 류).
- **수정** (`api.io_context.uring_cq.cpp`) : `_process_cqe` 가 accept op(`OP::opcode`, 제출 때 기록)의 CQE 에서 fd 를 받으면 handler 가 돌기 전에 `_bind_accepted()` 가 그 fd 를 `STATE_OWNED` ring 중 round robin 으로 바인딩한다(카운터는 그 accept 의 ring 의 `RING::index_bind_next` : RING 은 cpp 안이라 헤더 레이아웃 변경 없음). 이후 그 socket 의 op·poll 은 모두 그 ring 으로 간다 : listen ring owner 가 제출하면 staging + 그 ring 의 wake eventfd(다른 ring 에 enter 하지 않음). 옛 바인딩은 덮어쓴다(닫힌 socket 의 op 는 `OP::pring` 을 가진다). armed poll 이 있는 번호는 그대로, owned ring 이 없으면 기존대로 첫 제출이 정한다. 고아·빌린 ring 의 handle 이동 규칙(1.26)은 그대로.
- **"덜 바쁜 ring" 은 쓰지 않았다** : 배치 하나(최대 32 CQE)의 accept 를 모두 바인딩한 뒤에야 RECV 가 제출되므로 ring 의 op 수 같은 부하 지표는 배치 안에서 변하지 않는다(32개가 같은 ring 으로 감). 접속 수 기준은 완료 모드가 바인딩을 풀지 않아 셀 수 없다.
- **벤치 측정 방법이 틀려 있었다** : `bench_linux_io.cpp` 의 서버 socket 은 case 의 executor 가 아니라 **system executor**(기본 epoll, 2 x core + 2 = mistral 66 thread)에서 돌았다. linux acceptor 는 io_context 가 없는 socket 에 acceptor 의 io_context 가 아니라 system executor 를 준다(남은 일 19). 그래서 지금까지의 "uring_cq" 수치(남은 일 8 의 r11459 POKPUNG 벤치, 1.18/1.19, r11486 의 `socket.recv.html`)는 **클라이언트만 completion** 이었다. 이제 `lbn_acceptor` 로 서버도 case 의 executor 에 올리고, case 마다 handler 를 실행한 thread 수와 가장 바쁜 thread 의 몫을 찍는다("server 8 thr top 13%"). 첫 case 전 300ms 대기 : executor 를 만든 직후에는 worker ring 이 다 만들어지지 않아(ring 은 worker 가 시작할 때 생성) 첫 accept 들이 일부 ring 에만 갔다(수정본 1차 측정 r1 "server 4 thr").
- **측정** (mistral kernel 7.0, 32 CPU, Release, `CGDK_BENCH=3 CGDK_BENCH_THREADS=8`, 서버·클라이언트 모두 case 의 executor, 수정 전 / 후 교대 5라운드 중앙값, msg/s) :

| case | epoll 수정 전 / 후 | completion 수정 전 | completion 수정 후 | 수정 후 completion / epoll |
|---|---|---|---|---|
| tcp 64B x32 window16 | 13.64M / 13.71M | 2.48M (server 1 thr top 100%) | **10.58M** (server 8 thr) | -23% |
| tcp 64KB x4 window2 | 162K / 168K | 178K | **210K** | +25% |
| uds 64B x32 window16 | 15.00M / 14.53M | 13.90M | **25.14M** | +73% |
| tcp 16B x256 window1 | 889K / 877K | 878K | **1.18M** | +34% |
| udp 64B x16 window32 | 395K / 390K | 579K | 573K (무관 : socket 하나) | +47% |

  - 같은 날 settle 없이 잰 1차 A/B(다른 세션의 zerocopy 반복 테스트가 함께 돔)도 방향이 같다 : tcp 64B 2.26M -> 5.23M, uds 11.7M -> 22.2M, 16B x256 672K -> 1.19M. 1차의 tcp 64B 는 1.6M~7.5M 으로 흔들렸고 클라이언트 thread 수(3 ~ 8)와 같이 움직였다(첫 case 의 ring 미생성 + CONNECT post 의 배치, 남은 일 20).
  - 수정 전 completion 의 tcp 64B 는 서버 처리가 thread 하나에 몰려 epoll 의 1/5 였다 : 실서버에서 completion 을 켜면 acceptor 하나의 접속 전부가 thread 하나로 묶였다는 뜻.
- **테스트** :
  - `io_context_uring_cq.op_accept_binds_the_accepted_sockets_round_robin` (system.object.execute) : 4 worker, 32 접속, accept handler 가 각 socket 의 RECV 를 제출 -> RECV 완료가 4 thread 에 8개씩. 수정 전 3/3 실패(32개가 1 thread).
  - `linux_io.io_uring_completion_accepted_sockets_spread` (net_socket.http) : completion executor(4 thread) 위 acceptor(`lio_thread_acceptor` : socket 을 acceptor 의 io_context 에) + 64 접속, TCP 와 UDS -> 서버 socket 이 4 thread 에 고르게(최대 16+2), socket 마다 thread 하나. 수정 전 TCP 64/64 가 1 thread(UDS 는 옛 바인딩 때문에 4 thread 지만 최대 34~35).
  - `blocking_handler.does_not_hold_the_other_sockets` : 분포 출력을 단언으로(**completion 일 때만** : thread 2개 이상, 막힌 접속의 thread 에 절반 미만). 수정 후 completion 3/199(66 thread). **epoll 은 단언하지 않는다** : epoll 은 마지막에 기다린 thread 를 깨우므로(LIFO) 하나씩 주고받는 echo 가 한 thread 에서 돈다(198~199/199, socket 의 성질이 아님). readiness io_uring 은 0~6/199.
- **검증** (mistral, r11485 export + 변경, Debug) : net_socket.http 전체 epoll / io_uring / completion x2 132 통과, system.object.execute epoll / completion x2 74 통과. 미실행 : ASan / valgrind(바뀐 파일은 모두 linux 전용이라 windows 빌드 불필요).
- 헤더(`api.io_context.uring_cq.h`)는 private 함수 선언 하나와 주석만 바뀌었다 : 클래스 레이아웃 그대로, gruma 재빌드 불필요.

### 1.28 zerocopy 송신의 ENOBUFS = 사용자 단위 memlock 한도 : 복사로 다시 보낸다 (r11490, 2026-10-24)

1.21 에서 원인 미확인으로 남긴 "received 0" 의 원인과 수정.

- **원인** : MSG_ZEROCOPY(readiness)와 io_uring SEND_ZC(completion)가 고정하는 페이지는 **사용자(uid) 단위 RLIMIT_MEMLOCK** 에 합산된다(mistral `ulimit -l` 8192KB). 넘으면 송신이 **ENOBUFS(105)** 로 실패하는데, CGDK 는 이것을 송신 실패로 보고 연결을 닫았다(readiness `process_close_socket(FAIL)`, completion op 결과 `<= 0`). 이 한도는 같은 사용자의 **모든 프로세스**가 나눠 쓴다. 한 프로세스 안에서도 클라이언트·서버가 256KB 송신을 각자 송신 버퍼만큼 고정하면 8MB 근처이고, 다른 세션의 테스트 프로세스가 함께 돌면 넘는다. 닫힌 쪽에 읽지 않은 수신 데이터가 있으면 close 가 RST 를 보내 상대 수신 버퍼까지 버려진다 : 256KB 메시지가 하나도 완성되지 않은 클라이언트는 "received 0". 1.21 의 3건은 다른 세션이 mistral 에서 전체 스위트를 돌리던 시각(19:55 KST)에 났다. 1.21 에 적은 "여러 개 동시에 돌리면 대량 실패" 도 같은 원인이다.
  - 같은 한도 때문에 **io_uring ring 생성도 실패**해 "io_uring" executor 가 epoll 로 대체되는 일이 있다(8개 동시 실행에서 관찰).
  - 1.21 의 다른 후보(남은 일 13 과 같은 flush 누락, t_defer_submit 지연, 포트 재할당, fd 재사용)는 아니었다 : t_defer_submit 경로는 길어야 1초, 포트는 `local_endpoint()` 의 실제 포트로 접속.
- **확인** (mistral kernel 7.0) :
  - C 프로브(`zc_probe.c`, 이 작업의 scratchpad) : `ulimit -l 256` 이면 첫 256KB MSG_ZEROCOPY 송신부터 ENOBUFS. 기본 한도에서도 수신자가 느리거나(recv 마다 1ms) SO_SNDBUF 8MB 면 32~44번째 송신에서 ENOBUFS(error queue 를 안 읽는 프로브).
  - 테스트 프로세스 8개 동시 : readiness 80회 중 72, completion 80회 중 80 이 첫 대용량 송신 errno 105 -> 연결 닫힘 -> 다음 송신 실패.
  - 단독·CPU 굶김·느린 수신자(on_message 지연)·명시적 8MB 버퍼로는 재현되지 않았다 : CGDK 는 EPOLLOUT 처리 앞에서 EPOLLERR(통지)를 비우므로 프로브처럼 통지가 쌓이지 않는다. 남는 계기는 다른 프로세스와 나눠 쓰는 memlock 이다.
- **수정** :
  - `Itcp_gather::_zerocopy_send()` : MSG_ZEROCOPY send 가 ENOBUFS 면 같은 데이터를 MSG_ZEROCOPY 없이 다시 보낸다(flags 를 고쳐 돌려줌 : 소유자 보관은 실제로 zerocopy 로 나간 것만). readiness 송신 4곳(`tcp_async_gather.epoll.cpp` 2, `tcp_async_gather_buffered.epoll.cpp` 2)이 `::send` 대신 쓴다.
  - completion `_process_complete_send_op()` : SEND_ZC 결과가 `-ENOBUFS` 면 같은 버퍼를 SENDMSG 로 다시 제출. 쉬는 중의 ENOBUFS(SENDMSG 의 것)는 실패.
  - ENOBUFS 뒤 그 socket 은 `ZEROCOPY_PAUSE_AFTER_ENOBUFS`(100ms) 동안 zerocopy 를 쓰지 않는다(`_zerocopy_pause`, `_is_zerocopy_paused` : `_is_zerocopy_op` 이 const 이고 여러 번 불려 횟수 대신 시간). `zerocopy_fallback_count()` 추가. **Itcp_gather 헤더 변경 : gruma 등 재빌드.**
- **테스트** : `linux_io.tcp_zerocopy_falls_back_to_copy_on_enobufs`(새) - 접속 뒤 RLIMIT_MEMLOCK 을 0 으로 낮추고 256KB x16 에코, 모두 돌아오고 끊김 없음, fallback > 0. 수정 전 readiness / completion 5/5 실패, 수정 후 5/5. root 면 skip. `tcp_zerocopy_large_messages_io_uring` 은 sent + fallback > 0 으로. 두 테스트 모두 executor 가 epoll 로 대체되면 skip.
- **검증** (mistral, r11489 + 변경, Debug) : 8개 동시 `linux_io.tcp_zerocopy*` x20 readiness 455 / completion 464 통과·실패 0(나머지 epoll 대체 skip). 단독 x50 readiness / completion / epoll 150/150. net_socket.http 전체 epoll / readiness / completion 135 통과(skip 3, 테스트 단언 수정 전 빌드). Windows 는 linux 전용 변경이라 빌드 안 함.
- **실서버 의미** : memlock 한도는 서버 프로세스 전체(같은 계정의 다른 프로세스 포함)가 나눠 쓴다. zerocopy 를 실제로 쓰려면 `ulimit -l`(systemd `LimitMEMLOCK=`)을 송신 버퍼 x 동시 대용량 송신 socket 수만큼 키울 것. 모자라면 이제 복사로 동작하고 `zerocopy_fallback_count()` 가 는다.
- 공유 작업 사본의 `tcp_async_gather{,_buffered}.epoll.cpp` / `.uring.cpp` / `test_linux_io.cpp` 에 다른 세션의 미커밋 수정이 있어 별도 checkout 에서 커밋했다. 그 세션은 `svn update` 로 합쳐야 한다(같은 줄은 건드리지 않음).

### 1.29 completion 송신 경로 - buffered 의 SEND op, CQE 배치 끝 송신(batch send defer) (r11494, r11497, 2026-10-24)

남은 일 8 의 "남은 것 : 송신 쪽". 먼저 이미 들어간 것 : gather(`tcp<>`)는 r11441/r11457 부터 **직접 send + socket 버퍼가 차면 SENDMSG op**(EPOLLOUT 대신), 큰 단일 버퍼는 SEND_ZC op, 선택(기본 off)으로 소유 worker 송신 op(`CGDK_IO_URING_SEND_OP=1`). IOCP 와 같은 모델이다(WSASend 도 대개 그 자리에서 끝나고 막힐 때만 완료를 기다린다). 남은 것은 셋이었다.

- **buffered 가 completion 이 아니었다 (r11494)** : socket 은 receivable·sendable 이 모두 `is_io_completion_supported()` 여야 completion 이 되는데 `Ntcp_async_gather_buffered` 는 false 라 `tcp_buffered<>` / `tcp_queue` / redis querier session 은 completion executor 위에서도 통째로 readiness(POLL_ADD)였다. 이제 gather 와 같은 구조 : EPOLLOUT 대기 3곳을 `_process_post_send_wait()`(completion 이면 SENDMSG op). op 는 `m_queue_pending` 앞(되돌려 둔 나머지 + 그 뒤에 쌓인 것, IOV_MAX 개까지)을 `m_queue_sending` 으로 옮겨 보내고(iovec 은 executable 안), 완료가 소비 -> 남으면 op 다시, 다 보냈으면 `_process_send_pending()` 으로 직접 송신을 잇는다. op 묶음은 큐 한도에 세지 않고(r11361 의미) `sending_queued_bytes_now()` 에는 넣는다(`m_bytes_sending_op`). 사용자 버퍼 하나(>= 16KB, IP)는 SEND_ZC op(-ENOBUFS 면 r11490 처럼 복사로 다시). op 중 reset 은 노드를 executable 의 `m_queue_hold` 로. 새 파일 `net.io.sendable.tcp_async_gather_buffered.uring.cpp`, **헤더 멤버 추가 : gruma 재빌드**.
- **gather 의 op 완료가 대기 바이트를 되돌리지 않았다 (r11494)** : `_process_complete_send_op()` 가 `m_count/bytes_sending_parked` 를 0 으로 안 돌려(epoll 은 `process_recover_sendable` 이 돌림) 다 보낸 뒤에도 `sending_queued_bytes_now()` 가 옛 값(r11441 부터).
- **completion 의 64B x32 열세의 원인 = CQE 마다 send 하나** : 공정 벤치(r11487 : 서버도 case 의 executor)에서 POKPUNG completion tcp 64B x32 가 epoll 의 -37%(9.91M / 15.68M). `strace -c -f`(rate 는 왜곡, 비율만) : epoll 은 recv 한 번이 창(16개)의 메시지를 받아 **송신 1회에 메시지 ~6.6개**(sendto 0.15/msg), completion 은 multishot RECV 가 도착(세그먼트)마다 CQE 를 하나씩 내고 CQE 마다 send cork 가 끝나 **sendto 0.76/msg**. syscall 을 없앤 수신 대신 송신 syscall 이 5배.
  - **배치 송신 defer** (r11497) : completion executor 가 CQE 배치(최대 32, 실행 10us 에 맞춰 적응) 전체를 `send_defer` 배치로 묶는다(`send_defer::begin_batch()/end_batch()`, `executor.io_context.cpp`). 배치 안의 송신은 그룹 락 안처럼 모였다가 배치 끝에 socket 마다 한 번 나간다. 의미 유지 : 배치 안에서도 cork 나이 확인(`is_cork_age_checked()` : 그룹 락만 아니면 true, sendable 의 `depth() == 0` 을 대체), 배치 안의 그룹 락은 풀릴 때 자기 송신을 보냄(`enter/leave` 가 배치 depth 를 바깥으로 봄), `send_defer::enable(false)` 와 무관, 배치가 100us 넘게 돌면 다음 CQE 전에 모은 것을 보냄(`BATCH_SEND_DEFER_DURATION`, 이미 읽는 실행 시작 시각을 써서 clock 추가 없음 : 긴 handler 가 같은 배치의 다른 socket 송신을 붙잡는 상한), 배치 하나짜리는 그대로(socket cork), handler 안의 펌핑·thread block 중에는 열지 않음, `CGDK_IO_URING_BATCH_DEFER=0` 으로 끔. epoll / readiness 경로는 무변경(`batch.pcontext` 가 completion 일 때만).
  - 소유 worker 송신 op(`CGDK_IO_URING_SEND_OP=1`, 송신을 다음 enter 에 실어 syscall 0)는 배치 defer 와 같이 써도 손해(아래 표 : tcp 64B 16.25M vs 17.29M, uds 22.0M vs 29.4M) : 기본 off 유지. 그리고 **graceful close 위험** : 그 모드에서 owner 가 staging 한 송신 op 는 다음 enter 까지 제출되지 않는데 그 사이 같은 handler 가 close 하면 `process_close_socket` 이 shutdown + `cancel_fd` 로 지워 데이터를 잃는다(직접 send 는 커널 버퍼에 들어간 뒤라 안전). 남은 일 21.
- **측정** (POKPUNG kernel 7.0, 32 CPU, Release, `CGDK_BENCH=3 CGDK_BENCH_THREADS=8`, r11487 벤치, 교대 5라운드 중앙값, msg/s) :

| case | epoll r11488 / 변경 | completion r11488 | completion + 배치 defer | 배치 defer 끔(변경 트리) | 변경 completion / epoll |
|---|---|---|---|---|---|
| tcp 64B x32 window16 | 15.68M / 15.92M | 9.91M (9.2~12.7) | **18.16M** (17.5~19.0) | 13.95M (10.1~14.6) | **+16%** (전 -37%) |
| uds 64B x32 window16 | 16.96M / 16.75M | 30.18M | 29.99M | 29.13M | +77% |
| tcp 16B x256 window1 | 1.06M / 1.06M | 1.53M | 1.48M | 1.56M | +40% |
| tcp 64KB x4 window2 | 176K / 190K | 150K | 186K | 158K | -2% |
| udp 64B x16 | 472K / 472K | 599K | 651K | 645K | +38% |

  - 배치 defer 의 효과는 tcp 64B(소켓당 창이 깊은 파이프라인)에서 크고 편차도 줄였다. 16B x256(창 1)은 노이즈 안. uds 는 이미 epoll 보다 빨라 변화 없음(UDS 는 한 번 쓰기가 한 번에 상대 수신 큐에 들어가 CQE 당 메시지가 많다 : 추정).
  - 앞서 WIP 18 패치 위에서 잰 4개 변형(같은 날, 5라운드) : 배치 defer 없음 13.46M, 배치 defer 17.29M, 배치 defer + 소유 worker op 16.25M, epoll 16.04M (uds 28.8M / 29.4M / 22.0M / 16.6M).
  - **측정 방법 주의** : r11459 의 "-32% / -37%" 는 서버가 system executor(epoll 66 thread)에서 돈 측정이었다(1.27, 남은 일 19). 66 thread 가 각자 40% 안팎을 쓰며(5천 wakeup/s) 32 코어를 거의 채운다. 같은 방법으로는 이 변경의 효과(서버·클라이언트 양쪽 송신)가 보이지 않는다.
- **테스트** `test_uring_send.cpp`(linux, CMake 만, 포트 26400~, 서버 socket 도 테스트 executor) : tcp<> 와 tcp_buffered<> 각각 socket_buffer_full(작은 SO_SNDBUF + 안 읽는 OS 클라이언트, worker 아닌 thread 2000 + worker 500 송신 -> 읽으면 순서·내용 그대로, 대기 바이트 0, 64KB 는 SEND_ZC) / send_then_close(handler 에서 send 2개 뒤 closesocket : send cork / `send_defer::scope` / post 한 executable, x60 -> 응답 뒤 EOF) / close_while_sending(op 중 abortive close · RST x20 -> 끊김, 재사용 socket 큐 0, echo) / send_queue_limit(한도 64 : op 묶음 안 셈, 넘치면 실패 + 닫힘). r11488 에서는 5개 실패(buffered 4 : completion 아님, gather socket_buffer_full : 대기 바이트).
- **검증** : r11494 = r11492 + buffered/gather(POKPUNG Debug completion / epoll / readiness, Release completion / epoll 143 통과, 관련 테스트 x10). 배치 defer 포함 트리(r11488 + 전부) : net_socket.http Debug completion x3 / epoll / readiness, Release completion x2 / epoll / readiness 140 통과, system.object.execute completion / epoll 74, server_system.service Debug 170 / Release completion 174 통과, `uring_send.* + send_cork* + io_uring_completion* + blocking_handler.*` x20 Debug / Release 통과. Windows : net.socket 라이브러리 개인 빌드 Debug / Release(배치 판 send_defer.h 를 `CL=/I` 로) 오류 없음. 미실행 : ASan / valgrind, 실 NIC.
- **배치 defer 커밋 (r11497)** : 같은 파일의 `send_defer::auto_scope`(executable 하나 안의 같은 socket 연속 송신 묶기)가 r11496(다른 세션)으로 먼저 들어간 뒤 그 위에 얹었다(패치 : side-session-notes `local_bca2e593.../uring_send_batch_defer/`). auto 와 함께 써도 배치 안에서는 depth 조건이 먼저라 auto 는 동작하지 않는다(`send_defer.h` 주석 9). 검증 : r11496 + 변경 export 트리, mistral(kernel 7.0) Debug net_socket.http 147 / system.object.execute 74 / server_system.service 170, epoll / readiness / completion 3 모드 모두 통과.
- **r11496 / r11497 교대 벤치** (POKPUNG, Release, `CGDK_BENCH=3 CGDK_BENCH_THREADS=8`, 5라운드 중앙값, msg/s) :

| case | epoll r11496 / r11497 | readiness r11496 / r11497 | completion r11496 | completion r11497 | r11497 배치 defer 끔 |
|---|---|---|---|---|---|
| tcp 64B x32 window16 | 15.73M / 15.15M (재측정 15.51M / 15.33M) | 8.04M / 8.05M | 10.83M (5.1~15.3) | **17.71M** (12.9~19.0) | 9.97M |
| uds 64B x32 window16 | 16.82M / 16.40M | 8.73M / 8.76M | 28.34M | 29.25M | 28.52M |
| tcp 16B x256 window1 | 1.06M / 1.04M | 514K / 520K | 1.55M | 1.50M | 1.52M |
| tcp 64KB x4 window2 | 225K / 179K (재측정 184K / 193K) | 172K / 173K | 159K | 168K | 157K |
| udp 64B x16 | 474K / 469K | 605K / 629K | 667K | 657K | 630K |

  - completion tcp 64B x32 이 +64%(epoll 보다 빠름). 나머지는 범위 안. epoll / readiness 는 코드상 `is_cork_age_checked()`(TLS 비교 하나)만 다르고, epoll 만 다시 잰 교대 5라운드도 범위가 겹쳐(tcp 64B 14.7~16.0M / 15.1~15.8M) 회귀로 보지 않았다. r11497 completion 1라운드는 측정 시작 때 다른 부하(load 13.8)로 낮았다.

### 1.30 windows IOCP - 완료를 GetQueuedCompletionStatusEx 로 여러 개씩 받아 실행 (r11499, 2026-10-24)

남은 일 14 의 첫 항목. windows 만 바뀐다(linux 는 컴파일되지 않는 코드).

- 전에는 `executor::io_context` 의 windows 경로가 `GetQueuedCompletionStatus()` 로 syscall 한 번에 완료 하나를 꺼냈다.
- `api::io_context_iocp_direct::pop_results(RESULT*, max, wait)` : `GetQueuedCompletionStatusEx()` 로 최대 `MAX_EVENT_BATCH`(32)개. `RESULT` 는 `OVERLAPPED_ENTRY` 와 같은 배치라 배열을 그대로 넘긴다(static_assert).
- **성공/실패 판정은 `OVERLAPPED_ENTRY::Internal`(그 완료 packet 의 NTSTATUS, 음수면 실패)** : `GetQueuedCompletionStatus()` 가 FALSE 를 돌려주는 규칙과 같다. **`lpOverlapped->Internal` 을 읽으면 안 된다** : executable 은 `EXECUTABLE_NODE : OVERLAPPED` 이고 I/O 와 `PostQueuedCompletionStatus()` 양쪽에 쓰여, post 된 executable 의 OVERLAPPED 에는 이전 I/O 의 실패 상태가 남아 있을 수 있다. 이 PC 에서 확인 : 실패한 WSARecv(RST) entry.Internal 0xC000020D / 성공 0 / Internal 에 0xC000020D 가 남은 OVERLAPPED 를 post -> entry.Internal 0(`lpOverlapped->Internal` 은 0xC000020D 그대로). `GetQueuedCompletionStatus` 는 FALSE / TRUE / TRUE.
- `executor::io_context`(windows) : 받은 것을 한 번의 `process_execute()` 안에서 모두 실행(꺼낸 completion 은 다시 오지 않으므로 하나가 예외를 던져도 나머지를 실행하고 첫 예외를 다시 던짐). 받는 수는 linux 와 같은 적응형 규칙(batch 하나가 `EVENT_BATCH_DURATION` 10us 정도, 1~32) : 오래 걸리는 handler 뒤에 다른 완료가 묶이지 않는다. windows 경로에는 linux 의 `send_defer::auto_scope`(r11496)를 넣지 않았다(동작 변경 없음 : 필요하면 별도로).
- `api::io_context_iocp_queued`(쓰지 않는 대체 구현)에도 `pop_results()`(한 번에 하나, `MAX_EVENT_BATCH` 1) : `platform.h` 에서 바꿔도 빌드된다. 헤더는 함수·상수만 추가(멤버 변화 없음).
- 측정(DOLPUNG 24 thread, 순수 Win32 마이크로벤치 : 생산 4 thread 가 PostQueuedCompletionStatus, 소비 thread 가 꺼내 처리, M/s) : 처리 없음 소비 1 2.43 -> 3.85(x1.58), 4 1.73 -> 2.88(x1.66), 8 1.52 -> 1.66(x1.10). spin 200 소비 1 2.13 -> 4.06(x1.90), 4 1.81 -> 2.89(x1.60), 8 1.54 -> 1.84(x1.20). 생산 쪽 post 도 syscall 이라 꺼내는 쪽만의 이득은 더 크다. 실제 소켓 echo(IOCP) 로는 아직 재지 않았다.
- 검증(Windows VS18 x64 Release·Debug) : execute 61, server_system.service 176/172, log 24, time_event 17 통과. net_socket.http 는 Release 첫 실행 83/83, 그 뒤로는 `emergency.*` 순서 의존 실패(아래 남은 일 22 : 이 변경 전 기준선에서도 같음).
- 커밋 방법 : `executor.io_context.cpp` 를 다른 세션이 동시에 고치고 있어 별도 checkout(`svn co --depth empty` + `svn update --parents <파일>`)에서 HEAD + windows 분기 변경만 커밋했다(공유 작업 사본 규칙).


### 1.31 completion / epoll 재측정(서버도 같은 backend)과 격차 분석, 클라이언트 CONNECT 분산 (r11505, 2026-10-24)

남은 일 20. 그 전의 completion 수치(남은 일 8 의 r11459, 1.18, r11486 의 `socket.recv.html`)는 서버 socket 이 system executor(epoll)에서 돈 측정이었다(1.27).

- **측정 방법** : HEAD r11501 export(배치 send defer r11497, buffered SEND op r11494, auto_scope r11496 포함) + r11505 의 계측 벤치, mistral(i9-13900K : P 코어 8개 16 thread + E 코어 16개, kernel 7.0, g++ 15.2), Release, `CGDK_BENCH=3`, 구성 교대 5라운드 중앙값(msg/s). 회차마다 "장비 CPU - 프로세스 CPU" 를 재서 1 코어 넘게 겹친 회차는 뺀다(8·4 thread 의 30회 모두 유효. 같은 시각 다른 세션의 벤치와 겹친 r11491 측정은 버렸다). 구성 : epoll / readiness io_uring(`bench.io_uring`) / completion 단일 RECV(`CGDK_IO_URING_MULTISHOT=0`) / multishot 전환 끔(`CGDK_IO_URING_RECV_ADAPTIVE=0`) / 기본값(multishot + 수신 모드 전환 + 무복사 + 배치 send defer). 스크립트·로그 : side-session-notes `local_fb370608.../m20/`(아래).
- **8 thread** (괄호 : 5회 범위, 마지막 열은 기본값 / epoll) :

| case | epoll | readiness | completion 단일 RECV | completion multishot 전환 끔 | completion 기본값 | 기본값 / epoll |
|---|---|---|---|---|---|---|
| tcp 64B x32 window16 | 12.31M (11.9~14.0) | 7.07M | 10.22M | 15.01M | **15.55M** (15.0~17.6) | +26% |
| uds 64B x32 window16 | 15.22M (13.5~15.3) | 7.07M | 16.79M | 24.45M | **24.88M** (22.0~25.1) | +63% |
| tcp 16B x256 window1 | 900K (891~926K) | 398K | 1.05M | 1.16M | **1.24M** (1.14~1.30M) | +37% |
| tcp 64KB x4 window2 | 161K (150~166K) | 154K | 137K | 127K | **180K** (157~221K) | +12% |
| udp 64B x16 window32 | 419K | 561K | 605K | 612K | **601K** | +43% |

  - 메시지당 CPU(epoll / 기본값, us) : tcp 64B 0.65 / 0.50, uds 0.53 / 0.31, 16B x256 8.89 / 6.33, 64KB 43.8 / 22.6, udp 13.4 / 4.62. 모든 case 에서 바쁜 thread 가 100% 라 처리량 = thread 수 / 메시지당 CPU 다.
- **4 thread** : tcp 64B epoll 7.60M / readiness 6.58M / 단일 RECV 5.37M / 전환 끔 7.75M / 기본값 8.37M(+10%), uds 12.97M / 8.30M / 10.66M / 15.54M / 14.99M(+16%), 16B x256 647K / 451K / 659K / 745K / 729K(+13%), **64KB 284K / 125K / 130K / 106K / 137K(-52%)**, udp 594K / 608K / 631K / 624K / 633K(+7%).
- **16 thread** (epoll / 기본값) : tcp 64B 9.86M / 22.12M(+124%), uds 10.77M / 28.33M(+163%), 16B x256 624K / 1.70M(+172%), 64KB 109K / 170K(+55%), udp 293K / 585K(+100%). epoll 은 thread 를 늘리면 오히려 떨어진다(tcp 64B 메시지당 CPU 0.65 -> 1.62us. 추정 : 공유 epoll fd 경합과 E 코어). completion 은 thread 마다 ring 이라 늘어난다.
- **mistral 은 하이브리드 CPU** 라 같은 구성도 thread 가 P / E 어느 코어에 놓였느냐로 흔들린다(epoll tcp 64B 8 thread : 회차에 따라 메시지당 CPU 0.52~0.65us, 11.9~15.4M). 표의 차이가 범위보다 작은 칸(64KB, udp 의 completion 구성끼리)은 같다고 볼 것. POKPUNG 의 같은 벤치(1.29)는 epoll 이 더 높다(tcp 64B 15.2~15.7M).
- **tcp 64B x32 window16 의 옛 격차(r11487 : -23%)** : r11491(배치 send defer 전) 계측에서 모든 thread 100%, 메시지당 CPU 0.80~0.90us 대 epoll 0.52us, `TcpExt.TCPOrigDataSent`(데이터 세그먼트) 0.254/msg = 세그먼트당 약 8 메시지 대 epoll 0.125(16 = 창 전체) + 순수 ACK 0.064/msg. multishot RECV 는 도착마다 CQE 를 따로 내고 CQE 마다 send cork 가 끝나므로 창 16개가 두 덩어리 이상으로 갈라진 채 다시 합쳐지지 않았다(epoll 은 recv 가 늦게 불려 socket 버퍼에서 합쳐진다). 1.29 의 strace(sendto 0.76/msg)와 같은 원인이고 r11497 이 고쳤다 : r11501 은 0.125/msg, 연속 handler(run) 16. 단일 RECV 는 처리 뒤에 다시 제출하므로 그 사이 쌓여 합쳐졌다(r11491 0.153~0.181).
- **uds 는 왜 completion 이 epoll 보다 빠른가** : 메시지당 CPU 0.31us 대 0.53us. recv / epoll_wait / epoll_ctl 이 없고(수신은 multishot CQE, 재무장 없음) CQE 하나가 창 대부분을 담는다(run 16). UDS 의 send 는 TCP 스택·softirq 를 거치지 않아 r11497 전에 덩어리가 나뉘었을 때도(run 12~13) 손해가 작았다 : 그래서 그때도 uds 만 epoll 보다 빨랐다. TCP 는 세그먼트 하나가 송신 syscall + 수신 softirq(loopback 은 보내는 thread 가 처리) 비용이라 세그먼트 수에 비례했다.
- **64KB x4** : (1) 접속이 4개라 completion 은 ring 4개(8 thread 중 4 thread)에서만 돈다("sockets 4 thr 1-1"). epoll 은 이벤트마다 아무 thread. (2) 16KB 이상 단일 버퍼는 SEND_ZC op 인데 loopback 에서는 받는 쪽이 어차피 복사하므로 통지·고정 비용만 더해진다 : `CGDK_BENCH_ZEROCOPY_THRESHOLD=1000000000`(복사로 보냄)로 교대 5라운드 중앙값 completion 4 thread 190.8K -> 244.3K(+28%), 8 thread 216K -> 250K(+16%), softirq 1.83 -> 0.86 코어. epoll 은 그대로(284K / 168K). 실 NIC 에서는 반대일 수 있어 기본값은 두었다(남은 일 21 의 실 NIC 측정). 같은 구성의 회차 범위가 넓다(r11491 의 겹친 회차까지 78K~221K).
- **udp** : 서버 socket 하나 = completion 은 ring 하나(thread 하나 100%)인데도 epoll(여러 thread 가 번갈아 recvmmsg) 보다 빠르다.
- **클라이언트 CONNECT 의 몰림과 분산** : 계측으로 본 completion 의 클라이언트 socket / thread 는 고르지 않았다 : 16B x256 8 thread "11-85", "24-46", 4 thread "15-118", "26-90"(서버는 accept round robin 으로 32-32, 64-64). 클라이언트 socket 은 CONNECT post 를 실행한 worker 의 ring 에 묶이고 post 는 깨운 ring 이 몇 개씩 가져가기 때문이다. 시제품(`submit_connect` 에서 ring 0 의 별도 카운터로 round robin, 같은 바이너리 env 토글, 교대 5라운드) 8 / 4 thread : 16B x256 1.27M -> 1.41M(+11%) / 712K -> 931K(+31%, 분포 64-64), tcp 64B 13.98M -> 16.47M / 7.40M -> 8.01M, 64KB 159K -> 216K / 140K -> 220K(시작값 0 이라 한 프로세스 안의 짝이 같은 ring : 메시지당 CPU 30.7 -> 18.5us, 벤치에서만 생기는 이득), uds 그대로(즉시 접속은 CONNECT op 가 없음). 첫 시제품은 accept 와 같은 카운터를 써서 클라이언트·서버가 짝수·홀수 ring 으로 갈려(분포 "2-62") 무효였다.
  - **결정 (사용자, 2026-10-24) : CONNECT op 만 round robin**(submit_connect 안, 헤더 레이아웃·가상 함수 변경 없음). 즉시 접속되는 UDS 클라이언트는 지금처럼 첫 RECV 를 낸 worker 의 ring(op 가 없어 묶을 자리가 없다 : 하려면 Iio_context 가상 함수 추가 = gruma 재빌드). 구현·검증·커밋은 같은 일을 공유 사본에서 먼저 하고 있던 "send_cork 개선 사항" 세션이 맡았다(ring 0 의 `index_bind_connect`, 시작값 1 : 한 프로세스 안의 짝을 다른 ring 에). **r11507** 로 커밋 : 테스트 `io_context_uring_cq.op_connect_binds_the_sockets_round_robin`(한 worker 가 제출한 32 CONNECT -> 4 thread 에 8개씩) 과 `linux_io.io_uring_completion_accepted_sockets_spread` 의 클라이언트(TCP) 분포 단언, 수정 전 둘 다 3/3 실패. 검증 monsoon r11504 + 변경 Debug : net_socket.http 3 백엔드 x2 148, system.object.execute x2 75 통과(epoll 의 `send_defer_auto.*_auto_off` 간헐 실패는 변경 없는 r11504 에서도 3회 중 2회 : 무관, r11496 테스트). 위 시제품은 시작값 0(짝이 같은 ring)이라 tcp 64B · 64KB 의 이득에는 짝 효과가 섞여 있다 : 16B x256 의 분포 개선(+11~31%)이 그 구현에도 해당하는 몫이다.
- **계측 (r11505, `bench_linux_io.cpp`)** : case 마다 "[bench-cpu]"(프로세스 코어·메시지당 us·context switch, 10% 넘게 쓴 thread 수와 가장 바쁜 thread, 장비 전체 코어·sys·softirq, socket 마다 마지막 handler 의 thread 분포, 한 thread 에서 같은 socket 의 handler 가 연달아 실행된 수 = 수신 한 번의 메시지 수 근사), "[bench-tcp]"(메시지당 Tcp* / TcpExt* 중 움직인 것 : 네트워크 namespace 전체). `CGDK_BENCH_CASES="tcp echo 64B,uds"`(case 고르기), `CGDK_BENCH_ZEROCOPY_THRESHOLD=<bytes>`. 측정 구간 시작·끝에서 한 번씩 읽어 측정에 끼어들지 않는다. perf 는 mistral 에서 `perf_event_paranoid=4` 라 sudo 없이 못 쓴다.
- **본 것(미재현, 남은 일 23)** : r11491 + 계측 벤치의 4 thread `bench.epoll` 에서 udp case 중 1회 `*** stack smashing detected ***: terminated` 뒤 SIGSEGV(exit 139). 그 회차는 다른 세션의 벤치와 겹쳤다(장비 29.8 코어 / 프로세스 1.25 코어). core 는 apport 가 버림. 재현 시도 : SAKPUNG(kernel 6.12) r11501 `bench.epoll` 6~8개 동시 x 90회 = 660 실행, mistral 8개 동시 x 40회 : 0회(gdb 아래, SIGABRT/SIGSEGV 에서 모든 thread backtrace). `Ndatagram::process_complete_receivable`(recvmmsg) / `Nudp::_process_send_pending`(sendmmsg) 의 스택 배열은 배치 한도로 묶여 있다(코드 확인).

### 1.32 linux `send_defer_auto` 의 close case - `closesocket()` 은 abortive 다 (r11512, 2026-10-24)

남은 일 24. 라이브러리 버그가 아니라 r11496 테스트의 기대가 틀렸다(라이브러리 변경 없음).

- **현상** : `send_defer_auto.gather_auto_off` / `buffered_auto_off` 의 4) close case(한 executable 이 다른 socket 7개에 10개씩, socket 0 에 50개를 보낸 뒤 `servers[0]->closesocket()`)에서 socket 0 의 클라이언트가 EOF 전에 **매번 정확히 26개**만 받았다. monsoon r11511 Debug 재현 : epoll `--gtest_filter='send_defer_auto.*_auto_off' --gtest_repeat=20` 40회 중 15회. `send_defer_auto.*` x10(모드당 40회) 에서는 epoll 7 · readiness(`CGDK_IO_URING=io_uring`) 1 · completion 3 회 실패, 모두 auto 끈 판이고 모두 26개. auto 켠 판은 실패 0.
- **원인** : `Isocket::closesocket()` 은 설계상 abortive close 다(`DISCONNECT_REASON_ABORTIVE` -> `process_complete_closesocket()` 의 `abortive_closesocket()` = SO_LINGER{1,0} + close -> RST. 헤더 주석도 "Abortive close 는 closesocket()"). RST 는 커널 송신 큐에서 아직 나가지 않은 것을 버린다. 테스트 클라이언트는 하나씩 차례로 읽으므로 socket 0 의 클라이언트가 읽지 않는 동안 그 수신측은 quick ACK 16개 뒤로 ACK 를 미루고(받는 창이 늘지 않음), 송신측(TCP_NODELAY)은 cwnd 10(처음 값 그대로 : 그때까지 cwnd 에 막힌 적이 없어 늘지 않음)에 막혀 나머지를 커널에 쌓아 둔다 : 16 + 10 = 26.
- **진단**(진단 빌드 : closesocket 직전 executable 안에서 서버 socket 을 잼, 클라이언트는 recv 오류 errno 기록) : 실패 회차는 모두 `statistics_get_sent_message()` 50 · sent bytes 1600(CGDK 는 50개를 다 커널에 넘겼다), `SIOCOUTQ` 1088 · `SIOCOUTQNSD` 768(= 24개 미전송), `tcpi_snd_cwnd` 10 · `tcpi_unacked` 10, 클라이언트는 26개 뒤 `ECONNRESET`(104) 다음 EOF. 통과 회차는 대부분 미전송 0(cwnd 가 늘었거나 ACK 가 옴). 같은 case 를 `disconnect()`(graceful) 로 바꾸면 3 모드 각 40회 실패 0 : epoll 에서 close 시점에 미전송 768B 였던 8회도 50개가 다 왔다(shutdown 의 FIN 이 데이터 뒤에 감). completion 도 close 전 송신 통계는 50 이었다. 남은 일 24 에 적은 두 갈래 (a) graceful close 가 큐를 버림, (b) 클라이언트 `read()`(SO_RCVTIMEO 100ms, 300ms idle)가 일찍 끝남은 둘 다 아니다 : 읽기가 끝난 이유는 RST 다.
- **auto 켠 판이 통과한 이유** : 50개가 모여 있다가 close 의 `process_flush_deferred_send_on_close()` 에서 sendmsg 한 번(세그먼트 1개)으로 나가 cwnd 안이었다. abortive 라 이것도 보장은 아니다(socket 에 ACK 안 된 데이터가 cwnd 만큼 있으면 같이 버려진다).
- **테스트 수정** (`test_send_defer_auto.cpp`) :
  - 4) close : closesocket() 직후 executable 안에서 잰 socket 0 의 송신 통계가 50(close 전에 socket 에 넘어감 : 모아 둔 것은 close 가 보냄)인지, 도착분은 순서대로인 앞부분 + EOF 인지만 본다. 변이 확인 : 두 sendable(gather / buffered epoll)의 `process_flush_deferred_send_on_close()` 를 비우면 auto 켠 두 테스트가 4/4 실패(통계 0 또는 23. 도착분만 보면 0개도 "앞부분"이라 통과한다).
  - 5) disconnect 추가 : 같은 송신 뒤 `disconnect()`(graceful) -> 50개 모두 EOF 전에 도착, 다른 socket 은 EOF 없음.
  - 클라이언트와 서버 socket 을 포트(getsockname / getpeername)로 짝지어 socket 별 기대값으로 읽고 비교한다. 전에는 정렬해 비교해서 기대가 다른 클라이언트 7개가 300ms idle 을 두 번씩 기다렸다 : epoll 테스트당 5.4초 -> 1.1초(readiness / completion 0.13초).
- **검증** : r11511 export + 이 변경, Debug, g++ 15.2. monsoon : `send_defer_auto.*` x20 epoll / readiness / completion 각 80/80, net_socket.http 전체 3 모드 148/148. mistral : `send_defer_auto.*` x10 3 모드 각 40/40, net_socket.http 전체 3 모드 x2 148/148. 진단 빌드의 테스트 사본(`test_send_defer_auto.diag.cpp`, 환경변수 `SDA_DISCONNECT=1` 이면 disconnect)은 1.32 의 노트 폴더.
- **사용자에게** : `closesocket()` 은 보낸 직후 불러도 상대가 다 받는다는 보장이 없다(abortive). 다 보내고 닫으려면 `disconnect()` 를 쓸 것(5 장 함정에 적음).

### 1.33 `make_executable(lambda)` - std::function 없이 담고, 해제될 때 캡처를 지운다 (이 커밋, 2026-10-24)

남은 일 14 의 "lambda post" 항목. 헤더만 바뀐다(`executable.make.h`, `common.h` 에 `<optional>`) : 라이브러리 클래스 레이아웃·가상 함수 변화 없음.

- **전** : `make_executable(lambda)` 는 함수 안 지역 클래스 `executable::function<std::function<R()>>` 를 풀(`alloc_object`)에서 받아 lambda 를 `std::function` 에 넣었다. (1) libstdc++ 의 `std::function` 은 trivially copyable 이고 16B 이하인 것만 안에 담아 object_ptr·shared_ptr 캡처는 매번 malloc/free, MSVC 도 40B 를 넘으면 malloc. (2) 풀 객체는 소멸자 없이 TLS 스택으로 돌아가므로(`factory::object_pool_tls<T>::dealloc`) **캡처가 그 래퍼가 재사용될 때까지 살아 있었다** : 캡처한 object_ptr 의 객체(socket·group 등)의 해제가 늦어지고 그 사이 소멸 순서가 바뀐다. 확인 : 100개 post 뒤 캡처한 shared_ptr 의 use_count 62(61개가 풀에 남음). (3) `std::function` 은 복사 가능해야 해 move-only 캡처(unique_ptr 등)를 못 넣었다.
- **후** : `_executable_callable<std::decay_t<F>>`(`Iexecutable` + `factory::_traits_system`)가 lambda 를 `std::optional<F>` 로 그대로 담고, `on_final_release()`(참조 0, 풀로 돌아가기 전)에서 `reset()` 한다. 반환값 규칙은 같다(void 면 0, 아니면 `intptr_t` 로 변환). 같은 executable 을 여러 번 post 하면 마지막 참조가 놓일 때 지운다. 풀은 decay 된 lambda 타입마다 하나(전엔 make_executable 인스턴스마다 지역 클래스 하나 : 사실상 같음).
- **테스트** (`test_executor.cpp`, execute) : `CGDK10_executor.make_executable_releases_captures`(post 100개 실행 뒤·실행 안 한 채 해제·같은 executable 두 번 post 에서 캡처가 바로 놓임), `make_executable_move_only_and_result`(unique_ptr 캡처, 반환값). 옛 헤더로는 첫 테스트가 use_count 62 로 실패하고 둘째는 컴파일되지 않는다(`std::function target must be copy-constructible`) : SAKPUNG 에서 확인.
- **검증** : Windows VS18 x64 Debug(system·net.socket·server.system·server.service 빌드) : execute 63, server_system.service 172(skip 6), net_socket.http 87 통과. net_socket.http 첫 실행의 emergency.* 4개 실패는 환경(아래 남은 일 22 : 제외 포트 범위)이고 포트가 범위 밖인 회차는 87/87. SAKPUNG(CentOS, kernel 6.12, g++ 15.2, r11513 export + 이 변경) Debug : execute 63(skip 14 : io_uring), net_socket.http 130(skip 21), server_system.service 170(skip 5), 새 테스트 x20 통과.
- **벤치**(만들기 + `process_execute` + 해제 한 번의 ns, 4 thread 가 25만 개씩 system executor 에 post 한 처리량. 새/옛 헤더로 같은 소스를 빌드해 교대 5라운드, 범위) :

| | 옛 | 새 |
|---|---|---|
| linux(SAKPUNG, Release, epoll) 24B 캡처 | 25.7~26.4ns | 10.1~10.9ns |
| linux 64B 캡처 | 25.6~28.5ns | 11.1~12.4ns |
| linux post 4 thread | 1.25~1.33M/s | 1.42~1.45M/s(+11%) |
| windows(DOLPUNG, Release) 24B 캡처 | 37.6~39.7ns | 24.5~32.9ns |
| windows 64B 캡처 | 54.4~57.2ns | 24.9~33.2ns |
| windows post 4 thread | 1.32~1.41M/s | 1.30~1.40M/s(같음 : IOCP post 가 지배) |

  벤치 소스 `bench_make_executable.cpp` 는 저장소에 넣지 않았다(단일 cpp, 헤더 `/I` 순서로 옛 `executable.make.h` 를 앞에 두어 A/B).
- **동작 변화** : 캡처의 소멸이 "그 래퍼가 재사용될 때(아무 thread)" 에서 "마지막 참조가 놓일 때(보통 실행한 worker, 실행 직후)" 로 당겨진다. 캡처한 object_ptr 의 마지막 참조였다면 그 객체의 final release 도 그 자리에서 일어난다. 헤더 템플릿이라 **이 헤더를 쓰는 코드(gruma 포함)는 다시 빌드해야** 새 동작이 된다(옛 빌드도 그대로 링크·동작은 한다 : 지역 클래스라 이름이 겹치지 않음).
- `schedulable::executable_function` / 직접 쓰는 `executable::function<T>` 는 바꾸지 않았다(사용자가 set_function 후 여러 번 쓰는 객체).


### 1.34 completion - CPU 를 오래 쓰는 handler 의 ring 도 다른 worker 가 실행한다 (남은 일 17, 이 커밋, 2026-10-24)

1.26 이 남긴 경우(블로킹 없이 사용자 공간에서 오래 도는 handler)를 고쳤다.

- **1.26 의 전제가 틀렸다** : `COOP_TASKRUN` 이어도 바쁜 owner 의 CQE 는 올라온다. task work 는 IPI 없이 `TIF_NOTIFY_SIGNAL` 만 세우지만, 바쁜 thread 도 타이머 인터럽트에서 사용자 공간으로 돌아갈 때 그것을 실행한다. C 프로브(mistral kernel 7.0, HZ 1000 : RECV 를 제출한 thread 가 사용자 공간에서만 돌고, 다른 thread 가 데이터를 쓴 뒤 공유 메모리의 CQ tail 을 봄)에서 CQE 는 0.2~0.7ms 뒤 올라왔다(setup flag 0 이면 7us). 원인은 그 CQ 를 owner 말고 아무도 읽지 않는 것이었다.
- **동작** (`api.io_context.uring_cq.cpp`, `executor.io_context.cpp`) :
  - executor 는 가장 바깥 배치를 owner 의 ring 에 공개한다(`batch_begin` / `batch_next` / `batch_end`, `RING::batch_word` = seq | end | index). owner 는 결과마다 CAS 로 다음 결과를 가져간다.
  - worker 는 `pop_results()` 32번에 한 번(`BUSY_CHECK_POPS`) ring 들을 보고, 그중 한 worker 만 1ms(`BUSY_CHECK`)마다 실제로 본다(`_check_busy`). 같은 word(같은 executable 실행 중)를 2ms(`BUSY_LEND`) 넘게 보면 `_lend_busy` 가 1.26 과 같은 방식으로 빌린다 : `STATE_LENT`, 배치의 나머지는 word 를 CAS 로 잘라 `carry` 로(owner 가 그 사이 다음 결과로 넘어갔으면 CAS 실패 : 빌리지 않음), 실행 중인 executable 은 `pblocked`(그 결과는 owner 에게 deferred). 그 뒤는 1.26 의 경로(`_drain_orphans` -> `_take_lent`) 그대로다. 앞선 lend 의 carry / deferred 가 남아 있으면 빌리지 않는다(순서).
  - owner 는 그 executable 이 끝난 뒤 배치 밖의 다음 `pop_results()` 에서 되찾는다(`_reclaim_busy`, `RING::is_busy_lent` : unblock 으로는 되찾지 않음). handler 가 executor 를 pump 하면 바깥 배치 word 에 `BATCH_NESTED` 를 걸어 그 동안은 빌리지 않는다(자기가 CQ 를 읽으므로. 이미 빌려졌으면 그 pump 는 post 큐만).
  - 모든 worker 가 쉬고 있으면 아무도 `pop_results()` 에 오지 않는다 : 쉬는 owner 하나가 감시한다(`m_pring_watch`). 다른 ring 이 배치를 실행 중이면 대기 1ms, 아니면 10ms(`BUSY_WATCH`). 감시자가 일을 받으면 감시를 놓고, 곧 기다리러 올 다른 owner 가 없을 때만(다른 owned ring 이 모두 대기 중이거나 이미 오래 도는 것으로 본 것) 쉬는 owner 하나를 깨운다(`_hand_watch`).
- **테스트** `test_blocking_handler.cpp` :
  - `blocking_handler.busy_handler_does_not_hold_the_other_sockets` : 1.26 의 테스트를 300ms CPU 사용(`steady_clock` 바쁜 루프, syscall 없음)으로. 프로세스의 backend 로 돈다.
  - `blocking_handler.busy_handler_on_an_idle_executor` (completion 전용, 커널이 없으면 skip) : 2 thread completion executor, 같은 ring 의 두 socket(round robin 이라 0 과 2), 다른 worker 는 쉬는 중이고 다른 트래픽 없음 : 같은 ring 의 다른 socket 이 100ms 안에 응답, 바쁜 socket 에 이어 보낸 것은 순서대로.
  - 수정 전 라이브러리(r11515) mistral Debug completion : 앞 테스트 3/3 실패(같은 ring 접속 최대 261~263ms), 뒤 테스트 3/3 실패(280ms). 최종 코드는 completion 에서 각 7회(단독 x5, 전체 스위트 x2) 통과(0.4~1.7ms), 유휴 executor 테스트는 epoll / io_uring 실행에서도(자기 completion executor) 통과. 감시(`m_pring_watch`)만 끈 빌드는 뒤 테스트만 3/3 실패(280ms) : 감시 경로를 잡는 테스트다. epoll 은 수정 전에도 통과.
- **검증** (mistral kernel 7.0, Debug, r11515 export + 변경) : net_socket.http 전체 completion x2 / epoll / io_uring 153 통과(skip 3 = bench), system.object.execute completion x2 / epoll 75 통과. Windows VS18 x64 system.object Debug·Release 빌드(바뀐 코드는 linux 전용 경로). 미실행 : ASan / valgrind, gruma 재빌드.
- **비용** (mistral Release, `CGDK_BENCH=3 CGDK_BENCH_THREADS=8`, completion, 수정 전 / 후 교대 6라운드 중앙값) : tcp 64B x32 16.51M / 16.26M(-1.5%), uds 64B x32 25.49M / 25.30M(-0.8%), tcp 16B x256 1.272M / 1.252M(-1.6%). 회차 간 흔들림(±7%) 안이지만 부호는 대체로 음수다 : 결과마다 CAS 한 번. 첫 판(pop 마다 시계 읽기, 대기마다 감시 word 에 CAS, 감시자가 일을 받을 때마다 다른 owner 깨우기)은 tcp 64B 11쌍 중 9쌍이 낮아 지금 형태로 줄였다.
- **남는 것** : 감지까지 2~3ms(`BUSY_LEND` + `BUSY_CHECK`), 감시자만 볼 때는 최대 10ms 더. 그 동안 같은 ring 의 다른 socket 은 기다린다. 빌린 동안 쉬는 worker 의 대기는 1ms(`LENT_WAIT`, 1.26 과 같음). 나머지 worker 가 모두 쉬는 중에 바쁜 handler 둘이 거의 동시에 시작하면 감시가 늦을 수 있다(감시자가 일을 받을 때 다른 하나가 아직 오래 도는 것으로 보이지 않으면 깨우지 않음 : executor 대기 한도 1초까지). HZ 가 낮은 커널(250)은 CQE 가 올라오는 데 4ms 까지.
- 헤더 변경(`io_context_uring_cq::BATCH` 에 `pring`, 함수·멤버 추가) : gruma 등 재빌드.


### 1.35 `_buffer_unreal.h` - 실제 UE 5.7 / 5.8 빌드·테스트, FString 경로 (남은 일 6, r11520, 2026-10-24)

r11408 은 모의 타입으로만 검증했다. DOLPUNG 에 설치된 UE 5.7·5.8(`C:\Program Files\Epic Games\UE_5.x`, MSVC 14.44)로 처음 빌드했다.

- **고치기 전 헤더는 UE 5.7 에서 컴파일되지 않았다** : FStringView 경로가 UE5 에 없는 `TStringViewImpl` 을 썼다(C2672). 고친 것(r11520) :
  - 전방 선언 대신 엔진 헤더 포함(CoreMinimal, List, SortedMap, StaticArray, SparseArray, StringView, SharedPointer). 5.7 은 TMap/TSet 을 매크로로 여러 벌 만들고(TSparse*/TCompact*) 선언이 버전마다 달라 전방 선언으로는 따라갈 수 없다. TObjectPtr 는 `__has_include("UObject/ObjectPtr.h")` 일 때만(CoreUObject 의존 모듈).
  - FString / FStringView : `TStringView<T>`. FString extract 는 `TStringView(ptr, len)` 로 만든다(`FString(int32 Len, const TCHAR*)` 는 인자 순서가 (길이, 포인터)이고 5.4 부터 deprecated, 5.4+ 대안은 `ConstructFromPtrSize`).
  - `FTouchKeySet` 특수화 삭제 : TSet 이 아니라 SlateCore 의 FKey 두 개짜리 클래스다.
  - `TSparseArray::ElementType` 은 private, `TMultiMap` 에는 KeyType/ValueType 이 없다 : 요소·키·값 타입을 `begin()` 의 역참조로 얻는다(`ue4_element_type_t`, `ue4_map_key_t`, `ue4_map_value_t`).
  - TArray append 를 할당자 무관하게(TInlineAllocator 등).
  - TSharedPtr<T>(serializable 아님)의 get_size_of 가 `T::get_size_of()` 를 불렀고 peek 는 TSharedPtr 로 T 를 만들려 했다. serializable 의 2인자 extract 는 null(-1) 을 안 봤다. TSharedRef 는 nullptr 비교·`const_cast<TX*>` 로 컴파일 안 됨, peek 는 null TSharedRef 에 읽었다. TObjectPtr 도 같은 const_cast, null 에 읽는 peek, null(-1) 을 안 보는 extract.
- **코어 버그(UE 와 무관)** `_buffer_common.h` : `Ibuffer_serializable` 의 peek(`front<T>(offset)`)가 offset 을 전진시키지 않았다. `std::vector<serializable>` 등을 peek 하면 첫 원소를 거듭 읽고, peek 뒤의 offset 이 틀렸다. 이 테스트(FItem 의 peek offset 0)에서 발견, r11520 에서 같이 고침.
- **테스트** `build/src/system/unit_test/unit_test.object.buffer.unreal` (UE 프로젝트 CgdkUe, 모듈 하나, CMake/VS 솔루션에는 넣지 않음) : `run_test.bat [5.7|5.8] [엔진 루트]` 가 Editor Development 빌드 후 `UnrealEditor-Cmd -ExecCmds="Automation RunTests CGDK.Buffer.Unreal" -nullrhi` 를 돌리고 `Result={Success}` 를 본다. 내용 : FString(한글)/빈 FString/FStringView/TArray(int32, FString, TInlineAllocator, 빈 것)/TStaticArray(int32, FString)/TSparseArray(구멍)/TList/TLinkedList/TSet/TMap/TMultiMap/TSortedMap/TSharedPtr(값, null)/TSharedRef/UE 멤버를 가진 Ibuffer_serializable 의 왕복, 매번 `get_size_of == 쓴 크기`·peek offset·extract 가 다 읽음, std 대응 타입과 교차 extract(FString <-> `std::basic_string<TCHAR>`, TArray·TSparseArray -> vector, TList -> list, TMap -> map), type hash 가 std 대응 타입과 같음. 검증 : UE 5.7, 5.8 모두 빌드·통과(스크래치와 저장소 위치 둘 다).
- **남은 것** : `examples/6.x.game_mmo_field_*/client/unreal3d/Source/.../cgdk/` 안의 옛 CGDK 사본(UE 5.2 용, 그 사본의 `_buffer_unreal.h` 는 고치기 전 것)은 그대로. Linux / Android / Mac UE 빌드, Shipping 구성, 5.6 이하 엔진은 미확인. FUtf8String / FAnsiString 직렬화 없음. 본 `include/cgdk` 는 `_buffer_unreal.h` 를 자동으로 포함하지 않는다 : UE 모듈에서 `cgdk/buffer` 다음에 직접 포함할 것(예제 사본은 `system.object.h` 에서 포함).

### 1.36 completion 송신의 남은 것 - 소유 worker 송신 op 의 graceful close, ASan, 1GbE SEND_ZC 측정, loopback zerocopy (남은 일 21, r11522, r11523, 2026-10-24)

- **소유 worker 송신 op 의 graceful close 유실 (r11522)** : 실제로 잃는 경우는 handler 의 close 가 아니라 **상대 FIN 과 같이 온 요청**이다. `disconnect()` 는 `process_close_sender()` 가 보내는 중(`m_flag_sending` : op 가 staged 여도)이면 shutdown 을 송신이 끝날 때로 미뤄 안전하고, `closesocket()` 은 원래 abortive 다(1.32). graceful `process_close_socket()` 은 recv 0(상대 FIN)의 경로인데, 클라이언트가 요청을 보내고 바로 `shutdown(SHUT_WR)` 하면 요청 CQE 와 EOF CQE 가 한 배치에 와서 요청 handler 의 응답(소유 worker op, 다음 enter 까지 staged)을 EOF 의 close 가 shutdown + `submit_cancel_fd()` 로 지웠다(staged 는 synthetic -ECANCELED). 직접 send(기본)는 이미 socket 버퍼에 있어 무관.
  - 수정 : `Iio_context::submit_staged(fd)`(linux, 기본 false) / `io_context_uring_cq::submit_staged` : 이 thread 가 fd 의 ring owner(STATE_OWNED)이고 그 fd 의 op 가 staged 면 `lockable_sq` 안에서 flush_staged + submit(대기 없음). 쓸 수 있는 socket 의 SEND 는 submit 안에서 inline 으로 실행돼 shutdown 전에 socket 버퍼에 들어간다. 다른 thread·빌려준 ring 은 아무것도 안 함. `Ntcp::process_close_socket`(linux completion, graceful 만)이 shutdown 앞에서 부른다. **헤더 변경(executor.Iio_context.h virtual 추가 외) : gruma 재빌드.**
  - 남는 것 : 다른 thread 가 staged 한 op(socket 버퍼가 찬 경우의 SENDMSG op)는 그대로다. epoll 도 EPOLLOUT 을 기다리던 큐는 shutdown 뒤 EPIPE 로 버려지므로 같은 동작이다(IOCP 는 pending WSASend 뒤에 FIN). 버퍼가 찬 채로 상대 FIN 을 받은 graceful close 의 큐 유실은 linux 두 backend 공통.
  - 테스트 `uring_send.send_then_disconnect_{gather, gather_send_on_owner_ring, buffered}` : 60 접속, 짝수는 응답 2개 뒤 `disconnect()`, 홀수는 클라이언트가 요청 직후 `shutdown(SHUT_WR)`. 수정 전 owner op 판 3회 중 2회 실패("reply 1 recv 0"), 수정 후 x50 통과.
- **검증** : mistral(kernel 7.0) r11518 + 변경 Debug : net_socket.http 156(skip 7) / system.object.execute 77, epoll / readiness / completion 모두 통과. **ASan**(같은 트리, `-fsanitize=address` Debug, `ASAN_OPTIONS=detect_leaks=0`) : 같은 두 스위트 3 모드 통과, 보고 0(라이브러리도 계측됨 : `__asan_report_load` 확인). valgrind 는 쓰지 않았다 : completion 은 커널이 ring·수신 버퍼에 비동기로 써서 memcheck 가 못 따라간다(오탐), SAKPUNG 은 io_uring 이 꺼져 있다(아래).
- **1GbE 측정 (r11523 의 `bench.remote`)** : 장비는 모두 1GbE(r8169, SG/TSO 켜짐 : zerocopy 가 실제로 무복사). mistral 의 ixgbe(10G) 포트는 link down. **SAKPUNG(CentOS)은 `kernel.io_uring_disabled=2`** 라 io_uring backend 가 epoll 로 대체된다 -> 서버 POKPUNG(kernel 7.0, 8 thread), 클라이언트 SAKPUNG(epoll), 64KB x4 window2, 10초, 교대 5라운드 :

| 서버 backend | zerocopy 끔 us/msg | zerocopy 켬 us/msg | 처리량 |
|---|---|---|---|
| completion (SEND_ZC 라운드당 17.9K) | 90.6 (86.0~94.1) | 87.8 (84.9~91.1) | 108.4MB/s 모두 |
| readiness (MSG_ZEROCOPY 17.9K) | 135.1 (133.3~142.1) | 140.9 (137.2~145.4) | 108.4MB/s |
| epoll (zerocopy 안 씀 : 노이즈 기준) | 113.3 (107.6~123.5) | 119.6 (116.9~130.4) | 108.4MB/s |

  - 회선(1Gb)이 상한이라 처리량은 모두 같고, 서버 CPU 차이는 노이즈(±5%) 안이다. 108MB/s 의 복사는 코어의 1~2% 라 1GbE 로는 zerocopy 효과를 볼 수 없다. **실 NIC 판정에는 10GbE 이상 두 대가 필요**(mistral ixgbe 에 케이블 + 상대 10G 장비, 또는 25G). 측정은 `CGDK_BENCH_REMOTE=server` / `CGDK_BENCH_REMOTE=<ip>` + `CGDK_BENCH_BACKEND` + `CGDK_BENCH_ZEROCOPY_THRESHOLD`(순서 : 서버를 `CGDK_BENCH_REMOTE_ROUNDS=1` 로 nohup 실행, "listening" 줄을 기다린 뒤 클라이언트 실행, 서버 출력의 "zerocopy sent" 줄을 기다림).
- **loopback 의 zerocopy 는 커널이 복사한다 (확인)** : C 프로브(MSG_ZEROCOPY 64KB x8, error queue 의 `ee_code`)로 POKPUNG 에서 `127.0.0.1` -> 8개 모두 `SO_EE_CODE_ZEROCOPY_COPIED`, **자기 NIC 주소(192.168.0.69)로 접속해도 8개 모두 copied**, SAKPUNG -> POKPUNG(실 NIC) 은 copied 0. 같은 host 끼리는 pin + 통지 비용만 더해진다(1.31 의 loopback +16~28%). 사용자가 "커널 보고로 socket 별 끄기"를 골랐다(아래).
- **copy check (r11525)** : 커널이 복사했다고 보고한 zerocopy 송신이 **연속 `ZEROCOPY_COPIED_TO_STOP`(8)번**이면 그 socket 의 zerocopy 를 reset 까지 끈다(`Itcp_gather::_zerocopy_on_notified`, 복사 아닌 통지 하나가 횟수를 0 으로). 보고 : readiness 는 MSG_ZEROCOPY 통지의 `ee_code & SO_EE_CODE_ZEROCOPY_COPIED`, completion 은 SEND_ZC / SENDMSG_ZC 의 ioprio 에 `IORING_SEND_ZC_REPORT_USAGE`(uname 6.2+ 만 : 그 전 커널은 -EINVAL) -> 통지 CQE res 의 bit 31 `IORING_NOTIF_USAGE_ZC_COPIED`.
  - **첫 copied 로 끄면 안 된다** : 1GbE 실 NIC(POKPUNG 서버)에서도 한 라운드(약 8900 송신)에 copied 가 1~4개 보고됐다(completion 은 접속당 하나쯤, readiness 0~3, 원인은 보지 않음). 첫 copied 로 끄던 시험판은 한 라운드에서 zerocopy 송신이 8942 중 26개로 줄었다. 연속 8번 규칙으로 실 NIC 는 8943~8948 그대로, 끈 socket 0.
  - API : `zerocopy_copied_count()`, `is_zerocopy_copied()`, `enable_zerocopy_copy_check(bool)` / `is_zerocopy_copy_check_enabled()`(기본 on. off 면 세기만 : loopback 에서 zerocopy 경로를 시험하는 테스트용 - `tcp_zerocopy_large_messages_io_uring`, `uring_send.socket_buffer_full_*` 가 끄고 돈다). **Itcp_gather 헤더 멤버 추가 : gruma 재빌드.**
  - 측정(POKPUNG, Release 8 thread, loopback tcp 64KB x4 window2, 교대 5라운드 중앙값) : readiness 175K -> 269K(threshold 로 복사 강제 269K), completion 158~186K -> 216K(복사 215~243K).
  - 테스트 : `linux_io.tcp_zerocopy_stops_when_copied_{io_uring, io_uring_completion}`(loopback 256KB 12개 하나씩 : 연속 8 copied -> 꺼짐, 그 뒤 zerocopy 0. check off : 안 꺼지고 계속), `io_context_uring_cq.op_send_zerocopy_and_sendmsg` 에 loopback 통지의 COPIED 비트(6.2+). 검증 mistral r11524 + 변경 Debug : net_socket.http 158 / execute 77 3 모드 통과, zerocopy 테스트 x10.
  - 남는 것 : 실 NIC 에서 copied 가 왜 가끔 보고되는지(첫 송신? 재전송?)는 보지 않았다. 10GbE 측정 때 `bench.remote` 의 "copied" 를 같이 볼 것.

### 1.37 strand 의 destroy 대기, linux 기본 thread 수 (남은 일 14, 이 커밋, 2026-10-24)

남은 일 14 의 "strand 1ms sleep" 과 "기본 thread 수". `.cpp` 만 바뀐다(`executor.strand.cpp`, `api.system.linux.cpp`) : 헤더 변경 없음, gruma 재빌드 불필요(라이브러리만 다시 링크).

- **strand 의 1ms sleep 은 post 경로가 아니라 `_process_destroying()` 에만 있었다** (2026-10-24 재평가의 "post 한 실행이 1ms 단위로 밀릴 수 있음" 은 틀린 추정). 다른 thread 가 처리 중이면 `m_cs_queue_executable` 를 **잡은 채** `sleep_for(1ms)` 하고, 풀자마자 다시 잡았다. 그 사이 처리 중인 thread 의 `post()` 와 `process_post_next()`(처리 끝 표시)도 같은 락이 필요하다.
  - 측정(재현 프로그램 : 100ms executable 을 처리하는 중에 destroy, 다른 thread 가 계속 post) : windows(DOLPUNG Release) 원본은 post() 한 번이 최대 62ms~6.6초 막히고 destroy 가 executable 이 끝난 뒤 30us~4초 뒤에 돌아왔다(락이 공정하지 않아 destroy 쪽이 다시 잡음). **linux(SAKPUNG Release) 원본은 destroy 가 60초 안에 끝나지 않았다**(처리 thread 가 락을 못 잡음). 수정 후 windows post() 최대 0.14~0.31ms, destroy 는 executable 끝 뒤 0.01ms 또는 ~15.6ms(아래).
  - **strand 의 executable 안에서 그 strand 를 destroy 하면 영원히 멈췄다** : 처리 중 표시를 지울 thread 가 바로 자기다(lock_object 는 recursive 라 락은 잡힘). 양 OS 재현.
- **수정** (`executor.strand.cpp`) :
  - 기다릴 때는 락을 놓는다 : 잠깐 잡아 확인하고, 처리 중이면 놓은 채 처음 2ms 는 `yield`, 그 뒤 `sleep_for(1ms)`. windows 의 `sleep_for(1ms)` 는 timer 해상도 때문에 ~15.6ms 라 2ms 넘게 걸린 처리가 끝난 뒤 destroy 가 그만큼 늦게 돌아올 수 있다(전과 같은 크기. 깨우기를 넣으려면 헤더에 멤버가 필요해 두지 않음).
  - thread_local 로 "이 thread 가 지금 실행 중인 strand" 를 쌓는다(strand 의 executor 가 다른 strand 면 중첩). 자기 executable 안의 destroy 는 기다리지 않고 표시만 하고 돌아온다. 지금 executable 뒤에 남은 같은 배치와 그 사이 들어온 큐는 **같은 thread 가 순서대로** 실행하고(`process_post_next`), 처리 중 표시를 지운다. `post_at` 의 예약분은 버린다(전과 같음). destroy 뒤의 post 는 false(전과 같음).
  - post / 실행 경로의 비용 : 실행 배치마다 thread_local 포인터 두 번 쓰기, `process_post_next` 에서 목록 탐색 한 번(보통 깊이 1).
- **linux 기본 thread 수** (`api::get_core_count()`) : `hardware_concurrency()`(machine 의 CPU) 대신 **affinity(`sched_getaffinity`) 와 cgroup CPU 한도 중 작은 것**. cgroup v2 는 `/proc/self/cgroup` 의 경로부터 root 까지 `cpu.max` 를 올라가며 가장 작은 `ceil(quota / period)`, v1 은 `/sys/fs/cgroup/cpu/cpu.cfs_quota_us`. 처음 한 번 계산해 둔다(실행 중 affinity 변경은 반영 안 함). 이 값을 쓰는 것 : 실행기 기본 thread 수(core x2 + 2), extra thread 기준, acceptor prepare 수, redis 세션 수, `updatable` 배치 thread 수.
  - 확인(SAKPUNG, 32 CPU) : 그냥 32, `taskset -c 0-2` 3, `systemd-run --user --scope -p CPUQuota=250%` 3, `docker run --cpus=2`(mysql 이미지에 host loader 로 실행) 2. 원본은 모두 32.
  - windows 는 그대로(`hardware_concurrency()` : 프로세스 affinity·job object 한도 미반영).
- **테스트** (`test_executor.cpp`, execute) : `CGDK10_executor.strand_destroy_inside_executable`(executable 안 destroy 가 돌아오고 순서 1234), `strand_destroy_while_processing`(destroy 대기 중 post 최대 < 100ms, destroy < 1s), linux 만 `core_count_follows_affinity`(get_core_count <= affinity CPU 수 : `taskset -c 0-1` 로 돌리면 원본은 실패). 원본으로 windows 앞 둘 실패(3/3), linux 는 `while` 이 60초 timeout·`inside` 실패.
- **검증** : windows DOLPUNG Release(cl 로 test_executor.cpp + 새 strand.cpp, 공유 lib 출력 안 건드림) 21 통과, strand 테스트 x30. SAKPUNG(kernel 6.12, g++ 15.2, r11528 export + 변경) Release execute 66 통과, `taskset -c 0-1` 통과, strand x30, Debug execute 66 통과, strand·core_count x20. net_socket.http 등 다른 스위트는 돌리지 않았다(strand 를 직접 쓰는 것은 system.object 의 coroutine(strand 에 묶인 coroutine : execute 의 `test_coroutine.cpp` 가 시험)과 tutorial `execute.05.strand`, `execute.06.coroutine` 뿐).

### 1.38 windows gather 송신 - WSASend 를 송신 큐 락 밖에서 (남은 일 14, 이 커밋, 2026-10-24)

남은 일 14 의 "락 안 WSASend". windows 만 바뀐다. **헤더 멤버 추가(`Ntcp_async_gather`, `Ntcp_async_gather_buffered` 의 `m_lockable_send_call`) : CGDK lib 전체 + gruma 재빌드 필요**(redis session 도 `Ntcp_async_gather_buffered` 를 상속).

- **전** : 보내는 thread(`m_flag_sending`)가 `m_lockable_sending`(recursive_mutex)을 잡은 채 WSASend 를 불렀다. 같은 socket 에 보내는 다른 thread 는 큐에 넣기만 하면 되는데도 그 syscall 이 끝날 때까지 락에서 잠들었다. 완료 처리도 락 안에서 다음 묶음을 WSASend.
- **후** : 큐 조작(`m_queue_pending` 에 넣기, `m_queue_sending` 으로 가져오기, `m_flag_sending`)만 `m_lockable_sending` 안. 보내는 thread 는 락을 놓고 `_process_send_queue_sending()` 으로 보낸다 : 그 사이 다른 thread 는 기다리지 않고 `m_queue_pending` 에 쌓는다(다음 묶음이 커진다). `m_queue_sending` 은 보내는 thread 만 쓰므로 순서·중복 송신 보장은 그대로(보내는 thread 는 언제나 하나).
  - handle 재사용 경합 : handle 을 읽고 WSASend / shutdown 하는 것은 새 락 `m_lockable_send_call` 안. close(`Ntcp::process_close_socket`)는 `process_get_lockable_sending()` 이 돌려주는 그 락 안에서 handle 을 INVALID 로 바꾼다 : 닫힌 뒤 같은 값으로 만들어진 다른 socket 에 보내지 않는다.
  - 완료가 WSASend 가 돌아오기 전에 다른 thread 에 올 수 있어 `m_queue_sending.clear()`(완료 처리, `process_reset_sendable`)도 `m_lockable_send_call` 안.
  - 락 순서 : `m_lockable_sending` -> `m_lockable_send_call` 만(반대 없음). 실패 시 `process_reset_sendable` 은 두 락 밖에서 부른다.
  - send defer 의 flush(`_process_send_deferred`)는 이제 락 안에서 모으기만 끝내고(`m_pdefer_owner` = nullptr), 부른 쪽이 락 밖에서 보낸다. close 전 flush(`process_flush_deferred_send_on_close`)도 handle 을 바꾸기 전에 그 자리에서 보낸다.
  - 바뀌지 않은 것 : `Ntcp_async`(tcp_no_gather : 여러 thread 가 동시에 WSASend 하므로 락 안이 순서 보장), raw / crypt / sync sendable, linux 전부.
- **측정**(DOLPUNG 24 thread, Release, 8 thread 실행기, 교대 5라운드 중앙값, msg/s. 벤치는 스크래치 `bench_windows_io.cpp` : linux bench 의 case + 여러 std::thread 가 보내는 case) :

| case | 전 | 후 |
|---|---|---|
| tcp fan-in 64B : 8 thread -> socket 1개 | 4.48M (메시지당 CPU 0.95us) | **14.85M** (0.62us) |
| tcp fan-out 64B : 8 thread -> socket 32개 돌아가며 | 0.68M (17.0us) | **20.2M** (0.77us) |
| tcp echo 64B x32 window16 / 64KB x4 / 16B x256 / udp | 7.64M / 114K / 526K / 369K | 7.64M / 118K / 526K / 369K (같음) |

  - 전에는 fan-in 에서 보내는 thread 들이 락에서 잠들어 CPU 4 코어만 썼고, fan-out 은 송신마다 WSASend 1번(메시지 1개)이었다. 보낸 수 = 받은 수(검증 출력).
  - echo 는 handler 안 송신이라 경합이 없어 같다. 위 표의 echo "전" 은 아래 auto_scope 를 넣은 기준이다.
- **테스트** : `test_send_defer_auto.cpp` 를 windows 에서도 빌드(전에는 linux 만 : vcxproj 두 개에 추가). select 로 기다리는 client. close case 의 송신 통계는 windows 가 WSASend 완료 때 세므로 읽기 뒤에 본다. windows auto 끈 판은 첫 WSASend 의 완료를 기다리던 큐가 abortive close 에 버려지므로(RST 가 어차피 버림) 1~50. 새 테스트 `send_defer_auto.close_while_sending_{gather,buffered}` : 4 std::thread 가 보내는 중에 closesocket, 60회 : 도착분은 sender 마다 순서대로, close 직후 accept 된 socket 은 아무것도 받지 않음.
- **검증**(Windows VS18 x64, 별도 체크아웃 D:\CGDK\wiocp r11529 + 변경, 공유 lib 안 건드림) : Debug net_socket.http 93 / execute 65 / server_system.service 172(skip 6), Release 93 / 65 / 176(skip 2) 통과. execute 의 `strand_destroy_inside_executable`(r11529)이 Debug 전체 실행에서 간헐 실패 : 변경 없는 r11529 도 5회 중 1회(아래 남은 일 26 : 테스트의 경쟁, r11532 가 고침).
- **FILE_SKIP_COMPLETION_PORT_ON_SUCCESS 는 미룸** : 순수 Win32 loopback echo(같은 장비, 교대 5라운드)로 상한을 쟀다. WSASend 는 loopback 에서 100% 즉시 완료, WSARecv 는 작은 메시지 0~4%, 64KB 71%. 송신만 그 자리에서 완료 처리 : 작은 메시지 +3~9%, 64KB +9%. 수신까지 : 64KB +66%(271K -> 450K). CGDK 의 메시지당 비용이 순수 Win32 의 2~6배라 CGDK 에서의 비율은 더 작다. 모든 I/O(accept·connect·disconnect(TF_REUSE_SOCKET 으로 handle 재사용)·UDP)가 동기 성공을 처리해야 해서 따로 설계할 것.

### 1.39 windows 실행기에도 send_defer::auto_scope (남은 일 14, 이 커밋, 2026-10-24)

- **결정 (사용자, 2026-10-24) : linux(r11496)처럼 windows 실행기도 executable 마다 auto_scope 를 열고, 기본 켜짐**. 끄려면 `send_defer::enable_auto(false)`.
- **전** : windows `executor::io_context::process_execute()` 에는 auto_scope 가 없었다(1.30 에서 동작 변경 없이 두었다). echo 처럼 수신 handler 가 받은 메시지마다 send 하면, 보내는 thread 가 없을 때마다 WSASend 하나(메시지 하나)가 나가고 그 완료가 IOCP 로 돌아왔다. linux 는 send cork / auto_scope 로 수신 한 번의 응답을 한 번에 보낸다.
- **후** : `executor.io_context.cpp` windows 분기에서 executable 마다 `send_defer::auto_scope`. 같은 socket 으로 이어지는 송신은 executable 이 끝나거나 다른 socket 으로 보낼 때 한 번의 WSASend 로 나간다(1.32·r11496 의 규칙 그대로 : close, scoped_thread_block, 큐 한도, send_defer::flush 에서는 먼저). `Ntcp_async_gather` / `_buffered` 의 iocp 쪽은 send defer 를 이미 지원했다(그룹 락 용). `.cpp` 만 변경 + `send_defer.h` 주석 : 재빌드 불필요(다시 링크).
- **측정**(1.38 과 같은 벤치·장비, 세 판 교대 5라운드 중앙값 msg/s : r11529 / auto_scope 만 / auto_scope + 1.38) :

| case | r11529 | auto_scope | + 락 밖 WSASend |
|---|---|---|---|
| tcp echo 64B x32 window16 | 2.38M (3.27us) | 7.55M (1.05us) | 7.54M (1.04us) |
| tcp echo 64KB x4 window2 | 107K | 99K | 101K (범위 96~117K, 같음) |
| tcp echo 16B x256 window1 | 504K | 496K | 496K (같음) |
| udp echo 64B x16 window32 | 352K | 352K | 353K |
| tcp fan-in 64B 8 thread -> 1 | 4.04M | 3.94M | 10.5M (범위 10.0~14.9M) |
| tcp fan-out 64B 8 thread -> 32 | 0.69M | 0.76M | 17.2M |

  - fan-in / fan-out 은 executor 밖 std::thread 가 보내므로 auto_scope 와 무관하다(1.38 이 고침).
- **검증** : 1.38 과 같은 실행(auto_scope 를 넣은 트리) : net_socket.http 93 / execute 65 / server_system.service 통과. `send_defer_auto.*` 가 windows 에서 auto 켬·끔 모두 통과.

### 1.40 linux epoll UDP 수신 - recvmmsg 배치 수의 두 번 저장이 스택 배열을 넘었다 (남은 일 23, 이 커밋, 2026-10-24)

- **증상** : `bench.epoll` udp case 의 "stack smashing detected"(남은 일 23). r11535 export 의 `-fstack-protector-strong -D_FORTIFY_SOURCE=3 -g` Release 를 gdb 아래에서 `CGDK_BENCH=3 CGDK_BENCH_THREADS=4` 로 8개(monsoon 6개) 동시 반복 : mistral 320회 중 1회, monsoon(Ultra 7 270K) 300회 중 1회, POKPUNG 320회 0. ASan(`detect_stack_use_after_return=1`) mistral·POKPUNG 260회 0. 두 재현 모두 `Ndatagram::process_complete_receivable`(`net.io.receivable.datagram.epoll.cpp`) 이 돌아갈 때의 `__stack_chk_fail`, 그 위 반환 주소 자리가 `0x0000555500000010`, 스택 주소, 1, 0 (mmsghdr 의 msg_namelen 16, msg_iov, msg_iovlen 1, msg_control 모양).
- **원인** : `m_count_receive_batch`(recvmmsg 한 번의 datagram 수, 1~32 적응)가 plain `int` 였다. 등록이 EPOLLET(ONESHOT 아님)라 같은 UDP socket 을 executor 의 여러 thread 가 동시에 받는다(한 thread 가 비우는 중 도착한 datagram 이 다른 thread 를 깨운다). g++ 15 -O2 는 `m_count_receive_batch = (count_batch * 2 < 32) ? count_batch * 2 : 32;` 를 `count_batch * 2` 저장 뒤 cmov 로 32 를 다시 저장하는 **두 store** 로 만든다(디스어셈블 확인). 그 사이 다른 thread 가 64 를 읽으면 32칸 스택 배열(`shared_buffer` / `sockaddr` / `iovec` / `mmsghdr`)에 64개를 채우고 recvmmsg 가 그 뒤에 쓴다. 프레임 배치는 `mmsghdr[32]` 바로 위가 canary 라 canary 와 반환 주소가 깨진다(canary 없는 빌드는 SIGSEGV : 처음 본 회차의 exit 139). 창이 명령 하나라 드물고, 바쁜 장비에서 thread 가 그 사이에 끊길 때 난다.
- **수정** : `std::atomic<int>`(relaxed : 한 번에 저장) + 읽은 값을 `std::clamp(…, 1, COUNT_RECEIVE_BATCH_MAX)`. 헤더 멤버 타입 변경(linux 만) : 사용하는 쪽 재빌드. windows 변경 없음. 같은 socket 을 여러 thread 가 동시에 받는 것 자체(UDP 라 순서 보장 없음)는 그대로 두었다.
- **테스트** : `linux_io.udp_receive_on_many_threads`(`test_linux_io.cpp`) - epoll executor 16 thread 가 UDP socket 하나를 받고, std::thread 8개가 raw socket 의 sendmmsg 로 3초 퍼붓는다(배치가 늘 가득 차 2배가 계속 일어남). mistral stack protector Release : 고치기 전 20/20 "stack smashing detected"(같은 프레임), 고친 뒤 20/20 통과.
- **검증** : 수정본(r11535 + 이 변경) mistral stack protector Release : net_socket.http 전체 epoll / io_uring / completion 각 161 통과, bench.epoll 8개 동시 x20 = 160회 재현 0(고치기 전 재현율이 320회 중 1회라 이것만으로는 판정이 안 된다 : 판정은 위 테스트의 20/20 대 0/20).
- 스크립트(`repro_m23.sh` gdb 반복, `ab_m23.sh` 고치기 전/후, `verify_m23.sh`)는 이 작업의 노트 폴더(DOLPUNG `%TEMP%/claude/D--CGDK-CGDK10-Cpp/6cb999c7-a833-4ed9-ae82-2b30335dab39/scratchpad/t/`)에 있다. 이 문제를 찾은 방법 : 재현 1회의 gdb backtrace 로 함수를 정하고, `disassemble /s` 로 배열 offset 과 canary(-0x38) 위치, `m_count_receive_batch` 의 store 를 봤다.

### 1.41 socket 송수신 점검 - 정확성 수정과 남은 최적화 (r11538~r11554, 2026-10-24 저녁)

r11537 까지의 socket 송수신(Windows IOCP, Linux epoll / io_uring readiness / completion, TCP / UDP)을 다시 읽어 나온 후보를 사용자 결정으로 처리했다. 측정은 모두 Release, 교대 5라운드 중앙값.

- **정확성 (linux)**
  - r11538 : UDP 송신이 EPOLLOUT 을 잃고 영원히 멈추던 경쟁. `_process_set_epollout()` 의 set_event 를 송신 락 안으로, EPOLLOUT 처리는 `m_flag_epollout` 이 켜져 있을 때만 큐를 이어받는다. 테스트 `udp_send_epollout.*`(테스트 실행 파일의 sendto / sendmmsg 를 바꿔 EAGAIN 주입 : loopback UDP 는 EAGAIN 이 안 난다) 고치기 전 epoll 10회 중 8회 정지.
  - r11539 : FIN 과 같이 온 데이터를 `MAX_RECEIVE_PER_EVENT`(8)에서 끊고 닫던 문제(epoll / readiness). 상대가 닫은 event 는 한도 없이 비운다. 테스트 `tcp_receive_at_close.*`(고치기 전 12MB 중 4.5MB 만 받고 RST).
  - r11540 : accept 의 EMFILE / ENFILE 에서 worker 가 listen socket 에서 돌던 문제(epoll CPU 4코어). 예비 fd(/dev/null)로 대기 접속을 받아 바로 닫는다(libuv 방식). readiness 의 accept() 는 전역 shared_mutex 의 shared 안 : 예비 fd 를 닫은 순간 다른 worker 가 그 번호를 가져가 다시 돌던 것을 막는다. completion 의 multishot accept CQE 도 같은 처리(테스트에서는 completion 이 낮춘 RLIMIT_NOFILE 아래서도 fd 를 받아 EMFILE 이 나지 않아 확인 못 함). 테스트 `accept_fd_exhausted.*`.
  - r11554 : epoll TCP 수신 버퍼 크기 `_required_bytes + bytes_stored` 이중 계산(windows 는 r11550 에서 같이 고침).
- **정확성 (windows)** r11548 : UDP port unreachable 보고(r11499 의 GetQueuedCompletionStatusEx 뒤 last error 가 그 I/O 의 것이 아님 : OVERLAPPED::Internal 의 NTSTATUS 로), 0 byte datagram 을 message 로. r11549 : Nudp 송신의 handle 을 수신 락(shared) 안에서 읽는다. r11551 : 송신 완료가 `statistics_on_increase_async_sending` 을 줄이지 않아(Ntcp_async 는 송신 I/O 를 두 번 셈) 한 번이라도 보낸 socket 의 `statistics_get_now_async_io()` 가 0 이 되지 않았다 : acceptor stop 의 좀비 정리·emergency acceptor 종료 판정이 그 socket 을 놓쳤다. raw 송신 두 파일도. socket.crypt 의 같은 계열은 손대지 않았다.
- **성능 (linux epoll)** r11542(+ r11541 에 섞인 한 줄) EPOLLWAKEUP 제거(권한 없는 실행에서는 차이 없음, root 실행은 미측정), r11543 UDP 송신 배치를 list splice 로(4.6KB malloc 제거), r11544 UDP recvmmsg 배치 최소 2(하나씩 오는 event 의 recvmmsg 메시지당 2.57 -> 1.44번), r11545 등록 표 bucket alignas(64)(헤더), r11546 송수신 바이트 통계 시각을 `chrono::tick::now_coarse()`(CLOCK_MONOTONIC_COARSE, 헤더). 루프백 echo 처리량은 모두 ±2% 안.
- **성능 (windows)** r11548 UDP 수신 락을 shared_mutex(SRW)로 + 버퍼 재사용(헤더 : `m_lockable_receiving`), r11549 UDP 송신을 먼저 비overlapped sendto(overlapped 송신 중이면 안 함) : UDP fan-in 319K -> 741K, echo 342K -> 390K msg/s. r11550 TCP 수신 버퍼 규칙을 epoll 판으로(헤더) : 64KB +5.8%. D4(완료의 m_queue_sending 정리를 락 밖으로)는 재측정에서 손해라 넣지 않음.
- **성능 (completion)** r11552 RING 의 다른 thread 가 읽는 필드를 캐시 라인으로 분리 + RING_GUARD 를 worker 자기 ring 카운터로, r11553 ring 별 OP free list : POKPUNG +1~4%(monsoon +3~12%), 단일 RECV 모드(`CGDK_IO_URING_MULTISHOT=0`) +32~135%. C3(무복사 수신의 io_context 참조), C4(wake 의 eventfd_read : kernel 7.0 은 읽지 않아도 multishot poll 이 write 마다 CQE), C6(UDP multishot POLL : -13%)은 이득이 없어 넣지 않음.
- **epoll ONESHOT 조사(남은 일 27)와 백엔드 비교 표** : `/mnt/project-files/bench_backends_2026-10-24.md`(POKPUNG 32 CPU : completion 이 작은 메시지 모든 thread 수에서 가장 빠름, epoll 은 8 thread 가 정점이고 16 thread 에서 떨어짐, 64KB 4 thread 만 epoll 이 가장 빠름, readiness io_uring 은 TCP 작은 메시지에서 epoll 의 절반).
- 헤더 변경 커밋 : r11545, r11546, r11548, r11550 -> CGDK lib 전체와 gruma 재빌드.

### 1.42 linux send_defer::auto_scope(r11496)의 64KB echo 재확인 - 잡음 (2026-10-24 밤)

- **커밋** : `send_defer::auto_scope` / `enable_auto()`(실행기가 executable 마다 auto_scope, 기본 켜짐)는 **r11496**(send_defer.h, executor.io_context.cpp, api.io_context.epoll_direct.cpp, api.io_context.uring.cpp, test_send_defer_auto.cpp, CMakeLists.txt). HEAD r11555 까지 되돌림 없음(이후 r11497 배치 defer, r11512 테스트 기대 수정, r11531 windows 도 auto_scope).
- **의문** : 커밋 전 echo 측정에서 64KB(large 16 x 2 x 65536)가 auto 판 gather 에서 8회 중 4회 35.2~35.7만(기준 8회 중 1회, 중앙값 37.2만 / 37.3만). 그때의 auto 판은 r11486 + auto 패치(`bench_echo_server_slow_g_auto`, `lib_xauto`), 기준은 r11486.
- **재측정** (POKPUNG, `~/cgdk_epoll_opt/bench/run_echo_large_rot.sh` = run_echo_retry.sh 의 64KB 만 + 회차마다 구성 순서 회전, 서버 tcp<> gather 8 thread core 0-7, 클라 16-31, 8초, 다른 cpu 1 core 이상인 회차는 버리고 다시) :
  - 구성 : B = 정확한 r11495(`~/cgdk_bench_r11495` = r11496 트리에서 auto 4 파일만 r11495 로, `bench_echo_server_slow_g_r11495`), A / A0 = 정확한 r11496(`~/cgdk_auto2`, `bench_echo_server_slow_g_r11496t` : 6번째 인자로 `enable_auto(1/0)`, 같은 바이너리).
  - 1차 12회 교대(23:08~23:18, 버린 회차 0) msg/s : B 중앙값 37.28만(36.76~37.47만), A 37.22만(37.08~37.55만), A0 37.26만(36.57~37.45만). cpu us/msg 중앙값 17.22 / 17.44 / 17.22(범위 16.9~17.9 로 셋이 겹침). p50 81.6 / 81.7 / 81.7us. **36만 아래 회차 없음**(A 최솟값 37.08만).
  - 2차 8회 교대(23:18~23:25, 도중 다른 부하 : load 8~9, 3회 버림) r11486 / r11486+auto(옛 gA) / r11496 : 35만대 회차가 **기준 r11486 에도 3회**(35.5~35.6만), 옛 gA 2회(33.7만, 35.3만), r11496 0회(36.1만이 최저). 중앙값 36.2만 / 37.1만 / 37.0만.
  - 원시 파일 : `echo64k_main_1024_230833.txt`, `echo64k_main_1024_231837.txt`(+ `.rejected`).
- **판정 : 잡음**. 조용할 때 같은 바이너리의 auto 켬/끔과 r11495 가 처리량·CPU·지연 모두 범위 안에서 같고, 35만대 저하는 auto 와 무관하게 기준에서도 다른 부하가 있을 때 나온다. 64KB echo 는 handler 하나에 송신 하나라 auto_scope 가 묶을 것이 없다(flush 는 executable 끝 한 번).

### 1.43 10GbE zerocopy 송신 측정과 completion SEND_ZC 의 -ENOMEM 끊김 (남은 일 25, 2026-10-25)

- **장비** : 서버 MISTRAL(192.168.0.94, i9-13900K), 클라이언트 MONSOON(192.168.0.95, Core Ultra 7 270K), 둘 다 ixgbe 10G(enp5s0 / enp130s0, MTU 1500, SG·TSO 켜짐), kernel 7.0, `ulimit -l` 8MB(sudo 없음). 트래픽이 10G 포트로 간 것은 서버 enp5s0 의 `/proc/net/dev` 카운터로 확인(측정 전체 양방향 각 1.49TB). 회선 상한 1.11GB/s.
- **방법** : r11579(scheduler·runnext 변경의 release_note 까지 들어간 2026-10-25 HEAD) Release, `bench.remote`(서버 tcp<> echo). 클라이언트는 completion 8 thread zerocopy 끔으로 고정, 8 접속 x window 2, 6초. 서버만 바꿈 : `CGDK_BENCH_THREADS` 1 / 2 / 8, backend completion · io_uring(readiness) 의 zerocopy 켬(threshold 16KB) / 끔 + epoll(zerocopy 안 씀 : 기준), 구성을 돌아가며 5라운드, 서버 us/msg 중앙값. 스크립트·원시 결과 : MISTRAL `~/cgdk_zc10g/results`(`zc10g_v2.sh`, `summ.pl`, `main.txt`, `big.txt`, `big2.txt`).
- **256KB 넘는 메시지** : 수신 메시지 버퍼 한도(`default_message_buffer_max_size()`, 기본 256KB = 메모리 풀 최대 블록)를 넘으면 서버가 끊는다. `bench.remote` 가 `CGDK_BENCH_REMOTE_SIZE` 에 맞춰 양쪽 한도를 올리게 했다(이 커밋). 서버 socket 이 이쪽 오류로 닫히면 사유와 errno 를 찍는다.

| 크기 | thread | completion 끔 -> 켬 (us/msg) | readiness 끔 -> 켬 (us/msg) | epoll |
|---|---|---|---|---|
| 16KB | 1 / 2 / 8 | 9.40 -> 9.44 / 17.5 -> 17.7 / 31.7 -> 31.1 (0%) | 9.27 -> 9.25 / 17.2 -> 20.0 / 33.6 -> 45.1 (**+16%, +34%**) | 9.66 / 18.1 / 38.6 |
| 64KB | 1 / 2 / 8 | 26.9 -> 25.2 / 47.3 -> 45.1 / 82.0 -> 79.1 (**-6, -5, -3%**) | 27.3 -> 26.2 / 47.6 -> 45.4 / 91.8 -> 92.9 (-4, -5, +1%) | 27.5 / 49.0 / 127 |
| 256KB | 1 / 2 / 8 | 107 -> 99 / 173 -> 158 / 320 -> 278 (**-7, -9, -13%**) | 109 -> 100 / 177 -> 150 / 344 -> 299 (-9, -15, -13%) | 106 / 161 / 373 |
| 512KB | 1 / 8 | 215 -> 202 / 559 -> 536 (-6, -4%) | 217 -> 203 / 497 -> 449 (-6, -10%) | 215 / 475 |
| 1MB | 1 / 8 | 425 -> 416 / 1127 -> 1073 (-2, -5%) | 427 -> 416 / 1014 -> 934 (-3, -8%) | 427 / 938 |

  - 처리량 : 64KB 이상은 모두 회선 상한(1.11GB/s)이라 zerocopy 효과는 서버 CPU 로만 보인다. 예외는 **readiness 켬의 처리량 저하** : 16KB 2 thread 790 -> 647MB/s, 8 thread 829 -> 599MB/s, 64KB 8 thread 1112 -> 1002MB/s(MSG_ZEROCOPY 통지를 error queue 로 읽는 비용, 16KB 는 클라이언트 window 로 회선 아래). completion 은 저하 없음.
  - copied : 실 NIC 에서 130만 송신에 0~40개(1.36 의 1GbE 와 같은 수준), copy check 로 끈 socket 0.
  - memlock : 512KB·1MB 는 접속당 2MB 가 ACK 까지 고정돼 8MB 한도를 넘는다. readiness 는 ENOBUFS -> 복사(fallback 1MB 라운드 합 1296~1385), completion 은 1MB 8 thread 에서 32K 송신 중 SEND_ZC 9.8K 뿐(나머지는 100ms 쉼의 복사). 그래서 512KB 이상의 이득이 256KB 보다 작다. 실서버는 `LimitMEMLOCK` 을 동시 대용량 송신 바이트 이상으로.
- **completion SEND_ZC 의 -ENOMEM 으로 socket 이 닫힘 (발견)** : 처음 512KB·1MB 측정에서 completion 켬 1 thread 가 10회 중 2회 거의 멈췄다(7.6, 4.5MB/s). 서버 socket 8개 중 7개가 메시지 1~3개 뒤 `errno 12` 로 닫혔다. io_uring 은 memlock 초과(`__io_account_mem`)를 **-ENOMEM** 으로 돌려주는데 `_process_complete_send_op` 은 -ENOBUFS(MSG_ZEROCOPY 의 errno)만 복사로 다시 보내고 나머지는 실패로 닫았다(r11490 의 대체가 completion 에서는 안 걸림). 8 thread 는 남은 socket 이 회선을 채워 처리량으로는 안 보였고, 그때의 completion 켬 CPU(1MB 636us)는 socket 이 줄어든 탓이었다. -ENOMEM 도 같은 대체로 고친 원격 사본에서 다시 쟀다(위 표의 completion 512KB·1MB, 60회 끊김 0). 라이브러리 수정은 r11588(1.44) : -ENOMEM 도 -ENOBUFS 처럼 복사로 다시 보내고, 실패한 SEND_ZC 의 통지(COPIED 비트로 온다 : fallback == copied, 연속 8번이면 socket 의 zerocopy 가 꺼질 수 있었다)는 sent / copied 로 세지 않는다.
- **결론** : completion 의 큰 버퍼 SEND_ZC op(직접 send 를 건너뜀)는 유지 : 손해가 없고 64KB 이상에서 서버 CPU 3~13% 이득. `zerocopy_threshold()` 기본 16KB 는 높이는 것이 맞다 : 16KB 는 두 backend 모두 이득이 없고 readiness 는 처리량이 떨어진다. 64KB 로 올리는 안은 사용자 결정으로 남김(남은 일 25). epoll 대비로는 completion 이 64KB 8 thread 에서 38% 싸고(82 vs 127us), 512KB 이상 8 thread 는 epoll 이 15~20% 싸다(이유는 보지 않음).

### 1.44 전체 점검 - 버그 수정과 남은 최적화 (r11582~r11620, 2026-10-25)

r11579 의 코드를 네 영역(io_uring completion, 실행기 · strand, windows socket · 암호화 socket, server 계층)으로 나눠 다시 읽고 나온 후보를 사용자 결정으로 모두 처리했다. 각 수정은 고치기 전 실패하는 테스트를 붙였다(없는 것은 따로 적음). 확인 : windows VS18 x64 Debug / Release(개인 빌드), POKPUNG(kernel 7.0, g++ 15.2) HEAD export 에서 epoll / io_uring readiness / io_uring completion 세 backend 모두 통과(net_socket.http 188, execute 108). 커밋별 결과는 각 커밋 메시지.

- **io_uring completion**
  - r11582 : SEND_ZC 버퍼 owner 를 그 op 의 통지가 올 때만 놓는다. 전에는 모든 op(일반 SENDMSG repost 포함)의 마지막 CQE 가 가장 오래된 owner 를 지워, 실 NIC 의 재전송이 재사용된 메모리를 보낼 수 있었다. 테스트 `uring_send.zerocopy_owner_till_notification_*`(가짜 io_context 로 CQE 순서를 만듦).
  - r11583 : disable 중에 multishot accept 가 끝나도 다시 건다(전에는 enable 뒤에도 accept 가 없어 backlog 에 쌓임). 동작 변경 : disable 중에도 accept op 가 살아 있고 받은 것은 닫는다.
  - r11584 : lent ring 에서 끝난 wake eventfd poll 을 owner 가 다시 건다(`RING::is_wake_poll_lost`). 테스트 없음(CQ 가득 / 내부 poll 취소로만 남).
  - r11585 : fd 취소 scan 도중 `_adopt_staged` 가 op 를 다른 ring 으로 옮기면 다시 훑는다(`s_count_adopt`). 테스트 없음(창이 결정적으로 안 만들어짐).
  - r11588 : SEND_ZC 의 -ENOMEM(memlock 초과) 대체와 실패한 op 통지 미집계(1.43). 헤더 변경(linux, `m_owners_zerocopy` 원소 : `ZEROCOPY_OP`).
  - r11590 : close 의 fd 취소가 inflight 0 이면 바로 반환. r11591 : LENT ring 의 handle 을 다른 ring 으로 영구히 옮기지 않는다(ORPHAN / DRAINING 만). 8 thread echo 64B 19.32M -> 19.70M, 16B 1.470M -> 1.559M, uds 범위 안.
- **실행기 · strand · thread**
  - r11598 : strand 가 executor 의 post 거부(정지)를 무시해 `m_is_processing` 이 켜진 채 남고 destroy() 가 영원히 돌던 문제, handler 안의 `post(); destroy();` 가 runnext 슬롯 / 1 thread executor 에서 끝나지 않던 문제. 이제 실행 중인 thread 가 있을 때만 기다리고, post 만 된 상태면 destroy() 가 직접 실행한다.
  - r11599 : strand post_at 이 system scheduler thread 에서 실행되던 것을 strand 의 executor 로, multimap 정렬 + 가장 이른 깨우기 하나만 예약.
  - r11600 : linux `force_raise_exception` 은 아무것도 하지 않고 false(동작 변경). 전에는 SIGUSR2 handler 를 잠깐 바꿔 끼워 비동기 도착 신호가 emergency 종료 handler 로 갈 수 있었고 signal handler 의 throw 는 UB 였다. deadlock 감시의 로그 · dump 는 그대로.
  - r11601 : queue_list 가 stop 뒤 post 를 거부(`m_is_accepting`), execute_all 이 예외를 잡는다(전에는 noexcept 안 throw 로 terminate).
  - r11602 : schedulable 의 executor / manager 포인터를 락 안에서, `now() + interval` 포화. r11605 : extra thread 수를 시작 / 깨울 때 셈(spawn 유실), queue_list_threads 의 park 조건에 threshold, blocked 수 초기화를 mutex 안에서.
  - r11613 : strand · queue_list 큐 락 recursive_mutex -> std::mutex(POKPUNG queue_list_threads +26~28%). r11614 : windows 기본 thread 수 = 활성 프로세서, affinity popcount, job CPU 한도 중 작은 값(동작 변경 : affinity / job 아래서 thread 가 줄어듦).
  - r11618 : 블로킹(scoped_thread_block)하는 worker 가 들고 있던 묶음의 나머지를 다른 worker 에 넘긴다(`BATCH_HANDOFF`, runnext_suspend 에서). epoll / readiness 는 post 묶음(최대 16)을 post 큐 앞에 되돌림, completion 은 전부터 있음. 테스트 `io_context_threads_extra_threads_batch`(고치기 전 epoll / readiness 22/36). 동작 변경 : 나머지가 다른 thread 에서 먼저 돌 수 있다.
- **windows socket · 암호화 socket**
  - 후보 A1(windows close 가 send_call 락 없이 handle 교체)은 아니었다 : gather / gather_buffered 는 r11530 부터 `process_get_lockable_sending()` 이 `m_lockable_send_call` 을 돌려준다. 같은 경쟁이 raw 판 두 개에 남아 있어 r11619 로 고쳤다(헤더 변경).
  - r11594 : `Icryptable` 의 cryptor 를 mutex 안에서(헤더 변경, gruma 재빌드). r11595 : 클라이언트가 ACCEPT_CERTIFICATION 과 같은 수신으로 온 암호문을 평문으로 처리하던 문제, on_receive 에 버퍼 끝 너머 view, epoll 쪽 재복호화 · 재할당 · EAGAIN 처리. r11596 : `Ntcp_crypt_async_gather_buffered` 의 reset 이 송신 flag 를 끄지 않던 것(재사용 socket 이 영영 안 보냄), 완료가 async 송신 수를 줄이지 않던 것(r11551 계열), 락 밖 암호화(여러 thread 송신 시 keystream 순서 꼬임), epoll 의 EPOLLOUT 대기 정지. r11597 : `sendable::Ntcp_crypt` 의 이중 집계 · 락 밖 handle.
  - r11620 : epoll 이 EPOLLOUT 처리 중 다시 요청된 EPOLLOUT 을 잃던 문제(registered 마스크에서 지우지 않아 MOD 가 안 감, EPOLLET 라 새 edge 없음). 테스트 `epollout_rearm.*`. bench.epoll 범위 안.
  - r11612 : 남은 일 28.
- **server 계층**
  - r11586 : HTTP 서버 chunk 크기 overflow(한 클라이언트가 메모리를 제곱으로 늘림), chunk 뒤 CRLF 검사, Transfer-Encoding 대소문자 무시 · 마지막 coding 이 chunked 가 아니면 400, 잘못된 Content-Length 400(전에는 0), websocket 길이 wrap(제한 0 일 때). 동작 변경 : 위 요청은 400.
  - r11587 : time_event `unregister_event` 조건 반대(있는 event 는 안 지워지고 없는 것은 erase(end())), heap 안 entity 의 시각 변경을 manager 경유로 재정렬, stop 의 lost wakeup. 헤더 변경(`server.time_event.manager.h`). r11603 / r11604 / r11609 / r11611 : `reset_next_time()` 이 엉뚱한 필드, manager entity set 락 · raw 포인터 find, `Ievent::m_pentity` 미설정(이제 atomic, 참조 없음), 무한 횟수 + interval 0 의 무한 루프(이제 한 번 실행). 헤더 변경 4개.
  - r11589 : redis RESP 파서 경계 읽기 · 이어 받기 정렬 · 예외 삼킴, 그리고 null bulk 가 다음 응답 2바이트를 건너뛰던 것, 배열 원소 type 을 4바이트로 읽어 **배열 응답이 하나도 파싱되지 않던 것**. 동작 변경 : 잘못된 응답 · process_message 예외면 session 을 닫는다. 2.5KB 넘는 GET 은 r11606(`_basic_buffer::data<T>(offset)`, 다른 작업)에 기댄다.
  - r11592 : http query 의 chunk 마다 빈 std::function 호출(chunked 응답이 모두 502). r11593 : server.web 상태 줄 `HTTP/1.1 <code> <reason>`(전에는 이유 뒤 코드, 코드 0 은 500), folder 자식 해제, 빈 tail 송신 제거. 헤더 변경(`server_web.definitions_message.h`).
- **헤더 변경 정리** : executor.strand.h, executor.queue_list(_threads).h, executor.io_context(_threads).h, Ischedulable.h, util.Icryptable.h, net.io.receivable.stream_crypt.h, net.io.sendable.tcp_crypt.async_gather_buffered.h, raw gather 두 개, net.io.sendable.tcp_async_gather(_buffered).h(linux), time_event 헤더 5개, server_web.definitions_message.h -> CGDK lib 전체와 gruma 재빌드.

### 1.45 Redis 요청/응답 경로와 비교 결과 (r11623, 2026-10-26)

- `execute_query`로 동기/코루틴 이름 통합. 입력은 호출 중 소유하고 세션이 전송 직전에 인코딩한다. `m_buffer_query`/`m_lockable_state_query` 대신 세대별 원자적 완료 상태와 wait/notify를 사용한다.
- start에서 고정한 연결을 사용하고 stop에서 접근 가드를 닫아 실패시킨다. 일반 공유 연결 선택은 TLS 캐시/세션별 참조 가드이며 querier mutex가 없다. 전용/관리/대기열은 필요한 락이 남는다. TLS 캐시의 strong 참조는 닫힌 세션 객체를 thread의 다음 사용/종료까지 보존할 수 있지만 stop 후 새 요청은 거부한다.
- circular_list 미전송/응답대기 큐를 유지한다. 개별 응답은 도착 즉시 완료하고 명시적 batch만 전체 결과를 모아 완료한다. batch 응답은 최대256개씩 매칭/통계를 합산한다. RESP memchr 적용, AVX2 미채택.
- `post_execute_query` 사용자 callback post 기본 on. `START_PARAMETER.post_response=false`로 직접 처리, `presponse_executor` 미지정이면 Redis I/O. 동기 단건/배치 및 callback 없는 완료 통지는 직접 처리한다. 파싱/매칭은 I/O에 남는다. post가 모든 부하에서 빠른 것은 아니다(1연결 개별 등록 -11.8%, 4연결 +2.4%).
- Redis 자동 send_cork/전용 옵션 제거. 공용 TCP의 cork 기능과 send_defer 옵션은 유지한다. coroutine은 편의성 제공이 목적이며 Redis I/O 기본/외부 executor 선택을 지원한다.
- I/O worker 1개에서 callback 내 동기 Redis 요청이 진행되도록 여분 worker 활성화 임계값을 조정했다. 이 변화는 공용 executor 코드이므로 다른 사용자도 재빌드/회귀 확인 대상이다.
- 최종 비교60회 모두 errors=0. GET16B: CGDK/hiredis 단건 동기38,344/54,425/s, 비동기1024·4연결5.077/5.177M/s, batch1000·1연결4.468/3.736M/s, 4연결5.698/5.700M/s. 최신 기본값으로 측정했다. loopback 밖의 성능을 보장하지 않는다.
- 원본/요약/조건/정규화 소스 해시는 `benchmark/3.query_redis/results/clients_current_20261026.*`. post 비교42회는 `response_post_20261026.*`. `compare_clients.py --profile=current`로 동일60회 조건을 재실행한다. 생성 svn_version.h는 공개 해시 목록에서 제외한다.
- 검증: Windows/Linux Release Redis 테스트 각각50개(동기 단건/배치의 inline 완료 포함), 실제 Redis 순서20000건, 혼합4800건, 1/4연결 affinity 각32000건 및 동시 요청 각12000건 오류0, callback 동기 요청/stop 및 stop/restart 통과. Windows Net/System, Linux Net/Query/Redis 격리 빌드. Windows 성능/전체 SDK/GRUMA 검증은 아니다.
- Nstream/REQUEST_QUERY/Iquery/execute_result 등 공개 구조 변경으로 SDK 라이브러리와 모든 사용 프로그램을 함께 재빌드한다. 단건 동기 처리량/개별 비동기 p99 개선은 남아 있다. 별도 작업의 zerocopy 기준/벤치 주석 및 VS 경로 수정4개는 이번 제출에서 제외했다.

---

## 2. 남은 일 (우선순위 순)

1. ~~Ntcp_reconnect 즉시 실패 시 재접속 미예약~~ - 해결(r11404, 1.12).
2. ~~accept 후 attach 사이 fd 재사용 창~~ - 해결(r11414).
3. ~~`connective::connector` UDS 지원~~ - 해결(r11406).
4. ~~그룹 락의 남은 교착 가능성·null 역참조·container 콜백 UAF~~ - 해결(r11402, r11409, r11410). on_member_* 이벤트 안에서 다른 멤버 락을 잡으면 여전히 교착 : 규칙으로 .h 주석에 둠.
5. ~~object pool 종료 시 TLS 잔여 객체 누수~~ - 해결(r11411, factory::object 통계 TLS 는 r11412).
6. ~~`_buffer_unreal.h` 결함 3건과 한글 주석~~ - 수정(r11408). ~~실제 UE 빌드로 확인 필요, FString 경로 문제는 남음.~~ - 해결(r11520, 1.35) : UE 5.7·5.8 실제 빌드(고치기 전은 컴파일 실패)와 자동화 테스트, FString/FStringView 경로·TMultiMap·TSparseArray·TSharedRef 등 수정, 코어의 serializable peek offset 버그도. 남은 것은 examples 안 옛 사본과 Linux/모바일 UE 빌드(1.35).
7. ~~Linux 빌드에서 `bld/` 와 `bld_debug/` 동시 빌드 시 `svn_version.h.tmp` 경합~~ - 해결(r11403).
8. ~~io_uring completion 모드~~(1.11) - 해결(2026-10-24 : 송신 쪽까지 끝남, 1.29·1.36. 남은 것은 10GbE 실 NIC 측정뿐 : 남은 일 25. 이 항목 끝의 "지금 백엔드는 준비 이벤트만 쓰므로" 이하는 처음 적은 문장) : **1단계(백엔드) 완료 r11437(1.14). 2단계(소켓 op 전환) r11441(1.15) + 백엔드 staging r11444(1.16) + r11448(handle 한도, transient worker, acceptor 대기) + r11451(1.17 : non-owner enter 금지, flush+submit 한 락, chunk handle 표) : mistral completion 전체 스위트 통과(1회차 uds stop 지연 3개 남음 : 1.17). 3단계(multishot recv+provided buffer) r11453(1.18) : WSL·mistral 전체 스위트 3 모드 통과. 4단계 r11457(무복사 수신, 소유 worker 송신 op : 다른 세션, 커밋 메시지 참고). 수신 모드 전환 r11459(1.19 : 큰 트래픽은 IOCP 식 가변 RECV 버퍼, 작은 메시지는 multishot). 남은 것 : 송신 쪽. emergency.* 기존 실패 3개는 해결(1.20 : 테스트 전제 2개 + io_uring 두 백엔드의 post 정체 버그). r11459 Release 벤치는 2026-10-24 POKPUNG(32 CPU, kernel 7.0, 8 thread, 교대 5라운드 중앙값)에서 쟀다(**서버 epoll 측정 : 서버 socket 이 system executor 에서 돌아 completion 은 클라이언트만**, 1.27. 서버도 같은 backend 인 HEAD 표는 1.31) : 수신 모드 전환 on/off 차이 없음(tcp 64KB x4 177.0K / 177.0K msg/s, 나머지도 노이즈 안). 같은 실행의 epoll 대비 completion : tcp 64B x32 -32%(6.58M / 9.74M), uds 64B x32 -37%(6.72M / 10.73M), tcp 16B x256 +17%(848K / 726K), tcp 64KB x4 +15%(177K / 154K), udp +40%(657K / 469K). 64B window16 의 격차는 r11457 메시지의 분석(접속당 메시지가 몰릴 때의 CQE 단위 처리)대로 남았다. => 송신 쪽 정리(1.29) : buffered 도 completion(SENDMSG / SEND_ZC op, r11494), gather op 완료의 대기 바이트 수정(r11494), 64B 격차의 원인은 CQE 마다 send 하나(sendto 0.76/msg vs epoll 0.15) -> CQE 배치 끝 송신(배치 send defer, r11497)으로 공정 벤치 POKPUNG tcp 64B x32 completion 9.91M -> 18.16M(epoll 15.68M 의 +16%). 위 -32%/-37% 는 서버가 system executor(epoll)에서 돈 측정이었다(1.27).** 지금 백엔드는 준비 이벤트(POLL_ADD)만 쓰므로 syscall 수가 epoll 과 같다. IORING_OP_RECV/SEND(+ provided buffer)로 송수신 계층을 다시 써야 이득이 난다. WSL2 loopback 에서는 epoll 보다 느리다(단일 CQ 대기자) -> POKPUNG 실 NIC 에서 다시 측정할 것.
9. ~~`socket_option.not_applied_without_option` 순서 의존 실패~~ - 해결(이 커밋). 원인은 늦은 `on_connect` 가 아니라 **풀에서 재사용된 서버 socket 이 직전 acceptor 의 `socket_option()` 을 그대로 가진 것**이었다(새 acceptor 는 옵션이 없으면 덮지 않으므로 옛 quickack/notsent_lowat/nodelay off 가 새 접속에 적용됨). acceptor(두 경로)와 connector 가 socket 을 할당한 직후 `socket_option()` 을 기본값으로 되돌린다 : '자신의 값' 은 `on_request_accept()`/`on_alloc_connectable()` 에서 코드로 정한 값이다. 교훈 : 풀로 재사용되는 socket 객체에 새 필드를 넣으면 재사용 시점의 초기화를 같이 넣어야 한다(`m_socket_option` 외에도 같은 종류의 필드가 있는지 점검할 것).
10. ~~1.11 의 Linux UDP 수정 전에는 Linux UDP 수신이 전혀 안 됐다~~ - 확인 끝(2026-10-24) : **영향받은 사용자 프로젝트 없음**.
    - 언제부터 : `svn://192.168.0.60/ProjectCGDK/CGDK10.C++` r6836(2021-08-11, "convert source file to UTF8 with BOM")에서 `net.io.socket.udp.cpp` 의 `m_pexecutable_socket->set_socket(this);` 가 빠졌다(r6811 까지는 있음, 같은 커밋의 diff 는 그 3줄과 번호 주석뿐). 그 뒤 r11394(1.11)까지 약 5년 2개월 Linux(epoll, `__ANDROID__` 포함) `Nudp` 는 수신 이벤트가 socket 에 닿지 않았다. 송신(`send_to`)은 동작. Windows IOCP 는 해당 없음.
    - 찾은 범위 : DOLPUNG D:\ 의 프로젝트 폴더 전부(vcpkg·boost 등 외부 라이브러리 제외)에서 `socket::udp` / `udp_resend` / `Nudp` / `Ndatagram` 을 쓰는 소스(CGDK 라이브러리 사본 제외)와, 사내 Linux 머신(POKPUNG·MISTRAL·MONSOON·SAKPUNG·FOEHN·HUKPUNG)의 ~ 와 /project. Linux 에서 빌드되는 것은 CGDK10.Cpp 와 project_gruma(Jenkins 작업 공간) 둘뿐이다.
    - 결과 : 사용자 프로젝트 중 `Nudp` 를 쓰는 것은 없다. gruma 서버는 UDP 를 쓰지 않고, gruma·lokon·project_unreal 의 UE 클라이언트 UDP 는 CGDK UE 플러그인(FSocket 기반)이라 epoll 경로가 아니다. 영향받은 것은 CGDK 자신의 Linux 빌드뿐 : `tutorials/network.1.socket/socket.04.udp_simple`(server 가 아무것도 못 받음), `examples/1.5.udp_echo` 콘솔 클라이언트(에코를 못 받음). 둘 다 r11394 이후 정상(코드는 그대로, 라이브러리 수정으로 고쳐짐). 다른 회사·외부에 나간 Linux 빌드가 있다면 r11394 이전 lib 의 UDP 수신은 안 되므로 lib 교체가 필요하다.
12. ~~io_uring 에서 남은 간헐 실패~~ - 해결(r11417 io_uring, r11418 acceptor. 1.12). 재접속 관리자 정지·정리 지연·uds 불안정·pure virtual call 모두.
12. ~~io_uring 에서 남은 간헐 실패 (조사 중)~~ - 위 12 와 같은 항목의 옛 문장(중복) : r11417·r11418 로 해결. 그 중 `linux_io.io_uring_executor` 의 "connected yet" 정리 지연은 2026-10-24 POKPUNG r11473 completion 실행 8회 중 1회 다시 봤다(41초, readiness executor 를 테스트 thread 가 만드는 테스트) : r11474 이후 재발하는지 볼 것. => r11474 이후에도 재발 : 2026-10-24 MONSOON(kernel 7.0, 24 CPU) r11478 Debug completion 전체 5회 중 1회 "connected yet 1"(41초, 3) 접속 직후 close 200회 단계). 같은 빌드의 epoll / readiness 전체는 통과.
    - **해결 (2026-10-24, net.io.socket.tcp.cpp)** : `process_close_socket()`(epoll/readiness)이 갈래를 고르며 `socket_state()` 를 세 번 따로 읽었다(`> SYN` / `== SYN` / `<= BINDED`). close 시작 때 SYN 이고 두 읽기 사이에 다른 thread 가 접속 완료(ESTABLISHED)를 처리하면 **어느 갈래도 타지 않았다** : handle 만 INVALID 로 가져가고 fd 는 닫히지 않으며 EPOLLHUP 도 큐에 없고 disconnect 도 없음 -> `stop()` 의 `wait_complete_closed()` 30초 timeout, 서버 쪽은 FIN 을 못 받아 목록에 남음(30 + 10 = 41초). 진단으로 확인한 상태 : 클라이언트 ESTABLISHED, handle -1, epoll handle 그대로, processing 0, queued 0, on_connect 1 / on_disconnect 0. 이제 state 를 한 번만 읽는다(SYN 이면 4.B : `process_complete_connecting(EPOLLHUP)` 의 `is_close_after_connect` 가 그 사이 접속된 socket 의 disconnect 를 끝낸다). epoll 에도 같은 창이 있었다(예전 epoll 기본 9회 중 1회).
    - 검증 (MONSOON Debug, 3) 단계를 5000회로 늘린 진단 빌드, completion 기본) : 원본 6회 중 1회 같은 상태로 재현, 수정본 26회(13만 close) 느린 stop 0. 수정본 전체 스위트 completion / epoll / readiness 각 2회 통과(epoll 1회에 `linux_io.udp_send_deferred_in_group` 1개 실패 : 단독 원본·수정 40/40 통과, 무관한 간헐로 봄).
    - 같이 고친 테스트 전제 : completion 기본에서 이 테스트를 단독으로 돌리면 system executor 가 worker 마다 ring 을 나중에 만들어 544행 fd 수가 8 대 50 으로 항상 실패했다 -> 세기 전에 fd 수가 200ms 안정될 때까지 기다림(`lio_count_io_uring_fd_settled`, `tcp_zerocopy_not_on_epoll` 도). 단독 completion 10/10 통과.
13. ~~readiness io_uring 의 `emergency.manager_signals` 간헐 실패~~ - 해결(r11474, 1.23). "응답이 다음 수신 때 나간다"는 오해였다 : 테스트 클라이언트의 `SO_RCVTIMEO` recv 가 io_uring task_work 로 **EINTR**. 요청·ring 을 가진 thread 규칙(1.23 표)과 수정(worker 만 enter, ring 은 임시 thread 가 생성, completion 깨우기는 eventfd).
14. **실행기·IOCP 쪽 남은 최적화 후보** (2026-10-20 분석. 2026-10-24 : 배치 dequeue r11499, lambda post r11516(1.33), strand destroy 대기·linux 기본 thread 수(1.37), windows gather 의 락 밖 WSASend(1.38). 2026-10-25(1.44) : strand · queue_list 큐 락 std::mutex(r11613), strand post_at 정렬 + 깨우기 하나(r11599), windows 기본 thread 수가 affinity · job CPU 한도를 따름(r11614), IOCP 수신 tail 64B 는 측정상 추가 완료가 없어 그대로(64KB · 100000B · 256KB 모두 메시지당 수신 I/O 1.000). strand 큐 capacity 유지는 보류(남은 일 30). 남은 것은 FILE_SKIP_COMPLETION_PORT_ON_SUCCESS(사용자 결정 : 안 함)뿐. 착수 전 `svn log` 로 이미 바뀌었는지 확인)
    - IOCP(`api.io_context.iocp_direct.cpp`) : ~~`GetQueuedCompletionStatus` 로 1건씩 -> `GetQueuedCompletionStatusEx` 배치~~ 해결(r11499, 1.30). ~~WSASend 를 `m_lockable_sending`(recursive_mutex) 안에서 호출~~ gather / gather_buffered 해결(1.38 : fan-in x3.3, fan-out x30). 남은 것 : `FILE_SKIP_COMPLETION_PORT_ON_SUCCESS` 미사용(1.38 의 상한 측정 : 미룸), 수신 tail 최소 64B(`MINIMUM_REMAINED_RECEIVE_BUFFER`)라 작은 창으로 재등록.
    - ~~windows 실행기에는 linux 의 `send_defer::auto_scope`(r11496)가 없다~~ - 해결(1.39, 사용자 결정 : 기본 켜짐) : tcp echo 64B x32 window16 2.38M -> 7.54M msg/s.
    - strand : 큐를 move 해 capacity 를 버림(배치마다 재할당), recursive_mutex, `post_at` O(N) list 삽입 + 전역 heap push.
    - ~~lambda post 가 `std::function` 으로 감싸짐(libstdc++ 에서 object_ptr 캡처면 malloc/free) + 풀로 돌아간 래퍼가 캡처를 지우지 않아 캡처한 object_ptr 가 재사용 때까지 살아 있음(수명 누수)~~ - 해결(이 커밋, 1.33) : linux 만들기+실행+해제 26 -> 10.5ns, post 4 thread +11%, windows 39 -> 27ns(64B 캡처 56 -> 25ns).
    - ~~기본 thread 수 2×core+2 가 cgroup·affinity 를 무시(컨테이너에서 과다)~~ linux 해결(이 커밋, 1.37 : affinity·cgroup 한도), windows 는 그대로. `queue_list::m_lockable_queue_executable` 은 recursive_mutex.
    - "RIO" : `*.rio.cpp` 는 실제로 WSARecv/WSASend, `api.io_context.rio.cpp` 는 `#if __linux__` 로 감싸여 의미 없음.
    - **2026-10-24 재평가(근거·효과·위험, 고르지 않은 이유)** :
      - `FILE_SKIP_COMPLETION_PORT_ON_SUCCESS`(소스에 `SetFileCompletionNotificationModes` 없음) : 효과 큼(즉시 완료된 WSARecv/WSASend 의 IOCP 왕복 제거), 위험 큼 - 수신·송신·accept·connect·UDP 모든 경로가 "동기 성공이면 그 자리에서 완료 처리"를 해야 하고 재귀(완료 처리 안에서 다시 요청 -> 다시 동기 성공)와 락 순서가 바뀐다. 따로 설계할 것.
      - ~~WSASend 를 락 안에서(`net.io.sendable.tcp_async.iocp.cpp:47,129,141`, gather 는 `net.io.sendable.tcp_async_gather.iocp.cpp:29`) : 락 밖으로 빼면 송신 순서·중복 송신 보장을 다시 짜야 한다. 경합은 한 socket 에 여러 thread 가 동시에 보낼 때만이라 효과는 부하 모양에 달림. 측정 먼저.~~ gather / gather_buffered 해결(1.38). `Ntcp_async`(tcp_no_gather)는 여러 thread 가 동시에 WSASend 하므로 그대로.
      - 수신 tail 64B(`net.definitions.socket.h:174`, `net.io.receivable.stream.cpp:67`) : 작은 창 재등록은 큰 메시지가 끊어 올 때만 손해. epoll 쪽 assert 와 같이 묶여 있어 값만 바꾸기 어렵고 효과 미측정.
      - strand(`executor.strand.cpp:60` 큐 move, `:184` O(N) list 삽입, `:200` post_at 마다 executable 하나 + system scheduler post) : 효과 중간(strand 를 많이 쓰는 서버만), 헤더 멤버가 바뀌어(예비 vector 등) gruma 재빌드. ~~같이 볼 것 : `:56` 락을 잡은 채 `sleep_for(1ms)`~~ 해결(이 커밋, 1.37) : post 경로가 아니라 destroy 대기에만 있었고, 지연이 아니라 기아였다(linux 는 destroy 가 60초 넘게 안 끝남, windows post 최대 6.6초) + strand 안의 destroy 는 영원히 멈춤. 큐 move·O(N) 삽입은 미착수.
      - ~~기본 thread 수~~ linux 해결(이 커밋, 1.37). 아래는 처음 적은 문장 : 기본 thread 수(`api.system.linux.cpp:508`, `api.system.windows.cpp:278` 의 `hardware_concurrency()`) : linux 는 `sched_getaffinity` 의 CPU 수로 바꾸면 위험이 작다. 컨테이너 cgroup quota 까지 보려면 `/sys/fs/cgroup/cpu.max` 를 읽어야 함. 정확도 문제라 성능 이득은 환경에 따라.
      - `queue_list` 의 recursive_mutex(`executor.queue_list.h:67`) : `lock_object` 는 프로젝트 전체에서 `std::recursive_mutex`(`common/lockable.h:34`) - 한 곳만 바꾸는 이득은 작고, 재진입 여부를 확인해야 함.
      - "RIO" 이름 정리 : 동작 변화 없음(정리 작업).
15. ~~completion 의 post 가 바쁜 ring 에 묶임~~ - 해결(r11475, 1.24). jam 테스트 간헐 실패의 원인.
16. ~~completion 의 `linux_io.send_cork_sends_old_responses_{gather,buffered}` 간헐 실패~~ - 원인 확정·테스트 수정(r11477, 1.25) : 클라이언트가 서버와 같은 ring(같은 worker thread)에 묶여 서버 handler 뒤에 실행됐다. cork 는 정상(배치 중 3메시지마다 송신). 테스트는 이제 서버가 보낸 바이트로 판정.
17. ~~completion 은 같은 ring 의 socket 들을 owner thread 하나에서 차례로 실행한다~~ (설계 특성, 1.25 가 보여 줌) - 해결 : 블로킹(`scoped_thread_block`)은 r11481(1.26 : 막힌 owner 의 ring 을 다른 worker 가 실행), CPU 를 오래 쓰는 handler 는 1.34(한 executable 이 2ms 넘게 돌면 다른 worker 가 그 ring 을 빌려 실행. "COOP_TASKRUN 이라 CQE 가 올라오지 않는다" 는 틀렸다). **남는 것 : 감지까지 2~3ms(모든 worker 가 쉬면 최대 10ms 더) 는 같은 ring 의 socket 이 기다린다 - 사용자 문서(`socket.io_uring.html`)에 "handler 는 짧게, 긴 계산은 다른 executor 로" 는 그대로 적을 것.** 아래는 처음 적은 문장 : - 한 socket 의 긴 handler 는 같은 ring 의 다른 socket 들의 처리를 그만큼 늦춘다(epoll·IOCP 는 다른 thread 가 가져감). **handler 안의 blocking(`scoped_thread_block` 의 sync query 등)도 마찬가지**다 : 늘어나는 extra thread 는 ring 이 없어(transient, 1.17) post 큐만 처리하므로 막힌 owner 의 ring 을 대신 실행하지 못한다(코드 읽기, 미측정). socket 의 ring 은 처음 제출한 worker 로 정해지고 옮겨지지 않는다(owner 가 끝날 때만 고아 ring 으로 다른 worker 가 드레인). 고치려면 막힌/바쁜 owner 의 ring 을 다른 worker 가 실행하게 해야 하는데, 한 socket 의 CQE 는 한 번에 하나씩·순서대로 실행돼야 하므로(1.18 : multishot 데이터의 순서) socket 단위 직렬화가 먼저 필요하다. 그때까지 completion 은 handler 가 짧은 서버(또는 blocking 작업을 별도 executor 로 넘기는 서버)에 맞다 : 사용자 문서(`socket.io_uring.html`)에 적을 것.
18. ~~completion 에서 한 acceptor 가 받은 socket 이 모두 listen socket 의 ring(thread 하나)에 묶인다~~ - 해결(r11487, 1.27) : accept CQE 에서 새 fd 를 owned ring 에 round robin 으로 바인딩. 8 thread 벤치 completion tcp 64B x32 2.48M -> 10.58M msg/s, uds 13.9M -> 25.1M, 16B x256 878K -> 1.18M. 기존 벤치는 서버가 system executor(epoll)에서 돌아 이 상태를 재지 못했다(1.27). 아래는 처음 적은 문장 : (2026-10-24 r11481 의 테스트로 확인) - socket 의 ring 은 처음 제출한 worker 로 정해지는데(HANDLE_ENTRY::binding), accept 완료는 listen ring 의 owner 가 실행하고 그 handler 가 첫 RECV 를 제출하므로 accept 된 socket 이 전부 그 ring 에 간다. mistral 200 접속에서 199/199 가 한 thread(epoll 은 이벤트마다 아무 thread). 그 ring 의 owner 가 그 acceptor 의 모든 접속을 처리하므로 다른 worker 가 놀아도 서버 처리량이 thread 하나로 묶일 수 있다(벤치에서 확인 필요 : 기존 completion 벤치가 이 상태에서 잰 것인지). 고칠 방향 : accept 된 socket 의 첫 제출을 round robin ring 으로(또는 accept 완료 뒤 receive 시작을 다른 worker 로 post), 기존 바인딩 규칙(고아·빌린 ring 의 handle 은 다음 제출자 ring 으로 옮김)과 맞출 것.
19. ~~linux acceptor 는 accept 한 socket 에 acceptor 의 io_context 가 아니라 system executor 를 준다~~ - 해결(r11489, 2026-10-24) : windows·헤더 주석과 같은 규칙으로 맞췄다.
    - 원인 : `net.io.connective.acceptor_tcp.cpp` 의 linux `request_accept(handle, af)` 는 `process_get_connectable_io_context()` 가 비면 socket 이 이미 가진 io_context, 그것도 없으면 `system_executor::executor()` 를 썼다. 헤더 주석(`process_get_connectable_io_context` : "empty (the io_context of the acceptor is used)")과 windows 경로(`pconnectable->io_context(pio_context)` : acceptor 의 것)와 달라, acceptor 에 io_context 를 준 linux 서버의 접속은 system executor 에서 돌았다. 테스트·벤치(`lio_check_tcp_echo`, `io_uring_completion_sockets` 등)의 서버 socket 도 system executor 였다(벤치는 "server 66 thr" 로 확인. 그 테스트들의 "completion socket" 은 클라이언트뿐 : `CGDK_IO_URING=completion` 전체 실행 때만 서버도 completion).
    - 수정 : hook(`process_get_connectable_io_context`) -> acceptor 의 io_context -> system executor 순서로 고르고, 고른 것을 항상 socket 에 넣는다. 옛 handle 의 detach 는 그 전에 옛 io_context 로 한다.
    - 풀 socket 의 옛 io_context : 정상 종료(`detach_epoll_handle`, completion 의 `process_complete_closesocket`)는 io_context 를 비우므로 보통은 빈 채로 돌아온다. 그러나 close 의 SYN·BINDED 갈래는 epoll handle 만 비우고 io_context 는 남긴다 -> 옛 코드는 그 io_context(멈춘 executor 일 수도 있음)를 그대로 썼다(코드 읽기, 재현 안 함). 이제는 accept 마다 다시 정하므로 해당 없음.
    - 영향 : acceptor 에 io_context 를 준 linux 서버만. gruma 는 acceptor 에 io_context 를 주는 코드가 없다(`D:\project_gruma2` 의 server·common·client 소스에 `io_context(`·`process_get_connectable_io_context`·`make_io_context_threads` 없음 : 기본값인 system executor 그대로). acceptor 의 io_context 는 listen socket 을 만들 때 비어 있으면 system executor 로 채워지므로(`process_create_socket`) io_context 를 주지 않은 서버는 동작이 같다. 라이브러리 안의 사용처 `net::emergency::Nacceptor` 는 hook 으로 socket 마다 executor 를 주므로 그대로. 바뀌는 것 : tutorial `socket.ex15.socket_option` 5번(busy poll executor 를 준 acceptor : 이제 서버 socket 도 그 executor, 예제의 의도대로), `test_linux_io`·`test_uring_send`·`bench_linux_io` 처럼 acceptor 에 executor 를 준 테스트의 서버 socket. r11487 의 `lbn_acceptor` / `lio_thread_acceptor` override 는 같은 결과라 남겼다(옛 트리 비교용).
    - 테스트 `test_acceptor_io_context.cpp`(net_socket.http, linux) : `the_executor_of_the_acceptor`(2 thread executor 를 준 acceptor + blocking 클라이언트 8 echo -> 서버 socket 의 on_connect / on_message 16번 모두 그 executor), `the_system_executor_by_default`(io_context 없는 acceptor -> system executor). r11489 트리에 r11488 의 `acceptor_tcp.cpp` 를 넣은 빌드에서 `the_executor_of_the_acceptor` 실패(socket 의 io_context = system executor) 확인(SAKPUNG, epoll, Debug).
    - 검증 : 커밋 때 monsoon(kernel 7.0) net_socket.http 전체 epoll / io_uring / completion x2 134 통과, mistral 새 테스트 3 백엔드 통과. 이후 SAKPUNG(kernel 6.12, io_uring 꺼짐) r11489 export Debug 전체 epoll x2 : 125 통과(skip 12 = io_uring 전용 9, bench 3). 3 백엔드 x2 를 mistral 에서 다시 돌리지는 않았다(그 시각 다른 세션이 mistral·POKPUNG 에서 벤치 중).
20. ~~completion / epoll 비교 수치를 다시 잴 것~~ - 해결(2026-10-24, 1.31) : HEAD r11501 을 서버도 같은 backend 로 mistral 4 / 8 / 16 thread 교대 5라운드. 8 thread completion 기본값 / epoll : tcp 64B +26%, uds +63%, 16B x256 +37%, 64KB +12%, udp +43%(4 thread 는 +7~+16%, 64KB 만 -52% : 접속 4개 = ring 4개 + loopback 의 SEND_ZC 손해. 16 thread 는 +55~+172%). tcp 64B 격차의 원인(CQE 마다 send, 세그먼트당 메시지 8 대 16)은 계측(r11505 의 TcpExt.TCPOrigDataSent)으로도 확인했고 r11497 이 고쳤다. uds 가 빠른 이유 : recv / epoll_wait / epoll_ctl 이 없고 UDS send 가 싸다(메시지당 CPU 0.31 vs 0.53us). 클라이언트 CONNECT 분산 : 시제품으로 16B x256 +11~31% 확인, 사용자 결정으로 CONNECT op 만 round robin(1.31, r11507). 아래는 처음 적은 문장 : (2026-10-24) - 남은 일 8 의 r11459 POKPUNG 수치, 1.18·1.19 의 벤치, r11486 의 `document/Web/site/socket.recv.html`(32 코어 표)는 서버가 system executor(epoll)에서 돈 측정이다(19, 1.27). r11487 의 벤치(서버도 case 의 executor)로 mistral 8 thread 에서 잰 completion / epoll : tcp 64B x32 -23%, 64KB x4 +25%, uds +73%, 16B x256 +34%, udp +47%. 남은 격차(tcp 64B x32)와 uds 와의 차이(uds 는 completion 이 epoll 보다 빠름)는 미분석. 클라이언트 쪽 분포 : 클라이언트 socket 은 CONNECT post 를 받은 worker 의 ring 에 묶이고 post 는 깨운 ring 이 최대 16개씩 가져가므로 몰릴 수 있다(settle 뒤 "client 8 thr top 20~37%", executor 를 만든 직후 1차 측정은 3 thr 까지). 접속을 많이 여는 클라이언트(부하 테스트기)라면 accept 와 같은 round robin 바인딩을 CONNECT 에도 줄지 검토. => tcp 64B x32 격차는 분석·해결(1.29 : CQE 마다 send 하나, 배치 send defer 로 POKPUNG completion +16% vs epoll). POKPUNG r11488 공정 벤치 : epoll 15.68M / 16.96M(uds) / 1.06M(16B) / 176K(64KB), completion 9.91M / 30.18M / 1.53M / 150K.
21. ~~completion 송신의 남은 것~~ (2026-10-24, 1.29) - 해결 : 아래 항목 모두 끝남, 10GbE 측정만 남은 일 25 로 옮김.
    - 배치 send defer 커밋 : 끝남(r11497, r11496 의 auto_scope 위, 1.29).
    - ~~소유 worker 송신 op(`CGDK_IO_URING_SEND_OP=1`, 기본 off)의 graceful close 유실~~ - 해결(r11522, 1.36) : 상대 FIN 과 같이 온 요청의 응답(staged op)을 EOF 의 close 가 지웠다. close 가 shutdown 전에 `submit_staged()` 로 제출한다. 기본 off 는 그대로(배치 defer 와 같이 쓰면 성능 손해, 1.29).
    - ~~풀 재사용 socket 의 per-socket 송신 한도~~ - 해결(r11502) : `Itcp_gather::reset_send_queue_limit()` 을 acceptor 두 경로·connector 가 풀에서 꺼낸 직후 부른다(첫 호출 때 생성자의 한도를 기억, 그 뒤 되돌림). 테스트 `socket_option.send_queue_limit_of_a_pooled_socket`(windows·linux, 수정 전 linux 3/3 실패). 원래 문장 : `maximum_depth_of_send_buffer(n)` / `maximum_depth_of_send_buffer_bytes(n)` 을 socket 에 걸면 그 객체가 풀로 돌아가 다음 접속에 재사용될 때 그대로 남는다(`process_reset_sendable` 은 SO_SNDBUF 만 되돌림, windows 도 같음). 남은 일 9 와 같은 부류. `test_uring_send` 는 테스트 끝에 되돌린다.
    - ~~ASan / valgrind~~ - ASan 3 모드 전체 스위트 보고 0(1.36). valgrind 는 completion 에 맞지 않음(커널의 비동기 쓰기 : 오탐).
    - 실 NIC 측정(SEND_ZC, 64KB) : 1GbE(POKPUNG 서버 / SAKPUNG 클라이언트, `bench.remote` r11523)는 회선이 상한이라 zerocopy 효과가 보이지 않음(서버 us/msg 차이는 노이즈 안, 1.36). **10GbE 이상 두 대가 있어야 판정 가능 : 남은 일 25 로 옮김.** mistral 의 ixgbe 포트는 link down(케이블·상대 장비 필요). SAKPUNG 은 `kernel.io_uring_disabled=2`(io_uring 을 쓰려면 sudo sysctl).
    - ~~loopback 의 zerocopy~~ - 해결(r11525, 1.36) : 커널이 복사를 연속 8번 보고하면 그 socket 의 zerocopy 를 끈다(사용자 결정 : 커널 보고 방식). loopback 64KB readiness +54%, completion +16~37%, 1GbE 실 NIC 는 그대로.
22. ~~windows `emergency.*` 의 순서 의존 실패~~ - 해결(2026-10-24, 테스트 수정, 라이브러리 변경 없음) : 순서가 아니라 Windows 제외 포트 범위와 pid 로 정한 테스트 포트의 충돌이었다(아래 원인). `test_emergency.cpp` 의 `emergency_test_port()` 가 이제 `20000 + n * 8 + offset`(n 은 pid % 1500 에서 37 씩, linux 임시 포트 범위 32768 아래)에서 `[::]:port` 와 `0.0.0.0:port` 를 실제로 bind 해 보고 되는 첫 포트를 쓴다(`is_port_bindable`). 검증 : Windows VS18 x64 Debug - `emergency.*` 를 프로세스 10번 따로 실행(옛 포트가 제외 범위인 회차 9번 포함) 10/10 각 8 통과, net_socket.http 전체 3회(3회 모두 옛 포트가 범위 안) 87/87. SAKPUNG Debug - `emergency.*` x5 반복 통과, net_socket.http 전체 2회 130 통과(skip 21). 같은 pid 고정 포트를 쓰는 다른 테스트 : Windows 에서도 도는 것은 `test_connect_requestable`(28400~), `test_ssl_loopback`(29400~), `test_http_client_timeout`(30400~ / 30900~), `test_socket_option`(32400~)으로 지금 이 PC 의 제외 범위(27319~27919, 13508)와는 겹치지 않는다. 제외 범위는 재부팅마다 바뀔 수 있으므로 같은 실패가 보이면 같은 방식으로 바꿀 것. linux 전용(`test_blocking_handler` 27900~ 는 범위와 일부 겹치지만 linux 전용, `test_uds`, `test_linux_io`, `test_uring_send`, `test_send_defer_auto`, `test_acceptor_io_context`, `bench_linux_io`, server_system 의 `test_logger_file`·`test_group_send_defer_socket`)은 해당 없음. 아래는 처음 적은 문장 : (2026-10-24, 1.30 검증 중 발견) - `emergency.thread_priority_set_current_thread` 뒤에 `emergency.acceptor_own_executors` 를 돌리면 acceptor 시작에서 `FAIL.API_FAILURE` 예외(15ms). `emergency.acceptor_*` 3개만 돌리면 매번, `emergency.*` 전체는 3회 중 2회 통과, 단독 실행은 통과. Release·Debug 모두, r11499 를 뺀 기준선(Debug)에서도 같은 조합으로 같은 실패. 앞 테스트가 main thread 우선순위를 TIME_CRITICAL 로 바꾼 채 두거나, emergency acceptor 의 고정 포트가 앞 테스트의 것과 겹치는지부터 볼 것. => **원인 (2026-10-24, 1.33 검증 중)** : 순서가 아니라 포트다. 테스트 포트는 `27400 + (pid % 100) * 8 + offset`(`test_emergency.cpp:44`)인데 이 PC 는 Windows 가 TCP 27319~27919 를 제외 포트 범위로 잡고 있다(`netsh int ipv4 show excludedportrange protocol=tcp` : Hyper-V/WSL 이 부팅 때 예약, 재부팅마다 바뀔 수 있음) -> bind 실패 = `FAIL.API_FAILURE`. `emergency.acceptor_*` 를 프로세스 8번 따로 띄운 결과 포트가 범위 안 4번 모두 실패, 밖 4번 모두 통과. 전체 실행의 `emergency.manager_signals` 실패(connect 실패)도 같은 실행에서만 났다. 고칠 방향 : 고정 포트 대신 빈 포트를 골라 쓰기(r11418 의 uds.reconnect 처럼), 또는 제외 범위를 피하는 기본 범위. 테스트 수정은 하지 않았다.
23. ~~linux `bench.epoll` 의 udp echo 중 "stack smashing detected" 1회~~ - 해결(2026-10-24, 1.40) : r11535 export 의 stack protector 빌드로 mistral 320회 중 1회·monsoon 300회 중 1회 재현, gdb 로 `Ndatagram::process_complete_receivable` 의 canary 가 깨진 것을 봤다. recvmmsg 배치 수 `m_count_receive_batch`(plain int)를 g++ 가 "2배 저장 -> 32 로 다시 저장" 두 store 로 만들어, 같은 socket 을 동시에 받는 다른 thread(EPOLLET)가 64 를 읽고 32칸 스택 배열을 넘었다. atomic + clamp 로 고침, 테스트 `linux_io.udp_receive_on_many_threads`(고치기 전 20/20 실패, 뒤 20/20 통과). 아래는 처음 적은 문장 : (2026-10-24, 1.31, 미재현) - mistral r11491 + 계측 벤치, `CGDK_BENCH=3 CGDK_BENCH_THREADS=4`, 다른 세션의 벤치와 겹친 회차(장비 29.8 코어, 프로세스 1.25 코어)의 마지막 case(udp 64B x16 window32, 바로 앞은 16B x256 클라이언트 256개 stop)에서 `*** stack smashing detected ***: terminated` 뒤 exit 139. core 없음(apport). SAKPUNG 660 실행, mistral 320 실행 재현 0. 다시 보이면 gdb 아래로 돌려(`-ex "handle all nostop noprint pass" -ex "handle SIGABRT stop print nopass"`, 스크립트 `repro_udp.sh` : 1.31 의 노트 폴더) `__stack_chk_fail` 을 부른 함수를 볼 것. 의심 : UDP 수신·송신의 스택 배열(배치 한도로 묶여 있음 : 코드상 넘치지 않음), 다른 thread 의 dangling 포인터가 스택을 씀.
24. ~~linux `send_defer_auto.gather_auto_off` / `buffered_auto_off` 의 epoll 간헐 실패~~ - 해결(r11512, 2026-10-24, 1.32 : 테스트 문제, 라이브러리 변경 없음) : `closesocket()` 은 abortive close(SO_LINGER 0 -> RST)라 커널이 아직 내보내지 않은 것을 버린다. 클라이언트를 차례로 읽어 socket 0 의 클라이언트가 읽지 않는 동안 ACK 가 늦어(quick ACK 16개 뒤) cwnd 10 에 막힌 24개가 커널 송신 큐에 남아 있었다(26 = 16 + 10). CGDK 는 close 전에 50개를 다 넘겼다(송신 통계 50, `SIOCOUTQNSD` 768, 클라이언트는 `ECONNRESET`). 테스트를 고쳤다 : close 는 closesocket 직후의 송신 통계 50 + 도착분은 순서대로인 앞부분 + EOF, graceful `disconnect()` case 추가(50개 모두 도착). 가설 (a) graceful close 가 큐를 버림, (b) 클라이언트 `read()` 가 일찍 끝남은 둘 다 아니었다. 아래는 처음 적은 문장 : (2026-10-24, r11507 검증 중 발견) - r11496 의 `test_send_defer_auto.cpp`. monsoon Debug 전체 스위트 epoll 에서 두 테스트가 1개씩 실패했고, 변경 없는 r11504 단독 실행도 3회 중 2회 같은 실패(HANDOVER.md r11507 절). 단언은 `counts` 의 마지막 값 **26 vs 50** : 기대값 50 은 4) "close" case 의 socket 0 뿐이다(한 executable 이 다른 socket 들에 10개씩, socket 0 에 50개를 보낸 뒤 `closesocket()` -> 클라이언트는 EOF 전에 50개를 모두 받아야 함). 즉 auto 를 끈(`send_defer::enable_auto(false)`) epoll 에서 **closesocket 직전에 보낸 메시지의 절반이 EOF 전에 오지 않았다** : graceful close 가 큐에 남은 송신을 버리는지(송신 중인 다른 thread·EPOLLOUT 대기 묶음과 close 순서), 아니면 클라이언트 `read()`(SO_RCVTIMEO 100ms, 3초 한도)가 일찍 끝나는지부터 가를 것. auto 켠 판(`gather` / `buffered`)과 completion / readiness 에서는 보고된 실패가 없다. 재현 : `--gtest_filter='send_defer_auto.*_auto_off' --gtest_repeat=20`(epoll 기본).
25. **10GbE zerocopy 송신 측정 - 측정 끝, `zerocopy_threshold()` 기본값만 결정 남음** (2026-10-25, 1.43 : MISTRAL·MONSOON 10G) - completion 의 큰 버퍼 SEND_ZC op 는 유지(64KB 이상 서버 CPU 3~13% 이득, 손해 없음). 기본 16KB 는 이득이 없고 readiness 는 처리량이 떨어진다(8 thread 829 -> 599MB/s) : 64KB 로 올릴지 사용자 결정. 측정 중 completion SEND_ZC 가 memlock 초과(-ENOMEM)에 socket 을 닫는 버그를 찾았다(1.43, r11588 로 고침). 아래는 처음 적은 문장 :1GbE 는 회선 상한(64KB 에코 108MB/s)이라 zerocopy 켬/끔 차이가 서버 CPU 노이즈(±5%) 안이었다. 두 장비를 10GbE 이상으로 잇고 `bench.remote`(r11523, r11526)로 잴 것 : 서버 쪽 `CGDK_BENCH_REMOTE=server`, 클라이언트 쪽 `CGDK_BENCH_REMOTE=<서버 IPv4>`, `CGDK_BENCH_BACKEND=completion` / `io_uring` / `epoll`, `CGDK_BENCH_ZEROCOPY_THRESHOLD=16384`(켬) / `1000000000`(끔), `CGDK_BENCH_THREADS=8`, `CGDK_BENCH_REMOTE_SIZE` 64KB · 256KB · 1MB, 교대 5라운드. 볼 것 : 처리량(MB/s), 서버 us/msg 와 softirq, `[bench-remote]` 의 zerocopy sent / copied / 끈 socket 수(실 NIC 에서는 0 이어야 함 : r11525 copy check), `zerocopy_fallback`(memlock : `ulimit -l` 을 송신 버퍼 x 동시 대용량 socket 수만큼). 결과로 정할 것 : `zerocopy_threshold()` 기본값(16KB), completion 에서 큰 버퍼를 SEND_ZC op 로 보내는 것(직접 send 를 건너뜀)의 유지 여부. 장비 : mistral 에 ixgbe(10G) 포트가 있으나 link down(케이블과 상대 10G 장비 필요). 상대가 SAKPUNG 같은 RHEL 계열이면 `kernel.io_uring_disabled=2` 를 sudo sysctl 로 풀어야 io_uring 을 쓴다.
26. ~~windows execute `CGDK10_executor.strand_destroy_inside_executable`(r11529) 간헐 실패~~ - 해결(r11532, 테스트 수정, 라이브러리 변경 없음) : 테스트 코드의 경쟁이었다. 첫 executable 이 테스트 thread 의 '2', '3' post 보다 먼저 돌아 destroy 하면 그 뒤의 post 는 거부되어 order 가 4개가 되지 않았다. 이제 첫 executable 이 '2', '3' 이 post 될 때까지 기다린다. 아래는 처음 적은 문장 : (2026-10-24, 1.38 검증 중 발견) - DOLPUNG Debug execute 전체 실행에서 test_executor.cpp:481(`order.size() == 4` 를 5초 기다림)이 timeout. 변경 없는 r11529 5회 중 1회, 1.38 판 4회 중 2회, 단독(`--gtest_filter=*strand*` x5)은 통과 : 앞 테스트가 남긴 상태와 겹칠 때 strand 의 남은 executable('2', '3', '4')이 실행되지 않는 것으로 보임. 원인은 보지 않았다(1.37 의 strand destroy 변경을 볼 것).
27. ~~epoll 의 EPOLLONESHOT 방향~~ - 해결(r11572, 2026-10-25, 사용자 결정 : 혼합 등록). 작은 수신은 EPOLLET(이벤트마다 MOD 없음), 창을 채운 4KB+ / 한 이벤트 16KB+ 수신이면 그 socket 만 EPOLLONESHOT, 작은 수신 이벤트 4번 연속이면 EPOLLET 복귀. 진입 · queued 에서는 EPOLLOUT 만 지운다(받은 비트를 다 지운 1차판은 처리 중 EPOLLIN 을 마스크로 걸러 멈춤). SAKPUNG epoll r11564 대비 작은 메시지 8 thread +10%, 16 · 32 thread +48 ~ +97%, 64KB · udp 범위 안. 기본 백엔드는 r11556 부터 io_uring completion(r11557 시작 로그), 비교표는 웹 benchmarks.linux.html(r11573). 테스트 epoll_receive_mode.*. 헤더 변경(net.io.Isocket.h) : 재빌드. 아래는 처음 적은 문장 : (사용자 결정 대기, 1.41) - TCP 는 이벤트마다 epoll_ctl(MOD)로 다시 건다. SAKPUNG(32 CPU, kernel 6.12) 실험판(EPOLLET, ONESHOT 없음)은 오류 없음(전체 스위트 Release 2회·Debug 1회, 벤치 104회 : processing_now / queued 가 동시 처리를 막음), 작은 메시지 8 thread +10~29%, 16 thread +50~97%, 64KB 8 thread -11%(EPOLLET 가 세그먼트마다 다른 thread 를 깨움 : 이벤트의 56%(8 thr) / 74%(16 thr) 가 헛깨움. ONESHOT 만 빼고 level 이면 97%). thread 별 epoll 분할은 socket 이 thread 에 고정되어 분산이 깨진다(completion 이 겪은 남은 일 17·18 과 같은 장치가 다시 필요). 제안 : epoll 은 ONESHOT 그대로 두고 completion 쪽으로 정리, 또는 "창을 채운 큰 수신 중에만 ONESHOT 처럼" 혼합안을 측정. 실험판 diff 는 이 작업의 노트 폴더(`.../15437fd6-.../scratchpad/b6/variantB.diff`).
28. ~~windows connector 의 SIO_LOOPBACK_FAST_PATH~~ - 해결(r11612, 2026-10-25, 1.44) : ConnectEx 뒤의 호출을 지웠다(connect 전 호출은 그대로). 접속마다 8MB SO_SNDBUF / SO_RCVBUF 는 그대로 둔다 : 끄면 256KB x4 echo 40.8K -> 12.6K msg/s(-69%, 메시지당 수신 I/O 1.00 -> 1.27), 나머지 크기는 범위 안. 아래는 처음 적은 문장 : (1.41) - ConnectEx 완료 뒤에 걸어 효과가 없다(연결 전 양쪽이 걸어야 함, MS 문서상 deprecated). 지우는 패치만 있다(`.../scratchpad/win/patch/D6_connector_loopback_fast_path.patch`). 접속마다 SO_SNDBUF / SO_RCVBUF 8MB 를 명시하는 것은 windows 수신 autotuning 을 끄고 socket 당 커널 메모리를 8MB 까지 허용한다 : 접속이 많은 서버라면 재검토.
29. **completion 의 UDP(EPOLLET 등록)는 실제로 level 처럼 동작한다** (1.41, C6 작업 중 발견) - one-shot POLL_ADD 를 이벤트마다 다시 걸어, 안 읽은 데이터가 남으면 바로 다시 온다(1바이트 남기면 100ms 에 22만 번). Ndatagram 은 EAGAIN 까지 비우므로 지금은 문제가 없다. 진짜 edge 로 만들려던 multishot POLL 은 -13% 라 넣지 않았다.
30. **strand 큐 capacity 유지** (2026-10-25, 1.44, 측정 후 보류) - strand 가 배치마다 `m_queue_executable` 을 move 해 capacity 를 버린다. 예비 vector swap 판은 POKPUNG S=16 P=4 +40%, chain K=64 +6% 이지만 S=1 P=1 이 7~17M/s 로 흔들려(기준 11~13M) 느려졌다 : swap 이 빨라지면 배치가 잘게 쪼개지고 배치마다 strand 를 다시 post 한다. 새 항목을 20us 더 기다리는 판도 S=1 P=1 -15%. 넣을지는 사용자 결정(부하 모양 trade-off).
31. **`sendable::Ntcp_crypt` 의 보낸 메시지 수 0** (1.44) - gather 완료가 executable 의 count 를 쓰지 않는다. 기본 socket 은 이 sendable 을 쓰지 않음. 고치려면 `process_complete_sendable` override(헤더 변경).

---

## 3. 검증 방법

### 3.1 단위 테스트

| 테스트 | 위치 | 비고 |
|---|---|---|
| 블로킹 handler (completion 의 빌린 ring) | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_blocking_handler.cpp` | Linux 전용, CMake 만, 프로세스의 backend 로 돈다(`CGDK_IO_URING`). 200 접속 중 하나가 300ms 블로킹하는 동안 나머지가 100ms 안에 응답, 순서 확인(1.26). 수정 전 completion 4/4 실패. completion 이면 접속이 여러 thread 에 퍼졌는지도 단언(1.27) |
| completion 송신 경로 | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_uring_send.cpp` | Linux 전용, CMake 만. tcp<> / tcp_buffered<> : socket 버퍼 가득(SENDMSG / SEND_ZC op, 순서), send 후 close, op 중 close · RST, 큐 한도(1.29). r11488 에서 5개 실패 |
| UDS / epoll 핸들 | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_uds.cpp` | Linux 전용(`_SYS_EPOLL_H`), CMake 만. 23개: listen / connect / 통신 / epoll 핸들 수명 |
| io_uring completion 백엔드 | `build/src/system/unit_test/unit_test.object.execute/test_io_context_uring_cq.cpp` | Linux 전용, CMake 만. 14개: post / 바쁜 ring 으로 간 post 를 쉬게 된 worker 가 가져감(1.24) / readiness 호환 / recv·send / ZC·gather / multishot accept·connect / accept 된 socket 의 round robin 바인딩(1.27) / connect 하는 socket 의 round robin 바인딩(1.31, r11507) / cancel / stop / 짧게 사는 thread / fd 한도 / multishot recv + provided buffer(1.18) |
| Linux 소켓 성능 요소 | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_linux_io.cpp` | Linux 전용, CMake 만. nonblock 생성 / TCP_DEFER_ACCEPT / 수신 버퍼 히스테리시스 / UDP 배치 echo(epoll, io_uring) / UDP socket 하나를 epoll 16 thread 가 동시에 수신(1.40) / zerocopy(io_uring 만) / cpu_affinity / io_uring executor(TCP·UDS·대용량·UDP·close 폭주·ring 해제) / worker 만 enter : 다른 thread 의 blocking 호출이 EINTR 되지 않음(readiness·completion, 1.23) / send cork / completion 에서 accept 된 socket 과 connect 한 TCP 클라이언트가 worker 에 퍼짐(1.27, 1.31) |
| accept 된 socket 의 io_context | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_acceptor_io_context.cpp` | Linux 전용, CMake 만, 프로세스의 backend 로 돈다. 2개: executor 를 준 acceptor 의 서버 socket 이 그 executor / io_context 없는 acceptor 는 system executor(2. 남은 일 19, r11489). 수정 전 첫 번째 실패 |
| Linux echo 벤치 | `build/src/net.socket/unit_test/unit_test.net_socket.http/bench_linux_io.cpp` | `CGDK_BENCH=<초>`(없으면 skip), `CGDK_BENCH_THREADS`. tcp 64B/64KB, uds, udp echo 를 epoll 과 io_uring 으로. r11392 트리에서도 빌드됨(기준선). r11487 부터 서버 socket 도 case 의 executor(전에는 system executor), case 마다 handler thread 분포 출력(1.27). r11505 부터 case 마다 "[bench-cpu]"(CPU·thread·장비·socket 분포·수신 묶음)와 "[bench-tcp]"(메시지당 TCP 카운터), `CGDK_BENCH_CASES`, `CGDK_BENCH_ZEROCOPY_THRESHOLD`(1.31). r11523 부터 `bench.remote` : 두 장비 사이(실 NIC), `CGDK_BENCH_REMOTE=server` / `<서버 IPv4>`(1.36) |
| 접속 실패·connector | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_connect_requestable.cpp` | Windows/Linux. 즉시 실패·거절의 on_fail_connect/CLOSED/stop/재접속, connector 대량 접속(2. 남은 일 11 의 재현 테스트) |
| TCP socket option | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_socket_option.cpp` | Windows/Linux. 12개: 기본값 off / 파라미터 파싱 / accept·connect 적용(getsockopt) / 코드 설정 / START_PARAMETER / context / connector / TFO connect / UDS skip(linux) |
| 그룹 락 | `build/src/server.system/unit_test/unit_test.server_system.service/test_group_*.cpp` | 원본 코드면 동시성 테스트가 60초 감시에 걸림 |
| 실행기(queue_list·scheduler) | `build/src/system/unit_test/unit_test.object.execute/test_executor.cpp` | Windows/Linux. 몰린 post 를 idle thread 가 동시에 처리, stop() 이 기다리는 thread 를 바로 깨움(+재시작), scheduler 실행·cancel 동시성(1.22). 시간 기준 assert(500ms·300ms)라 매우 바쁜 장비에서는 여유를 볼 것 |
| UE 컨테이너 직렬화 | `build/src/system/unit_test/unit_test.object.buffer.unreal` | Windows, UE 설치 필요(5.7·5.8 확인). gtest 아님 : UE 자동화 테스트 `CGDK.Buffer.Unreal`, `run_test.bat [엔진 버전]`(1.35). 고치기 전 헤더는 컴파일 실패 |
| doss | `build/src/server.service/unit_test/unit_test.server_service.doss` | |
| object_ptr | `unit_test.object.ptr` (r11328) | |

- `uds.close_at_connect` 의 기본 크기(16스레드 x 300회)는 빠르지만 경쟁 조건을 매번 잡지는 못한다. 수정 전 코드는 32 x 3000 에서 96,000번 중 4번 멈췄다(수정 후 0번). 접속 경로를 바꾸면 크기를 올려 한 번 돌려 볼 것.
- 테스트 helper 의 static 카운터는 테스트 시작에서 reset(`--gtest_repeat` 대비).
- acceptor 는 port 0 을 거부하고, port 가 겹치면 **조용히 port+1** 로 재할당한다(`reallocate_port_at_duplicate_port_error` 기본 true). 테스트는 반드시 `pacceptor->local_endpoint().port()` 로 접속.

### 3.2 Linux 빌드·테스트

- 장비: POKPUNG(Ubuntu, g++ 15.2, gtest, 벤치용), SAKPUNG(CentOS Stream 10, gcc-toolset-15, valgrind, 단위 테스트용), FOEHN(8스레드, 전체 스위트 가능). Linux 노드에는 svn 인증이 없으니 Windows 에서 `svn export -r <rev>` 한 트리(include, build/src, build/cmake, build/CMakeLists.txt, 약 19MB)를 tar 로 보내 빌드한다(다른 세션의 미커밋 수정이 섞이지 않음).
- 2026-10-23 부터 Windows 의 `~/.ssh/config` 에 별칭이 있다 : `hukpung`(cgcii.co.kr:32, Ubuntu 24.04, 8 CPU), `monsoon`(:42, Ubuntu 26.04, 24 CPU), `mistral`(:52, Ubuntu 26.04 kernel 7.0, 32 CPU, Debug 빌드 15초), `sakpung`(:62, CentOS Stream 10 : 키 거부 중). 모두 gcc 15.2 + gtest + ninja. BatchMode ssh/scp 가 되므로 `svn export` -> `tar --force-local` -> `scp` -> `nohup 스크립트 ; echo $? > done.flag` -> ssh 로 flag 대기 흐름이 가장 간단하다(1.11 의 mistral 검증).
- 2026-10-24 : `monsoon` 은 루트 파일시스템이 I/O 오류로 읽기 전용(`emergency_ro`)이 되어 쓸 수 없다(복구에 sudo 필요). POKPUNG 은 ssh 별칭이 없다 : `sangduck@192.168.0.69`(32 CPU, kernel 7.0, `ulimit -l` 8MB).
- io_uring 의 EINTR·소유 문제를 의심하면 errno 를 먼저 찍고, 커널 동작은 C 프로브로 확인한다(1.23 표 : ring 을 munmap + close 해 실제로 해제할 것).
- `cmake -S build -B bld -G Ninja -DCMAKE_BUILD_TYPE=Debug` -> `ninja CGDK10.unit_test.<이름>.Debug` -> `build/src/unit_test/` 의 바이너리 실행.
- CGDK dump_manager 는 크래시 때 gcore(~6GB)를 남긴다. 반복 실행 시 PATH 앞에 가짜 gcore 를 두고 `crash_*` 를 지울 것. 원인 추적은 `gdb -batch -ex run -ex bt`.
- tar 에 Windows 경로를 줄 때는 `--force-local`, `.vs` 폴더 제외.
- ssh 로 `pkill -f <이름>` 을 쓰면 그 이름이 든 ssh 명령 자신도 죽는다 -> `pkill -x` 또는 스크립트 파일. `cd x && server &` 는 cd 까지 백그라운드 서브셸에서 실행된다.

### 3.3 벤치 (POKPUNG)

- epoll A/B 하네스: `~/cgdk_epoll_opt/bench` (`run_ab.sh`, `run_strace.sh` 등). 서버 0-7, 클라 16-31 코어 고정, 교대 실행 + 중앙값. 다른 세션 빌드가 돌면 수치가 무의미하니 `uptime`/`pgrep -x cc1plus` 확인.
- UDS 벤치: `~/cgdk_uds_bench` (`run_uds.sh`, `run_tcp_ab.sh`, `run_connect.sh`, `run_connect_cgdk.sh`).
- CGDK 는 accept 한 TCP socket 에 SO_SNDBUF 8MB 를 건다 -> EAGAIN 경로를 시험하려면 `net::io::socket::Ntcp::set_default_maximum_send_buffer_size()` 로 줄일 것.

### 3.4 Windows

- 공유 작업 사본에서 여러 세션이 동시에 빌드한다 -> MSBuild/cl.exe 가 없을 때 빌드(겹치면 C1041/LNK4020).
- 공유 출력물을 건드리지 않는 개인 빌드: `msbuild <proj> -p:Configuration=Release;Platform=x64;BuildProjectReferences=false;IntDir=<scratch>;OutDir=<scratch>` (짧은 경로를 써야 MAX_PATH 오류가 안 난다).

---

## 4. 작업 규칙 (이 저장소에서 지켜 온 것)

- **인코딩**: `.h/.cpp` 대부분이 CP949 한글 주석이다. 편집기가 UTF-8 로 다시 저장하면 한글이 전부 깨진다. `iconv -f cp949 -t utf-8` 사본을 만들고, 되돌린 결과가 원본과 바이트 동일한지 `cmp` 로 확인한 뒤 사본을 고치고 다시 CP949 로 저장한다. `include/cgdk/buffers/` 일부는 이미 U+FFFD 로 손상돼 있으니 바이트만 보존.
- `.inl` 에는 주석을 넣지 않는다(순수 ASCII 유지).
- 문자열 함수는 std 우선. 자체 함수는 std 에 없거나 더 빠를 때만(`stream_string`, `__stricmp` 등).
- 셸 치환에서 `"\0"` 을 쓰면 NUL 바이트가 들어간다 -> `\x5c0` 으로 쓸 것.
- 커밋: 한 수정 = 한 커밋, 자기 파일만. `svn status -u <파일>` 로 갱신 여부 확인 후 `svn commit --encoding UTF-8 -F <메시지 파일> <파일들>`. 경로 없이 `svn update` 하지 말 것(다른 세션 파일까지 갱신).
- svn 로그는 `propset` 으로 고칠 수 없다(pre-revprop-change hook 없음). 커밋 전에 메시지(특히 원인 설명)를 확인할 것.
- `D:\project_gruma2\server\CGDK10.Cpp` 는 이 저장소로의 링크다. CGDK 의 구조체/레이아웃이 바뀌면 gruma 전체 재빌드.
- 예외 모델: Debug /EHsc (no -fnon-call-exceptions), Release /EHa + -fnon-call-exceptions.

---

## 5. 코딩 함정 모음

- MSVC 에서 `_except` / `_leave` / `_try` / `_finally` 는 SEH 매크로라 이름으로 못 쓴다.
- CGDK 클래스는 `std::is_abstract` 가 항상 true(make_object 래퍼). 추상 여부는 `make_object<T>()` 컴파일로 확인.
- `SCOPED_LOCK(x) { ... }` 는 if 매크로다: 블록 안 `break/continue` 는 바깥 루프에 걸린다.
- 풀 객체(`Npoolable`)는 재사용된다: 필드를 추가하면 **모든 할당 지점에서 초기화**하고, 다른 객체 참조는 `on_final_release` 에서 놓을 것.
- epoll 에서 fd 는 닫히자마자 다른 socket 이 같은 번호를 받는다: 닫은 뒤 그 번호로 detach / set_event / close 를 부르는 코드가 없어야 한다(1.2).
- 새 connect 경로는 connect() 의 즉시 성공(0)과 즉시 실패를 모두 처리할 것(즉시 성공도 epoll attach 후 EPOLLOUT 으로 완료).
- CGDK 헤더로 독립 TU(벤치 등)를 만들면 표준 헤더가 namespace CGDK 안에 들어가 깨진다 -> 유닛테스트 pch.h 를 먼저 include.
- GCC 는 MSVC 가 통과시키는 const/초기화 순서 오류를 잡는다(-Wreorder 등). Linux 빌드를 꼭 돌릴 것.
- `Isocket::closesocket()` 은 abortive close(SO_LINGER 0 -> RST)다 : 보낸 직후 부르면 커널이 아직 내보내지 않은 것(상대가 읽지 않아 ACK 가 늦고 cwnd 가 막은 것 등)이 버려진다. 다 보내고 닫으려면 `disconnect()`(graceful). 테스트에서 closesocket 뒤 전부 도착을 기대하지 말 것(1.32).
