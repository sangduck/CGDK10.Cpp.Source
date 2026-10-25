"""Compare freshly built clients against an already running, isolated Redis.

No containers are created/stopped. Only the benchmark key prefix is populated.
CPU and server PID are supplied explicitly for reproducible same-host runs.
"""
import argparse
import collections
import os
import pathlib
import platform
import statistics
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('build', type=pathlib.Path)
parser.add_argument('--output', type=pathlib.Path, required=True)
parser.add_argument('--port', type=int, default=6390)
parser.add_argument('--rounds', type=int, default=3)
parser.add_argument('--seconds', type=float, default=2)
parser.add_argument('--warmup', type=float, default=.5)
parser.add_argument('--cpus', default='0-7,16-23')
parser.add_argument('--keys', type=int, default=1024)
args = parser.parse_args()
args.build = args.build.resolve()
args.output.parent.mkdir(parents=True, exist_ok=True)

# name, value bytes, operation, callers (= connections), commands per batch
cases = [
    ('get16_rtt1', 16, 'get', 1, 1),
    ('get16_batch100', 16, 'get', 1, 100),
    ('get16_batch1000', 16, 'get', 1, 1000),
    ('get16_batch100_4callers', 16, 'get', 4, 100),
    ('get16_batch1000_4callers', 16, 'get', 4, 1000),
    ('set16_batch100', 16, 'set', 1, 100),
    ('get16kb_batch10', 16384, 'get', 1, 10),
    ('get256kb_batch10', 262144, 'get', 1, 10),
]
clients = [
    ('hiredis', 'bench_hiredis', []),
    ('redis++', 'bench_redispp', []),
    ('boost.redis', 'bench_boost_redis', []),
    ('cgdk.shared', 'bench_cgdk', ['--variant=shared']),
    ('cgdk.batch.async', 'bench_cgdk', ['--variant=batch']),
    ('cgdk.batch.sync', 'bench_cgdk', ['--variant=batch_sync']),
]
records = []
with args.output.open('w') as out:
    out.write(f'# {time.strftime("%Y-%m-%dT%H:%M:%S%z")} host={platform.node()} kernel={platform.release()}\n')
    out.write(f'# Release -O3 -DNDEBUG; rounds={args.rounds} seconds={args.seconds} warmup={args.warmup} cpus={args.cpus} keys={args.keys} port={args.port}\n')
    out.write('# ops/s counts commands; pipeline latency is for the complete batch, including request construction; CPU is all client process threads\n')
    for round_index in range(args.rounds):
        for case_index, (name, size, op, callers, count) in enumerate(cases):
            subprocess.run([str(args.build/'bench_hiredis'), f'--port={args.port}', '--populate', f'--keys={args.keys}', f'--size={size}'], stdout=subprocess.DEVNULL, check=True, timeout=90)
            offset = (round_index*2 + case_index) % len(clients)
            for label, binary, extra in clients[offset:] + clients[:offset]:
                mode = 'pipeline' if count > 1 or label.startswith('cgdk.batch') else 'sync'
                options = [f'--port={args.port}', f'--seconds={args.seconds}', f'--warmup={args.warmup}', f'--keys={args.keys}', f'--size={size}', f'--op={op}', f'--callers={callers}', f'--sessions={callers}', f'--pipeline={count}', f'--mode={mode}'] + extra
                if label == 'cgdk.shared' and count > 1: options.append('--defer')
                result = subprocess.run(['taskset', '-c', args.cpus, str(args.build/binary)] + options, capture_output=True, text=True, timeout=args.seconds+35)
                lines = [line for line in result.stdout.splitlines() if line.startswith('RESULT ')]
                if result.returncode or len(lines) != 1:
                    raise RuntimeError((name, label, result.returncode, result.stdout, result.stderr))
                values = dict(field.split('=', 1) for field in lines[0].split()[1:])
                if int(values['errors']) != 0 or int(values['ops']) == 0:
                    raise RuntimeError((name, label, lines[0]))
                values.update(test_case=name, competitor=label, round=str(round_index+1))
                records.append(values)
                line = lines[0] + f' test_case={name} competitor={label} round={round_index+1}'
                print(line, flush=True)
                out.write(line+'\n'); out.flush()

groups = collections.defaultdict(list)
for record in records:
    groups[(record['test_case'], record['competitor'])].append(record)
with args.output.with_suffix('.md').open('w') as out:
    out.write(f'# Redis client comparison\n\n{len(records)} successful runs; {args.rounds} rounds. Each number is the median of its own metric.\n\n')
    out.write('Latency is for one complete batch, not latency divided by the number of commands. CPU us/op includes every client process thread.\n\n')
    for name, size, op, callers, count in cases:
        out.write(f'## {name}\n\n{op.upper()} {size} B, {callers} callers/connections, batch {count}.\n\n')
        out.write('| Client | Commands/s | Batch p50 us | Batch p99 us | Client CPU us/command | Redis cores |\n|---|---:|---:|---:|---:|---:|\n')
        for label, _, _ in clients:
            rows = groups[(name, label)]
            median = lambda field: statistics.median(float(row[field]) for row in rows)
            out.write(f"| {label} | {median('ops_per_s'):,.0f} | {median('p50_us'):.1f} | {median('p99_us'):.1f} | {median('cpu_us_per_op'):.3f} | {median('server_cores'):.2f} |\n")
        out.write('\n')
print(f'COMPLETE {len(records)} successful runs', flush=True)
