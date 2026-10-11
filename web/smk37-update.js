// SPDX-License-Identifier: GPL-3.0-only
import {Updater} from './fm1ota.js';
export const HEADER=1024, LO=0x4000, HI=0x9d000;
export function crc32(data){let c=0xffffffff;for(const b of data){c^=b;for(let i=0;i<8;i++)c=(c>>>1)^((0-(c&1))&0xedb88320);}return (c^0xffffffff)>>>0;}
export async function parsePackage(bytes,expectedSha){
  if(bytes.length!==HEADER+HI-LO)throw Error('Wrong SMK package size. FM-1 packages and full flash dumps are not accepted.');
  const h=bytes.slice(0,HEADER),v=new DataView(h.buffer),str=(a,b)=>new TextDecoder().decode(h.slice(a,b)).replace(/\0+$/,'');
  if(str(0,16)!=='SMK37-SALT-USB1'||v.getUint32(16,true)!==1||v.getUint32(20,true)!==LO||v.getUint32(24,true)!==HI||v.getUint32(28,true)!==HI-LO)throw Error('Not a supported SMK-37 update package.');
  const saved=v.getUint32(44,true);v.setUint32(44,0,true);
  if(crc32(h)!==saved)throw Error('Package header checksum failed.');
  if(crc32(bytes.subarray(HEADER))!==v.getUint32(40,true))throw Error('Package data checksum failed.');
  for(let s=0;s<(HI-LO)/4096;s++)if(crc32(bytes.subarray(HEADER+s*4096,HEADER+(s+1)*4096))!==v.getUint32(128+s*4,true))throw Error('Package sector checksum failed.');
  const product=str(48,80),version=str(80,112);
  if(!/^SMK-37_\d{3}$/.test(product)||!version)throw Error('Invalid SMK identity.');
  const sha=[...new Uint8Array(await crypto.subtle.digest('SHA-256',bytes))].map(b=>b.toString(16).padStart(2,'0')).join('');
  if(!expectedSha||sha!==expectedSha)throw Error('This package is not the reviewed candidate for this local installer.');
  return {bytes,product,version,sha};
}
const START=[0xf0,0x22,0x24,0x35,0x7f,0xf7];
export async function installSmk(access,pkg,log=()=>{}){
  const up=new Updater(access,log);let link;
  try {
    // Exact model filter; the shared transport never sends START to FM-1.
    let dev=await up.find(id=>id.model==='ota-SMK-37'||id.model==='SMK-37');
    if(!dev)throw Error('Updater-enabled SMK not found. Test10 and stock firmware need the one-time adapter installation first. Close Studio and other MIDI apps.');
    link=dev.link;
    if(dev.id.model==='SMK-37'){
      log('Entering SMK update mode. No flash has been written.');
      if(!link.send(START))throw Error('SMK disconnected before update mode.');
      link.close();link=null;
      dev=await up.waitFor(id=>id.model==='ota-SMK-37',30000);
      if(!dev)throw Error('SMK RAM loader did not appear. No browser programming started.');
      link=dev.link;
    }
    log('Checking package and device; then programming application only. Keep power and USB connected.');
    if(!link.send(START))throw Error('Loader disconnected.');
    const r=await up.serve(link,pkg.bytes,0xf0000000,20000,(n)=>{if(n%64===0)log(`Transfer request ${n}`);});
    if(!r.finished)throw Error('Update stopped. Keep SMK powered on; retry while its loader is connected. If power was lost during programming, use the recovery adapter.');
    link.close();link=null;log('Application readback verified. Waiting for restart...');
    const back=await up.waitFor(id=>id.model==='SMK-37',40000);
    if(!back)throw Error('Programming verified, but normal USB identity did not return. Inspect the device before taking further action.');
    link=back.link;
    if(back.id.text!==pkg.product)throw Error(`Unexpected returned version: ${back.id.text}`);
    log(`Installed ${pkg.version}. Device returned as ${back.id.text}.`);
  } finally {link?.close();}
}
