# SPDX-License-Identifier: GPL-3.0-only
import importlib.util,tempfile,unittest,zipfile,json
from pathlib import Path
spec=importlib.util.spec_from_file_location('backup',Path(__file__).resolve().parents[1]/'tools/smk37_storage_backup.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
class Backup(unittest.TestCase):
 def test_roundtrip_and_rejections(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d);source=bytes(range(256))*4096;(p/'a').write_bytes(source);(p/'b').write_bytes(source)
   b.export(p/'a',p/'b',p/'stock.zip');formatted=bytearray(source)
   for _,lo,hi in b.REGIONS:formatted[lo:hi]=bytes([255])*(hi-lo)
   (p/'formatted').write_bytes(formatted);b.restore(p/'stock.zip',p/'formatted',p/'restored');self.assertEqual((p/'restored').read_bytes(),source)
   with self.assertRaises(FileExistsError):b.restore(p/'stock.zip',p/'formatted',p/'restored')
   formatted[0xc1000]^=1;(p/'wrong').write_bytes(formatted)
   with self.assertRaises(ValueError):b.restore(p/'stock.zip',p/'wrong',p/'bad')
   with self.assertRaises(ValueError):b.export(p/'a',p/'wrong',p/'bad.zip')
   with zipfile.ZipFile(p/'stock.zip') as z:
    entries={n:z.read(n) for n in z.namelist()}
   payload=bytearray(entries['stock-vm.bin']);payload[0]^=1;entries['stock-vm.bin']=payload
   with zipfile.ZipFile(p/'corrupt.zip','w') as z:
    for n,v in entries.items():z.writestr(n,v)
   with self.assertRaises(ValueError):b.restore(p/'corrupt.zip',p/'a',p/'bad')
   self.assertFalse((p/'bad').exists());self.assertEqual((p/'a').read_bytes(),source)
if __name__=='__main__':unittest.main()
