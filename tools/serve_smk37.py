#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Loopback-only SMK preview. Only explicitly staged candidate files are served."""
import argparse,json
from pathlib import Path
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--port',type=int,default=8770);p.add_argument('--candidate',type=Path);a=p.parse_args()
    candidate=a.candidate.resolve() if a.candidate else None
    class Handler(SimpleHTTPRequestHandler):
        def __init__(self,*args,**kw):super().__init__(*args,directory=str(ROOT/'web'),**kw)
        def do_GET(self):
            path=self.path.split('?',1)[0]
            if path in ('/installer/','/webapp/installer/'):self.send_response(302);self.send_header('Location','/');self.end_headers();return
            if path in ('/editor/','/webapp/editor/'):self.send_response(302);self.send_header('Location','/editor.html');self.end_headers();return
            if path in ('/smk37-candidate.json','/firmware/smk37-candidate.smkupdate'):
                try:
                    if not candidate:raise ValueError('No candidate')
                    meta=json.loads(candidate.read_text(encoding='utf-8'))
                    name=meta['package']
                    if Path(name).name!=name or not meta.get('private'):raise ValueError('Invalid private candidate')
                    data=(json.dumps(meta).encode() if path.endswith('.json') else (candidate.parent/name).read_bytes())
                except (OSError,ValueError,KeyError):self.send_error(404,'No staged SMK candidate');return
                self.send_response(200);self.send_header('Content-Type','application/json' if path.endswith('.json') else 'application/octet-stream');self.send_header('Cache-Control','no-store');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
            super().do_GET()
        def list_directory(self,path):self.send_error(404,'Directory listing disabled');return None
    print(f'SMK private preview: http://localhost:{a.port}/',flush=True)
    ThreadingHTTPServer(('127.0.0.1',a.port),Handler).serve_forever()
if __name__=='__main__':main()
