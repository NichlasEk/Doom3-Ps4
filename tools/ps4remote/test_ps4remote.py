import concurrent.futures
import hashlib
import http.server
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch
import urllib.error
import urllib.request
import ps4remote as r

class Tests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.root = Path(self.temp.name)
        self.pkg = self.root/'test.pkg'; self.data = b'\x7fCNT' + bytes(range(256))*1024
        self.pkg.write_bytes(self.data)
        self.manifest = self.root/'manifest.json'
        self.manifest.write_text(json.dumps(dict(package=str(self.pkg),sha256=hashlib.sha256(self.data).hexdigest(),bytes=len(self.data),title_id='DOOM00001',version='00.13')))
        self.info = r.package_info(self.manifest)
        self.server = r.package_server(self.info,'127.0.0.1',0)
        self.thread = threading.Thread(target=self.server.serve_forever); self.thread.start()
        self.url = f'http://127.0.0.1:{self.server.server_port}'+r.package_route(self.info)
    def tearDown(self):
        self.server.shutdown(); self.server.server_close(); self.thread.join(); self.temp.cleanup()
    def fetch(self,range=None,method='GET',url=None):
        req = urllib.request.Request(url or self.url,method=method,headers={'Range':range} if range else {})
        with r.OPENER.open(req,timeout=3) as response:return response.status,dict(response.headers),response.read()
    def test_full_and_head(self):
        code,h,b=self.fetch(); self.assertEqual(b,self.data); self.assertEqual(code,200)
        code,h,b=self.fetch(method='HEAD'); self.assertEqual(b,b''); self.assertEqual(int(h['Content-Length']),len(self.data))
    def test_ranges_and_parallel(self):
        for spec,expected in [('bytes=0-3',self.data[:4]),('bytes=-13',self.data[-13:]),('bytes=100-',self.data[100:]),('bytes=0-9999999',self.data)]:
            code,h,b=self.fetch(spec); self.assertEqual(code,206); self.assertEqual(b,expected)
        with concurrent.futures.ThreadPoolExecutor() as pool:
            parts=list(pool.map(lambda _:self.fetch('bytes=40-90')[2],range(12)))
        self.assertTrue(all(p==self.data[40:91] for p in parts))
    def test_reject_ranges_paths_and_changed_package(self):
        for spec in ['bytes=9-2','bytes=-0','bytes=0-1,4-8','bytes=999999-']:
            with self.assertRaises(urllib.error.HTTPError) as c:self.fetch(spec)
            self.assertEqual(c.exception.code,416); c.exception.close()
        with self.assertRaises(urllib.error.HTTPError) as c:self.fetch(url=self.url+'/../manifest.json')
        self.assertEqual(c.exception.code,404); c.exception.close()
        self.pkg.write_bytes(b'changed')
        with self.assertRaises(urllib.error.HTTPError) as c:self.fetch()
        self.assertEqual(c.exception.code,409); c.exception.close()
        with self.assertRaises(ValueError):r.package_info(self.manifest)
    def test_protocol_hex_and_failures(self):
        self.assertEqual(r.decode_rpi(b'{"status":"success","bits":0xF,"name":"0xB"}')['bits'],15)
        self.assertEqual(r.decode_rpi(b'{"status":"success","name":"0xB"}')['name'],'0xB')
        for raw in [b'{"status":"fail","error_code":0xFF}',b'[]',b'<html>']:
            with self.assertRaises(ValueError):r.decode_rpi(raw)
    def test_install_exchange(self):
        received=[]
        class Receiver(http.server.BaseHTTPRequestHandler):
            def log_message(self,*args):pass
            def do_POST(self):
                received.append((self.path,json.loads(self.rfile.read(int(self.headers['Content-Length'])))))
                body=b'{"status":"success","task_id":42}'
                self.send_response(200);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
        api=http.server.ThreadingHTTPServer(('127.0.0.1',0),Receiver)
        t=threading.Thread(target=api.serve_forever);t.start()
        try:
            proc=subprocess.run([sys.executable,str(Path(r.__file__)),'--host','127.0.0.1','--rpi-port',str(api.server_port),'install','--manifest',str(self.manifest),'--url',self.url,'--out',str(self.root/'installs')],capture_output=True,text=True,timeout=10)
            self.assertEqual(proc.returncode,0,proc.stdout+proc.stderr)
            result=json.loads(proc.stdout);self.assertEqual(result['task_id'],42)
            self.assertFalse(result['installation_verified']);self.assertFalse(result['launch_verified'])
            self.assertEqual(received,[('/api/install',{'type':'direct','packages':[self.url]})])
            self.assertEqual(json.loads(Path(result['record']).read_text())['state'],'accepted')
        finally:api.shutdown();api.server_close();t.join()
    def test_collect_identity_and_unique_runs(self):
        class FTP:
            def __enter__(self):return self
            def __exit__(self,*a):pass
            def connect(self,*a,**kw):pass
            def login(self):pass
            def retrbinary(self,cmd,callback):callback(b'BUILD 0.13 graphics=test\n')
            def nlst(self,path):return []
        with patch.object(r.ftplib,'FTP',FTP):
            a=r.collect('fake',2121,self.root/'runs','0.13')
            b=r.collect('fake',2121,self.root/'runs','0.12')
        self.assertTrue(a['complete']);self.assertFalse(b['complete'])
        self.assertNotEqual(a['directory'],b['directory'])
        self.assertEqual(a['files']['gpu.log']['sha256'],hashlib.sha256(b'BUILD 0.13 graphics=test\n').hexdigest())

if __name__=='__main__':unittest.main()
