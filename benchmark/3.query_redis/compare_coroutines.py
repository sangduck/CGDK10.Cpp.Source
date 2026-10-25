"""One outstanding request per caller: sync, callbacks, native co_await.

The SDK must be rebuilt for the revision recorded in --sdk-revision. Redis is
already running; this script never changes its configuration or restarts it.
"""
import argparse
import collections
import pathlib
import os
import platform
import statistics
import subprocess
import time

p = argparse.ArgumentParser()
p.add_argument('build', type=pathlib.Path)
p.add_argument('--output', type=pathlib.Path, required=True)
p.add_argument('--sdk-revision', default='unspecified')
p.add_argument('--port', type=int, default=6390)
p.add_argument('--rounds', type=int, default=3)
p.add_argument('--seconds', type=float, default=2)
p.add_argument('--warmup', type=float, default=.5)
p.add_argument('--cpus', default='0-7,16-23')
p.add_argument('--near-cpus', default='9-15,25-31')
p.add_argument('--keys', type=int, default=1024)
p.add_argument('--case', action='append')
a = p.parse_args()
a.build = a.build.resolve()
if a.rounds < 1 or a.seconds <= 0 or a.warmup < 0:
    p.error('rounds/seconds must be positive and warmup nonnegative')
env = os.environ.copy()
env['CGDK_IO_URING'] = 'epoll'

clients = [
    ('hiredis', 'bench_hiredis', []),
    ('redis++', 'bench_redispp', []),
    ('boost.redis', 'bench_boost_redis', []),
    ('cgdk.sync', 'bench_cgdk', ['--variant=shared']),
    ('cgdk.sync.dedicated', 'bench_cgdk', ['--variant=dedicated']),
    ('cgdk.callback', 'bench_cgdk', ['--variant=async']),
    ('cgdk.coroutine.blocking', 'bench_cgdk', ['--variant=coroutine_blocking']),
    ('cgdk.coroutine.system', 'bench_cgdk', ['--variant=coroutine']),
    ('cgdk.coroutine.private', 'bench_cgdk', ['--variant=coroutine_separate']),
    ('cgdk.coroutine.io', 'bench_cgdk', ['--variant=coroutine_io']),
    ('cgdk.coroutine.dedicated.io', 'bench_cgdk', ['--variant=coroutine_dedicated_io']),
]
# case, operation, value bytes, caller chains/threads, connections, CPU set
cases = [
    ('serial_get16', 'get', 16, 1, 1, a.cpus),
    ('serial_set16', 'set', 16, 1, 1, a.cpus),
    ('concurrent4_get16', 'get', 16, 4, 4, a.cpus),
    ('concurrent16_get16', 'get', 16, 16, 16, a.cpus),
    ('multiplex64_sessions4_get16', 'get', 16, 64, 4, a.cpus),
    ('serial_get16kb', 'get', 16384, 1, 1, a.cpus),
    ('same_l3_serial_get16', 'get', 16, 1, 1, a.near_cpus),
]
if a.case:
    if set(a.case) - {case[0] for case in cases}: p.error('unknown case')
    cases = [case for case in cases if case[0] in a.case]

def eligible(case):
    name, _, _, callers, sessions, _ = case
    choices = clients
    if callers > sessions:
        # Hiredis has one connection per thread; dedicated calls require one
        # connection per caller, so they cannot satisfy this multiplex workload.
        choices = [c for c in choices if c[0] not in ('hiredis', 'redis++', 'cgdk.sync.dedicated', 'cgdk.coroutine.dedicated.io')]
    if name.startswith('same_l3'):
        choices = [c for c in choices if c[0] in ('hiredis', 'cgdk.sync', 'cgdk.callback', 'cgdk.coroutine.io', 'cgdk.coroutine.dedicated.io')]
    return choices

total = sum(len(eligible(case)) for case in cases) * a.rounds
a.output.parent.mkdir(parents=True, exist_ok=True)
records = []
with a.output.open('w') as out:
    out.write(f'# {time.strftime("%Y-%m-%dT%H:%M:%S%z")} host={platform.node()} kernel={platform.release()} SDK={a.sdk_revision} fresh SDK/benchmark Release builds\n')
    out.write(f'# One outstanding command per caller (no batching); rounds={a.rounds} seconds={a.seconds} warmup={a.warmup} keys={a.keys}; CGDK_IO_URING=epoll\n')
    out.write('# Redis remains pinned to CPU8; default client set is L3=0, near set L3=1 on SAKPUNG; context switches are for all client process threads per 1000 commands\n')
    for round_index in range(a.rounds):
        for case_index, case in enumerate(cases):
            name, op, size, callers, sessions, cpus = case
            subprocess.run([str(a.build/'bench_hiredis'), f'--port={a.port}', '--populate', f'--keys={a.keys}', f'--size={size}'], check=True, stdout=subprocess.DEVNULL, timeout=60)
            choices = eligible(case)
            offset = (round_index*3+case_index) % len(choices)
            for label, binary, extra in choices[offset:] + choices[:offset]:
                cmd = ['taskset', '-c', cpus, str(a.build/binary), f'--port={a.port}', f'--seconds={a.seconds}', f'--warmup={a.warmup}', f'--keys={a.keys}', f'--size={size}', f'--op={op}', f'--callers={callers}', f'--sessions={sessions}', '--mode=sync'] + extra
                result = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=a.seconds+40)
                lines = [s for s in result.stdout.splitlines() if s.startswith('RESULT ')]
                if result.returncode or len(lines) != 1:
                    raise RuntimeError((name, label, result.returncode, result.stdout, result.stderr))
                values = dict(field.split('=', 1) for field in lines[0].split()[1:])
                if int(values['errors']) or int(values['ops']) == 0:
                    raise RuntimeError((name, label, lines[0]))
                values.update(test_case=name, competitor=label, round=str(round_index+1), connections=str(sessions), client_cpus=cpus)
                records.append(values)
                for line in result.stdout.splitlines():
                    if line.startswith('CONFIG '): out.write(f'# {name} {label} round={round_index+1} {line}\n')
                line = lines[0] + f' test_case={name} competitor={label} connections={sessions} client_cpus={cpus} round={round_index+1}'
                out.write(line+'\n'); out.flush()
                print(f'[{len(records)}/{total}] {line}', flush=True)

grouped = collections.defaultdict(list)
for row in records: grouped[(row['test_case'], row['competitor'])].append(row)
with a.output.with_suffix('.md').open('w') as out:
    out.write(f'# Redis single-request and coroutine comparison\n\n{len(records)} successful runs; SDK {a.sdk_revision}, freshly built dependency libraries; {a.rounds} rounds.\n\n')
    out.write('Each caller has at most one outstanding command. Callers greater than connections denotes multiplexing, not batches. Medians are calculated independently for each metric.\n\n')
    for case in cases:
        name, op, size, callers, sessions, cpus = case
        out.write(f'## {name}\n\n{op.upper()} {size} B, {callers} callers, {sessions} connections, CPUs {cpus}.\n\n')
        out.write('| Client | Commands/s | p50 us | p99 us | CPU us/command | Voluntary CS/1k commands | Involuntary CS/1k | Redis cores |\n|---|---:|---:|---:|---:|---:|---:|---:|\n')
        for label, _, _ in eligible(case):
            rows = grouped[(name, label)]
            m = lambda key: statistics.median(float(row[key]) for row in rows)
            out.write(f"| {label} | {m('ops_per_s'):,.0f} | {m('p50_us'):.1f} | {m('p99_us'):.1f} | {m('cpu_us_per_op'):.3f} | {m('voluntary_cs_per_kop'):.2f} | {m('involuntary_cs_per_kop'):.2f} | {m('server_cores'):.2f} |\n")
        out.write('\n')
print(f'COMPLETE {len(records)} successful runs', flush=True)
