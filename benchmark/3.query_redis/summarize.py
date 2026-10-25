#!/usr/bin/env python3
#-----------------------------------------------------------------------------
# benchmark - query redis : medians of the rounds in a raw file (run_bench.sh)
#
#  python3 summarize.py results/raw_xxx.txt > results/summary_xxx.md
#-----------------------------------------------------------------------------
import sys, statistics
from collections import OrderedDict

KEY = ("build", "client", "variant", "mode", "op", "size", "callers", "pipeline")
VAL = ("ops_per_s", "p50_us", "p99_us", "cpu_us_per_op", "server_cores", "errors")

rows = OrderedDict()
failed = []
for line in open(sys.argv[1], encoding="utf-8"):
    if not line.startswith("RESULT"):
        continue
    if "FAILED" in line:
        failed.append(line.strip())
        continue
    kv = dict(p.split("=", 1) for p in line.split()[1:] if "=" in p)
    rows.setdefault(tuple(kv[k] for k in KEY), []).append(kv)

def med(lst, k):
    return statistics.median(float(x[k]) for x in lst)

print("| build | client | variant | mode | op | size | callers | pipeline | ops/s | p50 us | p99 us | client cpu us/op | redis cores | errors | n |")
print("|---|---|---|---|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|")
for key, lst in rows.items():
    v = {k: med(lst, k) for k in VAL}
    print("| " + " | ".join(key) + f" | {v['ops_per_s']:,.0f} | {v['p50_us']:.1f} | {v['p99_us']:.1f} | {v['cpu_us_per_op']:.2f} | {v['server_cores']:.2f} | {int(sum(float(x['errors']) for x in lst))} | {len(lst)} |")
for f in failed:
    print("\nFAILED: " + f)
