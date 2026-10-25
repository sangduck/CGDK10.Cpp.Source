#!/bin/bash
#-----------------------------------------------------------------------------
# benchmark - query redis : runs the whole matrix and writes raw RESULT lines
#
#  ./run_bench.sh <bld dir> [<bld dir of a prototype cgdk>] [rounds]
#
#  - redis : docker 'redis-bench' (redis:8.2, --network host, port 6390,
#            pinned to CLIENT_CPUS's other CCD by REDIS_CPU, no persistence)
#  - every RESULT line gets 'build='. the bench reads the redis-server cpu
#    of the measure phase from /proc/<REDIS_PID>/stat ('server_cores=')
#  - rounds are interleaved (round -> case -> client) : median them later
#-----------------------------------------------------------------------------
set -u
BLD=${1:?bld dir}
BLD_PROTO=${2:-}
ROUNDS=${3:-3}
CLIENT_CPUS=${CLIENT_CPUS:-0-7,16-23}
REDIS_CPU=${REDIS_CPU:-8}
PORT=6390
SEC=${SEC:-3}
OUT=${OUT:-results/raw_$(date +%Y%m%d_%H%M%S).txt}
mkdir -p "$(dirname "$OUT")"

# 1) redis server
docker rm -f redis-bench >/dev/null 2>&1
docker run -d --name redis-bench --network host --cpuset-cpus "$REDIS_CPU" redis:8.2 \
	redis-server --port $PORT --save "" --appendonly no --protected-mode no >/dev/null
sleep 1
export REDIS_PID=$(docker inspect -f '{{.State.Pid}}' redis-bench)
{
	echo "# $(date -Is) host=$(hostname) kernel=$(uname -r)"
	echo "# cpu=$(lscpu | sed -n 's/^Model name:[ ]*//p') client_cpus=$CLIENT_CPUS redis_cpu=$REDIS_CPU"
	echo "# redis=$(docker exec redis-bench redis-server --version)"
	echo "# seconds=$SEC rounds=$ROUNDS"
} >> "$OUT"

# run <build name> <binary> <args...>
run() {
	local build=$1 bin=$2; shift 2
	local line
	line=$(timeout -k 5 $(( ${SEC%.*} + 30 )) taskset -c "$CLIENT_CPUS" "$bin" --port=$PORT --seconds=$SEC "$@" 2>&1 | grep '^RESULT')
	[ -z "$line" ] && line="RESULT FAILED $bin $*"
	echo "$line build=$build" | tee -a "$OUT"
}

populate() { "$BLD/bench_hiredis" --port=$PORT --populate --size=$1 >/dev/null; }

# clients of a case : name|binary|extra args
clients_sync() {
	echo "hiredis|$BLD/bench_hiredis|"
	echo "redis++|$BLD/bench_redispp|"
	echo "boost.redis|$BLD/bench_boost_redis|"
	echo "cgdk|$BLD/bench_cgdk|--variant=shared"
	echo "cgdk|$BLD/bench_cgdk|--variant=dedicated"
	echo "cgdk|$BLD/bench_cgdk|--variant=async"
}
clients_pipeline() {
	echo "hiredis|$BLD/bench_hiredis|"
	echo "redis++|$BLD/bench_redispp|"
	echo "boost.redis|$BLD/bench_boost_redis|"
	echo "cgdk|$BLD/bench_cgdk|--variant=shared"
	echo "cgdk|$BLD/bench_cgdk|--variant=shared --defer"
	echo "cgdk|$BLD/bench_cgdk|--variant=batch"
	echo "cgdk|$BLD/bench_cgdk|--variant=batch_sync"
}

run_case() {	# <clients function> <case args...>
	local fn=$1; shift
	while IFS='|' read -r name bin extra; do
		run base "$bin" "$@" $extra
	done < <($fn)
	if [ -n "$BLD_PROTO" ] && [[ "$fn" == clients_pipeline || "$*" == *callers=1[6]* || "$*" == *callers=64* ]]; then
		if [[ "$fn" == clients_pipeline ]]; then
			run proto "$BLD_PROTO/bench_cgdk" "$@" --variant=shared --defer
		else
			run proto "$BLD_PROTO/bench_cgdk" "$@" --variant=async
		fi
	fi
}

for ((r = 1; r <= ROUNDS; ++r)); do
	echo "# round $r" >> "$OUT"
	populate 16

	# A/B) sync : latency (1 caller) and concurrency
	for op in get set; do run_case clients_sync --mode=sync --op=$op --callers=1; done
	for c in 4 16 64; do run_case clients_sync --mode=sync --op=get --callers=$c; done

	# C) pipeline : one caller (connection), batch of N
	for op in get set; do
		for p in 10 100 1000; do run_case clients_pipeline --mode=pipeline --op=$op --callers=1 --pipeline=$p; done
	done
	# - 4 threads x batch 100 (cgdk dedicated : 4 sessions)
	run_case clients_pipeline --mode=pipeline --op=get --callers=4 --pipeline=100
	run base "$BLD/bench_cgdk" --mode=pipeline --op=get --callers=4 --pipeline=100 --variant=dedicated

	# D) value size : pipeline batch 10, one caller
	#    - cgdk SET : the command buffer is 4KB -> up to 4000
	#    - cgdk GET over ~2.5KB stalls on r11579 (shared_buffer::data<T>(offset) bound check) -> the prototype build (fixed) only
	for size in 16 1024 4000 16384 262144; do
		populate $size
		while IFS='|' read -r name bin extra; do
			[[ "$extra" == *defer* ]] && continue
			if [[ "$name" == cgdk && $size -gt 2048 ]]; then
				[ -n "$BLD_PROTO" ] && run proto "$BLD_PROTO/bench_cgdk" --mode=pipeline --pipeline=10 --op=get --size=$size $extra
			else
				run base "$bin" --mode=pipeline --pipeline=10 --op=get --size=$size $extra
			fi
			if [[ "$name" != cgdk || $size -le 4000 ]]; then
				run base "$bin" --mode=pipeline --pipeline=10 --op=set --size=$size $extra
			fi
		done < <(clients_pipeline)
	done
done

docker rm -f redis-bench >/dev/null 2>&1
echo "raw : $OUT"
