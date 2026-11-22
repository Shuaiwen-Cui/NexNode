#!/usr/bin/env node
/*
 * Renders the <liftnode-viewer> cinematic loop to MP4 / GIF for slides.
 *
 *   cd DOC/tools/viewer-capture && npm install
 *   node render.mjs                          # 1920x1080 @60fps MP4 + 960px GIF, English labels, dark
 *   node render.mjs --lang zh --theme light  # Chinese labels on light background
 *   node render.mjs --still 0,7,13           # just a few PNG stills (quick look)
 *   node render.mjs --brand NEXNODE --out <dir>   # same hardware, sister-project branding
 *   node render.mjs --size 2560x1440 --gif false  # 2K (QHD) master, same layout, no GIF
 *
 * It serves DOC/docs on a local port, opens the viewer in headless Edge/Chrome,
 * steps the deterministic 16 s timeline frame by frame, screenshots each frame
 * and encodes with ffmpeg (bundled via ffmpeg-static). Masters land in DOC/media/exploded-view/.
 */
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import puppeteer from 'puppeteer-core';
import ffmpegPath from 'ffmpeg-static';

const here = path.dirname(fileURLToPath(import.meta.url));
const DOCS = path.resolve(here, '../../docs');

const args = Object.fromEntries(process.argv.slice(2).reduce((acc, a, i, arr) => {
  if (a.startsWith('--')) acc.push([a.slice(2), arr[i + 1] && !arr[i + 1].startsWith('--') ? arr[i + 1] : 'true']);
  return acc;
}, []));
const W = +(args.width || 1920), H = +(args.height || 1080), FPS = +(args.fps || 60);
const DSF = +(args.scale || 2);                       // supersampling factor for crisp edges
const LANG = args.lang || 'en', THEME = args.theme || 'dark';
const OUT = path.resolve(args.out || path.join(here, '../../media/exploded-view')); // tracked, not published by MkDocs
const GIF_W = +(args['gif-width'] || 960), GIF_FPS = +(args['gif-fps'] || 20);
const BRAND = args.brand || 'LIFTNODE';
// --size 2560x1440: encode at another resolution while keeping the 1920×1080 page layout
// (labels keep their proportions; frames are supersampled at W·scale and resized down).
const [OW, OH] = (args.size || `${W}x${H}`).split('x').map(Number);
const MAKE_GIF = args.gif !== 'false';
const tag = `${BRAND.toLowerCase()}-exploded-${LANG}-${THEME}` + (OH !== 1080 ? `-${OH}p` : '');

const BROWSERS = [
  process.env.BROWSER_PATH,
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  '/usr/bin/google-chrome', '/usr/bin/chromium', '/usr/bin/chromium-browser',
].filter(Boolean);
const browserPath = BROWSERS.find((p) => fs.existsSync(p));
if (!browserPath) { console.error('No Chrome/Edge found; set BROWSER_PATH'); process.exit(1); }

const MIME = { '.html': 'text/html', '.js': 'text/javascript', '.stl': 'application/octet-stream', '.css': 'text/css', '.png': 'image/png', '.jpg': 'image/jpeg' };
const server = http.createServer((req, res) => {
  const p = path.join(DOCS, decodeURIComponent(new URL(req.url, 'http://x').pathname));
  if (!p.startsWith(DOCS) || !fs.existsSync(p) || fs.statSync(p).isDirectory()) { res.writeHead(404); return res.end(); }
  res.writeHead(200, { 'Content-Type': MIME[path.extname(p).toLowerCase()] || 'application/octet-stream', 'Cache-Control': 'no-store' });
  fs.createReadStream(p).pipe(res);
});
await new Promise((r) => server.listen(0, '127.0.0.1', r));
const port = server.address().port;

const browser = await puppeteer.launch({
  executablePath: browserPath,
  headless: true, dumpio: !!process.env.DUMPIO,
  args: ['--use-angle=d3d11', '--enable-gpu', '--ignore-gpu-blocklist', '--enable-webgl', '--hide-scrollbars', '--force-color-profile=srgb'],
});
const page = await browser.newPage();
page.on('console', (m) => { if (m.type() === 'error') console.error('[page]', m.text()); });
await page.setViewport({ width: W, height: H, deviceScaleFactor: DSF });
await page.goto(`http://127.0.0.1:${port}/static/liftnode-viewer/index.html?capture=1&lang=${LANG}&theme=${THEME}&brand=${encodeURIComponent(BRAND)}`);
await page.waitForFunction('window.lnvReady === true', { timeout: 60000 });
const LOOP = await page.evaluate('window.lnv.LOOP');
fs.mkdirSync(OUT, { recursive: true });

if (args.still) {
  for (const t of String(args.still).split(',').map(Number)) {
    await page.evaluate((tt) => window.lnv.renderAt(tt), t);
    const f = path.join(OUT, `${tag}-t${t}.png`);
    await page.screenshot({ path: f });
    console.log('still', f);
  }
  await browser.close(); server.close(); process.exit(0);
}

const frameDir = path.join(OUT, `.frames-${tag}`);
fs.rmSync(frameDir, { recursive: true, force: true });
fs.mkdirSync(frameDir, { recursive: true });
const N = Math.round(LOOP * FPS);
const t0 = Date.now();
for (let i = 0; i < N; i++) {
  for (let attempt = 1; ; attempt++) {
    try {
      await page.evaluate((tt) => window.lnv.renderAt(tt), i / FPS);
      await page.screenshot({ path: path.join(frameDir, `f${String(i).padStart(5, '0')}.jpg`), type: 'jpeg', quality: 95 });
      break;
    } catch (err) { // Chrome occasionally refuses a capture (GPU busy); retry the same frame
      if (attempt >= 5) throw err;
      console.warn(`frame ${i}: ${err.message} — retry ${attempt}`);
      await new Promise((r) => setTimeout(r, 500 * attempt));
    }
  }
  if (i % 60 === 0) console.log(`frame ${i}/${N}  ${((Date.now() - t0) / 1000).toFixed(0)}s`);
}
await browser.close(); server.close();

const run = (a) => new Promise((res, rej) => {
  const p = spawn(ffmpegPath, a, { stdio: ['ignore', 'inherit', 'inherit'] });
  p.on('exit', (c) => (c === 0 ? res() : rej(new Error('ffmpeg exit ' + c))));
});
const input = ['-y', '-framerate', String(FPS), '-i', path.join(frameDir, 'f%05d.jpg')];
const mp4 = path.join(OUT, `${tag}.mp4`);
await run([...input, '-vf', `scale=${OW}:${OH}:flags=lanczos`, '-c:v', 'libx264', '-preset', 'slow', '-crf', '16',
  '-pix_fmt', 'yuv420p', '-movflags', '+faststart', mp4]);
const outputs = [mp4];
if (MAKE_GIF) {
  const gif = path.join(OUT, `${tag}.gif`);
  await run([...input, '-vf',
    `fps=${GIF_FPS},scale=${GIF_W}:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=192:stats_mode=diff[p];[b][p]paletteuse=dither=sierra2_4a:diff_mode=rectangle`,
    '-loop', '0', gif]);
  outputs.push(gif);
}
fs.rmSync(frameDir, { recursive: true, force: true });
for (const f of outputs) console.log(f, (fs.statSync(f).size / 1e6).toFixed(1) + ' MB');
