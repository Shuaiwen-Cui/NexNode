#!/usr/bin/env node
/*
 * Exports print-quality exploded-view figures for the thesis.
 *
 *   node figure.mjs                       # axial layout (one-piece enclosure, parts stacked along Z)
 *   node figure.mjs --layout unfold       # web-style layout with the enclosure walls hinged open
 *   node figure.mjs --theta 36 --elev 24  # view azimuth / elevation in degrees
 *
 * For each layout it writes (DOC/media/thesis-figure/):
 *   <name>-nolabel.png               white background, no callouts (annotate later)
 *   <name>-nolabel-transparent.png   same, transparent background
 *   <name>-numbered.png / .svg       numbered balloons (legend goes into the caption)
 *   <name>-zh.png / .svg             Chinese part names
 *   <name>-en.png / .svg             English part names
 * The SVGs embed the rendered image and keep every callout as editable vector text/lines.
 * Page size is 1600 CSS px ≈ 160 mm text width; --scale 3 → 4800 px wide (≈ 760 dpi at 16 cm).
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
const LAYOUT = args.layout || 'axial';
const W = +(args.width || 1600), H = +(args.height || (LAYOUT === 'axial' ? 1500 : 1000));
const DSF = +(args.scale || 3);
const OUT = path.resolve(args.out || path.join(here, '../../media/thesis-figure'));
const NAME = args.name || `liftnode-exploded-${LAYOUT}`;
if (!args.marginX) args.marginX = LAYOUT === 'unfold' ? '0.27' : '0.2'; // room for the callout columns
const view = ['theta', 'elev', 'marginX'].filter((k) => args[k]).map((k) => `&${k}=${args[k]}`).join('');

const BROWSERS = [process.env.BROWSER_PATH,
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome', '/usr/bin/google-chrome', '/usr/bin/chromium'].filter(Boolean);
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
const base = `http://127.0.0.1:${server.address().port}/static/liftnode-viewer/figure.html?layout=${LAYOUT}${view}`;

const browser = await puppeteer.launch({
  executablePath: browserPath, headless: true,
  args: ['--use-angle=d3d11', '--enable-gpu', '--ignore-gpu-blocklist', '--hide-scrollbars', '--force-color-profile=srgb'],
});
fs.mkdirSync(OUT, { recursive: true });

async function open(transparent) {
  const page = await browser.newPage();
  page.on('console', (m) => { if (m.type() === 'error') console.error('[page]', m.text()); });
  await page.setViewport({ width: W, height: H, deviceScaleFactor: DSF });
  await page.goto(base + (transparent ? '&transparent=1' : ''));
  await page.waitForFunction('window.figureReady === true', { timeout: 90000 });
  await new Promise((r) => setTimeout(r, 300));
  return page;
}
async function shot(page, file, transparent = false) {
  for (let attempt = 1; ; attempt++) {
    try { await page.screenshot({ path: file, omitBackground: transparent }); return; } catch (e) {
      if (attempt >= 5) throw e;
      await new Promise((r) => setTimeout(r, 600 * attempt));
    }
  }
}

const page = await open(false);
const base64 = async (file) => fs.readFileSync(file).toString('base64');
const plain = path.join(OUT, `${NAME}-nolabel.png`);
await page.evaluate(() => window.makeFigure({ labels: 'none' }));
await shot(page, plain);
console.log(plain);
const img = await base64(plain);
for (const labels of ['number', 'zh', 'en']) {
  const tag = labels === 'number' ? 'numbered' : labels;
  const overlay = await page.evaluate((l) => window.makeFigure({ labels: l }), labels);
  const clipped = await page.evaluate(() => window.figureClipped);
  if (clipped.length) console.warn(`WARNING ${labels}: labels clipped at the page edge → ${clipped.join(', ')} (raise --marginX)`);
  const png = path.join(OUT, `${NAME}-${tag}.png`);
  await shot(page, png);
  const inner = overlay.replace(/^<svg[^>]*>/, '').replace(/<\/svg>$/, '');
  const svg = `<?xml version="1.0" encoding="UTF-8"?>\n<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="${W}" height="${H}" viewBox="0 0 ${W} ${H}">\n` +
    `  <image width="${W}" height="${H}" xlink:href="data:image/png;base64,${img}"/>\n  ${inner}\n</svg>\n`;
  fs.writeFileSync(path.join(OUT, `${NAME}-${tag}.svg`), svg);
  console.log(png);
}
const tp = await open(true);
await tp.evaluate(() => window.makeFigure({ labels: 'none' }));
const transparentPng = path.join(OUT, `${NAME}-nolabel-transparent.png`);
await shot(tp, transparentPng, true);
console.log(transparentPng);
const info = await page.evaluate(() => window.figureInfo);
fs.writeFileSync(path.join(OUT, `${NAME}-callouts.json`), JSON.stringify(info, null, 2));
await browser.close(); server.close();
