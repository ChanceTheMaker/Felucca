"""Refresh locally hosted wordmark fonts with complete Latin alphabet and title punctuation."""
from concurrent.futures import ThreadPoolExecutor
from io import BytesIO
from pathlib import Path
from urllib.parse import urlencode
from urllib.request import Request, urlopen
import re
import string
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]
FAMILIES = [('Racing Sans One',400),('Orbitron',700),('Audiowide',400),
            ('Berkshire Swash',400),('Righteous',400),('Syncopate',700),
            ('Josefin Sans',700),('Sora',700),('Bungee',400)]
CHARACTERS = string.ascii_letters + string.digits + ' []&-.'

def fetch(entry):
    family, weight = entry
    url = 'https://fonts.googleapis.com/css2?' + urlencode({
        'family':f'{family}:wght@{weight}', 'text':CHARACTERS, 'display':'swap'})
    request = Request(url,headers={'User-Agent':'Mozilla/5.0 Chrome/130.0.0.0 Safari/537.36'})
    with urlopen(request,timeout=30) as response: sheet=response.read().decode()
    source = re.search(r'url\((https://[^)]+)\)',sheet).group(1)
    with urlopen(source,timeout=30) as response: data=response.read()
    if data[:4] != b'\x00\x01\x00\x00':
        raise ValueError(f'{family}: expected a TrueType response')
    font = TTFont(BytesIO(data))
    missing = set(map(ord,CHARACTERS)) - set(font.getBestCmap())
    if missing: raise ValueError(f'{family}: missing {sorted(missing)}')
    return family.lower().replace(' ',''), data

if __name__ == '__main__':
    # Validate every download before replacing any of the currently served files.
    with ThreadPoolExecutor(max_workers=4) as pool: assets=list(pool.map(fetch,FAMILIES))
    for slug,data in assets:
        (ROOT/'web/fonts'/f'{slug}.ttf').write_bytes(data)
        print(f'{slug}: {len(data)} bytes; A-Z, a-z, digits and title punctuation present')
