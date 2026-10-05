# SPDX-License-Identifier: GPL-3.0-only
"""SMK selection must stop before SDK access, build output or FM-1 packaging."""
import importlib.util
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('felucca_build', ROOT / 'tools/build.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


class TargetTest(unittest.TestCase):
    def test_smk_stops_before_sdk_and_generators(self):
        with patch.object(sys, 'argv', ['build.py', '--board', 'smk37']), \
                patch.object(build.fm1pkg_make, 'sdk_file') as sdk, \
                patch.object(build, 'generate') as generate, \
                patch.object(build.fm1pkg_make, 'ufw') as package:
            with self.assertRaises(SystemExit) as error:
                build.main()
            self.assertEqual(error.exception.code, 2)
            sdk.assert_not_called()
            generate.assert_not_called()
            package.assert_not_called()

    def test_default_and_explicit_fm1_reach_same_existing_builder(self):
        for args in ([], ['--board', 'fm1']):
            with patch.object(sys, 'argv', ['build.py', *args]), \
                    patch.object(build.fm1pkg_make, 'sdk_file', side_effect=RuntimeError('SDK reached')) as sdk:
                with self.assertRaisesRegex(RuntimeError, 'SDK reached'):
                    build.main()
                sdk.assert_called_once_with(next(iter(build.SDK_SHA256)))


if __name__ == '__main__':
    unittest.main()
