#!/usr/bin/env python3
"""Build an isolated fself converter that respects the selected kernel variant."""
from pathlib import Path
import shutil, subprocess
root=Path(__file__).resolve().parents[2]
source=Path('/home/nichlas/ut99-orbis/build/create-fself-host')
dest=root/'build/remote-receiver/fself-src'
shutil.copytree(source,dest,dirs_exist_ok=True,ignore=shutil.ignore_patterns('.git'))
p=dest/'pkg/oelf/OELFGenDynlibData.go'
s=p.read_text()
# The console's libkernel_sys.sprx exports module/library name libkernel.
# Keep that ABI name; select the system variant only in DT_NEEDED filename.
old='"libkernel":                  "libkernel.prx"'
assert old in s
s=s.replace(old, '"libkernel":                  "libkernel_sys.prx"', 1)
p.write_text(s)
subprocess.run(['go','build','-o',str(dest.parent/'create-fself-worker'),'.'],cwd=dest/'cmd/create-fself',check=True)
