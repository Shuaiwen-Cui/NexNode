#!/usr/bin/env node
/*
 * Planar power-wiring diagram of LiftNode, drawn on renders of the actual 3D models.
 *
 *   battery + ── self-locking push switch ──► module input terminal (+)   [upper-right green terminal]
 *   battery − ──────────────────────────────► module input terminal (−)
 *   module output terminal (+/−)              [upper-left green terminal] ──► main-board VIN (+/−)
 *
 * Layout (left → right): main board | power-management module | battery, so the two harnesses leave
 * the module's top edge in opposite directions and never cross. Runs are orthogonal with rounded corners.
 * Each part is rendered alone with the exploded-view materials; connector positions are projected from
 * the models so the wires land on the real terminals.
 *
 *   node power-wiring.mjs            → DOC/media/thesis-figure/liftnode-power-wiring.{png,svg}
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
const W = 1800, H = 880;

/* Connector positions in model space (mm, Z-up, enclosure frame).
 * Module (Solar Energy Manager A) glTF → model: (x, y, z) = (19.9 + gz, 66.3 + gx, 7.5 + gy); the two green
 * screw terminals on its top edge have poles at glTF x = −43.7/−38.7 (left, output) and −27.1/−22.1
 * (right, battery input); wires enter from the top edge (glTF y ≈ 60.6, z ≈ 2.5).
 * Main board VIN screw terminal P1 (KF301-2P): schematic pin 1 = VINPUT (→ D2 1N5819 → VCC_5V), pin 2 = GND;
 * silkscreen "GND" on the upper hole, "VIN" on the lower one; EasyEDA pins at board-local y 27.46 (+) / 32.54 (−).
 * Module terminals: datasheet/silkscreen (bms-dim.png) puts + toward the USB/DC port edge, i.e. the left pole here.
 * Battery leads leave the left end of the pack (model x = 31.5). */
const TERM = (gx) => [22.4, 66.3 + gx, 68.1];
const PARTS = [
  { id: 'board', box: [130, 300, 450, 450], view: { theta: 0, elev: 89.9 },
    pts: { vinPos: [30.2, 34.96, 25], vinNeg: [30.2, 40.04, 25] } },
  { id: 'bms', box: [720, 300, 380, 440], view: { theta: 90, elev: 0.01 },
    pts: { outPos: TERM(-43.7), outNeg: TERM(-38.7), inPos: TERM(-27.1), inNeg: TERM(-22.1) } },
  { id: 'battery', box: [1250, 340, 520, 380], view: { theta: 0, elev: 89.9, exposure: 0.78, env: 0.6 },
    pts: { batPos: [31.5, 58, 64], batNeg: [31.5, 53, 64] } },
];

const BROWSERS = [process.env.BROWSER_PATH,
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe', '/usr/bin/google-chrome'].filter(Boolean);
const browserPath = BROWSERS.find((p) => fs.existsSync(p));
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
const shoot = async (page, opts) => {
  for (let a = 1; ; a++) {
    try { return await page.screenshot(opts); } catch (e) { if (a >= 5) throw e; await new Promise((r) => setTimeout(r, 600 * a)); }
  }
};

// ---- render each part alone (transparent) and project its connector points
const rendered = {};
for (const part of PARTS) {
  const [, , bw, bh] = part.box;
  const page = await browser.newPage();
  await page.setViewport({ width: bw, height: bh, deviceScaleFactor: DSF });
  await page.goto(`${origin}/static/liftnode-viewer/figure.html?layout=assembled&transparent=1`);
  await page.waitForFunction('window.figureReady === true', { timeout: 90000 });
  const names = Object.keys(part.pts);
  const info = await page.evaluate((id, view, pts) => window.lnv.figure({
    layout: 'assembled', only: [id], theta: view.theta, elev: view.elev, exposure: view.exposure, env: view.env, marginX: 0.03, marginY: 0.04,
    transparent: true, guides: false, points: pts,
  }), part.id, part.view, names.map((n) => part.pts[n]));
  await page.evaluate(() => window.makeFigure({ labels: 'none' }));
  await new Promise((r) => setTimeout(r, 200));
  const file = path.join(OUT, `.part-${part.id}.png`);
  await shoot(page, { path: file, omitBackground: true });
  await page.close();
  rendered[part.id] = {
    file,
    pts: Object.fromEntries(names.map((n, i) => [n, [part.box[0] + info.points[i][0], part.box[1] + info.points[i][1]]])),
  };
}
const V = rendered.board.pts, M = rendered.bms.pts, B = rendered.battery.pts;

// ---- harness geometry (CSS px)
const C = { pos: '#c62828', neg: '#1a1a1a' };
const topOfModule = Math.min(M.outPos[1], M.inPos[1]);
// output harness (to the left): + on the lower lane, − on the upper lane, so the left-pole wire never
// crosses the right-pole wire; they drop along the main board's left side into VIN (+ upper, − lower pin)
const L = { pos: topOfModule - 70, neg: topOfModule - 110 };
const xVin = { pos: V.vinPos[0] - 40, neg: V.vinPos[0] - 70 };
// input harness (to the right): + on the upper lane (through the switch), − on the lower lane
const R = { pos: topOfModule - 190, neg: topOfModule - 70 };
const xBat = { neg: B.batNeg[0] - 70, pos: B.batPos[0] - 40 };
const SW = { cx: (M.inNeg[0] + xBat.neg) / 2 + 10, cy: R.pos, r: 34 };
const swIn = [SW.cx - SW.r - 16, SW.cy], swOut = [SW.cx + SW.r + 16, SW.cy];

const HOP = 13;
const route = (pts, r = 18) => {
  let d = `M${pts[0][0].toFixed(1)},${pts[0][1].toFixed(1)}`;
  for (let i = 1; i < pts.length - 1; i++) {
    const [px, py] = pts[i - 1], [x, y] = pts[i], [nx, ny] = pts[i + 1];
    const l1 = Math.hypot(x - px, y - py), l2 = Math.hypot(nx - x, ny - y), rr = Math.min(r, l1 / 2, l2 / 2);
    if (!l1 || !l2) continue;
    const ax = x - ((x - px) / l1) * rr, ay = y - ((y - py) / l1) * rr, bx = x + ((nx - x) / l2) * rr, by = y + ((ny - y) / l2) * rr;
    d += ` L${ax.toFixed(1)},${ay.toFixed(1)} Q${x.toFixed(1)},${y.toFixed(1)} ${bx.toFixed(1)},${by.toFixed(1)}`;
  }
  const e = pts[pts.length - 1];
  return d + ` L${e[0].toFixed(1)},${e[1].toFixed(1)}`;
};
const cable = (pts, color, tail = '') => {
  const d = route(pts) + tail;
  return `<path d="${d}" stroke="#0d0d0d" stroke-width="9.5" fill="none" stroke-linecap="round" stroke-linejoin="round"/>` +
         `<path d="${d}" stroke="${color}" stroke-width="6.5" fill="none" stroke-linecap="round" stroke-linejoin="round"/>`;
};
const pin = ([x, y]) => `<circle cx="${x.toFixed(1)}" cy="${y.toFixed(1)}" r="5.5" fill="#d4a24c" stroke="#0d0d0d" stroke-width="1.6"/>`;
const f = (n) => (typeof n === 'number' ? n.toFixed(1) : n);
const T = (x, y, s, o = {}) => `<text x="${f(x)}" y="${f(y)}" font-family="'Times New Roman',Times,serif" font-size="${o.size || 26}"${o.anchor ? ` text-anchor="${o.anchor}"` : ''}${o.bold ? ' font-weight="bold"' : ''}${o.italic ? ' font-style="italic"' : ''} fill="${o.fill || '#1a1a1a'}">${s}</text>`;

const wires = [
  // module output → main board VIN
  // the module output is (+ left, − right) but VIN is (− upper, + lower), so the pair must cross once:
  // drawn as a schematic hop where the − lead jumps over the + lead just before the terminal
  cable([M.outPos, [M.outPos[0], L.pos], [xVin.pos, L.pos], [xVin.pos, V.vinPos[1]], V.vinPos], C.pos),
  cable([M.outNeg, [M.outNeg[0], L.neg], [xVin.neg, L.neg], [xVin.neg, V.vinNeg[1]], [xVin.pos - HOP, V.vinNeg[1]]], C.neg,
    ` A${HOP},${HOP} 0 0 1 ${f(xVin.pos + HOP)},${f(V.vinNeg[1])} L${f(V.vinNeg[0])},${f(V.vinNeg[1])}`),
  // battery + → switch → module input +
  cable([M.inPos, [M.inPos[0], R.pos], swIn], C.pos),
  cable([swOut, [xBat.pos, R.pos], [xBat.pos, B.batPos[1]], B.batPos], C.pos),
  // battery − → module input −
  cable([M.inNeg, [M.inNeg[0], R.neg], [xBat.neg, R.neg], [xBat.neg, B.batNeg[1]], B.batNeg], C.neg),
].join('');
const pins = [V.vinPos, V.vinNeg, M.outPos, M.outNeg, M.inPos, M.inNeg, B.batPos, B.batNeg, swIn, swOut].map(pin).join('');

// round self-locking push switch, seen from the front: threaded bezel, button, two solder tags
const switchSvg = `
  <g>
    <rect x="${f(swIn[0])}" y="${f(SW.cy - 6)}" width="${f(swOut[0] - swIn[0])}" height="12" rx="3" fill="#b9bec6" stroke="#0d0d0d" stroke-width="1.4"/>
    <circle cx="${f(SW.cx)}" cy="${f(SW.cy)}" r="${SW.r}" fill="#c9ced6" stroke="#0d0d0d" stroke-width="2"/>
    <circle cx="${f(SW.cx)}" cy="${f(SW.cy)}" r="${SW.r - 6}" fill="none" stroke="#8a9099" stroke-width="2" stroke-dasharray="3 3"/>
    <circle cx="${f(SW.cx)}" cy="${f(SW.cy)}" r="${SW.r - 13}" fill="#e7eaee" stroke="#0d0d0d" stroke-width="1.6"/>
    <circle cx="${f(SW.cx)}" cy="${f(SW.cy)}" r="${SW.r - 21}" fill="none" stroke="#9aa0a8" stroke-width="1.4"/>
  </g>`;

// + / − marks beside every connection
const mark = ([x, y], s, dx, dy, color) =>
  `<circle cx="${f(x + dx)}" cy="${f(y + dy)}" r="11" fill="#fff" stroke="${color}" stroke-width="1.8"/>` +
  T(x + dx, y + dy + 7.5, s, { size: 22, bold: true, anchor: 'middle', fill: color });
const marks = [
  mark(M.outPos, '+', -24, 0, C.pos), mark(M.outNeg, '−', 24, 0, C.neg),
  mark(M.inPos, '+', -24, 0, C.pos), mark(M.inNeg, '−', 24, 0, C.neg),
  mark(V.vinPos, '+', 22, 16, C.pos), mark(V.vinNeg, '−', 22, -16, C.neg),
  mark(B.batPos, '+', -22, -20, C.pos), mark(B.batNeg, '−', -22, 20, C.neg),
].join('');
// terminal tags on the module face (white chips so they read on the black board)
const tag = (x, y, s) => `<rect x="${f(x - 44)}" y="${f(y - 16)}" width="88" height="30" rx="6" fill="#fff" stroke="#0d0d0d" stroke-width="1.4"/>` +
  T(x, y + 6.5, s, { size: 20, bold: true, anchor: 'middle' });

const bmsBox = PARTS[1].box, boardBox = PARTS[0].box, batBox = PARTS[2].box;
const sub = { italic: true, size: 23, fill: '#444', anchor: 'middle' };
const cap = (box, title, line, dy = 40) => T(box[0] + box[2] / 2, box[1] + box[3] + dy, title, { anchor: 'middle', bold: true }) +
                                        T(box[0] + box[2] / 2, box[1] + box[3] + dy + 30, line, sub);
const labels = [
  cap(boardBox, 'Main board', 'VIN terminal P1 → D2 → 5 V → LDO 3.3 V', 16),
  cap(bmsBox, 'Power-management module', 'Solar Energy Manager A', 16),
  cap(batBox, '18650 Li-ion battery pack (3P)', '3.7 V nominal, 10.5 Ah', 30),
  T(SW.cx, SW.cy - SW.r - 18, 'Self-locking push switch', { anchor: 'middle', bold: true, size: 24 }),
  // terminal names
  tag((M.outPos[0] + M.outNeg[0]) / 2, M.outPos[1] + 66, '5 V OUT'),
  tag((M.inPos[0] + M.inNeg[0]) / 2, M.inPos[1] + 66, 'BAT IN'),
  tag(V.vinPos[0] + 92, (V.vinPos[1] + V.vinNeg[1]) / 2 - 2, 'VIN'),
  // net names on the runs
  T((xVin.neg + M.outPos[0]) / 2, L.neg - 14, '5 V supply to the main board', { anchor: 'middle', size: 22, fill: '#333' }),
  T((M.inNeg[0] + xBat.neg) / 2 + 10, R.neg - 14, 'battery 3.0–4.2 V', { anchor: 'middle', size: 22, fill: '#333' }),
].join('');
const legend = `
  <g transform="translate(${W / 2 - 330},${H - 40})">
    <line x1="0" y1="0" x2="60" y2="0" stroke="${C.pos}" stroke-width="7" stroke-linecap="round"/>${T(74, 8, 'positive (+)', { size: 22 })}
    <line x1="230" y1="0" x2="290" y2="0" stroke="${C.neg}" stroke-width="7" stroke-linecap="round"/>${T(304, 8, 'negative (−)', { size: 22 })}
    <circle cx="480" cy="0" r="6" fill="#d4a24c" stroke="#0d0d0d" stroke-width="1.6"/>${T(496, 8, 'terminal', { size: 22 })}
  </g>`;

const uri = (file) => `data:image/png;base64,${fs.readFileSync(file).toString('base64')}`;
const images = PARTS.map((p) => `<image href="${uri(rendered[p.id].file)}" x="${p.box[0]}" y="${p.box[1]}" width="${p.box[2]}" height="${p.box[3]}"/>`).join('');
const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="${W}" height="${H}" viewBox="0 0 ${W} ${H}">
  <rect width="${W}" height="${H}" fill="#ffffff"/>
  ${images}
  ${wires}
  ${switchSvg}
  ${pins}
  ${marks}
  ${labels}
  ${legend}
</svg>`;
fs.writeFileSync(path.join(OUT, 'liftnode-power-wiring.svg'), svg);

const page = await browser.newPage();
await page.setViewport({ width: W, height: H, deviceScaleFactor: DSF });
await page.setContent(`<!doctype html><html><body style="margin:0">${svg}</body></html>`, { waitUntil: 'load' });
const png = path.join(OUT, 'liftnode-power-wiring.png');
await shoot(page, { path: png, clip: { x: 0, y: 0, width: W, height: H } });
console.log(png);
for (const p of PARTS) fs.rmSync(rendered[p.id].file, { force: true });
await browser.close(); server.close();
