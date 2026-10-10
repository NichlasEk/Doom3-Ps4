from pathlib import Path
import subprocess,os,time,urllib.request,json,signal,sys
worker="--worker" in sys.argv
port=12802 if worker else 12801
role="worker" if worker else "receiver"
root=Path(__file__).resolve().parents[2];art=root/'artifacts/remote-receiver';profile=root/'build/receiver-profile'
(profile/'shadPS4/data').mkdir(parents=True,exist_ok=True)
key=(art/'receiver.key').read_text()
with (art/('emulator-'+role+'.log')).open('w') as log:
 p=subprocess.Popen(['/home/nichlas/ScummVM-PS4/.tools/shadps4/Shadps4-sdl.AppImage','--fullscreen','false',str(root/'build/remote-receiver/stage'/('remote-worker.self' if worker else 'eboot.bin'))],stdout=log,stderr=log,env=dict(os.environ,XDG_DATA_HOME=str(profile),SDL_VIDEODRIVER='x11'),start_new_session=True)
 try:
  for _ in range(40):
   try:
    with urllib.request.urlopen(f'http://127.0.0.1:{port}/v1/health',timeout=1) as r:result=json.load(r)
    assert result['protocol']=='ps4remote-1'
    assert result['role']==role
    assert result['launch_attempts']==0 and result['launch_limit']==10
    (art/('emulator-'+role+'-health.json')).write_text(json.dumps(result,indent=2)+'\n')
    checks=[]
    for body,auth,expected in [('DOOM00001','wrong',403),('DOOM00001',key,503),('../bad',key,400)]:
     req=urllib.request.Request(f'http://127.0.0.1:{port}/v1/exists',body.encode(),{'X-PS4Remote-Key':auth,'Content-Type':'text/plain'})
     try:
      with urllib.request.urlopen(req,timeout=3) as r:code=r.status
     except urllib.error.HTTPError as e:code=e.code;e.close()
     assert code==expected,(code,expected)
     checks.append(code)
    for mode in ['resolve','direct','sandbox','internal','bgft','bgft-init','bgft-public-init','dep-system','dep-user','dep-netctl','dep-http','dep-ssl','dep-np','ipc-connect','credentials','caps-test','system-test','lnc-init','app-slots','worker-register']:
     req=urllib.request.Request(f'http://127.0.0.1:{port}/v1/probe',mode.encode(),{'X-PS4Remote-Key':key})
     with urllib.request.urlopen(req,timeout=5) as r: probe=json.load(r)
     assert probe['probe']==mode and probe['app_init']<0,probe
     with urllib.request.urlopen(f'http://127.0.0.1:{port}/v1/health',timeout=2) as r: assert json.load(r)['version']=='0.24'
    for endpoint,body,auth,expected in [('launch-permission-probe','RMTS99999','wrong',403),('launch-permission-probe','RMTS00002',key,400),('launch-permission-probe','RMTS99999',key,503 if worker else 400),('launch-async-result','RMTS00002',key,503 if worker else 400),('launch-async-control','RMTS00002','wrong',403),('launch-async-control','DOOM00001',key,400),('launch-async-control','RMTS00002',key,503 if worker else 400),('launch-async-scoped','RMTS00002',key,503 if worker else 400),('worker-start','start','wrong',403),('worker-start','bad',key,400),('worker-start','start',key,400 if worker else 503),('launch-test','RMTS00002','wrong',403),('launch-test','DOOM00001',key,400),('launch-test','RMTS00002',key,503),('bgft-register','DOOM00001 '+'0'*64,'wrong',403),('bgft-register','DOOM00001 '+'0'*64,key,503),('bgft-start','0',key,400),('bgft-status','0',key,400),('probe','unlisted',key,400)]:
     req=urllib.request.Request(f'http://127.0.0.1:{port}/v1/'+endpoint,body.encode(),{'X-PS4Remote-Key':auth})
     try:
      with urllib.request.urlopen(req,timeout=3) as r:code=r.status
     except urllib.error.HTTPError as e:code=e.code;e.close()
     assert code==expected,(endpoint,code,expected)
    result['bgft_guard_checks']=[403,503,400,400,400]
    result['probe_checks']=['resolve','direct','sandbox','internal','bgft','bgft-init','bgft-public-init','dep-system','dep-user','dep-netctl','dep-http','dep-ssl','dep-np','ipc-connect','credentials','caps-test','system-test','lnc-init','app-slots','worker-register']
    result['http_checks']=checks
    (art/('emulator-'+role+'-health.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));break
   except OSError:time.sleep(.5)
  else:raise RuntimeError('Receiver did not answer in emulator')
 finally:
  os.killpg(p.pid,signal.SIGTERM)
  try:p.wait(timeout=5)
  except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait()
