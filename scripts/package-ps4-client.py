#!/usr/bin/env python3
"""Package the finite graphical diagnostic and explicitly supplied retail data."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-data', required=True, type=Path, help='Owned data directory containing base/pak000.pk4 through pak004.pk4')
parser.add_argument('--skip-build', action='store_true', help='Package the already built, tested client')
args = parser.parse_args()
sdk = Path(os.environ.get('OO_PS4_TOOLCHAIN', '/opt/openorbis/OpenOrbis/PS4Toolchain'))
pkgtool, gp4tool = sdk/'bin/linux/PkgTool.Core', sdk/'bin/linux/create-gp4'
stage, dist = root/'build/client-package', root/'dist'
artifacts = root/'artifacts/package'
for directory in (stage, dist, artifacts):
    directory.mkdir(parents=True, exist_ok=True)
if not args.skip_build:
    subprocess.run([str(root/'scripts/build-ps4-client.sh')], check=True)
elf = root/'build/ps4-client/dude'
needed = set(re.findall(r'\(NEEDED\).*?\[(.*?)\]', subprocess.check_output(['readelf', '-d', str(elf)], text=True)))
allowed = {'libkernel.so', 'libSceGnmDriver.so', 'libSceUserService.so', 'libScePad.so',
           'libSceAudioOut.so', 'libSceVideoOut.so', 'libSceNet.so'}
if not needed or needed - allowed:
    raise SystemExit('Unexpected runtime imports: '+repr(needed-allowed))

files = {}
def stage_file(source, name):
    source = Path(source).resolve(strict=True)
    dest = stage/name
    dest.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['cp', '--reflink=auto', '--', str(source), str(dest)], check=True)
    files[name] = source

stage_file(root/'build/client-runtime/eboot.bin', 'eboot.bin')
stage_file(root/'assets/icon0.png', 'sce_sys/icon0.png')
icon = (stage/'sce_sys/icon0.png').read_bytes()
if (icon[:8] != b'\x89PNG\r\n\x1a\n' or icon[24:26] != bytes([8,2]) or
    tuple(int.from_bytes(icon[i:i+4], 'big') for i in (16,20)) != (512,512)):
    raise SystemExit('Icon must be a 512x512 opaque 8-bit RGB PNG')
# Same SDK loader support files as the locally tested UT99 package recipe.
for name in ('sce_module/libc.prx', 'sce_module/libSceFios2.prx', 'sce_sys/about/right.sprx'):
    stage_file(sdk/'samples/SDL2'/name, name)
for source, name in [
    (root/'build/dude-client-source/COPYING.txt', 'DUDE-COPYING.txt'),
    (root/'build/native/opengnm/LICENSE', 'OpenGNM-LICENSE'),
    (root/'build/native/opengnm-psbc/LICENSE', 'PSBC-LICENSE'),
    (root/'build/native/vulkan-ps4/LICENSE', 'Vulkan-PS4-LICENSE'),
    (root/'thirdparty/ps4-native/README.md', 'Graphics-NOTICES.md'),
]:
    stage_file(source, 'notices/'+name)
for i in range(5):
    name = f'base/pak{i:03}.pk4'
    stage_file(args.game_data/name, name)

content_id = 'IV0000-DM3P00001_00-DOOM3PS4MENUTEST'
title_id = 'DM3P00001'
sfo = stage/'sce_sys/param.sfo'
sfo.unlink(missing_ok=True)
def tool(*arguments, cwd=None):
    subprocess.run([str(pkgtool), *map(str,arguments)], cwd=cwd, check=True)
tool('sfo_new', sfo)
for key, value in {'APP_TYPE':1, 'ATTRIBUTE':0, 'DOWNLOAD_DATA_SIZE':0, 'SYSTEM_VER':0}.items():
    tool('sfo_setentry', sfo, key, '--type', 'Integer', '--maxsize', 4, '--value', value)
for key, size, value in [('APP_VER',8,'00.01'), ('VERSION',8,'00.01'), ('CATEGORY',4,'gd'),
                          ('CONTENT_ID',48,content_id), ('TITLE_ID',12,title_id),
                          ('TITLE',128,'Doom 3 PS4 - Menu Test')]:
    tool('sfo_setentry', sfo, key, '--type', 'Utf8', '--maxsize', size, '--value', value)
files['sce_sys/param.sfo'] = sfo
subprocess.run([str(gp4tool), '-out', 'pkg.gp4', '--content-id='+content_id,
                '--files', ' '.join(files)], cwd=stage, check=True)
project = stage/'pkg.gp4'
tree = ET.parse(project)
rootdir = tree.getroot().find('rootdir')
rootdir.clear()
for name in files:
    node = rootdir
    for part in PurePosixPath(name).parts[:-1]:
        child = next((item for item in node if item.get('targ_name') == part), None)
        if child is None:
            child = ET.SubElement(node, 'dir', targ_name=part)
        node = child
ET.indent(tree)
tree.write(project, encoding='utf-8', xml_declaration=True)
with (artifacts/'pkg-build.log').open('w') as log:
    subprocess.run([str(pkgtool), 'pkg_build', 'pkg.gp4', '.'], cwd=stage, stdout=log, stderr=subprocess.STDOUT, check=True)
package = dist/'Doom3-PS4-Menu-Test-0.01.pkg'
shutil.copyfile(stage/(content_id+'.pkg'), package)
with (artifacts/'pkg-validate.log').open('w') as log:
    subprocess.run([str(pkgtool), 'pkg_validate', '--verbose', str(package)], stdout=log, stderr=subprocess.STDOUT, check=True)
validation=(artifacts/'pkg-validate.log').read_text()
if '[OK]' not in validation or re.search(r'\b(?:FAIL|FAILED|ERROR)\b',validation,re.I):
    raise SystemExit('Package validation failed; inspect artifacts/package/pkg-validate.log')
def sha(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()
checksum = sha(package)
(package.with_suffix('.pkg.sha256')).write_text(f'{checksum}  {package.name}\n')
manifest = dict(package=str(package), sha256=checksum, bytes=package.stat().st_size,
    title_id=title_id, version='00.01', engine_elf_sha256=sha(elf),
    source_revision=subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip(),
    contains_owned_retail_data=True, physical_ps4_tested=False,
    files={name:sha(stage/name) for name in files})
(artifacts/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({key:manifest[key] for key in ('package','sha256','bytes','title_id')},indent=2))
