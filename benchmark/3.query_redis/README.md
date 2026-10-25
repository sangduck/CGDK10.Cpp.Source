# query redis 벤치마크

CGDK10 `query::redis` 를 hiredis, redis-plus-plus, Boost.Redis 와 같은 작업량으로 비교한다.

- 측정 대상 : r11579 (측정 시점 HEAD. r11579 자체는 release_note 갱신. query redis 의 마지막 기능 변경은 r11346 = 공정성·재접속 관리자 수정)
- 측정일 : 2026-10-25
- 원본 : [results/raw_r11579.txt](results/raw_r11579.txt), 중앙값 표 : [results/summary_r11579.md](results/summary_r11579.md)

## 1. 환경

| 항목 | 내용 |
|---|---|
| 머신 | SAKPUNG, AMD Ryzen 9 8945HX (16코어 32스레드, CCD 2개), CentOS Stream 10, kernel 6.12, gcc 15.2.1 |
| Redis | 8.2.10 docker (`--network host`, port 6390, CPU 8 = CCD1 에 고정, 저장 끔, io-threads 1) |
| 클라이언트 | CPU 0-7,16-23 (CCD0) 에 고정. 같은 host loopback 이라 RTT 약 18us (CCD 를 넘어감) |
| CGDK | io_uring 이 꺼진 host (`io_uring_disabled=2`) 라 epoll 백엔드 |
| 비교 | hiredis 1.3.0, redis-plus-plus 1.3.15 (hiredis 위), Boost.Redis (Boost 1.90, `basic_connection<io_context::executor_type>`) |

측정 방식
- 0.5초 예열 후 3초 측정, 3회 교차 반복(회차 -> 경우 -> 클라이언트)의 중앙값.
- `client cpu us/op` = 클라이언트 프로세스 전체 CPU(getrusage) / 처리 수. Redis 가 병목이어도 클라이언트 비용을 비교할 수 있다.
- `redis cores` = 측정 구간의 redis-server CPU (`/proc/<pid>/stat`).
- 키 1만 개(`key:N`), 값은 'x' 반복. latency 는 sync 는 요청 하나, pipeline 은 묶음 하나의 왕복.

## 2. 비교 방식

| 방식 | hiredis | redis-plus-plus | Boost.Redis | CGDK |
|---|---|---|---|---|
| sync (caller 마다 요청 1개씩) | thread 마다 연결 | 연결 풀(크기 = caller) | 연결 1개에 coroutine N개 (다중화) | shared : `execute_query_sync()` / dedicated : thread 마다 `alloc_query()` / async : `post_execute_query()` 콜백 사슬 N개 |
| pipeline (N개 보내고 N개 받기) | `redisAppendCommandArgv` | `Pipeline` | request 1개에 명령 N개 | `post_execute_query()` N번 후 대기 (`+defer` : `send_defer::scope` 로 감쌈) |

`proto` 행은 시험 빌드다 : r11579 + `Iquery::_process_post_execute_query()` 의 `scoped_thread_block` 제거 + data<T>(offset) 경계 수정(r11606 과 같음). 커밋하지 않았다.

## 3. 결과 (GET 16B, 중앙값)

### 3.1 지연 (caller 1)

| 클라이언트 | ops/s | p50 us | p99 us | cpu us/op |
|---|---:|---:|---:|---:|
| hiredis | 53,522 | 18.6 | 20.6 | 5.82 |
| redis++ | 52,734 | 18.8 | 20.9 | 6.04 |
| Boost.Redis | 46,019 | 21.6 | 23.9 | 9.44 |
| CGDK shared | 43,425 | 22.9 | 25.5 | 10.25 |
| CGDK dedicated | 43,886 | 22.7 | 25.2 | 9.94 |
| CGDK async | 50,451 | 19.6 | 22.4 | 7.37 |

SET 도 같은 순서다 (표 참조).

### 3.2 동시 caller (sync, ops/s / cpu us/op)

| 클라이언트 | 4 | 16 | 64 |
|---|---:|---:|---:|
| hiredis | 120,115 | 127,896 | 127,720 / 5.91 |
| redis++ | 119,857 | 128,242 | 128,220 / 6.34 |
| Boost.Redis (연결 1) | 169,033 | 522,235 | 1,089,799 / 0.56 |
| CGDK shared | 89,159 | 74,605 | 74,431 / 19.93 |
| CGDK dedicated | 116,405 | 123,223 | 117,958 / 10.52 |
| CGDK async | 139,176 | 146,463 | 147,414 / 6.30 |
| CGDK async (proto) | - | 585,389 | 1,308,945 / 0.40 |

hiredis·redis++ 는 연결마다 요청이 1개라 13만/s 에서 멈춘다 (Redis 가 연결마다 read/write 하는 비용). Boost.Redis 는 한 연결에 쌓인 요청을 한 번에 쓴다.

### 3.3 pipeline (연결 1, ops/s / cpu us/op)

| 클라이언트 | 10 | 100 | 1000 |
|---|---:|---:|---:|
| hiredis | 467,514 | 2,173,017 / 0.17 | 3,567,946 |
| redis++ | 439,323 | 1,735,334 / 0.30 | 2,563,955 |
| Boost.Redis | 416,880 | 2,171,762 / 0.18 | 4,051,588 |
| CGDK shared | 120,423 | 131,531 / 8.89 | 133,997 |
| CGDK shared+defer | 119,916 | 130,597 / 8.97 | 131,997 |
| CGDK shared+defer (proto) | 325,513 | 885,518 / 1.42 | 1,149,644 |

4 thread x 묶음 100 : hiredis 4.06M, redis++ 4.01M, Boost.Redis 4.03M (Redis 0.85 core), CGDK shared 88K, CGDK dedicated 598K, proto 1.22M.

### 3.4 값 크기 (pipeline 10, 연결 1, ops/s)

| GET | 16 | 1KB | 4000 | 16KB | 256KB |
|---|---:|---:|---:|---:|---:|
| hiredis | 467,514 | 408,762 | 296,098 | 135,774 | 15,003 |
| redis++ | 439,323 | 383,995 | 279,121 | 131,090 | 7,743 |
| Boost.Redis | 416,880 | 335,799 | 209,277 | 75,825 | 4,653 |
| CGDK | 120,423 | 120,694 | 123,978 * | 92,905 * | 13,660 * |

| SET | 16 | 1KB | 4000 | 16KB | 256KB |
|---|---:|---:|---:|---:|---:|
| hiredis | 446,690 | 404,106 | 225,989 | 109,085 | 19,130 |
| redis++ | 423,969 | 382,980 | 218,440 | 105,832 | 18,876 |
| Boost.Redis | 401,502 | 364,759 | 210,536 | 106,451 | 18,650 |
| CGDK | 119,036 | 102,538 | 83,998 | - | - |

\* r11579 에서는 2.5KB 이상 GET 이 멈춰서(결함 1) proto 빌드로 쟀다. CGDK SET 은 r11579 에서 4KB 를 넘으면 프로세스가 죽어서(결함 2) 4000 까지만 쟀다.

## 4. 병목

1. **비동기 post 가 명령마다 send 를 한다.** (r11615 로 수정) `Iquery::_process_post_execute_query()` 가 `scoped_thread_block` 을 연다. 이것이 send_defer 를 비우고(명령마다 sendto 1회, strace 확인) executor 의 thread block 처리(락)를 명령마다 한다. 그래서 묶음 크기와 관계없이 명령당 CPU 가 7~9us, 처리량이 13만/s 에서 멈춘다. `send_defer::scope` 도 소용없다. 블로킹하지 않는 post 경로에서 이 줄을 빼면(proto) 묶음 100 이 13만 -> 89만/s, 콜백 사슬 64개가 15만 -> 131만/s 로 Boost.Redis(109만/s, 0.56us/op)보다 빠르다(0.40us/op). io thread 의 콜백 안 post 는 executor 의 auto_scope(r11496 : executable 안의 한 socket 연속 송신을 자동으로 묶음)가 묶어 준다.
2. **공유 query 는 세션 하나만 쓴다.** (r11616 로 수정) `process_get_shared_session()` 이 항상 `m_sessions_stack.front()` 를 준다. count_session 이 4 든 32 든 나머지 연결은 놀고(Redis `CLIENT LIST` 로 확인 : 한 연결 22만 건, 나머지 0), io thread 하나가 모든 응답을 처리한다. sync shared 는 caller 가 늘수록 오히려 느려진다(4 : 8.9만, 64 : 7.4만/s, 19.9us/op).
3. **sync 는 응답마다 thread 를 깨운다.** 응답을 io thread 가 받아 condition_variable 로 caller 를 깨운다. caller 가 직접 read 하는 hiredis 보다 p50 이 4us 길고 CPU 가 1.8배다(22.9 vs 18.6us, 10.3 vs 5.8us/op).
4. **명령당 고정 비용.** query 객체, std::function, 명령 버퍼 할당, 락 약 20회(querier, query 상태, session 큐, executor thread block. callgrind 에서 pthread_mutex 가 명령어의 23%). proto 로 send 문제를 없애도 1.4us/op 로 hiredis(0.17)의 8배다.
5. 큰 값은 CGDK 가 불리하지 않다 : GET 256KB 13,660/s 로 hiredis(15,003)와 비슷하고 redis++·Boost.Redis 보다 빠르다.

## 5. 측정 중 찾은 결함 (수정됨)

| 결함 | 증상 | 수정 |
|---|---|---|
| 1. `_basic_buffer::data<T>(offset)` 경계를 데이터 끝 뒤 남은 공간으로 검사 | 응답이 수신 버퍼 끝 근처면 RESP 파서가 예외 -> 그 세션이 영원히 응답 안 함 (GET 약 2.5KB 이상) | r11606 |
| 2. 명령 생성이 4KB 고정 버퍼 | SET 약 4KB 초과 시 버퍼 밖에 쓰고 std::length_error 로 종료. 긴 명령 잘림, 끝 공백이 빈 인자 | r11607 |
| 3. `Ireferenceable::release(0)` 이 count 0 에서 다시 final release | querier start -> stop -> 마지막 해제(destroy 없이) 시 pure virtual 로 종료 | r11608 |
| RESP 파서 (끝 넘어 읽기, 이어 받기 정렬 어긋남, 오류 삼킴, `$-1`, 배열) | 위 1 과 함께 정지·오파싱 | r11589 (다른 작업) |

## 6. 병목 수정 후 (r11615, r11616)

- r11615 : post 경로의 `scoped_thread_block` 제거 (병목 1). io thread 콜백 안의 post 는 auto_scope 가, 그 밖의 thread 는 `send_defer::scope` 로 감싸면 묶여서 보내진다. 감싸지 않은 외부 thread 의 post 는 예전처럼 명령마다 바로 보낸다.
- r11616 : 공유 query 를 thread 마다 정해진 세션(thread 번호 % 세션 수)으로 나눔 (병목 2). 한 thread 의 post 순서는 그대로다.
- 병목 3(sync 깨우기), 4(명령당 고정 비용)는 그대로다. 3 은 응답을 io thread 가 받는 구조에서 나온다 (커널 -> io thread -> caller 로 두 번 깨움). caller 가 기다리며 돌게(spin) 하면 응답 시간 동안 core 를 하나씩 태워서 넣지 않았다. 지연이 중요하면 async/coroutine 경로를 쓴다.

r11612(수정 전 HEAD) 대 r11612 + 두 수정, 3회 교차 중앙값 ([results/ab_r11612_opt.txt](results/ab_r11612_opt.txt)) :

| 경우 | 수정 전 ops/s | 수정 후 ops/s | cpu us/op 전 -> 후 |
|---|---:|---:|---:|
| sync shared, caller 1 | 44,021 | 43,752 | 10.1 -> 10.2 |
| sync shared, caller 16 | 75,953 | 216,378 | 19.4 -> 12.2 |
| sync shared, caller 64 | 75,139 | 187,247 | 19.6 -> 15.0 |
| async 콜백 사슬 16 | 149,794 | 597,248 | 6.22 -> 0.73 |
| async 콜백 사슬 64 | 149,264 | 1,359,157 | 6.21 -> 0.37 |
| pipeline 100 (연결 1, send_defer::scope) | 132,298 | 908,718 | 8.87 -> 1.38 |
| pipeline 1000 (send_defer::scope) | 136,332 | 1,201,308 | 7.04 -> 1.23 |
| pipeline 100 x 4 thread (send_defer::scope) | 87,798 | 2,879,152 | 14.2 -> 1.74 |
| pipeline 100 x 4 thread (scope 없음) | 105,398 | 588,556 | 11.8 -> 6.73 |

async 64 는 Boost.Redis(1,089,799/s, 0.56us/op)보다 빠르다. 연결 하나 pipeline 은 아직 hiredis(217만/s, 0.17us/op)의 절반 아래다(병목 4).

## 7. 다시 돌리기

```bash
# 3rd party : hiredis 1.3.0, redis-plus-plus 1.3.15 를 <prefix> 에 설치, Boost 1.84+ 헤더
cmake -S benchmark/3.query_redis -B bld -G Ninja -DCGDK_ROOT=<CGDK> -DBENCH_3RD=<prefix> -DBOOST_INCLUDE=<boost>
ninja -C bld
./run_bench.sh bld [<proto bld>] 3          # docker redis:8.2 를 port 6390 으로 띄운다
python3 summarize.py results/raw_xxx.txt > results/summary_xxx.md
```

`CLIENT_CPUS`, `REDIS_CPU`, `SEC` 환경 변수로 CPU 배치와 측정 시간을 바꾼다.

## 8. Codex 이어받기 (2026-10-25)

Claude 세션 `db76706e-6be0-4d6f-bf0d-69f25b0f50e6`의 마지막 완료 지점은 r11617이다. r11606~r11608의 오류 수정, r11615의 비동기 송신 묶음, r11616의 공유 세션 분산은 현재 작업 사본에 이미 반영되어 있어 다시 적용하지 않았다.

이번 변경은 `session::execute_query()`와 `session::process_message()`에서 이미 잡은 `m_cs_queue_query`를 `push_query()`/`pop_query()`가 다시 잡던 두 곳을 직접 큐 조작으로 바꾼 것이다. 큐 삽입과 송신 순서, 응답과 querier 참조를 함께 꺼내는 바깥 락, 락 밖 콜백 실행은 유지한다. 이 큐 정리는 객체 크기를 바꾸지 않으며 보호된 기존 helper도 유지한다.

SAKPUNG의 기존 Claude `cgdk_opt` 환경(r11612 + r11615/r11616)에 같은 session 소스를 각각 컴파일하고 Redis 정적 라이브러리의 해당 object만 교체했다. 나머지 의존성과 benchmark object는 동일하다. **현재 HEAD 전체 재빌드 비교는 아니다.** Redis 8.2, Release GCC 15, 기존 CPU 배치(클라이언트 0-7,16-23), 2초 측정 + 0.3초 준비, 전후 순서를 교차한 3회 중앙값이다. [원시 결과](results/ab_duplicate_queue_lock_20261025.txt) 30회 모두 errors=0.

| 경우 | 변경 전 ops/s | 변경 후 ops/s |
|---|---:|---:|
| 연결 1, pipeline 100 + defer | 942,025 | 952,071 |
| 연결 1, pipeline 1000 + defer | 1,245,970 | 1,254,960 |
| 4 thread, pipeline 100 + defer | 3,020,968 | 2,990,965 |
| async callback chain 64 | 1,400,847 | 1,426,338 |
| sync caller 1 | 44,608 | 44,563 |

차이는 약 -1%~+2%로 작고 측정 변동과 구별하기 어렵다. 큰 처리량 향상을 주장하지 않는다. 명령당 고정 비용의 대부분(객체/명령 버퍼 할당, query 상태 락 등)은 남아 있다. sync 깨우기 구조도 그대로다.

검증 로그와 재실행용 스크립트는 `build/build-audit/redis-continuation/`에 있다. Windows v145 x64 Debug/Release의 Redis 라이브러리는 현재 작업 사본으로 빌드했고 경고/오류가 0이다. Linux의 server_query 테스트는 아래 새 회귀 테스트를 포함해 23개가 변경한 Redis 라이브러리와 다시 링크하여 통과했다. Linux 테스트와 벤치마크의 다른 의존성은 위 Claude 환경을 재사용했다.
### 이어서 발견한 오류: 임시 문자열 post의 무한 재귀

`querier::post_execute_query(std::string&&, callback)`의 템플릿이 `std::move(_command)`로 자기 자신을 다시 호출했다. `post_execute_query("echo " + value, callback)`처럼 임시 문자열을 넘기면 Release에서 무한 루프가 된다(최적화 방식에 따라 스택 소진 가능). 실제 Redis 검사에서 수정 전 45초 timeout(exit 124)으로 재현했다. 해당 forwarding을 `std::string_view(_command)`로 바꾸어 기존 구현으로 보내도록 수정했다. 기존 구현은 반환 전에 실행용 문자열을 복사하므로 임시 문자열의 수명을 넘겨 참조하지 않는다. 공개 헤더 구현이 바뀌었으므로 이를 사용하는 프로그램은 다시 컴파일해야 한다. 객체 크기/레이아웃 변경은 없다.

- 회귀 테스트 `CGDK10_redis_resp.temporary_command_on_stopped_querier`: 일반 호출과 명시적 템플릿 호출 모두, 시작하지 않은 querier에 임시 문자열을 넘기면 정상적으로 예외를 반환하는지 검사한다. Redis 서버 없이 실행된다.
- Linux Release: server_query 23/23, Windows Release: Redis 테스트 7/7 통과. Windows 테스트는 처음에 10월 23일의 기존 Redis 라이브러리로 링크되어 RESP 테스트가 실패했으며, 이번에 빌드한 라이브러리로 다시 링크해 모두 통과했다. Windows 테스트 프로젝트의 격리 빌드에는 기존 project reference와 IntDir 공유 경고 MSB8028이 있었고, Redis 라이브러리 자체의 Debug/Release 빌드 경고는 0이다.
- 실 Redis: 4개 caller가 임시 문자열 ECHO 요청을 post, 세션 1/4개 각각 8,000건을 5회 검사(총 80,000건), 모두 응답 내용 일치/오류 0. 원래 대용량 검사(최대 8MB SET/GET, 6,000 인자, stop 후 마지막 참조 해제)도 5/5 통과했다.
- 위 처리량 비교는 임시 문자열을 사용하지 않는 기존 benchmark의 큐 락 변경 비교이다. 임시 문자열 재귀 수정의 성능 개선 수치로 해석하지 않는다.
## 9. 배치 최초 구현과 전후 비교 (2026-10-25)

명령별 `post_execute_query()` 반복과 별도로, `querier::post_execute_query(commands, callback)`를 추가했다. 배치 하나가 세션 하나, query 객체 하나, 요청 큐 노드 하나, RESP 명령 버퍼 하나를 사용한다. 명령 전체를 같은 버퍼에 인코딩한 뒤 `session::send()`를 한 번 호출한다(커널의 부분 송신은 기존 송신 경로에서 처리). 응답 파싱과 명령별 성공/실패 통계는 유지하고, callback은 배치 전체에 대해 한 번 실행한다. `send_cork`/`send_defer`만 추가한 구현이 아니다.

```cpp
querier->post_execute_query({"get user:1", "get user:2"},
    [](CGDK::query::redis::querier::batch_results&& results)
    {
        // results[0], results[1]은 입력 순서에 대응한다.
        // 필요하면 results를 소유할 vector로 move해서 보관한다.
    });
```

동적으로 구성한 명령은 `std::vector<std::string_view>` 또는 `std::span<const std::string_view>`로 전달한다. 문자열은 함수가 반환하기 전에 인코딩하므로 이후 원본을 해제해도 된다. 입력 문법은 기존 API와 같은 공백/탭 구분 문자열이며 바이너리 인자 API는 이번 범위에 포함하지 않았다.

### 완료와 취소 계약

- 빈 배치, 빈/공백뿐인 명령, 시작하지 않은 querier는 인수 단계에서 예외를 던지며 callback을 호출하지 않는다. 모든 명령을 인코딩한 후 요청을 등록하므로 잘못된 입력의 앞부분만 전송하지 않는다.
- 수락된 배치는 입력 개수와 같은 결과 vector를 한 번 전달한다. Redis 오류는 해당 위치의 `ERROR_`로 남기고 뒤 응답을 계속 수집한다. 세션이 모두 전용 query에 할당된 경우에는 기존 shared 대기열에서 기다린다.
- stop 또는 연결 단절 시 이미 받은 결과는 보존하고 나머지는 `NONE_`이다. `NONE_`은 응답을 못 받았다는 뜻이며 서버에서 실행되지 않았다는 보장은 아니다. 연결 단절 후 배치를 자동 재전송하지 않는다. 기존 단건 요청의 reconnect 정책은 유지한다.
- callback은 큐/querier 락 밖에서 실행하며, 전송 실패나 취소 상황에서는 호출 스레드에서 바로 실행될 수 있다. callback 안에서 다음 배치를 post할 수 있다. callback 예외는 내부 완료 경로 밖으로 전파하지 않는다.
- 각 명령이 RESP 응답 하나를 만드는 pipeline용 API이며 Redis transaction은 아니다. 응답을 억제하거나 Pub/Sub 모드로 바꾸는 명령 등은 이 계약에 맞지 않는다. 결과는 전체 완료까지 보관하므로 배치 크기에 따라 메모리가 증가한다.

### Release 측정

SAKPUNG, GCC 15.2.1, kernel 6.12.0-273.el10.x86_64, Redis 8.2.10. 클라이언트 CPU 0-7,16-23 / Redis CPU 8. GET 16B, 준비 0.3초 + 측정 2초, 각 경우 전후 순서를 교차한 3회 중앙값. 최초 배치 구현 시점의 동일한 라이브러리/benchmark 실행 파일에서 기존 `shared + send_defer::scope`와 `batch`를 비교했다. 연결 수는 caller 수와 같다. [원시 결과 36회](results/ab_post_batch_20261025.txt)는 모두 errors=0이다.

| caller / batch | 기존 ops/s | 배치 ops/s | 배율 | client CPU us/op 기존 → 배치 |
|---|---:|---:|---:|---:|
| 1 / 10 | 340,763 | 405,911 | 1.19x | 2.306 → 1.273 |
| 1 / 100 | 950,525 | 2,232,040 | 2.35x | 1.314 → 0.188 |
| 1 / 1000 | 1,249,458 | 4,264,865 | 3.41x | 1.193 → 0.078 |
| 4 / 10 | 981,179 | 1,011,136 | 1.03x | 2.563 → 1.359 |
| 4 / 100 | 2,971,619 | 4,033,659 | 1.36x | 1.689 → 0.199 |
| 4 / 1000 | 3,677,399 | 5,622,355 | 1.53x | 1.585 → 0.157 |

위 수치는 GET 16B 조건의 결과다. 큰 값이나 실제 애플리케이션에서의 효과는 아직 측정하지 않았다. Redis 서버가 상한에 가까워지면 클라이언트 CPU 감소율과 처리량 증가율은 일치하지 않는다.

측정에서는 현재 Redis 소스·헤더를 재컴파일하고 다른 CGDK 라이브러리는 Claude의 기존 `cgdk_opt` 환경에서 재사용했다. HEAD 전체의 재빌드 비교는 아니다. Linux 실통신은 기존 기본 I/O 환경에서 검증했으며 io_uring 각 모드의 개별 비교는 하지 않았다.

### 검증과 재실행

- Windows v145 x64 Redis 라이브러리 Debug/Release: 경고·오류 0. Redis 단위 테스트 10/10 통과. 격리된 테스트 빌드의 기존 project reference에서 MSB8028이 발생하지만 라이브러리 컴파일 경고는 아니다.
- Linux Release: server_query 26/26 통과. `check_batch`에서 세션 1/4개 각각 12,000건의 병렬 응답 내용·순서, Redis 오류, 단건, 128KB 값, 전용 세션 대기와 입력 수명, 대기 중 stop, 부분 응답 후 stop, 해당 연결의 CLIENT KILL, callback에서 다음 batch를 post하는 경우를 검사했고 실패 0이다.
- `REQUEST_QUERY` 구조가 바뀌므로 Redis 라이브러리와 해당 헤더를 사용하는 프로그램을 재빌드해야 한다. 공유 lib 폴더에 덮어쓰지 않았으며 Windows 검증 출력은 `build/build-audit/redis-batch/`에 있다.

```bash
# CMake 재생성 후 (7절의 의존 라이브러리 설정 필요)
cmake --build bld --target bench_cgdk check_batch
./bld/check_batch 6390     # 전용 테스트 Redis에서 실행
./bld/bench_cgdk --port=6390 --mode=pipeline --variant=batch --pipeline=100 --sessions=1 --callers=1
```

`run_bench.sh`의 pipeline 비교에도 batch를 추가했다. `check_batch`는 명시 실행용이며 빌드 중 Redis에 접속하지 않는다.
## 10. 타입 기반 API · 동기 배치 · 외부 클라이언트 비교

별도 함수 이름을 쓰지 않고 인자의 타입으로 단건/배치를 선택한다. 문자열 하나는 기존 단건 API, `span<const string_view>`, `span<const string>`, 이들과 호환되는 배열/벡터, initializer_list는 배치다. 기존 단건 반환 형식을 유지하기 위해 동기 배치도 query 객체를 반환하며 `result()`는 `ARRAY_`, `result()[i]`는 각 명령의 응답이다.

```cpp
// 단건
const auto single = querier->execute_query_sync("get key:1");
// 동기 배치
const auto batch = querier->execute_query_sync({"get key:1", "get key:2"});
std::cout << batch->result()[0].str_value;
// 비동기 배치
querier->post_execute_query({"get key:1", "get key:2"}, [](auto&& results) {
    // 입력 순서의 vector<resp_data>. 보관할 경우 move한다.
});
std::vector<std::string> commands{"get key:1", "get key:2"};
auto normal = querier->execute_query(commands);  // 일반 문맥에서 동기 실행
// CGDK coroutine 안에서는 co_await querier->execute_query(commands)
```

동기 경로는 배치당 query/버퍼/큐 노드를 하나씩 사용하고, 기존 동기 shared 세션 할당과 조건 변수 대기를 재사용한다. `scoped_thread_block`으로 지연 송신을 비우고 중지하므로 send_defer 범위 안에서도 대기할 수 있다. 진행 중 취소는 부분 응답을 배열로 유지한다. 세션 할당 대기 취소나 전송 실패는 기존 동기 함수와 같이 예외가 발생할 수 있다. coroutine 지연 실행은 원본 문자열의 수명을 보존한다.

### 비교 벤치마크

[전체 지표 요약](results/clients_compare_20261025.md), [원시 로그 144회](results/clients_compare_20261025.txt), [측정 소스 해시](results/clients_compare_20261025.sources.sha256).

- SAKPUNG AMD Ryzen 9 8945HX, GCC 15.2.1 Release, Redis 8.2.10, hiredis 1.3.0, redis-plus-plus 1.3.15, Boost 1.90.0.
- 8조건 × 6방식 × 3회, 전부 errors=0. 각 지표의 중앙값. 클라이언트 CPU 0-7,16-23 / Redis CPU 8, 준비 0.5초 + 측정 2초, keys=1024. 호출 수와 연결 수를 일치시켰다.
- GET 16B batch 100 연결1: hiredis 2,188,205/s, redis++ 1,750,701/s, Boost.Redis 2,200,878/s, CGDK 기존 944,519/s, 비동기 배치 2,261,440/s, 동기 배치 2,230,584/s.
- GET 16B batch 1000 연결1: hiredis 3,682,378/s, Boost.Redis 4,060,426/s, CGDK 비동기 4,409,884/s / 동기 4,333,383/s.
- 한 건씩 왕복하는 경우 hiredis 53,864/s, CGDK 기존 44,614/s로 hiredis가 빠르다. GET 256KB에서는 hiredis 15,225/s, CGDK 비동기 15,160/s로 비슷하며, 연결 4개·큰 배치에서는 Redis CPU가 상한에 가까워진다. CGDK가 모든 조건에서 가장 빠르다는 의미가 아니다.
- p50/p99는 배치 전체 완료 시간이다. 명령 수로 나눈 지연시간이 아니다. Boost.Redis도 요청 생성부터 시간을 재도록 맞췄다. CPU us/command는 I/O worker를 포함한 프로세스 전체다.
- 현재 Redis 소스와 벤치마크 실행 파일은 재컴파일했다. SDK의 다른 라이브러리와 제3자 라이브러리는 기존 설치를 재사용했다. SDK HEAD 전체의 재빌드 비교/Windows 실성능/io_uring 모드별 비교는 아니다.

### 다시 빌드·실행

7절의 CMake 설정 후 `cmake --build bld --target bench_cgdk bench_hiredis bench_redispp bench_boost_redis check_batch`로 모든 비교 소스를 빌드한다.

```bash
./bld/check_batch 6390
# Redis는 별도로 실행. 같은 호스트의 PID를 지정하면 서버 CPU도 수집한다.
export REDIS_PID=<redis-server host pid>
python3 benchmark/3.query_redis/compare_clients.py bld \
  --output results/clients_compare.txt --port 6390 --rounds 3 --seconds 2
```

`compare_clients.py`는 컨테이너를 생성/종료하지 않고 benchmark key만 채운다. 기존 `run_bench.sh` 전체 matrix에도 비동기 `--variant=batch`, 동기 `--variant=batch_sync`를 추가했다. 벤치마크 옵션의 variant 이름이며 공개 API는 모두 `post_execute_query` / `execute_query_sync` / `execute_query`다.

검증: Windows Redis 라이브러리 Debug/Release 경고·오류0, Windows Redis 테스트11개, Linux server_query 테스트27개 통과. 실 Redis에서 세션1/4개 각12,000건을 동기·비동기로 교차했고 coroutine 입력 수명, 동기 대기, 부분 stop과 연결 단절도 검증했다.

웹 문서: [기술적 설명 → Redis · 배치·성능](http://192.168.0.92:8080/query.redis.html).