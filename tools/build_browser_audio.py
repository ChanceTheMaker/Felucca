"""Build the hardware-free browser DSP with Zig 0.13 (zig cc)."""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--zig', default=str(ROOT / 'build/wasm-tools/ziglang/zig.exe'))
args = parser.parse_args()
gen = ROOT / 'build/browser-audio/gen'
gen.mkdir(parents=True, exist_ok=True)
for script, header in [('gen_tables.py', 'felucca_tables.h'), ('gen_samples.py', 'felucca_samples.h'), ('gen_fm6_patches.py', 'felucca_fm6.h')]:
    subprocess.run([sys.executable, str(ROOT / 'tools' / script), str(gen / header)], cwd=ROOT, check=True)
subprocess.run([args.zig, 'cc', '-target', 'wasm32-freestanding', '-O2', '-fno-builtin',
                '-fwrapv', '-nostdlib', '-Wl,--no-entry',
                *['-Wl,--export=' + name for name in ['synth_target', 'synth_select', 'synth_step', 'synth_motion_clear', 'synth_motion_event', 'synth_motion_on', 'synth_transport', 'synth_playing', 'synth_position', 'synth_init', 'synth_engine', 'synth_param', 'synth_global', 'synth_midi', 'synth_render', 'synth_fm6_buffer', 'synth_fm6_apply', 'engine_count', 'param_count', 'preset_count', 'preset_name', 'preset_value']],
                '-Wl,-z,stack-size=0x40000', '-Wl,--initial-memory=0x800000', '-Wl,--max-memory=0x800000',
                '-I' + str(gen), str(ROOT / 'web/audio/engine.c'),
                '-o', str(ROOT / 'web/audio/engine.wasm')], cwd=ROOT, check=True)
print('Built web/audio/engine.wasm')
