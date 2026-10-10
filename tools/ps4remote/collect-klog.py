#!/usr/bin/env python3
"""Bounded, receive-only capture of the GoldHEN klog stream (not BinLoader)."""
import argparse,datetime,hashlib,json,socket,time,uuid
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host',default='192.168.32.229')
    p.add_argument('--seconds',type=float,default=10)
    p.add_argument('--max-bytes',type=int,default=4*1024*1024)
    p.add_argument('--out',type=Path,default=Path('artifacts/remote-receiver/klog'))
    a=p.parse_args()
    if not 0<a.seconds<=60 or not 0<a.max_bytes<=64*1024*1024:p.error('Invalid capture bounds')
    a.out.mkdir(parents=True,exist_ok=True)
    name=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S')+'-'+uuid.uuid4().hex[:8]
    dest=a.out/(name+'.bin');digest=hashlib.sha256();count=0;reason='deadline';error=None
    deadline=time.monotonic()+a.seconds
    try:
        with socket.create_connection((a.host,3232),timeout=min(3,a.seconds)) as s,dest.open('xb') as f:
            while time.monotonic()<deadline and count<a.max_bytes:
                s.settimeout(min(1,max(.001,deadline-time.monotonic())))
                try:b=s.recv(min(65536,a.max_bytes-count))
                except socket.timeout:continue
                if not b:reason='eof';break
                f.write(b);digest.update(b);count+=len(b)
            if count==a.max_bytes:reason='byte_limit'
    except OSError as e:error=str(e);reason='error'
    result=dict(host=a.host,port=3232,path=str(dest),bytes=count,sha256=digest.hexdigest(),stop=reason,error=error,
                note='Stream can include historical messages; capture time is not event time.')
    dest.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2));return int(error is not None)
if __name__=='__main__':raise SystemExit(main())
