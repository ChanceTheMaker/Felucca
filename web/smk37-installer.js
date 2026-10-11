// SPDX-License-Identifier: GPL-3.0-only
import {parsePackage,installSmk} from './smk37-update.js';
const status=document.getElementById('status'),button=document.getElementById('install-button'),ack=document.getElementById('ack'),output=document.getElementById('log');
let candidate,busy=false;
const sync=()=>{button.disabled=busy||!candidate||!ack.checked||!navigator.requestMIDIAccess;};
const log=message=>{status.textContent=message;output.textContent+=message+'\n';};
ack.addEventListener('change',sync);
try{
 const r=await fetch('/smk37-candidate.json',{cache:'no-store'});
 if(!r.ok)throw Error('No candidate package is staged. Studio remains available.');
 candidate=await r.json();
 if(candidate.private!==true||!/^SMK-37_\d{3}$/.test(candidate.product)||!/^([a-f0-9]{64})$/.test(candidate.sha256))throw Error('Invalid candidate metadata.');
 const release=document.getElementById('release');
 if(release)release.textContent=`${candidate.version} · Private beta`;
 log(`${candidate.version} - ${candidate.usb_hardware_validated ? 'USB installation, restart and saved-project retention confirmed on the test device.' : 'private USB updater candidate; hardware validation pending.'}`);
}catch(e){candidate=null;log(e.message);const release=document.getElementById('release');if(release)release.textContent='No firmware staged';}
if(!navigator.requestMIDIAccess)log('Web MIDI is unavailable here. Open this local page in Chrome or Edge.');
sync();
button.addEventListener('click',async()=>{
 if(button.disabled)return;busy=true;sync();
 try{
   log('Checking the reviewed SMK package...');
   const r=await fetch('/firmware/smk37-candidate.smkupdate',{cache:'no-store'});
   if(!r.ok)throw Error('Candidate package is unavailable.');
   const pkg=await parsePackage(new Uint8Array(await r.arrayBuffer()),candidate.sha256);
   if(pkg.product!==candidate.product)throw Error('Candidate identity mismatch.');
   const midi=await navigator.requestMIDIAccess({sysex:true});
   await installSmk(midi,pkg,log);
 }catch(e){log(e.message);}finally{busy=false;sync();}
});
