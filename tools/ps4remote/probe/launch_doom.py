#!/usr/bin/env python3
"""One-shot Doom launch via our existing worker RPC; host restores system bit."""
import argparse,json,os,struct,time,uuid
from pathlib import Path
from debug_client import DebugClient,utc
from worker_scope import WorkerScope

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pid',type=int,required=True);p.add_argument('--stub',type=lambda x:int(x,0),required=True)
    p.add_argument('--host',default='192.168.32.229');a=p.parse_args()
    dest=Path('artifacts/ps4-probe/jobs')/(uuid.uuid4().hex+'.json')
    record=dict(started=utc(),pid=a.pid,stub=a.stub,title='DOOM00001',state='preparing')
    def save():
        with dest.open('w') as f:json.dump(record,f,indent=2);f.flush();os.fsync(f.fileno())
    save();rpc=DebugClient(a.host);guard=DebugClient(a.host);scope=None;restore_needed=False
    try:
        scope=WorkerScope(guard,a.pid)
        record.update(proc=hex(scope.proc),cred=hex(scope.cred));save()
        maps=rpc.maps(a.pid)
        base=next(m['start'] for m in maps if m['name']=='executable' and m['prot']==5)
        elf=Path('build/remote-receiver/worker.elf').read_bytes()
        for off in (0x5cc90,0x5cd80,0x5cd90):
            if rpc.read(a.pid,base+off,16)!=elf[0x4000+off:0x4000+off+16]:raise ValueError('Worker PLT fingerprint mismatch')
        if rpc.call(a.pid,a.stub,base+0x5cc90,[])!=a.pid:raise ValueError('RPC getpid mismatch')
        if struct.unpack('<i',rpc.read(a.pid,base+0x98088,4))[0]!=0:raise ValueError('Worker LNC not initialized')
        launch,=struct.unpack('<Q',rpc.read(a.pid,base+0x98310,8))
        fingerprint=bytes.fromhex('4989c84889d14889f24889fe488d3d7da20300e9184e00009090909090909090')
        if rpc.read(a.pid,launch,32)!=fingerprint:raise ValueError('Native launch entry mismatch')
        if any(i['name']=='eboot.bin' and rpc.info(i['pid'])['title_id']=='DOOM00001' for i in rpc.processes()):raise ValueError('Doom already running; no launch needed')
        scratch=rpc.alloc(a.pid,4096);record['scratch']=hex(scratch);save()
        def write(off,data):rpc.write(a.pid,'RMTT00001',scratch+off,data,rpc.read(a.pid,scratch+off,len(data)))
        write(0,b'\xff'*32)
        if rpc.call(a.pid,a.stub,base+0x5cd80,[scratch])!=0:raise ValueError('Foreground user query failed')
        if rpc.call(a.pid,a.stub,base+0x5cd90,[scratch+16])!=0:raise ValueError('Login users query failed')
        user,=struct.unpack('<i',rpc.read(a.pid,scratch,4))
        users=struct.unpack('<4i',rpc.read(a.pid,scratch+16,16))
        if user<0 or user not in users:raise ValueError('Foreground user is not logged in')
        write(64,b'DOOM00001\0');write(96,struct.pack('<Ii4I',24,user,0,0,0,0));write(128,struct.pack('<i',-1))
        record.update(state='elevating',user=user,launch=hex(launch));save()
        restore_needed=True;scope.elevate()
        record['state']='submitting';save()
        record['submission']=rpc.call(a.pid,a.stub,launch,[scratch+64,0,scratch+96,scratch+128]);save()
        record['ticket']=struct.unpack('<i',guard.read(a.pid,scratch+128,4))[0];save()
        if record['submission']!=0:raise RuntimeError('Native launch submission rejected')
        deadline=time.monotonic()+12
        while time.monotonic()<deadline:
            for proc in guard.processes():
                if proc['name']=='eboot.bin' and guard.info(proc['pid'])['title_id']=='DOOM00001':
                    record['doom_pid']=proc['pid'];record['state']='doom_process_observed';save();return 0
            time.sleep(.25)
        record['state']='launch_not_observed';return 1
    except Exception as e:record.update(state='unconfirmed',error=str(e));return 1
    finally:
        if restore_needed:
            try:scope.restore();record['credential_restored']=True
            except Exception as e:record['credential_restored']=False;record['restore_error']=str(e)
        record['finished']=utc();save();rpc.close();guard.close();print(json.dumps(dict(journal=str(dest),**record),indent=2))
        if record.get('credential_restored') is False:raise SystemExit(2)
if __name__=='__main__':raise SystemExit(main())
