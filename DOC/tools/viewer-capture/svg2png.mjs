#!/usr/bin/env node
// Rasterize an SVG figure with headless Chrome (fonts as installed, e.g. Times New Roman).
//   node svg2png.mjs <in.svg> [out.png] [--scale 4]
import fs from 'node:fs';
import path from 'node:path';
import puppeteer from 'puppeteer-core';

const argv = process.argv.slice(2);
const scale = +(argv.includes('--scale') ? argv[argv.indexOf('--scale') + 1] : 4);
const [input, output = input.replace(/\.svg$/i, '.png')] = argv.filter((a, i) => !a.startsWith('--') && argv[i - 1] !== '--scale');
const svg = fs.readFileSync(input, 'utf8');
const m = svg.match(/<svg[^>]*\swidth="(\d+(?:\.\d+)?)"[^>]*\sheight="(\d+(?:\.\d+)?)"/);
if (!m) { console.error('SVG needs numeric width/height attributes'); process.exit(1); }
const [w, h] = [Math.ceil(+m[1]), Math.ceil(+m[2])];
const exe = [process.env.BROWSER_PATH, 'C:/Program Files/Google/Chrome/Application/chrome.exe',
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe', '/usr/bin/google-chrome'].filter(Boolean).find((p) => fs.existsSync(p));
const browser = await puppeteer.launch({ executablePath: exe, headless: true, args: ['--force-color-profile=srgb'] });
const page = await browser.newPage();
await page.setViewport({ width: w, height: h, deviceScaleFactor: scale });
await page.setContent(`<!doctype html><html><body style="margin:0;background:#fff">${svg.replace(/<\?xml[^>]*\?>/, '')}</body></html>`, { waitUntil: 'load' });
await page.screenshot({ path: output, clip: { x: 0, y: 0, width: w, height: h } });
await browser.close();
console.log(path.resolve(output), `${w * scale}×${h * scale}`);
