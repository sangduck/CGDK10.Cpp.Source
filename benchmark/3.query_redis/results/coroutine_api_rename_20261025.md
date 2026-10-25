# Redis single-request and coroutine comparison

10 successful runs; SDK r11622+execute_query_API, freshly built dependency libraries; 1 rounds.

Each caller has at most one outstanding command. Callers greater than connections denotes multiplexing, not batches. Medians are calculated independently for each metric.

## serial_get16

GET 16 B, 1 callers, 1 connections, CPUs 0-7,16-23.

| Client | Commands/s | p50 us | p99 us | CPU us/command | Voluntary CS/1k commands | Involuntary CS/1k | Redis cores |
|---|---:|---:|---:|---:|---:|---:|---:|
| hiredis | 54,567 | 18.3 | 20.4 | 5.639 | 1000.00 | 0.27 | 0.40 |
| redis++ | 53,620 | 18.6 | 20.4 | 5.953 | 1000.02 | 0.29 | 0.40 |
| boost.redis | 46,409 | 21.4 | 23.7 | 9.259 | 1000.00 | 0.26 | 0.34 |
| cgdk.sync | 44,137 | 22.4 | 26.2 | 9.999 | 2001.17 | 0.52 | 0.32 |
| cgdk.sync.dedicated | 44,758 | 22.1 | 25.5 | 9.924 | 2001.12 | 0.31 | 0.33 |
| cgdk.callback | 50,826 | 19.6 | 22.9 | 6.976 | 999.98 | 0.29 | 0.38 |
| cgdk.coroutine.system | 41,571 | 23.9 | 27.8 | 12.769 | 3003.09 | 0.67 | 0.31 |
| cgdk.coroutine.private | 42,765 | 23.4 | 26.0 | 11.103 | 1999.99 | 0.32 | 0.31 |
| cgdk.coroutine.io | 46,430 | 21.4 | 23.4 | 8.961 | 1000.00 | 0.29 | 0.35 |
| cgdk.coroutine.dedicated.io | 46,735 | 21.4 | 23.4 | 8.810 | 999.97 | 0.30 | 0.35 |

