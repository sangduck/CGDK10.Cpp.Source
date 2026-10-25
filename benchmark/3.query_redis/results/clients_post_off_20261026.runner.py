"""Compare sync, callback async and pipeline clients against an isolated Redis.

Build all four clients first. --output writes JSONL plus summary/metadata JSON.
Only benchmark keys are populated; no containers or services are changed.
"""
from pathlib import Path
import subprocess,os,json,statistics,time,argparse,platform
parser=argparse.ArgumentParser(description="Compare current Redis clients; pipeline latency covers the complete group.")
parser.add_argument('build',type=Path)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--profile',choices=['full','current'],default='full')
parser.add_argument('--port',type=int,default=6390)
parser.add_argument('--keys',type=int,default=1024)
parser.add_argument('--rounds',type=int,default=3)
parser.add_argument('--seconds',type=float,default=2)
parser.add_argument('--warmup',type=float,default=.5)
parser.add_argument('--cpus',default='0-7,16-23')
args=parser.parse_args()
if args.rounds<1 or args.seconds<=0 or args.warmup<=0 or args.keys<1: parser.error('positive rounds, seconds, warmup and keys required')
b=args.build.resolve(); args.output=args.output.resolve(); args.output.parent.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env['CGDK_IO_URING']='epoll'
# REDIS_PID may be supplied to measure Redis CPU on the same host.
sync=[('hiredis.sync','bench_hiredis',[]),('hiredis.async','bench_hiredis',['--variant=async']),('redis++.sync','bench_redispp',[]),('Boost.Redis','bench_boost_redis',[]),('CGDK.sync','bench_cgdk',['--variant=shared']),('CGDK.async','bench_cgdk',['--variant=async'])]
asyncs=[('hiredis.async','bench_hiredis',['--variant=async']),('Boost.Redis','bench_boost_redis',[]),('CGDK.async','bench_cgdk',['--variant=async'])]
batched=[('hiredis.pipeline','bench_hiredis',[]),('redis++.pipeline','bench_redispp',[]),('Boost.Redis','bench_boost_redis',[]),('CGDK.individual8','bench_cgdk',['--variant=shared']),('CGDK.send_defer','bench_cgdk',['--variant=shared','--defer']),('CGDK.batch.async','bench_cgdk',['--variant=batch']),('CGDK.batch.sync','bench_cgdk',['--variant=batch_sync'])]
cases=[('get16_single',16,'get',1,1,1,sync),('get16_async64_c1',16,'get',64,1,1,asyncs),('get16_async1024_c1',16,'get',1024,1,1,asyncs),('get16_async1024_c4',16,'get',1024,4,1,asyncs),('get16_batch100_c1',16,'get',1,1,100,batched),('get16_batch1000_c1',16,'get',1,1,1000,batched),('get16_batch1000_c4',16,'get',4,4,1000,batched),('get16KB_batch10_c1',16384,'get',1,1,10,batched),('set16_batch1000_c1',16,'set',1,1,1000,batched)]
if args.profile=='current':
 batched=[('hiredis.pipeline','bench_hiredis',[]),('redis++.pipeline','bench_redispp',[]),('Boost.Redis','bench_boost_redis',[]),('CGDK.batch.async','bench_cgdk',['--variant=batch']),('CGDK.batch.sync','bench_cgdk',['--variant=batch_sync'])]
 cases=[('get16_single',16,'get',1,1,1,sync),('get16_async1024_c4',16,'get',1024,4,1,asyncs),('get16_batch1000_c1',16,'get',1,1,1000,batched),('get16_batch1000_c4',16,'get',4,4,1000,batched+[('CGDK.individual','bench_cgdk',['--variant=shared'])])]

meta={'profile':args.profile,'cgdk_post_response':False,'started':time.strftime('%Y-%m-%dT%H:%M:%S%z'),'host':platform.node(),'kernel':platform.release(),'build':str(b),'seconds':args.seconds,'warmup':args.warmup,'rounds':args.rounds,'cpus':args.cpus,'port':args.port,'keys':args.keys,'hiredis_async_adapter':'poll','latency':'Per command for single/async chains; full group for pipeline. CPU includes all client threads.'}
args.output.with_suffix('.metadata.json').write_text(json.dumps(meta,indent=2));rows=[];total=args.rounds*sum(len(c[-1]) for c in cases)
with args.output.open('w') as f:
 for rnd in range(args.rounds):
  for ci,(case,size,op,callers,sessions,count,clients) in enumerate(cases):
   subprocess.run([str(b/'bench_hiredis'),f'--port={args.port}','--populate',f'--keys={args.keys}',f'--size={size}'],check=True,stdout=subprocess.DEVNULL,timeout=90)
   offset=(rnd*2+ci)%len(clients)
   for label,binary,extra in clients[offset:]+clients[:offset]:
    mode='pipeline' if count>1 else 'sync'
    cmd=['taskset','-c',args.cpus,str(b/binary),f'--port={args.port}',f'--keys={args.keys}',f'--size={size}',f'--op={op}',f'--callers={callers}',f'--sessions={sessions}',f'--pipeline={count}',f'--mode={mode}',f'--seconds={args.seconds}',f'--warmup={args.warmup}']+extra
    if binary=='bench_cgdk': cmd+=['--post-response=off']
    completed=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=args.seconds+40)
    if binary=='bench_cgdk':
     assert 'post_response=0 ' in completed.stdout,completed.stdout
    with args.output.with_suffix('.stdout.log').open('a') as raw:raw.write(f'CASE {case} {label} round={rnd+1}\n'+completed.stdout+completed.stderr)
    lines=[line for line in completed.stdout.splitlines() if line.startswith('RESULT ')]
    if completed.returncode or len(lines)!=1:raise RuntimeError((case,label,completed.returncode,completed.stdout,completed.stderr))
    row=dict(field.split('=',1) for field in lines[0].split()[1:]);assert int(row['errors'])==0 and int(row['ops'])>0,row
    row.update(case=case,competitor=label,round=rnd+1,command=cmd);rows.append(row);f.write(json.dumps(row)+'\n');f.flush()
    print(f"[{len(rows)}/{total}] {case} {label}: ops={row['ops_per_s']} p99={row['p99_us']} cpu={row['cpu_us_per_op']} errors={row['errors']}",flush=True)
summary=[]
for case,*_rest,clients in cases:
 item={'case':case,'clients':{}}
 for label,*_ in clients:
  selected=[x for x in rows if x['case']==case and x['competitor']==label]
  item['clients'][label]={metric:statistics.median(float(x[metric]) for x in selected) for metric in ['ops_per_s','mean_us','p50_us','p99_us','p999_us','cpu_us_per_op','server_cores']}
  item['clients'][label]['min_ops_per_s']=min(float(x['ops_per_s']) for x in selected)
  item['clients'][label]['max_ops_per_s']=max(float(x['ops_per_s']) for x in selected)
 summary.append(item)
args.output.with_suffix('.summary.json').write_text(json.dumps(summary,indent=2));print(f'COMPLETE {total} successful runs',flush=True)
