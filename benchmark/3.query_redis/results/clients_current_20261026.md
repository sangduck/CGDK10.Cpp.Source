# Redis 이전 post=true 클라이언트 비교 (2026-10-26)

이 문서는 r11623의 post=true 기본값으로 측정한 기록이다. 현재 기본값은 false이며 [최신 false 비교](clients_post_off_20261026.md)를 기준으로 한다. 동기 execute_query 완료는 직접 처리한다. Redis 자동 send_cork 및 전용 옵션은 제거했고 send_defer는 독립적으로 설정한다. 파싱과 응답 매칭은 I/O에서 수행한다.

SAKPUNG Ryzen 9 8945HX, Linux 6.12, GCC 15.2.1 Release -O3 -DNDEBUG, epoll, Redis 8.2.10(io-threads=1, CPU 8), 클라이언트 CPU 0-7,16-23. hiredis 1.3.0(poll 비동기 어댑터), redis-plus-plus 1.3.15, Boost 1.90. GET 16B, 1024 keys, 준비 0.3초+측정 2초, 실행 순서 교차, 각 3회 중앙값. 60회 모두 errors=0. loopback 결과이며 원격 Redis나 다른 값 크기의 성능을 보장하지 않는다.

각 pipeline 지연시간은 명령 하나가 아니라 1000개 그룹 전체 완료 시간이다. 개별 비동기는 호출자 1024개가 각각 응답 후 다음 요청을 제출하며 연결 4개를 공유한다. redis++ 비동기 경로는 이번 하네스에서 비교하지 않는다.

| 조건 | 구현 | 요청/초 | p50 µs | p99 µs | 클라이언트 CPU µs/명령 |
|---|---|---:|---:|---:|---:|
| get16_single | hiredis.sync | 54,425 | 18.3 | 20.4 | 5.729 |
| get16_single | hiredis.async | 50,970 | 19.6 | 21.6 | 7.024 |
| get16_single | redis++.sync | 54,054 | 18.3 | 20.6 | 5.917 |
| get16_single | Boost.Redis | 46,958 | 21.1 | 23.7 | 9.257 |
| get16_single | CGDK.sync | 38,344 | 25.7 | 29.1 | 16.110 |
| get16_single | CGDK.async | 46,095 | 21.6 | 23.9 | 9.147 |
| get16_async1024_c4 | hiredis.async | 5,177,391 | 197.6 | 212.0 | 0.215 |
| get16_async1024_c4 | Boost.Redis | 4,172,839 | 244.7 | 255.0 | 0.500 |
| get16_async1024_c4 | CGDK.async | 5,077,493 | 197.6 | 288.8 | 0.445 |
| get16_batch1000_c1 | hiredis.pipeline | 3,735,873 | 268.3 | 276.5 | 0.120 |
| get16_batch1000_c1 | redis++.pipeline | 2,649,929 | 366.6 | 378.9 | 0.240 |
| get16_batch1000_c1 | Boost.Redis | 4,052,886 | 244.7 | 255.0 | 0.105 |
| get16_batch1000_c1 | CGDK.batch.async | 4,468,357 | 222.2 | 230.4 | 0.068 |
| get16_batch1000_c1 | CGDK.batch.sync | 4,445,349 | 224.3 | 234.5 | 0.068 |
| get16_batch1000_c4 | hiredis.pipeline | 5,700,374 | 700.4 | 725.0 | 0.129 |
| get16_batch1000_c4 | redis++.pipeline | 5,229,360 | 774.1 | 921.6 | 0.258 |
| get16_batch1000_c4 | Boost.Redis | 5,704,849 | 700.4 | 725.0 | 0.116 |
| get16_batch1000_c4 | CGDK.batch.async | 5,697,843 | 700.4 | 733.2 | 0.073 |
| get16_batch1000_c4 | CGDK.batch.sync | 5,717,846 | 700.4 | 733.2 | 0.077 |
| get16_batch1000_c4 | CGDK.individual | 4,103,395 | 962.6 | 1253.4 | 1.729 |

CGDK/hiredis 처리량: 단건 동기 -29.6%, 비동기 1024개·4연결 -1.9%, batch1000·1연결 +19.6%, batch1000·4연결 -0.04%. 4연결 batch의 CGDK CPU/명령은 hiredis보다 약 43% 작았다. 단건 동기 및 개별 비동기의 p99는 여전히 개선 대상이다.

[60회 원본](clients_current_20261026.jsonl), [중앙값과 범위](clients_current_20261026.summary.json), [조건](clients_current_20261026.metadata.json), [소스 해시](clients_current_20261026.sources.sha256).

재실행: `python3 compare_clients.py <Release-build> --profile=current --output=<outside-worktree>/results.jsonl --rounds=3 --seconds=2 --warmup=.3` (Redis port 6390).

## 응답 콜백 post on/off

각 조건 3회, 42회 모두 errors=0. 명시적으로 --post-response=off/on, 기본 Redis I/O executor를 사용했다. 이 측정 후 API 기본값만 post=true로 변경했으며 sync/coroutine-only 완료 통지는 기존처럼 inline이다.

| 조건 | off 요청/초 | on 요청/초 | 변화 | 평균 완료시간 off→on µs | p99 off→on µs |
|---|---:|---:|---:|---:|---:|
| post1 | 1,612,953 | 1,422,981 | -11.78% | 197.7→276.7 | 342.0→448.5 |
| post4 | 3,818,891 | 3,908,884 | +2.36% | 478.2→497.2 | 1237.0→856.1 |
| callback4 | 5,174,712 | 5,094,425 | -1.55% | 197.9→201.0 | 259.1→288.8 |
| batch4 | 5,721,799 | 5,713,843 | -0.14% | 699.1→699.9 | 725.0→733.2 |
| sync1 | 37,775 | 38,203 | +1.13% | 26.4→26.2 | 29.3→29.1 |
| coroutine1 | 46,364 | 46,278 | -0.19% | 21.5→21.6 | 23.9→23.4 |
| heavy4 | 588,080 | 581,751 | -1.08% | 108.8→110.0 | 116.2→118.3 |

post1/post4는 caller 1/4·연결 1/4, 1000개 개별 등록 후 대기하며 응답시간은 등록부터 개별 callback까지다. callback4는 1024 caller·4연결, batch4는 1000개·4연결이며 표의 지연은 그룹 완료다. heavy4는 64 caller·4연결, callback당 5µs 모의 작업이다. 서로 다른 행의 지연시간을 동일한 의미로 비교하지 않는다.

post는 모든 부하에서 빠르지 않다. 1연결 개별 등록 처리량은 약 11.8% 줄었으며 4연결에서는 약 2.4% 늘었다. 기본값은 사용자 지정에 따라 비동기 post 켜짐이며 성능상 필요하면 START_PARAMETER.post_response=false로 끈다.

[post 42회 로그](response_post_20261026.txt), [post 요약](response_post_20261026.summary.json).

## 정확성 검증

Windows Release 및 Linux Release Redis 테스트 각각 50개 통과. 응답 실행기를 막아도 동기 단건/배치가 즉시 완료되는 회귀 테스트 포함. 실제 Redis 20000건 순서 검증, callback 내 동기 요청/stop, stop/restart 후 접근 거부, 혼합 4800건, 1/4연결 callback affinity 각32000건 및 동시 요청 각12000건에서 오류0. Windows 전체 Net/System과 Linux Net/Redis를 격리 출력으로 재빌드했다. 전체 SDK/GRUMA 빌드, Windows 성능 및 원격 네트워크 성능은 이번 검증 범위가 아니다.

소스 해시는 CRLF를 LF로 정규화한 SHA256이다. Git checkout의 줄바꿈 차이를 제외하고 검증한다.
