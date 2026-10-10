import struct, unittest
from unittest.mock import patch
import debug_client as d

class Socket:
    def __init__(self,response=b''):self.response=response;self.sent=[]
    def recv(self,n):
        b=self.response[:min(n,3)];self.response=self.response[len(b):];return b
    def sendall(self,b):self.sent.append(b)
    def close(self):pass

def client(response=b''):
    c=d.DebugClient.__new__(d.DebugClient);c.sock=Socket(response);return c
S=struct.pack('<I',d.SUCCESS)
def info(title=b'RMTT00001',pid=214):return S+struct.pack('<I40s64s16s64s',pid,b'worker',b'/app0/remote-worker.self',title,b'content')
def maps(prot=7):return S+struct.pack('<I32sQQQH',1,b'worker',0x1000,0x3000,0,prot)

class Tests(unittest.TestCase):
    def test_fragmented_process_list(self):
        c=client(S+struct.pack('<I32si',1,b'worker',214))
        self.assertEqual(c.processes(),[dict(name='worker',pid=214)])
        self.assertEqual(c.sock.sent,[struct.pack('<III',d.MAGIC,0xBDAA0001,0)])
    def test_bad_count_and_eof(self):
        with self.assertRaises(ValueError):client(S+struct.pack('<I',0xffffffff)).processes()
        with self.assertRaises(EOFError):client(b'xx').receive(4)
    def test_failed_status(self):
        with self.assertRaises(RuntimeError):client(struct.pack('<I',0xf0000001)).processes()
    def test_identity_mismatch(self):
        c=client(info(b'OTHER0001'))
        with self.assertRaises(ValueError):c.identity(214,'RMTT00001')
        self.assertEqual(len(c.sock.sent),1)
    def test_read_range_and_wire(self):
        c=client(maps()+S+b'abcd');self.assertEqual(c.read(214,0x1010,4),b'abcd')
        self.assertEqual(c.sock.sent[-1],struct.pack('<III',d.MAGIC,0xBDAA0002,16)+struct.pack('<IQI',214,0x1010,4))
    def test_unmapped_and_exec_guard(self):
        with self.assertRaises(ValueError):client(maps()).read(214,0x2fff,4)
        c=client(maps()+maps(prot=3))
        with self.assertRaises(ValueError):c.call(214,0x1010,0x1200,[])
        self.assertTrue(all(struct.unpack('<III',b[:12])[1]==0xBDAA0004 for b in c.sock.sent))
    def test_compare_failure_never_writes(self):
        c=client(info()+maps()+maps()+S+b'old!')
        with self.assertRaises(ValueError):c.write(214,'RMTT00001',0x1010,b'new!',b'xxxx')
        self.assertNotIn(0xBDAA0003,[struct.unpack('<III',b[:12])[1] for b in c.sock.sent])
    def test_write_and_readback(self):
        c=client(info()+maps()+maps()+S+b'old!'+S+S+maps()+S+b'new!')
        c.write(214,'RMTT00001',0x1010,b'new!',b'old!')
        self.assertIn(b'new!',c.sock.sent)
    def test_rpc_wire_and_response(self):
        c=client(maps()+maps()+S+struct.pack('<IQ',214,42))
        self.assertEqual(c.call(214,0x1010,0x1200,[1,2]),42)
        header=c.sock.sent[-1][:12];self.assertEqual(struct.unpack('<III',header),(d.MAGIC,0xBDAA0006,68))
        self.assertEqual(struct.unpack('<IQQ6Q',c.sock.sent[-1][12:]),(214,0x1010,0x1200,1,2,0,0,0,0))
    def test_rpc_timeout_not_retried(self):
        c=client(maps()+maps())
        with self.assertRaises(EOFError):c.call(214,0x1010,0x1200,[])
        self.assertEqual(sum(struct.unpack('<III',b[:12])[1]==0xBDAA0006 for b in c.sock.sent),1)
    def test_payload_hash_before_connection(self):
        with patch.object(d.Path,'read_bytes',return_value=b'bad'),patch.object(d.socket,'create_connection') as connect:
            with self.assertRaises(ValueError):d.load_payload('host','bad.bin')
            connect.assert_not_called()

if __name__=='__main__':unittest.main()
