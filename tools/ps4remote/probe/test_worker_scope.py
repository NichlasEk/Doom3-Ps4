import unittest
from worker_scope import WorkerScope,BASELINE,ELEVATED
class NoWrite:
    def send(self,*args):raise AssertionError('Unexpected write')
class ScopeTests(unittest.TestCase):
    def scope(self,observed):
        s=WorkerScope.__new__(WorkerScope);s.c=NoWrite();s.proc=1;s.cred=2;s.observe=lambda:observed;return s
    def test_reused_pid_credential_rejected(self):
        with self.assertRaises(ValueError):self.scope((1,3,BASELINE)).elevate()
    def test_changed_bits_rejected(self):
        with self.assertRaises(ValueError):self.scope((1,2,(BASELINE[0],0,BASELINE[2]))).elevate()
    def test_restored_is_noop(self):self.scope((1,2,BASELINE)).restore()
    def test_arbitrary_transition_rejected(self):
        with self.assertRaises(ValueError):self.scope((1,2,BASELINE)).transition(BASELINE,(0,0,0))
if __name__=='__main__':unittest.main()
