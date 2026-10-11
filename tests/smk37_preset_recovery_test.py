# SPDX-License-Identifier: GPL-3.0-only
import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from smk37_preset_recovery import reset_plan,SECTORS,SIZE
class PresetRecovery(unittest.TestCase):
    def test_only_presets_reset(self):
        stock=bytes([255])*SIZE;current=bytearray(stock)
        current[0x4000]=1;current[0xeb000]=2
        for a in SECTORS:current[a:a+4096]=bytes([33])*4096
        result,addresses=reset_plan(bytes(current),stock)
        self.assertEqual(addresses,list(SECTORS))
        for i in range(SIZE):self.assertEqual(result[i],255 if any(a<=i<a+4096 for a in SECTORS) else current[i])
    def test_protected_changes_refused(self):
        stock=bytes([255])*SIZE
        for address in (0,0xc1000,0xf4000,0xff000):
            current=bytearray(stock);current[address]=0
            with self.assertRaises(ValueError):reset_plan(bytes(current),stock)
    def test_wrong_size_or_nonblank_stock_refused(self):
        stock=bytes([255])*SIZE
        with self.assertRaises(ValueError):reset_plan(stock[:-1],stock)
        altered=bytearray(stock);altered[SECTORS[0]]=0
        with self.assertRaises(ValueError):reset_plan(bytes(altered),bytes(altered))
    def test_blank_is_noop(self):
        stock=bytes([255])*SIZE
        self.assertEqual(reset_plan(stock,stock),(stock,[]))
if __name__=='__main__':unittest.main()
