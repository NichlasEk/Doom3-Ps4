#!/usr/bin/env python3
"""Send bounded SDL2 keyboard, mouse and text events to this verified Doom build.
SDL_PushEvent is thread-safe; engine commands are consumed by the main loop.
No automatic retries: a timed-out event may already have been queued.
"""
import argparse,fcntl,json,struct,subprocess,time
from pathlib import Path
from debug_client import DebugClient,utc
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--pid',type=int,required=True);p.add_argument('--elf',type=Path,default=Path('build/client-runtime/dude.oelf'));actions=p.add_mutually_exclusive_group(required=True);actions.add_argument('--command');actions.add_argument('--keys',help='Comma separated keys, for example w,shift');actions.add_argument('--move',nargs=2,type=int,metavar=('DX','DY'));actions.add_argument('--point',nargs=2,type=int,metavar=('X','Y'));actions.add_argument('--click',choices=['left','middle','right']);p.add_argument('--hold',type=float,default=.2);p.add_argument('--console-timeout',type=float,default=3,help='Seconds to await console state; extend for blocking map loads');p.add_argument('--release',action='store_true',help='Release named keys without pressing them (recovery)');p.add_argument('--reuse-session',action='store_true',help='Only for the same still-running process; never after reboot or PID reuse');a=p.parse_args()
if a.command is not None and (len(a.command)>200 or any(ord(c)<32 or ord(c)>126 for c in a.command)):raise SystemExit('One bounded ASCII command required')
if not 1<=a.console_timeout<=300:raise SystemExit('Console timeout must be 1..300 seconds')
if a.release and not a.keys:raise SystemExit('--release requires --keys')
if not .05 <= a.hold <= 2:raise SystemExit('Hold must be 0.05..2 seconds')
keymap={chr(97+i):(4+i,97+i) for i in range(26)}
keymap.update(enter=(40,13),escape=(41,27),space=(44,32),tab=(43,9),shift=(225,0x400000e1),ctrl=(224,0x400000e0),alt=(226,0x400000e2),grave=(53,96),up=(82,0x40000052),down=(81,0x40000051),left=(80,0x40000050),right=(79,0x4000004f))
selected=a.keys.lower().split(',') if a.keys else []
if len(selected)>4 or len(set(selected))!=len(selected) or any(k not in keymap for k in selected):raise SystemExit('Unknown or duplicate key; at most four keys')
if any(abs(v)>4096 for v in (a.move or a.point or [])):raise SystemExit('Mouse coordinates must be within +/-4096')
root=Path('artifacts/ps4-probe');root.mkdir(parents=True,exist_ok=True);(root/'jobs').mkdir(exist_ok=True);
lock=(root/f'doom{a.pid}-sdl.lock').open('w');fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
statepath=root/f'doom{a.pid}-sdl-state.json';jobpath=root/'jobs'/('sdl-'+str(time.time_ns())+'.json')
record=dict(started=utc(),pid=a.pid,arguments={k:str(v) if isinstance(v,Path) else v for k,v in vars(a).items()},events=[])
def save():jobpath.write_text(json.dumps(record,indent=2)+'\n')
save();c=None
try:
 c=DebugClient('192.168.32.229')
 info=c.identity(a.pid,'DOOM00001');maps=c.maps(a.pid);base=next(m['start'] for m in maps if m['name']=='executable' and m['prot']==5)
 symbols={}
 for line in subprocess.check_output(['readelf','-sW',str(a.elf)],text=True).splitlines():
  f=line.split()
  if len(f)>=8 and f[0].endswith(':'):
   try:symbols[f[-1]]=int(f[1],16)
   except ValueError:pass
 off=symbols['SDL_PushEvent'];data=a.elf.read_bytes()
 if c.read(a.pid,base+off,64)!=data[0x4000+off:0x4000+off+64]:raise ValueError('SDL function fingerprint mismatch')
 if statepath.exists():
  if not a.reuse_session:raise ValueError('Existing RPC state: confirm same running session with --reuse-session')
  state=json.loads(statepath.read_text())
  if state['identity']!=info or state['base']!=base:raise ValueError('Session identity mismatch')
  # The caller must not reuse this artifact after PID reuse or console reboot.
  stub=state['stub'];scratch=state['scratch']
 else:
  stub=c.install_rpc(a.pid);record['stub']=stub;save()
  scratch=c.alloc(a.pid,4096);state=dict(identity=info,base=base,stub=stub,scratch=scratch,created=utc())
  statepath.write_text(json.dumps(state,indent=2)+'\n')
 def push(event):
  record['pending_event']=event.hex();save()
  c.write(a.pid,'DOOM00001',scratch,event,c.read(a.pid,scratch,len(event)))
  result=c.call(a.pid,stub,base+off,[scratch])
  record['events'].append(dict(type=struct.unpack_from('<I',event)[0],result=result));record.pop('pending_event');save()
  if result!=1:raise RuntimeError('SDL event not accepted')
 def key(sc,code,down):
  e=bytearray(56);struct.pack_into('<III',e,0,0x300 if down else 0x301,0,0);e[12]=int(down);struct.pack_into('<iiH',e,16,sc,code,0);push(e)
 def toggle():
  keys=[(224,0x400000e0),(226,0x400000e2),(53,96)]
  try:
   for sc,code in keys:key(sc,code,True)
  finally:
   if 'pending_event' not in record:
    for sc,code in reversed(keys):key(sc,code,False)
   else:record['release_required']=['ctrl','alt','grave']
  time.sleep(.4)
 def console_active():
  offset=symbols['_ZN14idConsoleLocal6ActiveEv']
  expected=bytes.fromhex('0fb64710c3')
  if c.read(a.pid,base+offset,5)!=expected:raise ValueError('Console accessor layout mismatch')
  return bool(c.read(a.pid,base+symbols['_ZL12localConsole']+16,1)[0])
 def ensure_console(wanted):
  if console_active()!=wanted:toggle()
  deadline=time.monotonic()+a.console_timeout
  while console_active()!=wanted:
   if time.monotonic()>deadline:raise RuntimeError('Console did not reach requested state; command not retried')
   time.sleep(.1)
 if a.command is not None:
  record['console_before']=console_active();save()
  ensure_console(True)
  for i in range(0,len(a.command),31):
   e=bytearray(56);struct.pack_into('<I',e,0,0x303);chunk=a.command[i:i+31].encode();e[12:12+len(chunk)]=chunk;push(e)
  key(40,13,True)
  if a.command.strip()!='quit':
   key(40,13,False);time.sleep(.5);ensure_console(False)
 elif selected:
  # Observe engine state, not just SDL queue acceptance. Layout verified from IsDown.
  isdown=symbols['_ZN10idKeyInput6IsDownEi']
  if c.read(a.pid,base+isdown,95)!=data[0x4000+isdown:0x4000+isdown+95]:raise ValueError('Key state accessor fingerprint mismatch')
  def observed():
   ptr=struct.unpack('<Q',c.read(a.pid,base+symbols['keys'],8))[0]
   return {k:bool(c.read(a.pid,ptr+keymap[k][1]*56,1)[0]) for k in selected if len(k)==1}
  record['key_state_before']=observed()
  try:
   if not a.release:
    for k in selected:key(*keymap[k],True)
    time.sleep(a.hold)
    record['key_state_held']=observed()
  except OSError:
   record['transport_uncertain']=True
   raise
  finally:
   # A transport failure leaves state uncertain: never reuse a timed-out RPC stream.
   # Only release when no call remains pending; all normal/error-free holds release.
   if 'pending_event' not in record and not record.get('transport_uncertain'):
    for k in reversed(selected):key(*keymap[k],False)
   else:record['release_required']=selected
  time.sleep(.2);record['key_state_after']=observed()
 elif a.move or a.point:
  e=bytearray(56);struct.pack_into('<I',e,0,0x400)
  if a.move:struct.pack_into('<ii',e,28,*a.move)
  else:struct.pack_into('<ii',e,20,*a.point)
  push(e)
 elif a.click:
  def button(down):
   e=bytearray(56);struct.pack_into('<I',e,0,0x401 if down else 0x402);e[16]=['left','middle','right'].index(a.click)+1;e[17]=int(down);e[18]=1;push(e)
  try:button(True);time.sleep(a.hold)
  finally:
   if 'pending_event' not in record:button(False)
   else:record['release_required']=[a.click]
 record['state']='events_accepted_effect_unverified'
 if selected and not a.release and record.get('key_state_held') and all(record['key_state_held'].values()) and not any(record['key_state_after'].values()):
  record['state']='engine_key_press_and_release_verified'
except Exception as e:record.update(state='unconfirmed',error=str(e));raise
finally:
 record['finished']=utc();save()
 if c is not None:c.close()
 print(json.dumps(dict(journal=str(jobpath),**record),indent=2))
