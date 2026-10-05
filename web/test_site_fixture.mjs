// SPDX-License-Identifier: GPL-3.0-only
// Optional isolated built-site transport for browser tests. Never shipped.
import {readFile} from 'node:fs/promises';
import path from 'node:path';
export async function builtSiteFixture(page,directory) {
 if(!directory)return null;
 const root=path.resolve(directory);
 await page.route('**/webapp/editor/**',async route=>{
  const relative=decodeURIComponent(new URL(route.request().url()).pathname.split('/webapp/editor/')[1] || 'index.html');
  const file=path.resolve(root,relative);
  if(!file.startsWith(root+path.sep))throw Error('Fixture request escaped the built editor directory');
  const contentType={'.html':'text/html; charset=utf-8','.css':'text/css','.js':'text/javascript','.wasm':'application/wasm','.png':'image/png','.ttf':'font/ttf','.svg':'image/svg+xml'}[path.extname(file)] || 'application/octet-stream';
  await route.fulfill({status:200,contentType,body:await readFile(file)});
 });
 return ()=>readFile(path.join(root,'index.html'),'utf8');
}
