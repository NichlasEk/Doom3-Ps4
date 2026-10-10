"""Firmware-9.60, Receiver-worker-only system-bit scope with host restoration.
No general kernel write CLI. Revalidates PID, name, credential pointer and tuple
before each change. A lost debugger connection still leaves recovery unconfirmed.
"""
import struct
from debug_client import cstr
BASELINE=(0x3800000000000010,0x2000038000000000,0xff00)
ELEVATED=(BASELINE[0],BASELINE[1]|(1<<62),BASELINE[2])

def pointer(p):
    if not 0xffff800000000000<=p<0xffffffff00000000 or p&7:
        raise ValueError('Invalid kernel heap pointer')
    return p

class WorkerScope:
    def __init__(self,client,pid):
        self.c=client;self.pid=pid
        if client.version()['firmware_raw']!=960:raise ValueError('Firmware must be 9.60')
        info=client.identity(pid,'RMTT00001')
        if info['name']!='remote-worker.self' or info['path']!='/app0/remote-worker.self':
            raise ValueError('Only our Receiver worker may be targeted')
        client.send(0xBDCC0001);client.status();self.base,=client.unpack('<Q')
        if not 0xffffffff00000000<=self.base<2**64 or self.base&0x3fff:raise ValueError('Invalid kernel base')
        self.proc,self.cred,self.values=self.observe()
        if self.values!=BASELINE:raise ValueError('Worker credential baseline mismatch')
    def read(self,address,length):
        if not 0xffff800000000000<=address<2**64 or not 0<length<=2048:raise ValueError('Invalid kernel read')
        self.c.send(0xBDCC0002,struct.pack('<QI',address,length));self.c.status()
        return self.c.receive(length)
    def observe(self):
        p,=struct.unpack('<Q',self.read(self.base+0x221d2a0,8));seen=set()
        for _ in range(512):
            if not p or p in seen:break
            pointer(p);seen.add(p);data=self.read(p,0x474)
            pid,=struct.unpack_from('<I',data,0xb0)
            if pid==self.pid:
                if cstr(data[0x454:0x474])!='remote-worker.self':raise ValueError('Kernel process name mismatch')
                cred=pointer(struct.unpack_from('<Q',data,0x40)[0]);raw=self.read(cred,0x70)
                if struct.unpack_from('<I',raw,4)[0]!=1:raise ValueError('Worker UID mismatch')
                return p,cred,struct.unpack_from('<QQQ',raw,0x58)
            p,=struct.unpack_from('<Q',data)
        raise ValueError('Worker absent from kernel process list')
    def transition(self,expected,replacement):
        if (expected,replacement) not in ((BASELINE,ELEVATED),(ELEVATED,BASELINE)):raise ValueError('Invalid transition')
        p,cred,values=self.observe()
        if (p,cred,values)!=(self.proc,self.cred,expected):raise ValueError('Credential identity or expected bytes changed')
        self.c.send(0xBDCC0003,struct.pack('<QI',cred+0x60,8));self.c.status()
        self.c.sock.sendall(struct.pack('<Q',replacement[1]));self.c.status()
        if self.observe()!=(self.proc,self.cred,replacement):raise RuntimeError('Credential readback mismatch')
    def elevate(self):self.transition(BASELINE,ELEVATED)
    def restore(self):
        observed=self.observe()
        if observed==(self.proc,self.cred,BASELINE):return
        self.transition(ELEVATED,BASELINE)
