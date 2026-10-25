# Redis client comparison

144 successful runs; 3 rounds. Each number is the median of its own metric.

Latency is for one complete batch, not latency divided by the number of commands. CPU us/op includes every client process thread.

## get16_rtt1

GET 16 B, 1 callers/connections, batch 1.

| Client | Commands/s | Batch p50 us | Batch p99 us | Client CPU us/command | Redis cores |
|---|---:|---:|---:|---:|---:|
| hiredis | 53,864 | 18.6 | 20.1 | 5.720 | 0.40 |
| redis++ | 53,733 | 18.6 | 20.6 | 5.929 | 0.40 |
| boost.redis | 46,611 | 21.4 | 23.4 | 9.248 | 0.35 |
| cgdk.shared | 44,614 | 22.1 | 24.7 | 9.829 | 0.33 |
| cgdk.batch.async | 44,252 | 22.4 | 25.2 | 12.096 | 0.33 |
| cgdk.batch.sync | 44,314 | 22.4 | 25.5 | 9.929 | 0.33 |

## get16_batch100

GET 16 B, 1 callers/connections, batch 100.

| Client | Commands/s | Batch p50 us | Batch p99 us | Client CPU us/command | Redis cores |
|---|---:|---:|---:|---:|---:|
| hiredis | 2,188,205 | 45.3 | 49.4 | 0.172 | 0.50 |
| redis++ | 1,750,701 | 56.1 | 59.6 | 0.292 | 0.40 |
| boost.redis | 2,200,878 | 45.3 | 48.9 | 0.174 | 0.50 |
| cgdk.shared | 944,519 | 105.0 | 110.1 | 1.332 | 0.20 |
| cgdk.batch.async | 2,261,440 | 43.8 | 47.9 | 0.184 | 0.51 |
| cgdk.batch.sync | 2,230,584 | 44.8 | 48.9 | 0.166 | 0.50 |

## get16_batch1000

GET 16 B, 1 callers/connections, batch 1000.

| Client | Commands/s | Batch p50 us | Batch p99 us | Client CPU us/command | Redis cores |
|---|---:|---:|---:|---:|---:|
| hiredis | 3,682,378 | 272.4 | 284.7 | 0.121 | 0.61 |
| redis++ | 2,642,910 | 366.6 | 383.0 | 0.240 | 0.44 |
| boost.redis | 4,060,426 | 244.7 | 252.9 | 0.104 | 0.69 |
| cgdk.shared | 1,243,976 | 806.9 | 823.3 | 1.201 | 0.20 |
| cgdk.batch.async | 4,409,884 | 226.3 | 238.6 | 0.075 | 0.74 |
| cgdk.batch.sync | 4,333,383 | 230.4 | 242.7 | 0.074 | 0.73 |

## get16_batch100_4callers

GET 16 B, 4 callers/connections, batch 100.

| Client | Commands/s | Batch p50 us | Batch p99 us | Client CPU us/command | Redis cores |
|---|---:|---:|---:|---:|---:|
| hiredis | 4,131,440 | 97.8 | 107.0 | 0.173 | 0.85 |
| redis++ | 4,050,917 | 96.8 | 108.0 | 0.297 | 0.85 |
| boost.redis | 4,085,710 | 99.8 | 108.0 | 0.179 | 0.85 |
| cgdk.shared | 2,981,986 | 126.5 | 195.6 | 1.691 | 0.66 |
| cgdk.batch.async | 4,086,690 | 98.8 | 109.1 | 0.199 | 0.85 |
| cgdk.batch.sync | 4,116,681 | 97.8 | 108.0 | 0.168 | 0.85 |

## get16_batch1000_4callers

GET 16 B, 4 callers/connections, batch 1000.

| Client | Commands/s | Batch p50 us | Batch p99 us | Client CPU us/command | Redis cores |
|---|---:|---:|---:|---:|---:|
| hiredis | 5,698,952 | 700.4 | 725.0 | 0.128 | 0.95 |
| redis++ | 5,214,852 | 774.1 | 921.6 | 0.257 | 0.86 |
| boost.redis | 5,684,841 | 700.4 | 733.2 | 0.114 | 0.94 |
| cgdk.shared | 3,660,878 | 1019.9 | 1515.5 | 1.598 | 0.62 |
| cgdk.batch.async | 5,625,885 | 708.6 | 733.2 | 0.147 | 0.94 |
| cgdk.batch.sync | 5,649,893 | 708.6 | 741.4 | 0.149 | 0.95 |

## set16_batch100

SET 16 B, 1 callers/connections, batch 100.

| Client | Commands/s | Batch p50 us | Batch p99 us | Client CPU us/command | Redis cores |
|---|---:|---:|---:|---:|---:|
| hiredis | 1,786,914 | 55.6 | 59.6 | 0.193 | 0.54 |
| redis++ | 1,486,099 | 65.3 | 70.1 | 0.317 | 0.45 |
| boost.redis | 1,856,148 | 53.5 | 57.6 | 0.176 | 0.57 |
| cgdk.shared | 859,394 | 116.2 | 121.3 | 1.351 | 0.25 |
| cgdk.batch.async | 1,826,001 | 54.5 | 59.6 | 0.211 | 0.55 |
| cgdk.batch.sync | 1,804,851 | 55.0 | 60.2 | 0.182 | 0.55 |

## get16kb_batch10

GET 16384 B, 1 callers/connections, batch 10.

| Client | Commands/s | Batch p50 us | Batch p99 us | Client CPU us/command | Redis cores |
|---|---:|---:|---:|---:|---:|
| hiredis | 147,805 | 67.1 | 83.5 | 3.410 | 0.70 |
| redis++ | 142,963 | 69.1 | 77.3 | 3.734 | 0.67 |
| boost.redis | 80,272 | 123.4 | 136.2 | 9.095 | 0.36 |
| cgdk.shared | 138,786 | 71.2 | 81.4 | 5.757 | 0.65 |
| cgdk.batch.async | 141,540 | 69.1 | 79.4 | 3.682 | 0.67 |
| cgdk.batch.sync | 141,220 | 70.1 | 78.3 | 3.386 | 0.66 |

## get256kb_batch10

GET 262144 B, 1 callers/connections, batch 10.

| Client | Commands/s | Batch p50 us | Batch p99 us | Client CPU us/command | Redis cores |
|---|---:|---:|---:|---:|---:|
| hiredis | 15,225 | 643.1 | 921.6 | 41.366 | 0.72 |
| redis++ | 7,930 | 1155.1 | 1564.7 | 109.310 | 0.37 |
| boost.redis | 4,860 | 2072.6 | 2179.1 | 183.887 | 0.26 |
| cgdk.shared | 15,145 | 659.5 | 733.2 | 36.835 | 0.73 |
| cgdk.batch.async | 15,160 | 659.5 | 733.2 | 32.715 | 0.72 |
| cgdk.batch.sync | 15,090 | 659.5 | 733.2 | 36.531 | 0.72 |

