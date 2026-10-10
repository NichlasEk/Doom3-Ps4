#!/usr/bin/env python3
"""Client for the private foreground PS4 receiver (port 12801)."""
import argparse,ftplib,hashlib,json,sys,urllib.request,uuid
from pathlib import Path
from ps4remote import OPENER, package_info, now, save


def call(host,port,key,endpoint,body=None):
    headers={}
    if body is not None:headers={'X-PS4Remote-Key':key,'Content-Type':'text/plain'}
    req=urllib.request.Request(f'http://{host}:{port}/v1/'+endpoint,
        None if body is None else body.encode('ascii'),headers)
    with OPENER.open(req,timeout=120 if endpoint=='install-local' else 15) as r:
        value=json.loads(r.read(65536))
    if not isinstance(value,dict):raise ValueError('Unexpected receiver response')
    return value


def stage(host,port,info):
    with ftplib.FTP() as ftp:
        ftp.connect(host,port,timeout=20);ftp.login()
        try:ftp.mkd('/data/ps4remote')
        except ftplib.error_perm:ftp.cwd('/data/ps4remote')
        target='/data/ps4remote/'+info['sha256']+'.pkg'
        names=ftp.nlst('/data/ps4remote')
        exists=any(Path(n).name==Path(target).name for n in names)
        partial=target+'.'+uuid.uuid4().hex+'.partial'
        if not exists:
            with open(info['path'],'rb') as f:ftp.storbinary('STOR '+partial,f,1024*1024)
        digest=hashlib.sha256();count=0
        def read(b):
            nonlocal count
            count+=len(b);digest.update(b)
            if count>info['bytes']:raise ValueError('Readback exceeds expected size')
        ftp.retrbinary('RETR '+(target if exists else partial),read,1024*1024)
        if count!=info['bytes'] or digest.hexdigest()!=info['sha256']:
            raise ValueError('FTP readback mismatch; retained partial for inspection')
        if not exists:ftp.rename(partial,target)
        return {'remote':target,'bytes':count,'sha256':digest.hexdigest(),'readback_matches':True,'reused':exists}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host',default='192.168.32.229');p.add_argument('--port',type=int,default=12801)
    p.add_argument('--ftp-port',type=int,default=2121)
    p.add_argument('--key-file',type=Path,default=Path('artifacts/remote-receiver/receiver.key'))
    p.add_argument('--out',type=Path,default=Path('artifacts/receiver-jobs'))
    sub=p.add_subparsers(dest='command',required=True);sub.add_parser('health');sub.add_parser('launch-test');sub.add_parser('worker-start');sub.add_parser('launch-async-control');sub.add_parser('launch-async-scoped');sub.add_parser('launch-async-result');sub.add_parser('launch-permission-probe')
    c=sub.add_parser('probe');c.add_argument('--mode',required=True,choices=['resolve','direct','sandbox','internal','bgft','bgft-init','bgft-public-init','dep-system','dep-user','dep-netctl','dep-http','dep-ssl','dep-np','ipc-connect','credentials','caps-test','system-test','lnc-init','app-slots','worker-register'])
    for cmd in ('exists','launch'):
        c=sub.add_parser(cmd);c.add_argument('--title',required=True)
    for cmd in ('bgft-start','bgft-status'):
        c=sub.add_parser(cmd);c.add_argument('--task',type=int,required=True)
    for cmd in ('stage','install','bgft-register'):
        c=sub.add_parser(cmd);c.add_argument('--manifest',required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    record=a.out/(uuid.uuid4().hex+'.json')
    result=dict(started=now(),host=a.host,command=a.command,state='requesting')
    save(record,result)
    try:
        if a.command=='stage':result['transfer']=stage(a.host,a.ftp_port,package_info(a.manifest))
        else:
            key='' if a.command=='health' else a.key_file.read_text().strip()
            if a.command=='health':reply=call(a.host,a.port,key,'health')
            elif a.command=='worker-start':reply=call(a.host,a.port,key,'worker-start','start')
            elif a.command in ('launch-async-control','launch-async-scoped','launch-async-result'):reply=call(a.host,a.port,key,a.command,'RMTS00002')
            elif a.command=='launch-permission-probe':
                with ftplib.FTP() as ftp:
                    ftp.connect(a.host,a.ftp_port,timeout=10);ftp.login()
                    for directory in ('/user/app','/user/patch'):
                        entries=ftp.nlst(directory)
                        if any(entry.rstrip('/').split('/')[-1]=='RMTS99999' for entry in entries):
                            raise ValueError('Reserved probe title is installed; refusing launch')
                result['missing_title_verified']='RMTS99999';save(record,result)
                reply=call(a.host,a.port,key,a.command,'RMTS99999')
            elif a.command=='launch-test':reply=call(a.host,a.port,key,'launch-test','RMTS00002')
            elif a.command=='probe':
                result['probe']=a.mode;save(record,result)
                reply=call(a.host,a.port,key,'probe',a.mode)
            elif a.command in ('bgft-start','bgft-status'):
                result['task']=a.task;save(record,result)
                reply=call(a.host,a.port,key,a.command,str(a.task))
            elif a.command=='bgft-register':
                info=package_info(a.manifest)
                health=call(a.host,a.port,key,'health')
                if health.get('bgft_init')!=0 or not health.get('bgft_register_available'):
                    raise ValueError('BGFT not ready')
                result['package']=info;result['transfer']=stage(a.host,a.ftp_port,info);save(record,result)
                reply=call(a.host,a.port,key,'bgft-register',info['title_id']+' '+info['sha256'])
            elif a.command=='install':
                info=package_info(a.manifest)
                # Receiver must be reachable before transferring the selected package.
                health=call(a.host,a.port,key,'health')
                if health.get('protocol')!='ps4remote-1' or health.get('app_init',-1)<0:
                    raise ValueError('Receiver AppInst is not ready')
                result['package']=info;result['transfer']=stage(a.host,a.ftp_port,info);save(record,result)
                reply=call(a.host,a.port,key,'install-local',info['title_id']+' '+info['sha256'])
            else:reply=call(a.host,a.port,key,a.command,a.title)
            result['response']=reply
            if reply.get('code',0)!=0 or (a.command=='bgft-status' and reply.get('error',0)!=0):
                result['state']='platform_error'
                raise ValueError('Platform returned an error; see recorded response')
        result['state']='returned'
    except Exception as exc:
        result.update(state='platform_error' if result.get('state')=='platform_error' else 'unconfirmed',error=str(exc))
    result['finished']=now();save(record,result)
    print(json.dumps(dict(record=str(record),**result),indent=2))
    return 2 if 'error' in result else 0

if __name__=='__main__':sys.exit(main())
