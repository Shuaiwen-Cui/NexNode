#!/usr/bin/env node
/*
 * Builds thesis Figure 3.3 (LiftNode mechanical design) as one print-ready PNG:
 *   (a) assembled node   |   (b) axial exploded view with numbered callouts
 * Both panels share the same camera so they read as one object; the legend goes into the caption.
 *
 *   node thesis-figure.mjs                    → DOC/media/thesis-figure/liftnode-thesis-fig-mechanical.png
 *   node thesis-figure.mjs --copy <dir>       also copy it there (e.g. THESIS-R/FIGURES/CH3)
 *
 * Layout: 1800 CSS px ≈ 150 mm text width (12 px/mm), rendered at 3× → 5400 px wide (~915 dpi).
 */
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import puppeteer from 'puppeteer-core';

const here = path.dirname(fileURLToPath(import.meta.url));
const DOCS = path.resolve(here, '../../docs');
const args = Object.fromEntries(process.argv.slice(2).reduce((acc, a, i, arr) => {
  if (a.startsWith('--')) acc.push([a.slice(2), arr[i + 1] && !arr[i + 1].startsWith('--') ? arr[i + 1] : 'true']);
  return acc;
}, []));
const OUT = path.resolve(args.out || path.join(here, '../../media/thesis-figure'));
const DSF = +(args.scale || 3);
const THETA = args.theta || '36', ELEV = args.elev || '24';

// Geometry (CSS px). The thesis prints this at ~137 mm text width (≈13 px/mm). The overall aspect
// (2.28:1) matches the figure it replaces, so the float keeps its height and the thesis pagination
// (and every page number cited in the reply letters) stays unchanged.
// --layout unfold (default): panel (b) shows the enclosure walls hinged open — panel (a) already shows
// the closed exterior.  --layout axial: one-piece enclosure, parts lifted along Z.
const LAYOUT = args.layout || 'unfold';
const W = 1800, H = 790, LABEL_H = 78, PANEL_H = H - LABEL_H;
const GEOM = {
  unfold: { A_W: 420, B_W: 880, marginB: 0.1 },   // the opened layout is wide
  axial: { A_W: 520, B_W: 700, marginB: 0.12 },   // the stacked layout is tall
}[LAYOUT];
const { A_W, B_W } = GEOM, LEG_W = W - A_W - B_W;
const CALLOUT = { ball: 21, numSize: 30, line: 1.8, dot: 4, gapY: 54, stroke: '#1a1a1a' };
// legend wording follows the thesis text (Chapter 3, "Edge Node: LiftNode"); callouts are numbered in
// reading order (left column top→bottom, then right column) and the legend follows those numbers
const TERMS = {
  lid: 'enclosure lid', battery: '18650 Li-ion battery pack', plate: 'battery mounting plate', board: 'main board',
  extBoard: 'expansion board', bms: 'power-management module', bmsFrame: 'power-module mounting frame',
  antenna: 'external 2.4 GHz antenna', wallFront: 'enclosure walls (opened)', base: 'enclosure floor',
  bodyFull: 'enclosure body', magnetsR: 'magnetic mounts',
};

const BROWSERS = [process.env.BROWSER_PATH,
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome', '/usr/bin/google-chrome'].filter(Boolean);
const browserPath = BROWSERS.find((p) => fs.existsSync(p));
if (!browserPath) { console.error('No Chrome/Edge found; set BROWSER_PATH'); process.exit(1); }

const MIME = { '.html': 'text/html', '.js': 'text/javascript', '.stl': 'application/octet-stream', '.glb': 'model/gltf-binary' };
const server = http.createServer((req, res) => {
  const p = path.join(DOCS, decodeURIComponent(new URL(req.url, 'http://x').pathname));
  if (!p.startsWith(DOCS) || !fs.existsSync(p) || fs.statSync(p).isDirectory()) { res.writeHead(404); return res.end(); }
  res.writeHead(200, { 'Content-Type': MIME[path.extname(p).toLowerCase()] || 'application/octet-stream', 'Cache-Control': 'no-store' });
  fs.createReadStream(p).pipe(res);
});
await new Promise((r) => server.listen(0, '127.0.0.1', r));
const origin = `http://127.0.0.1:${server.address().port}`;
const browser = await puppeteer.launch({
  executablePath: browserPath, headless: true,
  args: ['--use-angle=d3d11', '--enable-gpu', '--ignore-gpu-blocklist', '--hide-scrollbars', '--force-color-profile=srgb'],
});
fs.mkdirSync(OUT, { recursive: true });

async function shoot(page, file) {
  for (let attempt = 1; ; attempt++) {
    try { return await page.screenshot({ path: file }); } catch (e) {
      if (attempt >= 5) throw e;
      await new Promise((r) => setTimeout(r, 600 * attempt));
    }
  }
}
async function panel({ layout, width, labels, marginX, marginY, file }) {
  const page = await browser.newPage();
  page.on('console', (m) => { if (m.type() === 'error' && !/404/.test(m.text())) console.error('[page]', m.text()); });
  await page.setViewport({ width, height: PANEL_H, deviceScaleFactor: DSF });
  await page.goto(`${origin}/static/liftnode-viewer/figure.html?layout=${layout}&theta=${THETA}&elev=${ELEV}&marginX=${marginX}&marginY=${marginY}`);
  await page.waitForFunction('window.figureReady === true', { timeout: 90000 });
  await new Promise((r) => setTimeout(r, 300));
  await page.evaluate((l, s) => window.makeFigure({ labels: l, style: s, renumber: true }), labels, CALLOUT);
  const clipped = await page.evaluate(() => window.figureClipped || []);
  if (clipped.length) console.warn('WARNING clipped labels:', clipped.join(', '));
  await shoot(page, file);
  const numbers = await page.evaluate(() => window.figureNumbers || []);
  await page.close();
  return numbers;
}

const fileA = path.join(OUT, 'liftnode-thesis-panel-a-assembled.png');
const fileB = path.join(OUT, 'liftnode-thesis-panel-b-exploded.png');
await panel({ layout: 'assembled', width: A_W, labels: 'none', marginX: 0.06, marginY: 0.1, file: fileA });
const numbers = await panel({ layout: LAYOUT, width: B_W, labels: 'number', marginX: GEOM.marginB, marginY: 0.035, file: fileB });

// compose (a) | (b) | legend, panel letters underneath, Times New Roman like the thesis body
const uri = (f) => `data:image/png;base64,${fs.readFileSync(f).toString('base64')}`;
const r = CALLOUT.ball;
const LEGEND = numbers.map(({ id }) => TERMS[id]);
const legendRows = LEGEND.map((name, i) => `<div class="li"><svg width="${2 * r + 4}" height="${2 * r + 4}"><circle cx="${r + 2}" cy="${r + 2}" r="${r}" fill="#fff" stroke="${CALLOUT.stroke}" stroke-width="${CALLOUT.line}"/><text x="${r + 2}" y="${r + 2}" text-anchor="middle" dominant-baseline="central" font-family="'Times New Roman',Times,serif" font-size="${CALLOUT.numSize}" fill="${CALLOUT.stroke}">${i + 1}</text></svg><span>${name}</span></div>`).join('');
const page = await browser.newPage();
await page.setViewport({ width: W, height: H, deviceScaleFactor: DSF });
await page.setContent(`<!doctype html><html><head><style>
  html,body{margin:0;background:#fff}
  .row{display:flex;width:${W}px;height:${H}px}
  figure{margin:0;display:flex;flex-direction:column;align-items:center}
  img{display:block;height:${PANEL_H}px}
  figcaption{height:${LABEL_H}px;line-height:${LABEL_H - 16}px;font:38px 'Times New Roman',Times,serif;color:#111}
  .legend{width:${LEG_W}px;height:${PANEL_H}px;display:flex;flex-direction:column;justify-content:center;gap:${LEGEND.length > 10 ? 6 : 12}px;padding-left:28px;box-sizing:border-box}
  .li{display:flex;align-items:center;gap:14px;font:30px 'Times New Roman',Times,serif;color:#111;white-space:nowrap}
</style></head><body><div class="row">
  <figure><img src="${uri(fileA)}" style="width:${A_W}px"><figcaption>(a)</figcaption></figure>
  <figure><img src="${uri(fileB)}" style="width:${B_W}px"><figcaption>(b)</figcaption></figure>
  <div class="legend">${legendRows}</div>
</div></body></html>`, { waitUntil: 'load' });
const final = path.join(OUT, 'liftnode-thesis-fig-mechanical.png');
await shoot(page, final);
console.log(final);
fs.writeFileSync(path.join(OUT, 'liftnode-thesis-fig-mechanical-callouts.json'), JSON.stringify(numbers.map(({ n, id }) => ({ n, id, term: TERMS[id] })), null, 2));
if (args.copy) {
  const dst = path.join(path.resolve(args.copy), 'node-design-exploded.png');
  fs.copyFileSync(final, dst);
  console.log('copied →', dst);
}
await browser.close(); server.close();
