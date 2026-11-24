#!/usr/bin/env node
/*
 * Close-up render of selected parts (for checking models against the EDA layout / photos).
 *   node part-closeup.mjs <out.png> --parts board,extBoard --layout axial --theta 30 --elev 35 [--width 1600 --height 1100]
 */
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import puppeteer from 'puppeteer-core';

const here = path.dirname(fileURLToPath(import.meta.url));
const DOCS = path.resolve(here, '../../docs');
const argv = process.argv.slice(2);
const out = argv.find((a) => !a.startsWith('--') && !argv[argv.indexOf(a) - 1]?.startsWith('--'));
const opt = (k, d) => (argv.includes(`--${k}`) ? argv[argv.indexOf(`--${k}`) + 1] : d);
const W = +opt('width', 1600), H = +opt('height', 1100);
const MIME = { '.html': 'text/html', '.js': 'text/javascript', '.jpg': 'image/jpeg', '.glb': 'model/gltf-binary' };
const server = http.createServer((req, res) => {
  const p = path.join(DOCS, decodeURIComponent(new URL(req.url, 'http://x').pathname));
  if (!p.startsWith(DOCS) || !fs.existsSync(p) || fs.statSync(p).isDirectory()) { res.writeHead(404); return res.end(); }
  res.writeHead(200, { 'Content-Type': MIME[path.extname(p).toLowerCase()] || 'application/octet-stream' });
  fs.createReadStream(p).pipe(res);
});
await new Promise((r) => server.listen(0, '127.0.0.1', r));
const browser = await puppeteer.launch({ executablePath: process.env.BROWSER_PATH, headless: true, args: ['--use-angle=d3d11', '--enable-gpu', '--ignore-gpu-blocklist'] });
const page = await browser.newPage();
page.on('console', (m) => { if (m.type() === 'error' && !/404/.test(m.text())) console.error('[page]', m.text()); });
await page.setViewport({ width: W, height: H, deviceScaleFactor: +opt('scale', 1) });
await page.goto(`http://127.0.0.1:${server.address().port}/static/liftnode-viewer/figure.html?layout=assembled`);
await page.waitForFunction('window.figureReady === true', { timeout: 90000 });
await page.evaluate((o) => window.lnv.figure(o), {
  layout: opt('layout', 'assembled'), only: opt('parts', 'board').split(','), theta: +opt('theta', 30), elev: +opt('elev', 35),
  marginX: 0.04, marginY: 0.04, guides: false,
});
await page.evaluate(() => window.makeFigure({ labels: 'none' }));
await new Promise((r) => setTimeout(r, 300));
await page.screenshot({ path: out });
console.log(out);
await browser.close(); server.close();
