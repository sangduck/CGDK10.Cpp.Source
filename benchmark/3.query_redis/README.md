# Redis API와 클라이언트 비교

## 최신 구현과 비교 (2026-10-26)

[최종 비교와 검증](results/clients_current_20261026.md). 이전 날짜의 측정은 당시 구현 결과이며 현재 기본값 비교는 이 문서를 기준으로 한다.

- `post_execute_query`: 사용자 callback을 post하는 것이 기본이다. `START_PARAMETER.post_response=false`로 직접 처리할 수 있다. `presponse_executor`를 비워 두면 Redis I/O executor를 사용한다.
- 동기 `execute_query`: post 여부와 관계없이 완료 통지는 직접 처리한다. RESP 파싱·응답 매칭은 I/O에서 수행한다.
- 입력은 반환 전에 소유하고 세션이 전송 직전에 인코딩한다. 개별 요청도 큐에서 모아서 보내며 응답은 도착 즉시 처리한다. 명시적 batch만 전체 결과를 모아 한 번 통지한다.
- start에서 고정한 연결을 사용하고 stop 이후 접근은 실패한다. 일반 공유 연결 선택은 querier mutex를 사용하지 않는다. 전용 연결·관리·대기열에는 필요한 동기화가 남아 있다.
- Redis 자동 `send_cork`와 해당 옵션은 제거했다. `send_defer`, `request_send_window`(인코딩 방식에 따른 적용 경로), coroutine executor는 별도 설정이다.
- 라이브러리와 공개 구조체가 변경되므로 SDK와 사용하는 프로그램을 함께 재빌드한다.


Redis 공개 요청 이름은 `execute_query`로 통합한다. 일반 호출은 동기로 실행하며 CGDK coroutine에서는 `co_await`로 응답을 기다린다. 문자열은 단건, 문자열 배열·벡터·span·initializer_list는 배치다.

```cpp
auto single = querier->execute_query("get key:1");
auto batch = querier->execute_query({"get key:1", "get key:2"});
// batch->result()는 ARRAY_, 각 응답은 batch->result()[i]

// CGDK coroutine 안에서
auto response = co_await querier->execute_query("get key:1");
auto responses = co_await querier->execute_query({"get key:1", "get key:2"});

querier->post_execute_query({"get key:1", "get key:2"},
    [](CGDK::query::redis::querier::batch_results&& results) {
        // 입력 순서대로 결과를 한 번 전달한다.
    });
```

`execute_query_sync` 공개 함수는 제거했다. 내부 동기 구현은 protected helper다. `querier`와 전용 `Iquery` 모두 `execute_query`를 사용한다. 반환형은 기존 `execute_query`의 `execute_result`이며 단건/배치 결과 query는 `->` 또는 `.get()`으로 접근한다.

배치는 query 하나·세션 하나·큐 노드 하나·RESP 버퍼 하나로 등록한다. 모든 명령은 반환 전 인코딩되며 coroutine 지연 실행도 입력 문자열의 수명을 보존한다. 빈 배치/잘못된 명령/중지된 querier는 예외로 거부한다. Redis 오류는 해당 위치의 ERROR_ 결과다. stop/연결 단절 시 받은 응답은 보존하고 미수신 항목은 NONE_으로 남기며 완료 callback은 락 밖에서 한 번 호출한다. 실행 여부가 불명확한 배치는 자동 재전송하지 않는다. pipeline은 transaction이 아니며 각 명령은 응답 하나를 만들어야 한다.

## 제공 목적과 성능 개선 우선순위

coroutine은 비동기 요청 흐름을 순차적인 코드로 작성하고 결과·예외·객체 수명을 다루기 쉽게 제공하는 API다. coroutine 평가는 사용 편의성, 정확성, 불필요한 오버헤드의 회귀 방지에 둔다.

성능 개선은 다음 세 경로에 집중한다.

1. 비동기 `post_execute_query`: 명령 인코딩·할당, 세션 선택, 큐/락, 완료 callback 비용을 줄인다.
2. 동기 `execute_query`: 응답 대기·깨움, 동기화, query 준비·재사용 비용을 줄여 단건 왕복시간을 개선한다.
3. 동기·비동기 batch: 명령별 준비·할당을 줄이고 연속 버퍼 인코딩, 큐 등록, 송신 및 응답 수집 비용을 개선한다.

개선 여부는 같은 조건의 변경 전후 처리량, p50/p99 응답시간, 명령당 CPU 시간으로 판단한다. batch는 전체 완료 지연시간도 함께 기록한다. 아래 coroutine 측정은 API 선택 비용을 이해하기 위한 참고 자료로 보존한다.

## 단건·coroutine 측정 (2026-10-25)

[183회 전체 지표](results/coroutine_request_20261025.md), [원시 로그](results/coroutine_request_20261025.txt).

- SAKPUNG, AMD Ryzen 9 8945HX, Linux 6.12, GCC 15.2.1, Redis 8.2.10, hiredis 1.3.0, redis-plus-plus 1.3.15, Boost 1.90.
- SDK r11622의 object/socket/query/redis를 새로 Release 빌드했다. 실제 백엔드는 epoll. 클라이언트 CPU 0-7,16-23, Redis CPU 8, 준비 0.5초+측정 2초, 조건별 3회 교차 실행.
- caller마다 미완료 요청은 하나다. caller가 여러 개면 동시 요청이며 배치가 아니다. GET/SET 16B, GET 16KB, caller 1/4/16, 64 caller·4연결 다중화와 같은 L3 CPU 배치를 포함해 183회 모두 errors=0.
- 측정 후 공개 이름을 통합했다. 새 API로 라이브러리/벤치마크 재빌드, 실 Redis 기능 검증, 단건 비교 10회를 추가 확인했다. 이름 통합은 실행 경로를 바꾸지 않는다.

GET 16B, caller 1·연결 1, 각 지표의 3회 중앙값:

| 호출 방식 | 명령/초 | p50 µs | p99 µs | CPU µs/명령 |
|---|---:|---:|---:|---:|
| hiredis | 54,813 | 18.0 | 20.4 | 5.698 |
| redis-plus-plus | 53,797 | 18.6 | 20.6 | 5.948 |
| Boost.Redis | 46,771 | 21.4 | 23.7 | 9.271 |
| execute_query 동기 | 44,724 | 22.1 | 25.0 | 9.833 |
| post_execute_query 콜백 | 50,989 | 19.6 | 21.6 | 6.963 |
| coroutine 기본 executor | 41,865 | 23.7 | 27.0 | 12.697 |
| coroutine 별도 executor | 42,586 | 23.4 | 26.0 | 11.051 |
| coroutine Redis I/O executor | 46,676 | 21.4 | 23.7 | 9.044 |

기본 executor의 coroutine은 자발적 문맥 전환이 명령당 약 3회, 같은 I/O executor는 약 1회였다. 이는 executor 배치에 따른 부가 비용을 보여주는 참고 결과다. coroutine의 제공 목적은 비동기 흐름의 작성 편의성이다.

같은 머신에서 클라이언트를 Redis와 같은 L3 CPU 9-15,25-31로 옮기면 I/O executor coroutine은 61,026/s, p50 16.3µs, p99 18.3µs였다. callback은 69,081/s, hiredis는 74,586/s였다. 원격 Redis에도 같은 개선율이 나온다는 의미는 아니다. 전용 query 재사용의 단건 이득은 작았고, TCP_NODELAY는 이미 기본 활성화다. 추가 성능 검토 대상은 동기·비동기 공통의 명령 인코딩·할당, 세션/큐 동기화, 동기 대기·깨움과 batch 송수신 비용이다. 각 후보의 개선율은 변경 전후 실측으로 확인한다.

## 배치 비교

[현재 비교 지표](../../document/Web/site/data/redis.clients.20261025.md), [표시 대상 120회 로그](../../document/Web/site/data/redis.clients.20261025.txt), [측정 소스 해시](results/clients_compare_20261025.sources.sha256).

hiredis·redis-plus-plus·Boost.Redis·CGDK 비동기 배치·동기 배치를 표시한다. 요청한 대로 이전 CGDK 구현의 비교 수치는 제외했다. 이전 원시 결과는 results/에 감사 기록으로 보존한다.

GET 16B batch 100 연결1: hiredis 2,188,205/s, redis-plus-plus 1,750,701/s, Boost.Redis 2,200,878/s, 비동기 배치 2,261,440/s, 동기 배치 2,230,584/s. batch 1000 연결1은 CGDK 비동기 4,409,884/s, 동기 4,333,383/s였다. GET 256KB는 hiredis 15,225/s, CGDK 비동기 15,160/s로 비슷했다.

배치 측정에서는 Redis 소스와 실행 파일을 재컴파일하고 나머지 SDK 의존 라이브러리는 이전 검증 환경을 재사용했다. 위 단건/coroutine의 SDK 전체 의존 재빌드 측정과 구분한다. 배치 지연시간은 배치 전체 완료 시간이며 명령 개수로 나눈 수치가 아니다.

## 빌드와 재실행

```sh
cmake -S benchmark/3.query_redis -B bld -G Ninja \
  -DCGDK_ROOT=<CGDK> -DBENCH_3RD=<hiredis/redis++ prefix> \
  -DBOOST_INCLUDE=<Boost 1.90>
cmake --build bld --target bench_cgdk bench_hiredis bench_redispp bench_boost_redis check_batch
./bld/check_batch 6390

export REDIS_PID=<redis-server host pid>
python3 benchmark/3.query_redis/compare_clients.py bld \
  --output results/clients_compare.txt --port 6390 --rounds 3 --seconds 2
python3 benchmark/3.query_redis/compare_coroutines.py bld \
  --output results/coroutine.txt --sdk-revision=<revision> --port 6390
```

비교 스크립트는 컨테이너를 생성/종료하지 않고 benchmark key만 채운다. `check_batch`는 격리된 테스트 Redis에서 명시 실행한다. mixed 동기·비동기 24,000건, 입력 수명, 응답 순서·오류, 큰 값, coroutine, stop 부분 결과, 연결 단절과 callback 재진입을 검사한다.

`bench_cgdk --variant`는 shared / dedicated / async / batch / batch_sync / coroutine / coroutine_separate / coroutine_io / coroutine_dedicated_io를 지원한다. coroutine_io와 coroutine_dedicated_io는 Redis I/O executor를 continuation에 사용한다. 전용 query 방식은 sessions >= callers여야 한다. 실제 I/O backend와 worker 수는 CONFIG에 기록한다.

[기술적 설명 → Redis · 배치·성능](http://192.168.0.92:8080/query.redis.html)