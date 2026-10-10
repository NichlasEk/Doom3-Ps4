#!/usr/bin/env python3
"""Read-only timing sampler for the verified local Doom ELF, no engine RPC."""
import argparse,hashlib,json,struct,subprocess,time
from pathlib import Path
from debug_client import DebugClient,utc
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--pid',type=int,required=True);p.add_argument('--seconds',type=int,default=30);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
if not 1<=a.seconds<=300:raise SystemExit('Duration outside1..300')
elf=Path('build/client-runtime/dude.oelf');data=elf.read_bytes();symbols={}
for line in subprocess.check_output(['readelf','-sW',str(elf)],text=True).splitlines():
 fields=line.split()
 if len(fields)>=8 and fields[0].endswith(':'):
  try:symbols[fields[-1]]=int(fields[1],16)
  except ValueError:pass
c=DebugClient('192.168.32.229');identity=c.identity(a.pid,'DOOM00001');maps=c.maps(a.pid)
base=next(m['start'] for m in maps if m['name']=='executable' and m['prot']==5)
for name in ['main','_ZN13idCommonLocal5FrameEv','_ZN19idRenderSystemLocal8EndFrameEPiS0_']:
 if name not in symbols:raise SystemExit('Required ELF fingerprint symbol absent: '+name)
 off=symbols[name]
 if c.read(a.pid,base+off,64)!=data[0x4000+off:0x4000+off+64]:raise SystemExit('ELF fingerprint mismatch')
names=['time_gameFrame','time_gameDraw','time_frontend','time_backend','com_frameTime','com_frameNumber']
lo=min(symbols[n] for n in names);hi=max(symbols[n] for n in names)+4
assert hi-lo<=128
c.range(a.pid,base+lo,hi-lo)
record=dict(started=utc(),identity=identity,elf_sha256=hashlib.sha256(data).hexdigest(),note='Asynchronous samples; renderer counters are not GPU timestamps. Game counters may accumulate or reset.',samples=[])
a.out.parent.mkdir(parents=True,exist_ok=True)
start=time.monotonic()
try:
 while time.monotonic()-start<a.seconds:
  before=time.monotonic();c.send(0xBDAA0002,struct.pack('<IQI',a.pid,base+lo,hi-lo));c.status();raw=c.receive(hi-lo);after=time.monotonic()
  record['samples'].append(dict(t=(before+after)/2-start,read_ms=(after-before)*1000,**{n:struct.unpack_from('<i',raw,symbols[n]-lo)[0] for n in names}))
  time.sleep(.2)
 record['complete']=True
except Exception as e:
 record['complete']=False;record['error']=str(e);raise
finally:
 c.close();record['finished']=utc();a.out.write_text(json.dumps(record,indent=2)+'\n')
s=record['samples'];dt=s[-1]['t']-s[0]['t'];frames=s[-1]['com_frameNumber']-s[0]['com_frameNumber']
print(json.dumps(dict(path=str(a.out),samples=len(s),elapsed_s=dt,engine_frames=frames,engine_fps=frames/dt if dt else None,first=s[0],last=s[-1]),indent=2))
