#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,os,secrets,shutil,subprocess,xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[2]
sdk=Path(os.environ.get('OO_PS4_TOOLCHAIN','/opt/openorbis/OpenOrbis/PS4Toolchain'))
out=root/'build/remote-receiver'; stage=out/'stage'; art=root/'artifacts/remote-receiver'
for p in [out,stage/'sce_sys',art,root/'dist']:p.mkdir(parents=True,exist_ok=True)
def run(*args,**kw):subprocess.run([str(a) for a in args],check=True,**kw)
secret=art/'receiver.key'
if not secret.exists():
    with secret.open('x') as f:f.write(secrets.token_hex(32))
    secret.chmod(0o600)
run('clang++','--target=x86_64-pc-freebsd12-elf','--sysroot='+str(sdk),'-fPIC','-D__PS4__','-D_BSD_SOURCE','-std=c++17','-O2','-nostdinc++','-isystem',sdk/'include','-isystem',sdk/'include/c++/v1','-c',root/'tools/ps4remote/receiver/main.cpp','-o',out/'main.o')
run('clang','--target=x86_64-pc-freebsd12-elf','-fPIC','-O2','-ffreestanding','-fno-stack-protector','-mno-red-zone','-mgeneral-regs-only','-c',root/'tools/ps4remote/receiver/credentials.c','-o',out/'credentials.o')
run('ld.lld','-m','elf_x86_64','-pie','--script='+str(sdk/'link.x'),'--eh-frame-hdr','-z','max-page-size=0x4000','-L'+str(sdk/'lib'),sdk/'lib/crt1.o',out/'main.o',out/'credentials.o','-lc++','-lc++abi','-lunwind','-lc','-lSceNet','-lSceSysmodule','-lSceSystemService','-lSceUserService','-lkernel','-o',out/'receiver.elf')
run('/home/nichlas/ut99-orbis/build/create-fself-current','-in='+str(out/'receiver.elf'),'-out='+str(out/'receiver.oelf'),'--eboot',stage/'eboot.bin','--paid','0x3800000000000010',env=dict(os.environ,OO_PS4_TOOLCHAIN=str(sdk)))
run('clang++','--target=x86_64-pc-freebsd12-elf','--sysroot='+str(sdk),'-fPIC','-D__PS4__','-D_BSD_SOURCE','-std=c++17','-O2','-DREMOTE_WORKER=1','-nostdinc++','-isystem',sdk/'include','-isystem',sdk/'include/c++/v1','-c',root/'tools/ps4remote/receiver/main.cpp','-o',out/'worker.o')
run('ld.lld','-m','elf_x86_64','-pie','--script='+str(sdk/'link.x'),'--eh-frame-hdr','-z','max-page-size=0x4000','-L'+str(sdk/'lib'),sdk/'lib/crt1.o',out/'worker.o',out/'credentials.o','-lc++','-lc++abi','-lunwind','-lc','-lSceNet','-lSceSysmodule','-lSceSystemService','-lSceUserService','-lkernel','-o',out/'worker.elf')
run('python3',root/'tools/ps4remote/build-worker-fself.py')
run(out/'create-fself-worker','-in='+str(out/'worker.elf'),'-out='+str(out/'worker.oelf'),'--eboot',stage/'remote-worker.self','--paid','0x3800000000000010',env=dict(os.environ,OO_PS4_TOOLCHAIN=str(sdk)))
files=['eboot.bin','receiver.key','sce_sys/icon0.png','sce_sys/param.sfo','sce_sys/about/right.sprx','sce_module/libc.prx','sce_module/libSceFios2.prx']
shutil.copyfile(secret,stage/'receiver.key')
shutil.copyfile(root/'tools/ps4remote/receiver/assets/icon0.png',stage/'sce_sys/icon0.png')
for name in files[4:]:
    (stage/name).parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(sdk/'samples/SDL2'/name,stage/name)
files.append('remote-worker.self')
content='IV0000-RMTT00001_00-PS4REMOTE0000000';sfo=stage/'sce_sys/param.sfo';sfo.unlink(missing_ok=True)
tool=sdk/'bin/linux/PkgTool.Core'
run(tool,'sfo_new',sfo)
for k,v in dict(APP_TYPE=1,ATTRIBUTE=0,DOWNLOAD_DATA_SIZE=0,SYSTEM_VER=0).items():run(tool,'sfo_setentry',sfo,k,'--type','Integer','--maxsize',4,'--value',v)
for k,n,v in [('APP_VER',8,'00.24'),('VERSION',8,'00.24'),('CATEGORY',4,'gde'),('CONTENT_ID',48,content),('TITLE_ID',12,'RMTT00001'),('TITLE',128,'PS4 Remote Receiver 0.24')]:run(tool,'sfo_setentry',sfo,k,'--type','Utf8','--maxsize',n,'--value',v)
run(sdk/'bin/linux/create-gp4','-out','pkg.gp4','--content-id='+content,'--files',' '.join(files),cwd=stage)
tree=ET.parse(stage/'pkg.gp4');rd=tree.getroot().find('rootdir');rd.clear()
for name in files:
    node=rd
    for part in Path(name).parts[:-1]:
        child=next((c for c in node if c.get('targ_name')==part),None)
        if child is None:child=ET.SubElement(node,'dir',targ_name=part)
        node=child
ET.indent(tree);tree.write(stage/'pkg.gp4',encoding='utf-8',xml_declaration=True)
run(tool,'pkg_build','pkg.gp4','.',cwd=stage)
pkg=root/'dist/PS4-Remote-Receiver-0.24.pkg';shutil.copyfile(stage/(content+'.pkg'),pkg)
with (art/'validation.log').open('w') as log:run(tool,'pkg_validate','--verbose',pkg,stdout=log,stderr=subprocess.STDOUT)
if '[OK]' not in (art/'validation.log').read_text():raise RuntimeError('Package validation failed')
m=dict(package=str(pkg),sha256=hashlib.sha256(pkg.read_bytes()).hexdigest(),bytes=pkg.stat().st_size,title_id='RMTT00001',version='00.24')
(art/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');print(json.dumps(m))
