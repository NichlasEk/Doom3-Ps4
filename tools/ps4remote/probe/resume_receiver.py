#!/usr/bin/env python3
"""One-shot activation of our existing Receiver from debugger-host ShellCore.
9.60 only. Verifies native code; never edits ShellCore code or credentials.
Installing RPC adds its dedicated thread and private allocation to ShellCore.
"""
import json,struct,time
from pathlib import Path
from debug_client import DebugClient,utc
c=DebugClient('192.168.32.229');out=Path('artifacts/ps4-probe/jobs')/('resume-'+str(time.time_ns())+'.json');r=dict(started=utc(),target='RMTT00001')
def save():out.write_text(json.dumps(r,indent=2)+'\n')
save()
try:
 assert c.version()['firmware_raw']==960
 c.identity(42,'NPXS20000');c.identity(218,'RMTT00001')
 maps=c.maps(42);wm=c.maps(218)
 kernel=next(m for m in maps if m['name']=='libkernel_sys.sprx' and m['prot']==5)
 code=c.read(42,kernel['start'],kernel['end']-kernel['start'])
 pattern=bytes.fromhex('48c7c0140000004989ca0f057201c3')
 assert code.count(pattern)==1
 getpid=kernel['start']+code.index(pattern)
 user=struct.unpack('<Q',c.read(218,0x494098,8))[0]
 source=next(m for m in wm if m['start']<=user<m['end'])
 dest=next(m for m in maps if m['name']==source['name'] and m['prot']==5)
 foreground=dest['start']+user-source['start']
 assert c.read(42,foreground,64)==c.read(218,user,64)
 system=next(m for m in maps if m['name']=='libSceSystemService.sprx' and m['prot']==5)['start']
 init=system+0x5820;launch=system+0x5dc0
 assert c.read(42,init,16)==bytes.fromhex('488d3d29a80300e95408000090909090')
 assert c.read(42,launch,32)==bytes.fromhex('4989c84889d14889f24889fe488d3d7da20300e9184e00009090909090909090')
 state=Path('artifacts/ps4-probe/shellcore-rpc-session.json')
 if state.exists():raise ValueError('Existing session artifact; do not reinstall RPC or relaunch blindly')
 stub=c.install_rpc(42);r['stub']=stub;save()
 scratch=c.alloc(42,4096);r['scratch']=scratch;save()
 state.write_text(json.dumps(dict(pid=42,title='NPXS20000',stub=stub,scratch=scratch,getpid=getpid,foreground=foreground,init=init,launch=launch,created=utc()),indent=2))
 assert c.call(42,stub,getpid,[])==42
 def write(off,data):c.write(42,'NPXS20000',scratch+off,data,c.read(42,scratch+off,len(data)))
 write(0,struct.pack('<i',-1));assert c.call(42,stub,foreground,[scratch])==0
 user=struct.unpack('<i',c.read(42,scratch,4))[0];assert user>=0
 r['user']=user;r['init']=c.call(42,stub,init,[]);save();assert r['init']==0
 write(64,b'RMTT00001\0');write(96,struct.pack('<Ii4I',24,user,0,0,0,0));write(128,struct.pack('<i',-1))
 r['state']='submitting';save();r['submission']=c.call(42,stub,launch,[scratch+64,0,scratch+96,scratch+128]);r['ticket']=struct.unpack('<i',c.read(42,scratch+128,4))[0];r['state']='returned_effect_unverified'
except Exception as e:r.update(state='unconfirmed',error=str(e));raise
finally:r['finished']=utc();save();c.close();print(json.dumps(dict(journal=str(out),**r),indent=2))
