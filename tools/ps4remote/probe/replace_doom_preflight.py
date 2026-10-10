#!/usr/bin/env python3
"""Remove only the stopped DOOM00001 package for the authorized0.14 replacement.
Requires a verified local rollback/backup and the already-uploaded candidate.
No savedata-delete API, retry loop, or arbitrary-title argument is provided.
"""
import hashlib,json,os,time
from pathlib import Path
from debug_client import DebugClient,utc
from sprx_exports import export
out=Path('artifacts/clock-014/remove-old-package.json')
if out.exists():raise SystemExit('An attempt is already journaled; inspect it, do not retry blindly')
backup=json.loads(Path('artifacts/clock-014/console-backup/manifest.json').read_text());assert backup['rollback_matches']
assert hashlib.sha256(Path('dist/Doom3-PS4-Playable-Alpha-0.13.pkg').read_bytes()).hexdigest()==backup['installed_package_sha256']
transfer=json.loads(Path('artifacts/receiver-jobs/a55408aee8fe43259bec14a273a3516a.json').read_text())['transfer'];assert transfer['readback_matches']
manifest=json.loads(Path('artifacts/clock-014/package-manifest.json').read_text());assert manifest['title_id']=='DOOM00001' and manifest['version']=='00.14' and manifest['sha256']==transfer['sha256']
s=json.loads(Path('artifacts/ps4-probe/shellcore-rpc-session.json').read_text());c=DebugClient('192.168.32.229',timeout=15);r=dict(started=utc(),title='DOOM00001',replacement_sha256=manifest['sha256'],state='preflight')
def save():
 with out.open('w') as f:json.dump(r,f,indent=2);f.flush();os.fsync(f.fileno())
save()
try:
 assert c.version()['firmware_raw']==960;c.identity(42,'NPXS20000');assert c.call(42,s['stub'],s['getpid'],[])==42
 for p in c.processes():
  if p['name']=='eboot.bin' and c.info(p['pid'])['title_id']=='DOOM00001':raise ValueError('Doom is still running')
 base=next(m['start'] for m in c.maps(42) if m['name']=='libSceAppInstUtil.sprx' and m['prot']==5)
 funcs={}
 for name in ['sceAppInstUtilInitialize','sceAppInstUtilAppUnInstall','sceAppInstUtilAppExists']:
  off,code=export('artifacts/remote-receiver/console-modules/libSceAppInstUtil.sprx',name)
  assert c.read(42,base+off,len(code))==code;funcs[name]=base+off
 r['initialize']=c.call(42,s['stub'],funcs['sceAppInstUtilInitialize'],[]);save();assert r['initialize']==0
 ptr=s['scratch'];c.write(42,'NPXS20000',ptr,b'DOOM00001\0',c.read(42,ptr,10))
 r['state']='uninstall_requested';save()
 r['return']=c.call(42,s['stub'],funcs['sceAppInstUtilAppUnInstall'],[ptr]);r['state']='returned_effect_unverified';save()
finally:r['finished']=utc();save();c.close();print(json.dumps(r,indent=2))
