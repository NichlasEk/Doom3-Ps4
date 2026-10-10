#!/usr/bin/env python3
"""Fetch the pinned, unmodified upstream release; verify before saving."""
import hashlib,json,urllib.request
from pathlib import Path
from debug_client import RELEASE_SHA
URL='https://github.com/Pharaoh2k/ps4debug-NG/releases/download/1.3.2/ps4debug-ng_v1.3.2_release_2026-09-20.bin'
DEST=Path('artifacts/ps4-probe/ps4debug-ng-1.3.2.bin')
def main():
    data=urllib.request.urlopen(URL,timeout=30).read(8*1024*1024)
    digest=hashlib.sha256(data).hexdigest()
    if digest!=RELEASE_SHA:raise ValueError('Release checksum mismatch')
    DEST.parent.mkdir(parents=True,exist_ok=True)
    part=DEST.with_suffix('.partial');part.write_bytes(data);part.replace(DEST)
    print(json.dumps(dict(path=str(DEST),bytes=len(data),sha256=digest)))
if __name__=='__main__':main()
