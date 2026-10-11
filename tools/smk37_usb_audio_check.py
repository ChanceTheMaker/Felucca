#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""List Windows recording inputs or capture an explicitly named Felucca input.

No default microphone fallback. This tests Windows' recording path, which may
resample; it is not proof of the USB endpoint's physical sample rate.
"""
import argparse,array,ctypes as C,json,math,sys,time,wave
from pathlib import Path
from ctypes import wintypes as W

class Caps(C.Structure):
    _fields_=[('manufacturer',W.WORD),('product',W.WORD),('version',W.DWORD),('name',W.WCHAR*32),('formats',W.DWORD),('channels',W.WORD),('reserved',W.WORD)]
class Format(C.Structure):
    _pack_=2
    _fields_=[('tag',W.WORD),('channels',W.WORD),('rate',W.DWORD),('bytes_sec',W.DWORD),('align',W.WORD),('bits',W.WORD),('extra',W.WORD)]
class Header(C.Structure):
    _fields_=[('data',C.c_void_p),('length',W.DWORD),('recorded',W.DWORD),('user',C.c_size_t),('flags',W.DWORD),('loops',W.DWORD),('next',C.c_void_p),('reserved',C.c_size_t)]
def analyze(path):
    with wave.open(str(path),'rb') as f:
        if f.getsampwidth()!=2 or f.getcomptype()!='NONE':raise ValueError('Expected uncompressed PCM16')
        channels,rate,frames=f.getnchannels(),f.getframerate(),f.getnframes();samples=array.array('h',f.readframes(frames))
    if sys.byteorder!='little':samples.byteswap()
    report={'file':str(path),'rate':rate,'channels':channels,'frames':frames,'seconds':frames/rate,'path_may_resample':True,'signal':[]}
    for ch in range(channels):
        values=samples[ch::channels];peak=max(map(abs,values),default=0);rms=math.sqrt(sum(x*x for x in values)/max(1,len(values)))
        run=longest=0
        for x in values:
            run=run+1 if x==0 else 0;longest=max(longest,run)
        windows=[max(map(abs,values[i:i+rate]),default=0) for i in range(0,len(values),rate)]
        report['signal'].append({'channel':ch+1,'peak':peak,'rms_dbfs':round(20*math.log10(rms/32768),2) if rms else None,'clipped_samples':sum(abs(x)>=32767 for x in values),'longest_exact_silence_seconds':round(longest/rate,4),'one_second_windows':len(windows),'windows_with_signal':sum(p>32 for p in windows),'minimum_window_peak':min(windows,default=0)})
    report['non_silent']=any(s['peak']>32 for s in report['signal'])
    return report
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--list',action='store_true');p.add_argument('--device');p.add_argument('--rate',type=int,choices=[44100,48000],default=48000);p.add_argument('--seconds',type=int,default=8);p.add_argument('--output',type=Path);p.add_argument('--analyze',type=Path);a=p.parse_args()
    if a.analyze:print(json.dumps(analyze(a.analyze),indent=2));return
    if sys.platform!='win32':p.error('Windows capture only; --analyze works on other systems')
    mm=C.WinDLL('winmm');mm.waveInGetNumDevs.restype=W.UINT
    mm.waveInGetDevCapsW.argtypes=[C.c_size_t,C.POINTER(Caps),W.UINT]
    mm.waveInOpen.argtypes=[C.POINTER(C.c_void_p),W.UINT,C.POINTER(Format),C.c_size_t,C.c_size_t,W.DWORD]
    for name in ('waveInPrepareHeader','waveInUnprepareHeader','waveInAddBuffer'):
        getattr(mm,name).argtypes=[C.c_void_p,C.POINTER(Header),W.UINT]
    for name in ('waveInStart','waveInReset','waveInClose'):getattr(mm,name).argtypes=[C.c_void_p]
    def check(rc):
        if rc:raise RuntimeError(f'Windows audio error {rc}')
    devices=[]
    for i in range(mm.waveInGetNumDevs()):
        caps=Caps();check(mm.waveInGetDevCapsW(i,C.byref(caps),C.sizeof(caps)));devices.append({'id':i,'name':caps.name,'channels':caps.channels})
    if a.list:print(json.dumps(devices,indent=2));return
    matches=[d for d in devices if d['name']==a.device and 'felucca' in d['name'].lower()]
    if len(matches)!=1:p.error('Specify the exact unique Felucca name from --list; no default input is selected')
    if not a.output or not 1<=a.seconds<=120:p.error('--output and --seconds 1..120 required')
    if a.output.exists():p.error('Output already exists; choose a new path')
    fmt=Format(1,2,a.rate,a.rate*4,4,16,0);handle=C.c_void_p();check(mm.waveInOpen(C.byref(handle),matches[0]['id'],C.byref(fmt),0,0,0))
    buffer=C.create_string_buffer(a.seconds*a.rate*4);header=Header(C.addressof(buffer),len(buffer),0,0,0,0,None,0);prepared=False
    try:
        check(mm.waveInPrepareHeader(handle,C.byref(header),C.sizeof(header)));prepared=True
        check(mm.waveInAddBuffer(handle,C.byref(header),C.sizeof(header)));check(mm.waveInStart(handle))
        print(f'Recording {a.device} at {a.rate} Hz for {a.seconds}s. Play the SMK now.',flush=True)
        deadline=time.monotonic()+a.seconds+5
        while not header.flags&1:
            if time.monotonic()>deadline:raise TimeoutError('Recording buffer did not complete')
            time.sleep(.025)
        if header.recorded!=len(buffer):raise RuntimeError(f'Short capture: {header.recorded}/{len(buffer)}')
        a.output.parent.mkdir(parents=True,exist_ok=True)
        with wave.open(str(a.output),'wb') as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(a.rate);f.writeframes(buffer.raw[:header.recorded])
    finally:
        mm.waveInReset(handle)
        if prepared:mm.waveInUnprepareHeader(handle,C.byref(header),C.sizeof(header))
        mm.waveInClose(handle)
    report=analyze(a.output);report['device']=a.device;print(json.dumps(report,indent=2));a.output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
