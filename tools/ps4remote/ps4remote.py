#!/usr/bin/env python3
"""PS4 LAN development tools v0.1. Python standard library only."""
import argparse
import datetime as dt
import ftplib
import hashlib
import http.server
import json
import os
from pathlib import Path
import re
import socket
import threading
import urllib.error
import urllib.parse
import urllib.request
import uuid

VERSION = '0.1'
DEFAULT_LOGS = {'engine.log': '/data/doom3-client/dudelog.txt',
                'gpu.log': '/data/client-vulkan.log'}
# Never use environment HTTP proxies for the local console.
OPENER = urllib.request.build_opener(urllib.request.ProxyHandler({}))


def now():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def save(path, value):
    path = Path(path)
    tmp = path.with_suffix(path.suffix + '.tmp')
    with tmp.open('w') as f:
        json.dump(value, f, indent=2, ensure_ascii=False)
        f.write('\n'); f.flush(); os.fsync(f.fileno())
    tmp.replace(path)


def sha(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def decode_rpi(raw):
    # Original RPI emits unquoted hexadecimal numbers (not standard JSON).
    # Tokenise quoted strings first so their contents are never rewritten.
    text = raw.decode('utf-8')
    text = re.sub(r'"(?:\\.|[^"\\])*"|\b0x[0-9a-fA-F]+\b',
                  lambda m: str(int(m[0], 16)) if m[0].startswith('0x') else m[0], text)
    result = json.loads(text)
    if not isinstance(result, dict) or result.get('status') != 'success':
        raise ValueError(f'RPI rejected request: {result!r}')
    return result


class RPI:
    def __init__(self, host, port=12800, timeout=10):
        self.base = f'http://{host}:{port}/api/'
        self.timeout = timeout

    def call(self, method, data):
        request = urllib.request.Request(self.base + method,
            json.dumps(data).encode(), {'Content-Type': 'application/json'})
        with OPENER.open(request, timeout=self.timeout) as response:
            raw = response.read(1024*1024 + 1)
        if len(raw) > 1024*1024:
            raise ValueError('RPI response too large')
        return decode_rpi(raw)


def package_info(manifest):
    manifest = Path(manifest).resolve()
    m = json.loads(manifest.read_text())
    package = Path(m['package'])
    if not package.is_absolute(): package = manifest.parent / package
    package = package.resolve(strict=True)
    if not re.fullmatch(r'[A-Z]{4}[0-9]{5}', m['title_id']):
        raise ValueError('Invalid title ID')
    if not re.fullmatch(r'[0-9a-f]{64}', m['sha256']):
        raise ValueError('Invalid SHA-256')
    if package.stat().st_size != m['bytes'] or sha(package) != m['sha256']:
        raise ValueError('Package differs from manifest')
    with package.open('rb') as f:
        if f.read(4) != b'\x7fCNT': raise ValueError('Not a PS4 package')
    return dict(path=str(package), bytes=m['bytes'], sha256=m['sha256'],
                title_id=m['title_id'], version=m.get('version'))


def package_route(info):
    return '/' + info['sha256'] + '/package.pkg'


def package_server(info, bind, port):
    """Serve exactly one opened/verified file, no directory listing or traversal."""
    fd = os.open(info['path'], os.O_RDONLY)
    with os.fdopen(os.dup(fd), 'rb') as f:
        verified = hashlib.file_digest(f, 'sha256').hexdigest()
    if verified != info['sha256']:
        os.close(fd); raise ValueError('Package changed before server opened it')
    initial = os.fstat(fd)
    def unchanged():
        s = os.fstat(fd)
        return (s.st_size, s.st_mtime_ns, s.st_ctime_ns) == (
            initial.st_size, initial.st_mtime_ns, initial.st_ctime_ns)
    class Handler(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.1'
        def setup(self):
            super().setup(); self.connection.settimeout(10)
        def log_message(self, *args): pass
        def do_HEAD(self): self.send_file(False)
        def do_GET(self): self.send_file(True)
        def send_file(self, body):
            if self.path != package_route(info): self.send_error(404); return
            if not unchanged(): self.send_error(409, 'Package changed; restart server'); return
            size = initial.st_size
            begin, end, code = 0, size - 1, 200
            range_header = self.headers.get('Range')
            if range_header:
                match = re.fullmatch(r'bytes=(\d*)-(\d*)', range_header)
                try:
                    if not match or not any(match.groups()): raise ValueError()
                    a, b = match.groups()
                    if a:
                        begin = int(a); end = min(int(b), size - 1) if b else size - 1
                    else:
                        if int(b) <= 0: raise ValueError()
                        begin = max(0, size - int(b))
                    if begin >= size or end < begin: raise ValueError()
                except ValueError:
                    self.send_response(416); self.send_header('Content-Range', f'bytes */{size}')
                    self.send_header('Content-Length', '0'); self.end_headers(); return
                code = 206
            self.send_response(code)
            self.send_header('Connection', 'close'); self.close_connection = True
            self.send_header('Content-Type', 'application/octet-stream')
            self.send_header('Content-Length', str(end-begin+1))
            self.send_header('Accept-Ranges', 'bytes')
            self.send_header('ETag', '"' + info['sha256'] + '"')
            if code == 206: self.send_header('Content-Range', f'bytes {begin}-{end}/{size}')
            self.end_headers()
            if body:
                try:
                    while begin <= end:
                        block = os.pread(fd, min(1024*1024, end-begin+1), begin)
                        if not block or not unchanged(): self.close_connection = True; return
                        self.wfile.write(block); begin += len(block)
                except (BrokenPipeError, ConnectionResetError): pass
    try:
        server = http.server.ThreadingHTTPServer((bind, port), Handler)
    except BaseException:
        os.close(fd); raise
    server.daemon_threads = False
    original_close = server.server_close
    def close():
        original_close(); os.close(fd)
    server.server_close = close
    return server


def collect(host, port, root, expected=None, logs=None):
    folder = Path(root) / (dt.datetime.now(dt.timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '-' + uuid.uuid4().hex[:8])
    folder.mkdir(parents=True)
    report = dict(tool_version=VERSION, host=host, started=now(), snapshot=True,
                  launch_verified=False, files={}, expected_build=expected)
    save(folder/'manifest.json', report)
    try:
        with ftplib.FTP() as ftp:
            ftp.connect(host, port, timeout=15); ftp.login()
            for name, remote in (logs or DEFAULT_LOGS).items():
                if Path(name).name != name or name in ('.', '..'):
                    raise ValueError('Invalid output filename')
                part = folder/(name + '.partial')
                entry = dict(remote=remote, started=now())
                try:
                    with part.open('xb') as out:
                        count = 0
                        def write(block):
                            nonlocal count
                            count += len(block)
                            if count > 64*1024*1024: raise ValueError('Log exceeds 64 MiB limit')
                            out.write(block)
                        ftp.retrbinary('RETR ' + remote, write)
                        out.flush(); os.fsync(out.fileno())
                    part.rename(folder/name)
                    entry.update(bytes=count, sha256=sha(folder/name), complete=True)
                except ftplib.error_perm as exc:
                    entry.update(complete=False, error=str(exc))
                report['files'][name] = entry
                save(folder/'manifest.json', report)
            try: report['coredump_listing'] = ftp.nlst('/data/sce_coredumps')
            except ftplib.error_perm as exc: report['coredump_listing_error'] = str(exc)
    except Exception as exc:
        report['error'] = str(exc)
    text = (folder/'gpu.log').read_text(errors='replace') if (folder/'gpu.log').exists() else ''
    build = re.search(r'\bBUILD ([^\r\n]+)', text)
    report['observed_build'] = build[1] if build else None
    report['build_matches'] = None if expected is None else bool(build and build[1].split()[0] == expected)
    report['complete'] = (not report.get('error') and bool(report['files']) and
        all(e['complete'] for e in report['files'].values()) and report['build_matches'] is not False)
    report['finished'] = now(); save(folder/'manifest.json', report)
    return dict(directory=str(folder), **report)


def doctor(args):
    result = dict(tool_version=VERSION, host=args.host, checked=now(), capabilities={})
    caps = result['capabilities']
    try:
        with ftplib.FTP() as ftp:
            ftp.connect(args.host, args.ftp_port, timeout=3); ftp.login(); ftp.pwd()
        caps['ftp_read'] = True
    except Exception as exc: caps['ftp_read'] = False; result['ftp_error'] = str(exc)
    try:
        reply = RPI(args.host, args.rpi_port, 3).call('is_exists', {'title_id':'DOOM00001'})
        exists = reply.get('exists')
        if type(exists) is not bool and exists not in ('true', 'false'):
            raise ValueError('Unexpected is_exists response')
        caps['rpi'] = True; result['doom_installed'] = exists is True or exists == 'true'
    except Exception as exc: caps['rpi'] = False; result['rpi_error'] = str(exc)
    caps.update(remote_launch=False, controller=False, capture=False)
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host', default='192.168.32.229')
    p.add_argument('--ftp-port', type=int, default=2121)
    p.add_argument('--rpi-port', type=int, default=12800)
    sub = p.add_subparsers(dest='command', required=True)
    sub.add_parser('doctor')
    c = sub.add_parser('collect'); c.add_argument('--out', default='artifacts/remote-runs'); c.add_argument('--expect-build')
    s = sub.add_parser('serve'); s.add_argument('--manifest', required=True); s.add_argument('--bind', required=True); s.add_argument('--port', type=int, default=8089)
    i = sub.add_parser('install'); i.add_argument('--manifest', required=True); i.add_argument('--url', required=True); i.add_argument('--out', default='artifacts/remote-installs')
    s = sub.add_parser('status'); s.add_argument('--task', type=int, required=True)
    args = p.parse_args()
    if args.command == 'doctor': result = doctor(args)
    elif args.command == 'collect':
        result = collect(args.host,args.ftp_port,args.out,args.expect_build)
    elif args.command == 'serve':
        info = package_info(args.manifest)
        server = package_server(info,args.bind,args.port)
        print(json.dumps(dict(url=f'http://{args.bind}:{server.server_port}'+package_route(info),package=info)), flush=True)
        try: server.serve_forever()
        except KeyboardInterrupt: pass
        finally: server.server_close()
        return 0
    elif args.command == 'install':
        info = package_info(args.manifest)
        url = urllib.parse.urlsplit(args.url)
        if url.scheme != 'http' or url.path != package_route(info) or url.query or url.fragment or url.username:
            raise ValueError('Use the exact URL emitted by serve for this package')
        with OPENER.open(urllib.request.Request(args.url,method='HEAD'),timeout=10) as response:
            if int(response.headers.get('Content-Length','-1')) != info['bytes'] or response.headers.get('ETag') != '"'+info['sha256']+'"':
                raise ValueError('Server identity differs from verified package')
        folder = Path(args.out); folder.mkdir(parents=True,exist_ok=True)
        record = folder/(uuid.uuid4().hex+'.json')
        result = dict(started=now(),host=args.host,package=info,url=args.url,state='requesting',installation_verified=False,launch_verified=False)
        save(record,result)
        try:
            reply = RPI(args.host,args.rpi_port,30).call('install',{'type':'direct','packages':[args.url]})
            if type(reply.get('task_id')) is not int or reply['task_id'] < 0: raise ValueError('Missing valid task ID')
            result.update(state='accepted',task_id=reply['task_id'],response=reply)
        except Exception as exc:
            # A lost response can follow a successfully accepted request. Never retry automatically.
            result.update(state='unconfirmed',error=str(exc)); save(record,result); raise
        save(record,result); result['record'] = str(record)
    else:
        if args.task < 0: raise ValueError('Task ID must be nonnegative')
        result = RPI(args.host,args.rpi_port).call('get_task_progress',{'task_id':args.task})
        result['installation_verified'] = False
        result['note'] = 'Raw receiver progress; transferred bytes do not prove installation or launch.'
    print(json.dumps(result,indent=2,ensure_ascii=False))
    return 2 if result.get('complete') is False or result.get('error',0) else 0


if __name__ == '__main__':
    try: raise SystemExit(main())
    except (OSError,ValueError,KeyError,ftplib.Error) as exc:
        print(json.dumps({'error':str(exc)},ensure_ascii=False)); raise SystemExit(2)
