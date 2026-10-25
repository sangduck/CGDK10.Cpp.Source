# Redis single-request and coroutine comparison

183 successful runs; SDK r11622, freshly built dependency libraries; 3 rounds.

Each caller has at most one outstanding command. Callers greater than connections denotes multiplexing, not batches. Medians are calculated independently for each metric.

## serial_get16

GET 16 B, 1 callers, 1 connections, CPUs 0-7,16-23.

| Client | Commands/s | p50 us | p99 us | CPU us/command | Voluntary CS/1k commands | Involuntary CS/1k | Redis cores |
|---|---:|---:|---:|---:|---:|---:|---:|
| hiredis | 54,813 | 18.0 | 20.4 | 5.698 | 1000.02 | 0.22 | 0.41 |
| redis++ | 53,797 | 18.6 | 20.6 | 5.948 | 1000.02 | 0.27 | 0.39 |
| boost.redis | 46,771 | 21.4 | 23.7 | 9.271 | 1000.02 | 0.28 | 0.34 |
| cgdk.sync | 44,724 | 22.1 | 25.0 | 9.833 | 2001.11 | 0.46 | 0.33 |
| cgdk.sync.dedicated | 44,577 | 22.4 | 25.0 | 9.751 | 2001.33 | 0.38 | 0.33 |
| cgdk.callback | 50,989 | 19.6 | 21.6 | 6.963 | 1000.02 | 0.26 | 0.38 |
| cgdk.coroutine.system | 41,865 | 23.7 | 27.0 | 12.697 | 3002.91 | 0.58 | 0.32 |
| cgdk.coroutine.private | 42,586 | 23.4 | 26.0 | 11.051 | 2000.05 | 0.32 | 0.32 |
| cgdk.coroutine.io | 46,676 | 21.4 | 23.7 | 9.044 | 1000.02 | 0.27 | 0.34 |
| cgdk.coroutine.dedicated.io | 46,774 | 21.4 | 23.9 | 8.843 | 1000.03 | 0.28 | 0.35 |

## serial_set16

SET 16 B, 1 callers, 1 connections, CPUs 0-7,16-23.

| Client | Commands/s | p50 us | p99 us | CPU us/command | Voluntary CS/1k commands | Involuntary CS/1k | Redis cores |
|---|---:|---:|---:|---:|---:|---:|---:|
| hiredis | 53,455 | 18.6 | 20.9 | 5.709 | 1000.02 | 0.27 | 0.41 |
| redis++ | 53,277 | 18.6 | 20.6 | 5.980 | 1000.02 | 0.24 | 0.40 |
| boost.redis | 45,920 | 21.6 | 23.7 | 9.395 | 1000.00 | 0.28 | 0.35 |
| cgdk.sync | 43,854 | 22.7 | 25.5 | 9.913 | 2001.27 | 0.31 | 0.33 |
| cgdk.sync.dedicated | 44,024 | 22.7 | 25.5 | 9.784 | 2001.40 | 0.39 | 0.33 |
| cgdk.callback | 50,834 | 19.6 | 21.9 | 6.966 | 1000.00 | 0.27 | 0.39 |
| cgdk.coroutine.system | 41,813 | 23.7 | 27.0 | 12.687 | 3002.50 | 0.55 | 0.32 |
| cgdk.coroutine.private | 42,394 | 23.4 | 27.0 | 11.115 | 2000.00 | 0.52 | 0.32 |
| cgdk.coroutine.io | 45,480 | 21.9 | 24.2 | 9.106 | 1000.02 | 0.27 | 0.35 |
| cgdk.coroutine.dedicated.io | 46,483 | 21.4 | 23.9 | 8.829 | 1000.02 | 0.26 | 0.36 |

## concurrent4_get16

GET 16 B, 4 callers, 4 connections, CPUs 0-7,16-23.

| Client | Commands/s | p50 us | p99 us | CPU us/command | Voluntary CS/1k commands | Involuntary CS/1k | Redis cores |
|---|---:|---:|---:|---:|---:|---:|---:|
| hiredis | 122,817 | 30.6 | 41.7 | 6.052 | 1000.00 | 0.22 | 0.63 |
| redis++ | 121,480 | 31.1 | 41.7 | 6.349 | 1000.00 | 0.25 | 0.63 |
| boost.redis | 120,015 | 33.0 | 41.7 | 9.875 | 999.94 | 0.62 | 0.62 |
| cgdk.sync | 117,851 | 34.6 | 42.8 | 10.616 | 2000.57 | 0.56 | 0.61 |
| cgdk.sync.dedicated | 116,148 | 35.1 | 43.8 | 10.411 | 2000.61 | 0.68 | 0.61 |
| cgdk.callback | 191,405 | 20.9 | 23.4 | 2.020 | 250.02 | 0.07 | 0.38 |
| cgdk.coroutine.system | 123,672 | 31.9 | 46.8 | 13.449 | 2950.61 | 1.42 | 0.64 |
| cgdk.coroutine.private | 124,177 | 31.6 | 46.8 | 13.436 | 2917.41 | 0.99 | 0.64 |
| cgdk.coroutine.io | 129,396 | 30.6 | 51.5 | 8.901 | 1405.90 | 0.53 | 0.62 |
| cgdk.coroutine.dedicated.io | 117,478 | 32.6 | 43.8 | 7.449 | 1000.41 | 0.51 | 0.59 |

## concurrent16_get16

GET 16 B, 16 callers, 16 connections, CPUs 0-7,16-23.

| Client | Commands/s | p50 us | p99 us | CPU us/command | Voluntary CS/1k commands | Involuntary CS/1k | Redis cores |
|---|---:|---:|---:|---:|---:|---:|---:|
| hiredis | 128,830 | 120.3 | 230.4 | 5.764 | 1000.02 | 0.59 | 0.60 |
| redis++ | 129,232 | 116.2 | 228.4 | 6.053 | 1000.03 | 0.68 | 0.60 |
| boost.redis | 125,768 | 123.4 | 224.3 | 9.716 | 1000.02 | 1.14 | 0.60 |
| cgdk.sync | 121,813 | 127.5 | 234.5 | 10.370 | 2000.70 | 1.18 | 0.57 |
| cgdk.sync.dedicated | 123,821 | 125.4 | 230.4 | 10.295 | 2000.72 | 1.00 | 0.57 |
| cgdk.callback | 602,351 | 26.2 | 29.8 | 0.712 | 62.53 | 0.02 | 0.38 |
| cgdk.coroutine.system | 221,975 | 70.1 | 132.1 | 10.902 | 2028.08 | 0.86 | 0.59 |
| cgdk.coroutine.private | 216,489 | 71.2 | 136.2 | 11.052 | 2047.87 | 1.58 | 0.59 |
| cgdk.coroutine.io | 213,530 | 72.2 | 138.2 | 9.909 | 1615.24 | 0.64 | 0.58 |
| cgdk.coroutine.dedicated.io | 128,801 | 116.2 | 228.4 | 7.407 | 1000.84 | 0.55 | 0.57 |

## multiplex64_sessions4_get16

GET 16 B, 64 callers, 4 connections, CPUs 0-7,16-23.

| Client | Commands/s | p50 us | p99 us | CPU us/command | Voluntary CS/1k commands | Involuntary CS/1k | Redis cores |
|---|---:|---:|---:|---:|---:|---:|---:|
| boost.redis | 1,433,222 | 45.3 | 54.0 | 1.044 | 62.50 | 0.05 | 0.69 |
| cgdk.sync | 189,799 | 391.2 | 1105.9 | 14.797 | 3401.77 | 7.38 | 0.61 |
| cgdk.callback | 1,401,385 | 45.3 | 49.4 | 0.356 | 15.63 | 0.00 | 0.38 |
| cgdk.coroutine.system | 596,148 | 107.0 | 183.3 | 16.605 | 1240.53 | 105.00 | 0.66 |
| cgdk.coroutine.private | 593,326 | 109.1 | 169.0 | 5.216 | 282.46 | 0.34 | 0.68 |
| cgdk.coroutine.io | 580,435 | 112.1 | 171.0 | 4.600 | 107.11 | 0.28 | 0.69 |

## serial_get16kb

GET 16384 B, 1 callers, 1 connections, CPUs 0-7,16-23.

| Client | Commands/s | p50 us | p99 us | CPU us/command | Voluntary CS/1k commands | Involuntary CS/1k | Redis cores |
|---|---:|---:|---:|---:|---:|---:|---:|
| hiredis | 38,420 | 26.0 | 29.1 | 7.752 | 1000.01 | 0.26 | 0.49 |
| redis++ | 37,693 | 26.5 | 29.6 | 8.297 | 1000.04 | 0.25 | 0.48 |
| boost.redis | 29,360 | 34.0 | 37.6 | 16.343 | 1000.00 | 0.12 | 0.37 |
| cgdk.sync | 33,982 | 29.3 | 33.0 | 11.551 | 2001.71 | 0.35 | 0.43 |
| cgdk.sync.dedicated | 34,513 | 28.8 | 32.6 | 11.406 | 2001.43 | 0.52 | 0.43 |
| cgdk.callback | 38,214 | 26.0 | 29.3 | 8.432 | 1000.03 | 0.22 | 0.48 |
| cgdk.coroutine.system | 32,850 | 30.1 | 34.6 | 14.238 | 3002.96 | 0.71 | 0.41 |
| cgdk.coroutine.private | 33,527 | 29.6 | 33.5 | 12.728 | 2000.04 | 0.27 | 0.41 |
| cgdk.coroutine.io | 34,865 | 28.5 | 32.1 | 10.712 | 1000.01 | 0.22 | 0.44 |
| cgdk.coroutine.dedicated.io | 34,794 | 28.5 | 32.1 | 10.605 | 1000.03 | 0.23 | 0.44 |

## same_l3_serial_get16

GET 16 B, 1 callers, 1 connections, CPUs 9-15,25-31.

| Client | Commands/s | p50 us | p99 us | CPU us/command | Voluntary CS/1k commands | Involuntary CS/1k | Redis cores |
|---|---:|---:|---:|---:|---:|---:|---:|
| hiredis | 74,586 | 13.4 | 15.4 | 4.581 | 999.89 | 0.18 | 0.45 |
| cgdk.sync | 57,866 | 17.0 | 19.6 | 8.623 | 2000.87 | 0.35 | 0.35 |
| cgdk.callback | 69,081 | 14.4 | 16.5 | 5.682 | 999.91 | 0.18 | 0.42 |
| cgdk.coroutine.io | 61,026 | 16.3 | 18.3 | 7.688 | 999.92 | 0.20 | 0.37 |
| cgdk.coroutine.dedicated.io | 61,261 | 16.2 | 18.3 | 7.649 | 999.93 | 0.17 | 0.38 |

