#!/usr/bin/env python3
"""Analyze observed log boundaries; intervals include logging/scheduling overhead."""
import argparse,json,re,statistics
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path);p.add_argument('--last',type=int,default=300);p.add_argument('--since-ms',type=float,default=0);p.add_argument('--scene',default='unidentified');a=p.parse_args()
rows=[]
for line in a.log.read_text(errors='replace').splitlines():
 m=re.match(r'\[(\d+)\] #\d+ (.*)',line)
 if m and int(m[1])/1000>=a.since_ms:rows.append((int(m[1])/1000,m[2]))
cycles=[];cur={};previous=None
for t,msg in rows:
 if 'EndCommandBuffer: result=0' in msg:cur['command_end']=t
 elif 'QueueSubmit: sceGnmSubmitCommandBuffers' in msg:cur['submit']=t
 elif 'SubmitDone experiment: returned=' in msg:cur.setdefault('done_returns',[]).append(t)
 elif 'QueuePresent: flip img=' in msg:cur['present']=t
 elif 'QueuePresent: flip done' in msg:
  cur['flip_done']=t
  if previous is not None and all(k in cur for k in ['command_end','submit','present','done_returns']) and len(cur['done_returns'])==2:
   cycles.append(dict(frame_ms=t-previous,prepare_until_command_end_ms=cur['command_end']-previous,command_end_to_submit_ms=cur['submit']-cur['command_end'],submit_to_present_ms=cur['present']-cur['submit'],between_submit_done_returns_ms=cur['done_returns'][1]-cur['done_returns'][0],blocking_present_ms=t-cur['present']))
  previous=t;cur={}
cycles=cycles[-a.last:]
def stats(v):
 s=sorted(v);return dict(median=statistics.median(s),p95=s[min(len(s)-1,int(len(s)*.95))],mean=statistics.mean(s))
r=dict(log=str(a.log),cycles=len(cycles),scene=a.scene,since_ms=a.since_ms,note='CPU wall-clock boundaries, NOT GPU execution times',milliseconds={k:stats([c[k] for c in cycles]) for k in cycles[0]} if cycles else {})
print(json.dumps(r,indent=2))
