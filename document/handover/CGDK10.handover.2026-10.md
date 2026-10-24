# CGDK10.Cpp 인수인계 (2026-10)

2026-10 동안 CGDK10.Cpp 에서 한 작업과, 이어서 작업할 사람이 알아야 할 규칙·결정·남은 일·검증 방법을 정리한 문서다.
세부 변경은 각 리비전의 커밋 메시지에 있고, 여기에는 **코드만 봐서는 알기 어려운 것**을 적는다.

- 저장소: `ProjectCGDK/CGDK10.C++.public` (svn)
- 작성 시점 HEAD: r11387 + 이 문서가 들어간 커밋 (이후 갱신: r11401, r11418 까지 1.12 에 반영, r11472 까지 1.22 에 반영, r11477 까지 1.25 에 반영, r11487 까지 1.27 에 반영)

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
- **벤치** (참고용, Debug, `CGDK_BENCH=3`, multishot / `CGDK_IO_URING_MULTISHOT=0` 교대 2회. 실행 간 흔들림이 차이보다 클 수 있다) :

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
- **CPU 를 오래 쓰는 handler 는 그대로** : `COOP_TASKRUN` 이라 owner 가 사용자 공간에서 바쁘면 CQE 자체가 올라오지 않는다(대신 실행할 것이 없음). 끄면 IPI 비용이 생긴다. completion 서버의 handler 는 짧게, 긴 계산은 다른 executor 로.
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

### 1.29 completion 송신 경로 - buffered 의 SEND op, CQE 배치 끝 송신(batch send defer) (r11494 + 배치 defer 는 커밋 대기, 2026-10-24)

남은 일 8 의 "남은 것 : 송신 쪽". 먼저 이미 들어간 것 : gather(`tcp<>`)는 r11441/r11457 부터 **직접 send + socket 버퍼가 차면 SENDMSG op**(EPOLLOUT 대신), 큰 단일 버퍼는 SEND_ZC op, 선택(기본 off)으로 소유 worker 송신 op(`CGDK_IO_URING_SEND_OP=1`). IOCP 와 같은 모델이다(WSASend 도 대개 그 자리에서 끝나고 막힐 때만 완료를 기다린다). 남은 것은 셋이었다.

- **buffered 가 completion 이 아니었다 (r11494)** : socket 은 receivable·sendable 이 모두 `is_io_completion_supported()` 여야 completion 이 되는데 `Ntcp_async_gather_buffered` 는 false 라 `tcp_buffered<>` / `tcp_queue` / redis querier session 은 completion executor 위에서도 통째로 readiness(POLL_ADD)였다. 이제 gather 와 같은 구조 : EPOLLOUT 대기 3곳을 `_process_post_send_wait()`(completion 이면 SENDMSG op). op 는 `m_queue_pending` 앞(되돌려 둔 나머지 + 그 뒤에 쌓인 것, IOV_MAX 개까지)을 `m_queue_sending` 으로 옮겨 보내고(iovec 은 executable 안), 완료가 소비 -> 남으면 op 다시, 다 보냈으면 `_process_send_pending()` 으로 직접 송신을 잇는다. op 묶음은 큐 한도에 세지 않고(r11361 의미) `sending_queued_bytes_now()` 에는 넣는다(`m_bytes_sending_op`). 사용자 버퍼 하나(>= 16KB, IP)는 SEND_ZC op(-ENOBUFS 면 r11490 처럼 복사로 다시). op 중 reset 은 노드를 executable 의 `m_queue_hold` 로. 새 파일 `net.io.sendable.tcp_async_gather_buffered.uring.cpp`, **헤더 멤버 추가 : gruma 재빌드**.
- **gather 의 op 완료가 대기 바이트를 되돌리지 않았다 (r11494)** : `_process_complete_send_op()` 가 `m_count/bytes_sending_parked` 를 0 으로 안 돌려(epoll 은 `process_recover_sendable` 이 돌림) 다 보낸 뒤에도 `sending_queued_bytes_now()` 가 옛 값(r11441 부터).
- **completion 의 64B x32 열세의 원인 = CQE 마다 send 하나** : 공정 벤치(r11487 : 서버도 case 의 executor)에서 POKPUNG completion tcp 64B x32 가 epoll 의 -37%(9.91M / 15.68M). `strace -c -f`(rate 는 왜곡, 비율만) : epoll 은 recv 한 번이 창(16개)의 메시지를 받아 **송신 1회에 메시지 ~6.6개**(sendto 0.15/msg), completion 은 multishot RECV 가 도착(세그먼트)마다 CQE 를 하나씩 내고 CQE 마다 send cork 가 끝나 **sendto 0.76/msg**. syscall 을 없앤 수신 대신 송신 syscall 이 5배.
  - **배치 송신 defer** (커밋 대기 : 아래) : completion executor 가 CQE 배치(최대 32, 실행 10us 에 맞춰 적응) 전체를 `send_defer` 배치로 묶는다(`send_defer::begin_batch()/end_batch()`, `executor.io_context.cpp`). 배치 안의 송신은 그룹 락 안처럼 모였다가 배치 끝에 socket 마다 한 번 나간다. 의미 유지 : 배치 안에서도 cork 나이 확인(`is_cork_age_checked()` : 그룹 락만 아니면 true, sendable 의 `depth() == 0` 을 대체), 배치 안의 그룹 락은 풀릴 때 자기 송신을 보냄(`enter/leave` 가 배치 depth 를 바깥으로 봄), `send_defer::enable(false)` 와 무관, 배치가 100us 넘게 돌면 다음 CQE 전에 모은 것을 보냄(`BATCH_SEND_DEFER_DURATION`, 이미 읽는 실행 시작 시각을 써서 clock 추가 없음 : 긴 handler 가 같은 배치의 다른 socket 송신을 붙잡는 상한), 배치 하나짜리는 그대로(socket cork), handler 안의 펌핑·thread block 중에는 열지 않음, `CGDK_IO_URING_BATCH_DEFER=0` 으로 끔. epoll / readiness 경로는 무변경(`batch.pcontext` 가 completion 일 때만).
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
- **배치 defer 가 커밋 대기인 이유** : 같은 두 파일(`send_defer.h`, `executor.io_context.cpp`)에 다른 세션의 미커밋 `send_defer::auto_scope`(executable 하나 안의 같은 socket 연속 송신 묶기)가 있다(`is_deferring` 같은 줄). 패치는 HEAD 판·그 WIP 판 모두에 적용됨을 확인했다 : side-session-notes `local_bca2e593.../uring_send_batch_defer/`(perl 스크립트 `send_defer_batch.pl`, `batch_defer.pl`, sendable 2줄 `sendables.sh`, diff). auto_scope 와 함께 써도 배치 안에서는 depth 조건이 먼저라 auto 는 동작하지 않는다(충돌 없음).


## 2. 남은 일 (우선순위 순)

1. ~~Ntcp_reconnect 즉시 실패 시 재접속 미예약~~ - 해결(r11404, 1.12).
2. ~~accept 후 attach 사이 fd 재사용 창~~ - 해결(r11414).
3. ~~`connective::connector` UDS 지원~~ - 해결(r11406).
4. ~~그룹 락의 남은 교착 가능성·null 역참조·container 콜백 UAF~~ - 해결(r11402, r11409, r11410). on_member_* 이벤트 안에서 다른 멤버 락을 잡으면 여전히 교착 : 규칙으로 .h 주석에 둠.
5. ~~object pool 종료 시 TLS 잔여 객체 누수~~ - 해결(r11411, factory::object 통계 TLS 는 r11412).
6. ~~`_buffer_unreal.h` 결함 3건과 한글 주석~~ - 수정(r11408). 실제 UE 빌드로 확인 필요, FString 경로 문제는 남음.
7. ~~Linux 빌드에서 `bld/` 와 `bld_debug/` 동시 빌드 시 `svn_version.h.tmp` 경합~~ - 해결(r11403).
8. **io_uring completion 모드**(1.11) - **1단계(백엔드) 완료 r11437(1.14). 2단계(소켓 op 전환) r11441(1.15) + 백엔드 staging r11444(1.16) + r11448(handle 한도, transient worker, acceptor 대기) + r11451(1.17 : non-owner enter 금지, flush+submit 한 락, chunk handle 표) : mistral completion 전체 스위트 통과(1회차 uds stop 지연 3개 남음 : 1.17). 3단계(multishot recv+provided buffer) r11453(1.18) : WSL·mistral 전체 스위트 3 모드 통과. 4단계 r11457(무복사 수신, 소유 worker 송신 op : 다른 세션, 커밋 메시지 참고). 수신 모드 전환 r11459(1.19 : 큰 트래픽은 IOCP 식 가변 RECV 버퍼, 작은 메시지는 multishot). 남은 것 : 송신 쪽. emergency.* 기존 실패 3개는 해결(1.20 : 테스트 전제 2개 + io_uring 두 백엔드의 post 정체 버그). r11459 Release 벤치는 2026-10-24 POKPUNG(32 CPU, kernel 7.0, 8 thread, 교대 5라운드 중앙값)에서 쟀다 : 수신 모드 전환 on/off 차이 없음(tcp 64KB x4 177.0K / 177.0K msg/s, 나머지도 노이즈 안). 같은 실행의 epoll 대비 completion : tcp 64B x32 -32%(6.58M / 9.74M), uds 64B x32 -37%(6.72M / 10.73M), tcp 16B x256 +17%(848K / 726K), tcp 64KB x4 +15%(177K / 154K), udp +40%(657K / 469K). 64B window16 의 격차는 r11457 메시지의 분석(접속당 메시지가 몰릴 때의 CQE 단위 처리)대로 남았다. => 송신 쪽 정리(1.29) : buffered 도 completion(SENDMSG / SEND_ZC op, r11494), gather op 완료의 대기 바이트 수정(r11494), 64B 격차의 원인은 CQE 마다 send 하나(sendto 0.76/msg vs epoll 0.15) -> CQE 배치 끝 송신(배치 send defer, 커밋 대기)으로 공정 벤치 POKPUNG tcp 64B x32 completion 9.91M -> 18.16M(epoll 15.68M 의 +16%). 위 -32%/-37% 는 서버가 system executor(epoll)에서 돈 측정이었다(1.27).** 지금 백엔드는 준비 이벤트(POLL_ADD)만 쓰므로 syscall 수가 epoll 과 같다. IORING_OP_RECV/SEND(+ provided buffer)로 송수신 계층을 다시 써야 이득이 난다. WSL2 loopback 에서는 epoll 보다 느리다(단일 CQ 대기자) -> POKPUNG 실 NIC 에서 다시 측정할 것.
9. ~~`socket_option.not_applied_without_option` 순서 의존 실패~~ - 해결(이 커밋). 원인은 늦은 `on_connect` 가 아니라 **풀에서 재사용된 서버 socket 이 직전 acceptor 의 `socket_option()` 을 그대로 가진 것**이었다(새 acceptor 는 옵션이 없으면 덮지 않으므로 옛 quickack/notsent_lowat/nodelay off 가 새 접속에 적용됨). acceptor(두 경로)와 connector 가 socket 을 할당한 직후 `socket_option()` 을 기본값으로 되돌린다 : '자신의 값' 은 `on_request_accept()`/`on_alloc_connectable()` 에서 코드로 정한 값이다. 교훈 : 풀로 재사용되는 socket 객체에 새 필드를 넣으면 재사용 시점의 초기화를 같이 넣어야 한다(`m_socket_option` 외에도 같은 종류의 필드가 있는지 점검할 것).
10. **1.11 의 Linux UDP 수정 전에는 Linux UDP 수신이 전혀 안 됐다** - `Nudp` 를 쓰는 프로젝트가 있으면 r11392 이전 트리에서 동작했는지 확인할 것(IOCP 는 해당 없음). gruma 는 UDP 를 쓰지 않음(확인).
12. ~~io_uring 에서 남은 간헐 실패~~ - 해결(r11417 io_uring, r11418 acceptor. 1.12). 재접속 관리자 정지·정리 지연·uds 불안정·pure virtual call 모두.
12. ~~io_uring 에서 남은 간헐 실패 (조사 중)~~ - 위 12 와 같은 항목의 옛 문장(중복) : r11417·r11418 로 해결. 그 중 `linux_io.io_uring_executor` 의 "connected yet" 정리 지연은 2026-10-24 POKPUNG r11473 completion 실행 8회 중 1회 다시 봤다(41초, readiness executor 를 테스트 thread 가 만드는 테스트) : r11474 이후 재발하는지 볼 것. => r11474 이후에도 재발 : 2026-10-24 MONSOON(kernel 7.0, 24 CPU) r11478 Debug completion 전체 5회 중 1회 "connected yet 1"(41초, 3) 접속 직후 close 200회 단계). 같은 빌드의 epoll / readiness 전체는 통과.
    - **해결 (2026-10-24, net.io.socket.tcp.cpp)** : `process_close_socket()`(epoll/readiness)이 갈래를 고르며 `socket_state()` 를 세 번 따로 읽었다(`> SYN` / `== SYN` / `<= BINDED`). close 시작 때 SYN 이고 두 읽기 사이에 다른 thread 가 접속 완료(ESTABLISHED)를 처리하면 **어느 갈래도 타지 않았다** : handle 만 INVALID 로 가져가고 fd 는 닫히지 않으며 EPOLLHUP 도 큐에 없고 disconnect 도 없음 -> `stop()` 의 `wait_complete_closed()` 30초 timeout, 서버 쪽은 FIN 을 못 받아 목록에 남음(30 + 10 = 41초). 진단으로 확인한 상태 : 클라이언트 ESTABLISHED, handle -1, epoll handle 그대로, processing 0, queued 0, on_connect 1 / on_disconnect 0. 이제 state 를 한 번만 읽는다(SYN 이면 4.B : `process_complete_connecting(EPOLLHUP)` 의 `is_close_after_connect` 가 그 사이 접속된 socket 의 disconnect 를 끝낸다). epoll 에도 같은 창이 있었다(예전 epoll 기본 9회 중 1회).
    - 검증 (MONSOON Debug, 3) 단계를 5000회로 늘린 진단 빌드, completion 기본) : 원본 6회 중 1회 같은 상태로 재현, 수정본 26회(13만 close) 느린 stop 0. 수정본 전체 스위트 completion / epoll / readiness 각 2회 통과(epoll 1회에 `linux_io.udp_send_deferred_in_group` 1개 실패 : 단독 원본·수정 40/40 통과, 무관한 간헐로 봄).
    - 같이 고친 테스트 전제 : completion 기본에서 이 테스트를 단독으로 돌리면 system executor 가 worker 마다 ring 을 나중에 만들어 544행 fd 수가 8 대 50 으로 항상 실패했다 -> 세기 전에 fd 수가 200ms 안정될 때까지 기다림(`lio_count_io_uring_fd_settled`, `tcp_zerocopy_not_on_epoll` 도). 단독 completion 10/10 통과.
13. ~~readiness io_uring 의 `emergency.manager_signals` 간헐 실패~~ - 해결(r11474, 1.23). "응답이 다음 수신 때 나간다"는 오해였다 : 테스트 클라이언트의 `SO_RCVTIMEO` recv 가 io_uring task_work 로 **EINTR**. 요청·ring 을 가진 thread 규칙(1.23 표)과 수정(worker 만 enter, ring 은 임시 thread 가 생성, completion 깨우기는 eventfd).
14. **실행기·IOCP 쪽 남은 최적화 후보** (2026-10-20 분석, 미착수 : 착수 전 `svn log` 로 이미 바뀌었는지 확인)
    - IOCP(`api.io_context.iocp_direct.cpp`) : `GetQueuedCompletionStatus` 로 1건씩 -> `GetQueuedCompletionStatusEx` 배치(epoll 처럼 적응형 event 배치), `FILE_SKIP_COMPLETION_PORT_ON_SUCCESS` 미사용, WSASend 를 `m_lockable_sending`(recursive_mutex) 안에서 호출하고 완료 시 버퍼 해제도 락 안, 수신 tail 최소 64B(`MINIMUM_REMAINED_RECEIVE_BUFFER`)라 작은 창으로 재등록.
    - strand : 큐를 move 해 capacity 를 버림(배치마다 재할당), recursive_mutex, `post_at` O(N) list 삽입 + 전역 heap push.
    - lambda post 가 `std::function` 으로 감싸짐(libstdc++ 에서 object_ptr 캡처면 malloc/free) + 풀로 돌아간 래퍼가 캡처를 지우지 않아 캡처한 object_ptr 가 재사용 때까지 살아 있음(수명 누수).
    - 기본 thread 수 2×core+2 가 cgroup·affinity 를 무시(컨테이너에서 과다). `queue_list::m_lockable_queue_executable` 은 recursive_mutex.
    - "RIO" : `*.rio.cpp` 는 실제로 WSARecv/WSASend, `api.io_context.rio.cpp` 는 `#if __linux__` 로 감싸여 의미 없음.
15. ~~completion 의 post 가 바쁜 ring 에 묶임~~ - 해결(r11475, 1.24). jam 테스트 간헐 실패의 원인.
16. ~~completion 의 `linux_io.send_cork_sends_old_responses_{gather,buffered}` 간헐 실패~~ - 원인 확정·테스트 수정(r11477, 1.25) : 클라이언트가 서버와 같은 ring(같은 worker thread)에 묶여 서버 handler 뒤에 실행됐다. cork 는 정상(배치 중 3메시지마다 송신). 테스트는 이제 서버가 보낸 바이트로 판정.
17. **completion 은 같은 ring 의 socket 들을 owner thread 하나에서 차례로 실행한다** (설계 특성, 1.25 가 보여 줌) - **블로킹(`scoped_thread_block`)은 해결(r11481, 1.26 : 막힌 owner 의 ring 을 다른 worker 가 실행). CPU 를 오래 쓰는 handler 는 남음(COOP_TASKRUN : CQE 가 올라오지 않음, 사용자 결정으로 범위 밖) : 사용자 문서에 "handler 는 짧게" 를 적을 것.** 아래는 처음 적은 문장 : - 한 socket 의 긴 handler 는 같은 ring 의 다른 socket 들의 처리를 그만큼 늦춘다(epoll·IOCP 는 다른 thread 가 가져감). **handler 안의 blocking(`scoped_thread_block` 의 sync query 등)도 마찬가지**다 : 늘어나는 extra thread 는 ring 이 없어(transient, 1.17) post 큐만 처리하므로 막힌 owner 의 ring 을 대신 실행하지 못한다(코드 읽기, 미측정). socket 의 ring 은 처음 제출한 worker 로 정해지고 옮겨지지 않는다(owner 가 끝날 때만 고아 ring 으로 다른 worker 가 드레인). 고치려면 막힌/바쁜 owner 의 ring 을 다른 worker 가 실행하게 해야 하는데, 한 socket 의 CQE 는 한 번에 하나씩·순서대로 실행돼야 하므로(1.18 : multishot 데이터의 순서) socket 단위 직렬화가 먼저 필요하다. 그때까지 completion 은 handler 가 짧은 서버(또는 blocking 작업을 별도 executor 로 넘기는 서버)에 맞다 : 사용자 문서(`socket.io_uring.html`)에 적을 것.
18. ~~completion 에서 한 acceptor 가 받은 socket 이 모두 listen socket 의 ring(thread 하나)에 묶인다~~ - 해결(r11487, 1.27) : accept CQE 에서 새 fd 를 owned ring 에 round robin 으로 바인딩. 8 thread 벤치 completion tcp 64B x32 2.48M -> 10.58M msg/s, uds 13.9M -> 25.1M, 16B x256 878K -> 1.18M. 기존 벤치는 서버가 system executor(epoll)에서 돌아 이 상태를 재지 못했다(1.27). 아래는 처음 적은 문장 : (2026-10-24 r11481 의 테스트로 확인) - socket 의 ring 은 처음 제출한 worker 로 정해지는데(HANDLE_ENTRY::binding), accept 완료는 listen ring 의 owner 가 실행하고 그 handler 가 첫 RECV 를 제출하므로 accept 된 socket 이 전부 그 ring 에 간다. mistral 200 접속에서 199/199 가 한 thread(epoll 은 이벤트마다 아무 thread). 그 ring 의 owner 가 그 acceptor 의 모든 접속을 처리하므로 다른 worker 가 놀아도 서버 처리량이 thread 하나로 묶일 수 있다(벤치에서 확인 필요 : 기존 completion 벤치가 이 상태에서 잰 것인지). 고칠 방향 : accept 된 socket 의 첫 제출을 round robin ring 으로(또는 accept 완료 뒤 receive 시작을 다른 worker 로 post), 기존 바인딩 규칙(고아·빌린 ring 의 handle 은 다음 제출자 ring 으로 옮김)과 맞출 것.
19. ~~linux acceptor 는 accept 한 socket 에 acceptor 의 io_context 가 아니라 system executor 를 준다~~ - 해결(r11489, 2026-10-24) : windows·헤더 주석과 같은 규칙으로 맞췄다.
    - 원인 : `net.io.connective.acceptor_tcp.cpp` 의 linux `request_accept(handle, af)` 는 `process_get_connectable_io_context()` 가 비면 socket 이 이미 가진 io_context, 그것도 없으면 `system_executor::executor()` 를 썼다. 헤더 주석(`process_get_connectable_io_context` : "empty (the io_context of the acceptor is used)")과 windows 경로(`pconnectable->io_context(pio_context)` : acceptor 의 것)와 달라, acceptor 에 io_context 를 준 linux 서버의 접속은 system executor 에서 돌았다. 테스트·벤치(`lio_check_tcp_echo`, `io_uring_completion_sockets` 등)의 서버 socket 도 system executor 였다(벤치는 "server 66 thr" 로 확인. 그 테스트들의 "completion socket" 은 클라이언트뿐 : `CGDK_IO_URING=completion` 전체 실행 때만 서버도 completion).
    - 수정 : hook(`process_get_connectable_io_context`) -> acceptor 의 io_context -> system executor 순서로 고르고, 고른 것을 항상 socket 에 넣는다. 옛 handle 의 detach 는 그 전에 옛 io_context 로 한다.
    - 풀 socket 의 옛 io_context : 정상 종료(`detach_epoll_handle`, completion 의 `process_complete_closesocket`)는 io_context 를 비우므로 보통은 빈 채로 돌아온다. 그러나 close 의 SYN·BINDED 갈래는 epoll handle 만 비우고 io_context 는 남긴다 -> 옛 코드는 그 io_context(멈춘 executor 일 수도 있음)를 그대로 썼다(코드 읽기, 재현 안 함). 이제는 accept 마다 다시 정하므로 해당 없음.
    - 영향 : acceptor 에 io_context 를 준 linux 서버만. gruma 는 acceptor 에 io_context 를 주는 코드가 없다(`D:\project_gruma2` 의 server·common·client 소스에 `io_context(`·`process_get_connectable_io_context`·`make_io_context_threads` 없음 : 기본값인 system executor 그대로). acceptor 의 io_context 는 listen socket 을 만들 때 비어 있으면 system executor 로 채워지므로(`process_create_socket`) io_context 를 주지 않은 서버는 동작이 같다. 라이브러리 안의 사용처 `net::emergency::Nacceptor` 는 hook 으로 socket 마다 executor 를 주므로 그대로. 바뀌는 것 : tutorial `socket.ex15.socket_option` 5번(busy poll executor 를 준 acceptor : 이제 서버 socket 도 그 executor, 예제의 의도대로), `test_linux_io`·`test_uring_send`·`bench_linux_io` 처럼 acceptor 에 executor 를 준 테스트의 서버 socket. r11487 의 `lbn_acceptor` / `lio_thread_acceptor` override 는 같은 결과라 남겼다(옛 트리 비교용).
    - 테스트 `test_acceptor_io_context.cpp`(net_socket.http, linux) : `the_executor_of_the_acceptor`(2 thread executor 를 준 acceptor + blocking 클라이언트 8 echo -> 서버 socket 의 on_connect / on_message 16번 모두 그 executor), `the_system_executor_by_default`(io_context 없는 acceptor -> system executor). r11489 트리에 r11488 의 `acceptor_tcp.cpp` 를 넣은 빌드에서 `the_executor_of_the_acceptor` 실패(socket 의 io_context = system executor) 확인(SAKPUNG, epoll, Debug).
    - 검증 : 커밋 때 monsoon(kernel 7.0) net_socket.http 전체 epoll / io_uring / completion x2 134 통과, mistral 새 테스트 3 백엔드 통과. 이후 SAKPUNG(kernel 6.12, io_uring 꺼짐) r11489 export Debug 전체 epoll x2 : 125 통과(skip 12 = io_uring 전용 9, bench 3). 3 백엔드 x2 를 mistral 에서 다시 돌리지는 않았다(그 시각 다른 세션이 mistral·POKPUNG 에서 벤치 중).
20. **completion / epoll 비교 수치를 다시 잴 것** (2026-10-24) - 남은 일 8 의 r11459 POKPUNG 수치, 1.18·1.19 의 벤치, r11486 의 `document/Web/site/socket.recv.html`(32 코어 표)는 서버가 system executor(epoll)에서 돈 측정이다(19, 1.27). r11487 의 벤치(서버도 case 의 executor)로 mistral 8 thread 에서 잰 completion / epoll : tcp 64B x32 -23%, 64KB x4 +25%, uds +73%, 16B x256 +34%, udp +47%. 남은 격차(tcp 64B x32)와 uds 와의 차이(uds 는 completion 이 epoll 보다 빠름)는 미분석. 클라이언트 쪽 분포 : 클라이언트 socket 은 CONNECT post 를 받은 worker 의 ring 에 묶이고 post 는 깨운 ring 이 최대 16개씩 가져가므로 몰릴 수 있다(settle 뒤 "client 8 thr top 20~37%", executor 를 만든 직후 1차 측정은 3 thr 까지). 접속을 많이 여는 클라이언트(부하 테스트기)라면 accept 와 같은 round robin 바인딩을 CONNECT 에도 줄지 검토. => tcp 64B x32 격차는 분석·해결(1.29 : CQE 마다 send 하나, 배치 send defer 로 POKPUNG completion +16% vs epoll). POKPUNG r11488 공정 벤치 : epoll 15.68M / 16.96M(uds) / 1.06M(16B) / 176K(64KB), completion 9.91M / 30.18M / 1.53M / 150K.
21. **completion 송신의 남은 것** (2026-10-24, 1.29)
    - 배치 send defer 커밋(1.29 의 패치 : `send_defer.h`, `executor.io_context.cpp`, gather/buffered epoll.cpp 의 `depth() == 0` -> `is_cork_age_checked()`). 같은 파일의 `send_defer::auto_scope` WIP 와 순서를 정할 것.
    - 소유 worker 송신 op(`CGDK_IO_URING_SEND_OP=1`, 기본 off)의 graceful close : staging 된(아직 enter 전) 송신 op 를 같은 handler 의 close 가 shutdown + `cancel_fd` 로 지워 데이터를 잃는다. 켜려면 close 경로가 그 op 를 먼저 제출하거나(owner 의 즉시 submit API : 백엔드 변경) 직접 send 로 바꿔야 한다. 배치 defer 와 같이 쓰면 성능도 손해라 우선순위 낮음.
    - 풀 재사용 socket 의 per-socket 송신 한도 : `maximum_depth_of_send_buffer(n)` / `maximum_depth_of_send_buffer_bytes(n)` 을 socket 에 걸면 그 객체가 풀로 돌아가 다음 접속에 재사용될 때 그대로 남는다(`process_reset_sendable` 은 SO_SNDBUF 만 되돌림, windows 도 같음). 남은 일 9 와 같은 부류. `test_uring_send` 는 테스트 끝에 되돌린다.
    - 실 NIC 측정(SEND_ZC, 64KB), ASan / valgrind.

---

## 3. 검증 방법

### 3.1 단위 테스트

| 테스트 | 위치 | 비고 |
|---|---|---|
| 블로킹 handler (completion 의 빌린 ring) | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_blocking_handler.cpp` | Linux 전용, CMake 만, 프로세스의 backend 로 돈다(`CGDK_IO_URING`). 200 접속 중 하나가 300ms 블로킹하는 동안 나머지가 100ms 안에 응답, 순서 확인(1.26). 수정 전 completion 4/4 실패. completion 이면 접속이 여러 thread 에 퍼졌는지도 단언(1.27) |
| completion 송신 경로 | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_uring_send.cpp` | Linux 전용, CMake 만. tcp<> / tcp_buffered<> : socket 버퍼 가득(SENDMSG / SEND_ZC op, 순서), send 후 close, op 중 close · RST, 큐 한도(1.29). r11488 에서 5개 실패 |
| UDS / epoll 핸들 | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_uds.cpp` | Linux 전용(`_SYS_EPOLL_H`), CMake 만. 23개: listen / connect / 통신 / epoll 핸들 수명 |
| io_uring completion 백엔드 | `build/src/system/unit_test/unit_test.object.execute/test_io_context_uring_cq.cpp` | Linux 전용, CMake 만. 13개: post / 바쁜 ring 으로 간 post 를 쉬게 된 worker 가 가져감(1.24) / readiness 호환 / recv·send / ZC·gather / multishot accept·connect / accept 된 socket 의 round robin 바인딩(1.27) / cancel / stop / 짧게 사는 thread / fd 한도 / multishot recv + provided buffer(1.18) |
| Linux 소켓 성능 요소 | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_linux_io.cpp` | Linux 전용, CMake 만. nonblock 생성 / TCP_DEFER_ACCEPT / 수신 버퍼 히스테리시스 / UDP 배치 echo(epoll, io_uring) / zerocopy(io_uring 만) / cpu_affinity / io_uring executor(TCP·UDS·대용량·UDP·close 폭주·ring 해제) / worker 만 enter : 다른 thread 의 blocking 호출이 EINTR 되지 않음(readiness·completion, 1.23) / send cork / completion 에서 accept 된 socket 이 worker 에 퍼짐(1.27) |
| accept 된 socket 의 io_context | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_acceptor_io_context.cpp` | Linux 전용, CMake 만, 프로세스의 backend 로 돈다. 2개: executor 를 준 acceptor 의 서버 socket 이 그 executor / io_context 없는 acceptor 는 system executor(2. 남은 일 19, r11489). 수정 전 첫 번째 실패 |
| Linux echo 벤치 | `build/src/net.socket/unit_test/unit_test.net_socket.http/bench_linux_io.cpp` | `CGDK_BENCH=<초>`(없으면 skip), `CGDK_BENCH_THREADS`. tcp 64B/64KB, uds, udp echo 를 epoll 과 io_uring 으로. r11392 트리에서도 빌드됨(기준선). r11487 부터 서버 socket 도 case 의 executor(전에는 system executor), case 마다 handler thread 분포 출력(1.27) |
| 접속 실패·connector | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_connect_requestable.cpp` | Windows/Linux. 즉시 실패·거절의 on_fail_connect/CLOSED/stop/재접속, connector 대량 접속(2. 남은 일 11 의 재현 테스트) |
| TCP socket option | `build/src/net.socket/unit_test/unit_test.net_socket.http/test_socket_option.cpp` | Windows/Linux. 12개: 기본값 off / 파라미터 파싱 / accept·connect 적용(getsockopt) / 코드 설정 / START_PARAMETER / context / connector / TFO connect / UDS skip(linux) |
| 그룹 락 | `build/src/server.system/unit_test/unit_test.server_system.service/test_group_*.cpp` | 원본 코드면 동시성 테스트가 60초 감시에 걸림 |
| 실행기(queue_list·scheduler) | `build/src/system/unit_test/unit_test.object.execute/test_executor.cpp` | Windows/Linux. 몰린 post 를 idle thread 가 동시에 처리, stop() 이 기다리는 thread 를 바로 깨움(+재시작), scheduler 실행·cancel 동시성(1.22). 시간 기준 assert(500ms·300ms)라 매우 바쁜 장비에서는 여유를 볼 것 |
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
