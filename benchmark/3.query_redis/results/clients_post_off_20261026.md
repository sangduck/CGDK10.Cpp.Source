# Redis post_response=false 기본값과 클라이언트 비교 (2026-10-26)

현재 기본값은 `START_PARAMETER.post_response=false`다. `post_execute_query` 사용자 callback은 I/O에서 직접 처리하며, `true`를 명시하면 post한다. 동기 `execute_query` 완료는 직접 처리한다. 파싱·응답 매칭은 I/O에 남고 Redis 자동 send_cork는 제거된 상태다.

## 측정 조건

SAKPUNG Ryzen 9 8945HX, Linux 6.12, GCC15.2.1 Release(-O3 -DNDEBUG), epoll, Redis8.2.10(io-threads=1, CPU8), 클라이언트 CPU0-7,16-23. hiredis1.3.0(poll async 어댑터), redis-plus-plus1.3.15, Boost1.90. GET16B, 1024 keys, 준비0.3초+측정2초, 라이브러리 실행 순서 교차, 각3회 중앙값. 60회 모두 errors=0. CGDK24회 모두 명시적으로 --post-response=off, 로그의 post_response=0을 확인했다. 옵션 없는 실행에서도 기본값0을 별도로 확인했다.

평균 열은 각 실행에서 계산한 실제 평균 완료시간의 3회 중앙값이며 p50과 다르다. 단건/개별 비동기는 명령별 완료시간, pipeline은 1000개 그룹 전체 완료시간이다. 비동기1024는 각 caller가 응답 후 다음 요청을 제출하며 연결4개를 공유한다. redis++ 비동기는 이 하네스에서 측정하지 않는다.

| 조건 | 구현 | 요청/초 | 평균 µs | p50 µs | p99 µs | CPU µs/명령 |
|---|---|---:|---:|---:|---:|---:|
| get16_single | hiredis.sync | 55,010 | 18.2 | 18.0 | 19.8 | 5.694 |
| get16_single | hiredis.async | 51,280 | 19.5 | 19.6 | 21.4 | 6.985 |
| get16_single | redis++.sync | 53,615 | 18.6 | 18.6 | 20.6 | 5.924 |
| get16_single | Boost.Redis | 46,681 | 21.4 | 21.4 | 23.4 | 9.299 |
| get16_single | CGDK.sync | 38,281 | 26.1 | 26.0 | 29.1 | 15.971 |
| get16_single | CGDK.async | 50,950 | 19.6 | 19.6 | 21.4 | 7.050 |
| get16_async1024_c4 | hiredis.async | 5,184,434 | 197.5 | 197.6 | 209.9 | 0.207 |
| get16_async1024_c4 | Boost.Redis | 4,183,795 | 244.7 | 244.7 | 255.0 | 0.472 |
| get16_async1024_c4 | CGDK.async | 5,225,703 | 195.9 | 193.5 | 250.9 | 0.354 |
| get16_batch1000_c1 | hiredis.pipeline | 3,737,381 | 267.6 | 264.2 | 276.5 | 0.122 |
| get16_batch1000_c1 | redis++.pipeline | 2,679,409 | 363.6 | 362.5 | 374.8 | 0.237 |
| get16_batch1000_c1 | Boost.Redis | 4,062,863 | 246.1 | 244.7 | 252.9 | 0.106 |
| get16_batch1000_c1 | CGDK.batch.async | 4,559,869 | 219.3 | 218.1 | 226.3 | 0.066 |
| get16_batch1000_c1 | CGDK.batch.sync | 4,482,390 | 223.1 | 222.2 | 232.4 | 0.068 |
| get16_batch1000_c4 | hiredis.pipeline | 5,748,346 | 695.8 | 692.2 | 716.8 | 0.130 |
| get16_batch1000_c4 | redis++.pipeline | 5,213,396 | 755.6 | 766.0 | 913.4 | 0.258 |
| get16_batch1000_c4 | Boost.Redis | 5,769,838 | 693.3 | 692.2 | 725.0 | 0.115 |
| get16_batch1000_c4 | CGDK.batch.async | 5,747,844 | 695.8 | 692.2 | 725.0 | 0.072 |
| get16_batch1000_c4 | CGDK.batch.sync | 5,771,845 | 693.0 | 692.2 | 716.8 | 0.077 |
| get16_batch1000_c4 | CGDK.individual | 4,183,910 | 956.1 | 905.2 | 1581.1 | 1.661 |

CGDK 단건 비동기50,950/s vs hiredis async51,280/s, 평균19.6 vs19.5µs/p99 모두21.4µs. 비동기1024·4연결은5,225,703 vs5,184,434/s로 처리량은 비슷하지만 p99는250.9 vs209.9µs로 CGDK가 길다. 단건 동기38,281 vs55,010/s는 여전히 느리다. batch1000·1연결4,559,869 vs3,737,381/s(약22% 높음), 4연결5,747,844 vs5,748,346/s(비슷함).

loopback/GET16B 결과이며 다른 값 크기·원격 네트워크의 성능을 보장하지 않는다. [이전 post=true 기록](clients_current_20261026.md)은 측정 시점이 다르므로 두 자료의 차이를 옵션 하나의 순수 효과로 해석하지 않는다. 통제된 on/off 비교는 [기존42회](response_post_20261026.txt)를 참고한다.

## 재실행과 원본

기본 비교기: `python3 compare_clients.py <Release-build> --profile=current --post-response=off --output=<outside-worktree>/results.jsonl --rounds=3 --seconds=2 --warmup=.3`

이번 실행에 사용한 정확한 스크립트: `python3 results/clients_post_off_20261026.runner.py <Release-build> --profile=current --output=<outside-worktree>/results.jsonl --rounds=3 --seconds=2 --warmup=.3`

[60회 원본](clients_post_off_20261026.jsonl), [전체 stdout/설정 확인](clients_post_off_20261026.stdout.log), [요약·범위](clients_post_off_20261026.summary.json), [환경](clients_post_off_20261026.metadata.json), [실행 스크립트](clients_post_off_20261026.runner.py), [소스 해시](clients_post_off_20261026.sources.sha256). 소스 해시는 CRLF를 LF로 정규화한다. 생성 svn_version.h는 제외한다.

## 검증과 기본값 적용

Windows/Linux Release Redis 테스트 각각50개 통과. 옵션 true일 때에도 동기 단건/배치가 inline 완료되는 회귀 테스트는 유지하고 기본값 assertion을 false로 바꿨다. 실제 Redis 순서20000건, 혼합4800건, 1/4연결 affinity 각32000건 및 동시 요청 각12000건 오류0, callback 내 동기 요청/stop, stop/restart 통과. 전체 SDK/GRUMA 및 Windows 성능 검증은 아니다.

이번 변경은 구조체 레이아웃을 바꾸지 않는다. 기존 이용 프로그램이 START_PARAMETER 초기값true로 빌드되어 있으면 재빌드 전까지 명시적으로true를 전달할 수 있으므로 Redis 라이브러리와 이용측을 재빌드해 새 기본값을 적용한다.
