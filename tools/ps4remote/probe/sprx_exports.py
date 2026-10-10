"""Resolve named exports in locally extracted Orbis ELF modules by NID."""
import base64,hashlib,struct
from pathlib import Path

def export(path,name):
 b=Path(path).read_bytes()
 if b[:4]!=b'\x7fELF':raise ValueError('Expected extracted ELF')
 ph=[struct.unpack_from('<IIQQQQQQ',b,64+i*56) for i in range(struct.unpack_from('<H',b,56)[0])]
 base=next(h[2] for h in ph if h[0]==0x61000000);d=next(h for h in ph if h[0]==2)
 tags=dict(struct.unpack_from('<QQ',b,x) for x in range(d[2],d[2]+d[5],16))
 nid=base64.b64encode(hashlib.sha1(name.encode()+bytes.fromhex('518D64A635DED8C1E6B039B1C3E55230')).digest()[:8][::-1]).decode().rstrip('=').replace('/','-')
 for x in range(base+tags[0x61000039],base+tags[0x61000039]+tags[0x6100003f],24):
  n,info,other,section,value,size=struct.unpack_from('<IBBHQQ',b,x);off=base+tags[0x61000035]+n;s=b[off:b.index(0,off)].decode()
  if section and s.split('#')[0]==nid:
   h=next(h for h in ph if h[0]==1 and h[1]&1 and h[3]<=value and value+32<=h[3]+h[5])
   fileoff=h[2]+value-h[3];return value,b[fileoff:fileoff+32]
 raise ValueError('Export not found: '+name)
