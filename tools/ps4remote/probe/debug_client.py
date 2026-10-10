#!/usr/bin/env python3
"""Local PS4 debugger client. Journals commands; never retries mutations."""
import argparse, datetime, hashlib, json, socket, struct, uuid
from pathlib import Path

MAGIC=0xFFAABBCC
SUCCESS=0x80000000
MAX_READ=1024*1024
RELEASE_SHA='28829f48465d3dfdf97afc15f01edafd8802f8213c44635af6f21e0e8b48c8f6'

def integer(s):return int(s,0)
def cstr(b):return b.split(b'\0',1)[0].decode('utf-8',errors='replace')
def utc():return datetime.datetime.now(datetime.timezone.utc).isoformat()

class DebugClient:
    def __init__(self,host,port=744,timeout=5):
        self.sock=socket.create_connection((host,port),timeout=timeout)
        self.sock.settimeout(timeout)
    def close(self):self.sock.close()
    def receive(self,n):
        if not 0<=n<=MAX_READ:raise ValueError('Response length exceeds limit')
        out=bytearray()
        while len(out)<n:
            b=self.sock.recv(n-len(out))
            if not b:raise EOFError('Debugger disconnected; command outcome may be unknown')
            out.extend(b)
        return bytes(out)
    def unpack(self,fmt):return struct.unpack(fmt,self.receive(struct.calcsize(fmt)))
    def send(self,cmd,body=b''):
        self.sock.sendall(struct.pack('<III',MAGIC,cmd,len(body))+body)
    def status(self):
        status,=self.unpack('<I')
        if status!=SUCCESS:raise RuntimeError(f'Debugger status 0x{status:08x}')
    def text(self,cmd):
        self.send(cmd);n,=self.unpack('<I')
        if n>4096:raise ValueError('Invalid debugger string length')
        return self.receive(n).decode('utf-8',errors='replace')
    def version(self):
        version=self.text(0xBD000001);branding=self.text(0xBD000501)
        self.send(0xBD000500);fw,=self.unpack('<H')
        self.send(0xBD000502);platform,=self.unpack('<H')
        return dict(version=version,branding=branding,firmware_raw=fw,platform=platform)
    def processes(self):
        self.send(0xBDAA0001);self.status();n,=self.unpack('<I')
        if n>4096:raise ValueError('Invalid process count')
        return [dict(name=cstr(name),pid=pid) for name,pid in
                (self.unpack('<32si') for _ in range(n))]
    def info(self,pid):
        self.send(0xBDAA000A,struct.pack('<I',pid));self.status()
        actual,name,path,title,content=self.unpack('<I40s64s16s64s')
        if actual!=pid:raise ValueError('Process PID mismatch')
        return dict(pid=actual,name=cstr(name),path=cstr(path),title_id=cstr(title),content_id=cstr(content))
    def identity(self,pid,title):
        info=self.info(pid)
        if not title or info['title_id']!=title:raise ValueError('Process title mismatch; refusing target operation')
        return info
    def maps(self,pid):
        self.send(0xBDAA0004,struct.pack('<I',pid));self.status();n,=self.unpack('<I')
        if n>16384:raise ValueError('Invalid memory map count')
        return [dict(start=a,end=b,offset=o,prot=p,name=cstr(name))
                for name,a,b,o,p in (self.unpack('<32sQQQH') for _ in range(n))]
    def range(self,pid,address,length,protection=1):
        if not 0<length<=MAX_READ or not 0<address<2**47 or address+length>=2**47:
            raise ValueError('Invalid or excessive user memory range')
        if not any(m['start']<=address and address+length<=m['end'] and
                   m['prot']&protection==protection for m in self.maps(pid)):
            raise ValueError('Range is not wholly inside a matching mapped region')
    def read(self,pid,address,length):
        self.range(pid,address,length)
        self.send(0xBDAA0002,struct.pack('<IQI',pid,address,length));self.status()
        return self.receive(length)
    def write(self,pid,title,address,data,expected):
        if len(data)!=len(expected):raise ValueError('Expected/replacement size mismatch')
        self.identity(pid,title);self.range(pid,address,len(data),3)
        if self.read(pid,address,len(data))!=expected:raise ValueError('Expected bytes mismatch; no write sent')
        self.send(0xBDAA0003,struct.pack('<IQI',pid,address,len(data)));self.status()
        self.sock.sendall(data);self.status()
        if self.read(pid,address,len(data))!=data:raise RuntimeError('Write readback mismatch')
    def alloc(self,pid,length):
        if not 0<length<=MAX_READ:raise ValueError('Allocation exceeds limit')
        self.send(0xBDAA000B,struct.pack('<II',pid,length));self.status();address,=self.unpack('<Q')
        self.range(pid,address,length,3);return address
    def install_rpc(self,pid):
        self.send(0xBDAA0005,struct.pack('<I',pid));self.status();stub,=self.unpack('<Q')
        self.range(pid,stub,16);return stub
    def call(self,pid,stub,address,args):
        if len(args)>6 or any(not 0<=v<2**64 for v in args):raise ValueError('RPC accepts up to six u64 arguments')
        self.range(pid,stub,16);self.range(pid,address,1,4)
        self.send(0xBDAA0006,struct.pack('<IQQ6Q',pid,stub,address,*(args+[0]*(6-len(args)))))
        self.status();actual,value=self.unpack('<IQ')
        if actual!=pid:raise ValueError('RPC response PID mismatch')
        return value

def load_payload(host,path):
    data=Path(path).read_bytes()
    if hashlib.sha256(data).hexdigest()!=RELEASE_SHA:raise ValueError('Payload does not match pinned upstream release')
    # Never probe BinLoader with an empty connection: GoldHEN may disable it.
    with socket.create_connection((host,9090),timeout=10) as s:
        s.sendall(data);s.shutdown(socket.SHUT_WR)
    return dict(bytes_sent=len(data),sha256=RELEASE_SHA,execution_verified=False)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host',default='192.168.32.229')
    parser.add_argument('--port',type=int,default=744)
    parser.add_argument('--journal',type=Path,default=Path('artifacts/ps4-probe/jobs'))
    sub=parser.add_subparsers(dest='command',required=True)
    sub.add_parser('version');sub.add_parser('processes')
    p=sub.add_parser('load');p.add_argument('--payload',type=Path,default=Path('artifacts/ps4-probe/ps4debug-ng-1.3.2.bin'))
    for name in ('inspect','read','write','alloc','rpc-install','rpc-call'):
        p=sub.add_parser(name);p.add_argument('--pid',type=int,required=True);p.add_argument('--title',required=True)
        if name in ('read','write','rpc-call'):p.add_argument('--address',type=integer,required=True)
        if name in ('read','alloc'):p.add_argument('--length',type=integer,required=True)
        if name=='read':p.add_argument('--output',type=Path,required=True)
        if name=='write':
            p.add_argument('--input',type=Path,required=True);p.add_argument('--expected',type=Path,required=True)
        if name=='rpc-call':
            p.add_argument('--stub',type=integer,required=True);p.add_argument('--args',type=integer,nargs='*',default=[])
    a=parser.parse_args();a.journal.mkdir(parents=True,exist_ok=True)
    file=a.journal/(uuid.uuid4().hex+'.json')
    record=dict(started=utc(),host=a.host,port=9090 if a.command=='load' else a.port,
                command=a.command,state='requesting',arguments={k:str(v) for k,v in vars(a).items() if k!='journal'})
    def save():file.write_text(json.dumps(record,indent=2)+'\n')
    save();client=None
    try:
        if a.command=='load':result=load_payload(a.host,a.payload)
        else:
            client=DebugClient(a.host,a.port)
            if a.command=='version':result=client.version()
            elif a.command=='processes':result=client.processes()
            else:
                record['identity']=client.identity(a.pid,a.title);save()
                if a.command=='inspect':result=dict(identity=record['identity'],maps=client.maps(a.pid))
                elif a.command=='read':
                    data=client.read(a.pid,a.address,a.length)
                    with a.output.open('xb') as f:f.write(data)
                    result=dict(path=str(a.output),bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
                elif a.command=='write':
                    data=a.input.read_bytes();expected=a.expected.read_bytes()
                    record['replacement_sha256']=hashlib.sha256(data).hexdigest();record['expected_sha256']=hashlib.sha256(expected).hexdigest();save()
                    client.write(a.pid,a.title,a.address,data,expected);result=dict(bytes=len(data),readback_verified=True)
                elif a.command=='alloc':result=dict(address=client.alloc(a.pid,a.length),length=a.length)
                elif a.command=='rpc-install':result=dict(stub=client.install_rpc(a.pid))
                elif a.command=='rpc-call':result=dict(rax=client.call(a.pid,a.stub,a.address,a.args),native_effect_verified=False)
        record.update(state='returned',result=result)
    except Exception as e:record.update(state='unconfirmed',error=str(e))
    finally:
        if client:client.close()
        record['finished']=utc();save()
    print(json.dumps(dict(journal=str(file),**record),indent=2))
    return 1 if 'error' in record else 0

if __name__=='__main__':raise SystemExit(main())
