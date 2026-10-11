# SPDX-License-Identifier: GPL-3.0-only
import importlib.util,unittest
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('bootstrap',Path(__file__).resolve().parents[1]/'tools/smk37_bootstrap.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class BootstrapTests(unittest.TestCase):
    def setUp(self):
        self.stock=bytes([255])*m.SIZE
        old=bytearray(self.stock);old[0x4100:0x4110]=b'old-app-content!';self.old=bytes(old)
        new=bytearray(self.stock);new[0x4100:0x4110]=b'new-app-content!';self.new=bytes(new)
        self.patches=[patch.object(m,'STOCK_SHA',m.sha(self.stock)),patch.object(m,'PREVIOUS_SHA',m.sha(self.old)),patch.object(m,'TEST_SHA',m.sha(self.new))]
        for p in self.patches:p.start()
        self.addCleanup(lambda:[p.stop() for p in self.patches])
        current=bytearray(self.old);current[0xeb000:0xf3000]=bytes(range(256))*128;self.current=bytes(current)
    def test_preserve_saves_and_rollback(self):
        target,sectors=m.plan(self.current,self.stock,self.new,'install',self.old)
        self.assertEqual(sectors,[0x4000]);self.assertEqual(target[m.END:],self.current[m.END:])
        restored,_=m.plan(target,self.stock,self.new,'restore',self.old)
        self.assertEqual(restored,self.current)
    def test_non_save_tail_rejected(self):
        for a in (0,0x9d000,0xc1000,0xf3000,0xff000):
            data=bytearray(self.current);data[a]^=1
            with self.assertRaises(ValueError):m.plan(bytes(data),self.stock,self.new,'install',self.old)
    def test_unknown_app_rejected(self):
        data=bytearray(self.current);data[0x7000]^=1
        with self.assertRaises(ValueError):m.plan(bytes(data),self.stock,self.new,'install',self.old)
    def test_corrupt_candidate_rejected(self):
        data=bytearray(self.new);data[0x7000]^=1
        with self.assertRaises(ValueError):m.plan(self.current,self.stock,bytes(data),'install',self.old)
    def test_apply_protected_range(self):
        target,sectors=m.plan(self.current,self.stock,self.new,'install',self.old)
        class IO:
            def __init__(self,data):self.data=bytearray(data);self.writes=[]
            def read(self,a,n):return bytes(self.data[a:a+n])
            def write(self,a,data):self.writes.append(a);self.data[a:a+len(data)]=data;return 'OK'
        io=IO(self.current);self.assertEqual(m.apply(io,self.current,target,sectors,lambda x:None),target)
        self.assertEqual(io.writes,[0x4000])
        io=IO(self.current)
        with self.assertRaises(ValueError):m.apply(io,self.current,target,sectors+[0xeb000],lambda x:None)
        self.assertEqual(io.writes,[])
if __name__=='__main__':unittest.main()
