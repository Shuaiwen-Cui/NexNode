/*
 * LiftNode interactive exploded view — scene + UI.
 * Loaded lazily by liftnode-viewer.js. All model coordinates below are in millimetres,
 * Z-up, in the same frame as the Fusion 360 STL exports (DOC/docs/HARDWARE-ESP/FUSION).
 *
 * Geometry sources
 *   - Enclosure body / lid, battery plate, BMS frame: real STL exports from Fusion 360.
 *   - PCBs, battery pack, antenna, magnets, wires: procedural stand-ins sized from the
 *     enclosure features and photos. Replace with EasyEDA / vendor STEP exports when available.
 */
import * as THREE from './vendor/three.module.min.js';
import { OrbitControls } from './vendor/OrbitControls.js';
import { STLLoader } from './vendor/STLLoader.js';
import { GLTFLoader } from './vendor/GLTFLoader.js';
import { RoomEnvironment } from './vendor/RoomEnvironment.js';
import { RoundedBoxGeometry } from './vendor/RoundedBoxGeometry.js';
import { toCreasedNormals } from './vendor/BufferGeometryUtils.js';
import { EffectComposer } from './vendor/postprocessing/EffectComposer.js';
import { RenderPass } from './vendor/postprocessing/RenderPass.js';
import { GTAOPass } from './vendor/postprocessing/GTAOPass.js';
import { OutputPass } from './vendor/postprocessing/OutputPass.js';

const DEG = Math.PI / 180;
const clamp01 = (v) => Math.min(1, Math.max(0, v));
const lerp = (a, b, t) => a + (b - a) * t;
const easeInOut = (t) => (t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2);
const smooth = (a, b, t) => { const x = clamp01((t - a) / (b - a)); return x * x * (3 - 2 * x); };

/* ------------------------------------------------------------------ text */
const TEXT = {
  en: {
    explode: 'Explode', assemble: 'Assemble', studio: 'Studio', xray: 'X-ray', blueprint: 'Blueprint',
    rotate: 'Auto-rotate', fullscreen: 'Fullscreen', reset: 'Reset view', loading: 'Loading model',
    hint: 'Drag to orbit · Click, then scroll to zoom · Double-click to explode', brand: 'LIFTNODE',
    tagline: 'AIoT sensing node · mechanical assembly', explodeAmt: 'Explode amount', parts: 'parts',
  },
  zh: {
    explode: '爆炸视图', assemble: '装配', studio: '渲染', xray: '透视', blueprint: '蓝图',
    rotate: '自动旋转', fullscreen: '全屏', reset: '重置视角', loading: '模型加载中',
    hint: '拖动旋转 · 点击后滚轮缩放 · 双击爆炸/装配', brand: 'LIFTNODE',
    tagline: 'AIoT 传感节点 · 机械装配', explodeAmt: '爆炸程度', parts: '个零件',
  },
};

/* Parts: explode direction (model mm), stagger start (0..0.5), label anchor (model mm, assembled),
 * guides: points (model mm, assembled) from which dashed travel lines are drawn while exploded.
 * The enclosure body is split into base + four walls (DOC/tools/split_enclosure.py) so the walls
 * can fall away and reveal the interior. label:false parts move but carry no callout. */
const WALL_SUB = { en: 'Split for clarity — printed as one body', zh: '为展示而拆分，实际一体打印' };
const PARTS = [
  { id: 'lid', dir: [0, 0, 104], start: 0.0, anchor: [18, 6, 77.5],
    guides: [[0, 0, 75], [100, 0, 75], [0, 75, 75], [100, 75, 75]],
    name: { en: 'Enclosure lid', zh: '外壳上盖' }, sub: { en: '8228 resin · SLA printed', zh: '8228 树脂 · 光固化打印' } },
  { id: 'wallFront', dir: [0, -14, -3], rot: { axis: [1, 0, 0], pivot: [0, 0, 3], angle: 84 }, start: 0.07, anchor: [50, 0, 40],
    name: { en: 'Enclosure side walls ×4', zh: '外壳侧壁 ×4' }, sub: WALL_SUB },
  { id: 'wallBack', dir: [0, 14, -3], rot: { axis: [1, 0, 0], pivot: [0, 75, 3], angle: -84 }, start: 0.07, label: false },
  { id: 'wallLeft', dir: [-14, 0, -3], rot: { axis: [0, 1, 0], pivot: [0, 0, 3], angle: -84 }, start: 0.11, label: false },
  { id: 'wallRight', dir: [14, 0, -3], rot: { axis: [0, 1, 0], pivot: [100, 0, 3], angle: 84 }, start: 0.11, label: false },
  { id: 'antenna', dir: [70, 0, 30], start: 0.12, anchor: [114, 37.5, 128], guides: [[100, 37.5, 40]],
    name: { en: '2.4 GHz antenna', zh: '2.4 GHz 外置天线' }, sub: { en: 'SMA bulkhead · IPEX pigtail', zh: 'SMA 穿墙座 · IPEX 转接线' } },
  { id: 'magnetsL', dir: [-6, 0, -24], start: 0.15, label: false, guides: [[-6.2, 11.1, -5], [-6.2, 63.9, -5]] },
  { id: 'magnetsR', dir: [6, 0, -24], start: 0.15, anchor: [106.2, 11.1, -5], guides: [[106.2, 11.1, -5], [106.2, 63.9, -5]],
    name: { en: 'Magnetic mounts', zh: '磁吸安装座' }, sub: { en: 'Tool-free mounting on steel', zh: '免工具吸附于钢结构' } },
  { id: 'battery', dir: [0, 0, 66], start: 0.2, anchor: [88, 24, 70.8], guides: [[31.5, 9.6, 52], [96.5, 65.4, 52]],
    name: { en: 'Li-ion battery pack', zh: '锂离子电池组' }, sub: { en: '18650 × 3P · 3.7 V · 10.5 Ah', zh: '18650 三并 · 3.7 V · 10.5 Ah' } },
  { id: 'plate', dir: [0, 0, 48], start: 0.25, anchor: [96, 6, 55.5], guides: [[27.5, 3, 51], [97.5, 72, 51]],
    name: { en: 'Battery mounting plate', zh: '电池挂载板' }, sub: { en: 'Seats on internal ledge', zh: '搭在内壁台阶上' } },
  { id: 'board', dir: [0, 0, 48], start: 0.29, anchor: [70.5, 54.5, 24.4], guides: [[30, 7.5, 19.6], [95, 67.5, 19.6]],
    name: { en: 'LiftNode main board', zh: 'LiftNode 主控板' }, sub: { en: 'ESP32-S3-WROOM-1U · TF card · 2× USB-C', zh: 'ESP32-S3-WROOM-1U · TF 卡 · 双 USB-C' } },
  { id: 'extBoard', dir: [0, 0, 20], start: 0.33, anchor: [61.5, 51.5, 11.1], guides: [[30, 67.5, 7], [95, 7.5, 7]],
    name: { en: 'Sensor extension board', zh: '传感器扩展板' }, sub: { en: 'ADXL355 + ADXL367 accelerometers', zh: 'ADXL355 + ADXL367 加速度计' } },
  { id: 'bms', dir: [-22, 0, 8], start: 0.36, anchor: [24, 52, 44], guides: [[19.9, 66, 68]],
    name: { en: 'BMS board', zh: 'BMS 电源管理板' }, sub: { en: 'Solar power manager · charging & 5 V output', zh: '太阳能电源管理 · 充电与 5V 输出' } },
  { id: 'bmsFrame', dir: [-42, 0, 8], start: 0.4, anchor: [9.5, 69, 71], guides: [[2.5, 10, 72], [2.5, 70, 72]],
    name: { en: 'BMS mounting frame', zh: 'BMS 挂载框' }, sub: { en: 'Slot-mounts the BMS board', zh: '插槽式固定 BMS 板' } },
  { id: 'base', dir: [0, 0, 0], start: 0.0, anchor: [62, 40, 3],
    name: { en: 'Enclosure base', zh: '外壳底座' }, sub: { en: 'Floor plate with PCB standoffs', zh: '底板与 PCB 支柱' } },
];
const STAGGER_SPAN = 0.5;
const COMPONENT_COUNT = 10; // physical items: body, lid, plate, BMS frame, main board, extension board, BMS, battery, antenna, magnets
/* hovering any wall highlights the whole enclosure-wall group (one callout) */
const ALIAS = { wallBack: 'wallFront', wallLeft: 'wallFront', wallRight: 'wallFront', magnetsL: 'magnetsR' };
/* Academic figure layouts (thesis). n = callout number (top → bottom reading order).
 * axial  : classic explode along Z with the one-piece enclosure kept whole (dir = model-mm offset).
 * unfold : the web view's fully exploded state (walls hinged open), re-styled for print. */
const FIGURE_LAYOUTS = {
  axial: {
    lid:      { dir: [0, 0, 173],   n: 1, side: 'L' },
    battery:  { dir: [0, 0, 140],   n: 2, side: 'R' },
    plate:    { dir: [0, 0, 117],   n: 3, side: 'R' },
    board:    { dir: [0, 0, 110.4], n: 4, side: 'R' },
    extBoard: { dir: [0, 0, 91],    n: 5, side: 'L', anchor: [34, 12, 8.6] },  // front-left corner: leader passes under the BMS
    bms:      { dir: [-38, 0, 70],  n: 6, side: 'L' },
    bmsFrame: { dir: [-64, 0, 70],  n: 7, side: 'L' },
    antenna:  { dir: [58, 0, 0],    n: 8, side: 'R' },
    bodyFull: { dir: [0, 0, 0],     n: 9, side: 'L', anchor: [62, 0, 40],
      name: { en: 'Enclosure body', zh: '外壳主体' }, sub: { en: '8228 resin · SLA printed', zh: '8228 树脂 · 光固化打印' } },
    magnetsR: { dir: [0, 0, -34],   n: 10, side: 'R' },
    magnetsL: { dir: [0, 0, -34] },
  },
  unfold: {
    // side: which callout column (L/R) — fixed here so leaders never cross the antenna
    lid: { n: 1, side: 'L' }, battery: { n: 2, side: 'L' }, plate: { n: 3, side: 'L', anchor: [30, 6, 55.5] },
    board: { n: 4, side: 'L', anchor: [50, 9, 21.2] }, extBoard: { n: 5, side: 'R', anchor: [92, 12, 8.6] }, bms: { n: 6, side: 'L' },
    bmsFrame: { n: 7, side: 'L' }, antenna: { n: 8, side: 'R' }, wallFront: { n: 9, side: 'L' }, base: { n: 10, side: 'R' },
    magnetsR: { n: 11, side: 'R' },
  },
};
const WALL_IDS = ['base', 'wallFront', 'wallBack', 'wallLeft', 'wallRight'];
FIGURE_LAYOUTS.assembled = Object.fromEntries([
  ...PARTS.filter((p) => !WALL_IDS.includes(p.id)).map((p) => [p.id, { dir: [0, 0, 0] }]),
  ['bodyFull', { dir: [0, 0, 0] }],
]);
const SHELL = new Set(['lid', 'base', 'wallFront', 'wallBack', 'wallLeft', 'wallRight']);

/* [part id, file, folder] — enclosure pieces come from the split, the rest straight from Fusion. */
const STL_FILES = [
  ['base', 'body-base.stl', 'local'], ['wallFront', 'body-wall-front.stl', 'local'], ['wallBack', 'body-wall-back.stl', 'local'],
  ['wallLeft', 'body-wall-left.stl', 'local'], ['wallRight', 'body-wall-right.stl', 'local'],
  ['lid', 'ENCLOSURE-3D-LID.stl', 'fusion'], ['plate', 'FRAME-BATTERY.stl', 'fusion'], ['bmsFrame', 'FRAME-BMS.stl', 'fusion'],
];
/* The battery plate and BMS frame STLs were exported in part-local positions;
 * these offsets seat them on the enclosure's internal ledge (z = 51) and BMS slot. */
const STL_OFFSETS = { plate: [0, 0, 51], bmsFrame: [10.5, 0, 0] };

/* ------------------------------------------------------------------ utils */
function rng(seed) {
  return () => {
    seed |= 0; seed = (seed + 0x6d2b79f5) | 0;
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

function makeCanvas(wmm, hmm, ppm) {
  const c = document.createElement('canvas');
  c.width = Math.round(wmm * ppm); c.height = Math.round(hmm * ppm);
  const g = c.getContext('2d');
  g.scale(ppm, ppm);
  return { c, g };
}
function canvasTexture(c) {
  const t = new THREE.CanvasTexture(c);
  t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8;
  return t;
}

/* ------------------------------------------------------------------ materials */
function makeMaterials() {
  const P = THREE.MeshPhysicalMaterial;
  // 8228 SLA resin as printed: pale yellow-green, slightly waxy (matches the enclosure photos)
  const resin = () => new P({ color: 0xb4da84, roughness: 0.46, clearcoat: 0.25, clearcoatRoughness: 0.45,
    emissive: 0x1a2a0c, sheen: 0.3, sheenRoughness: 0.8, sheenColor: 0xffffff, transparent: true });
  return {
    shellBody: resin(),
    shellLid: resin(),
    frame: new P({ color: 0xa6cf7a, roughness: 0.5, clearcoat: 0.2, clearcoatRoughness: 0.5, emissive: 0x121c0a }),
    fr4: new P({ color: 0x2f2c22, roughness: 0.8 }),
    steel: new P({ color: 0xd6d9de, metalness: 1, roughness: 0.3 }),
    nickel: new P({ color: 0xc8ccd2, metalness: 1, roughness: 0.18 }),
    gold: new P({ color: 0xdcaa4f, metalness: 1, roughness: 0.24 }),
    black: new P({ color: 0x1a1b1d, roughness: 0.55 }),
    rubber: new P({ color: 0x151617, roughness: 0.82, sheen: 0.5, sheenColor: 0x555555, sheenRoughness: 0.6 }),
    blue: new P({ color: 0x1f64cf, roughness: 0.42, clearcoat: 0.3 }),
    white: new P({ color: 0xf2f0ea, roughness: 0.48 }),
    ferrite: new P({ color: 0x3a3b3e, roughness: 0.7 }),
    socket: new P({ color: 0x050505, roughness: 0.9 }),
    pcbBottom: new P({ color: 0x0d0e10, roughness: 0.6 }),
    ceramic: new P({ color: 0xe9e6df, roughness: 0.35, clearcoat: 0.3 }),
    bmsMask: new P({ color: 0x131416, roughness: 0.5, clearcoat: 0.2, clearcoatRoughness: 0.45 }),
    bmsCopper: new P({ color: 0x1e2321, roughness: 0.55, polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -1 }),
    smdR: new P({ color: 0x1e1e1f, roughness: 0.5 }),
    smdC: new P({ color: 0xa7895f, roughness: 0.55 }),
    cell: new P({ color: 0xbfe6a6, roughness: 0.28, clearcoat: 1, clearcoatRoughness: 0.15 }),
    led: new P({ color: 0xffffff, roughness: 0.15, emissive: 0x77aaff, emissiveIntensity: 0.6 }),
    wireRed: new P({ color: 0xc8261e, roughness: 0.45, clearcoat: 0.4, transparent: true }),
    wireBlue: new P({ color: 0x1f4fd1, roughness: 0.45, clearcoat: 0.4, transparent: true }),
    wireBlack: new P({ color: 0x141414, roughness: 0.45, clearcoat: 0.4, transparent: true }),
    coax: new P({ color: 0x2b2b2d, roughness: 0.6, transparent: true }),
  };
}

/* ------------------------------------------------------------------ geometry helpers (model mm, Z-up) */
function box(parent, mat, a, b, r = 0) {
  const s = [b[0] - a[0], b[1] - a[1], b[2] - a[2]];
  const g = r > 0
    ? new RoundedBoxGeometry(s[0], s[1], s[2], 2, Math.min(r, Math.min(...s) / 2 - 1e-3))
    : new THREE.BoxGeometry(s[0], s[1], s[2]);
  const m = new THREE.Mesh(g, mat);
  m.position.set((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2);
  parent.add(m);
  return m;
}
function cyl(parent, mat, c, r, h, axis = 'z', seg = 32, rTop = r) {
  const g = new THREE.CylinderGeometry(rTop, r, h, seg);
  if (axis === 'z') g.rotateX(Math.PI / 2);
  else if (axis === 'x') g.rotateZ(-Math.PI / 2);
  const m = new THREE.Mesh(g, mat);
  m.position.set(c[0], c[1], c[2]);
  parent.add(m);
  return m;
}
function tube(parent, mat, pts, r) {
  const curve = new THREE.CatmullRomCurve3(pts.map((p) => new THREE.Vector3(...p)), false, 'centripetal');
  const m = new THREE.Mesh(new THREE.TubeGeometry(curve, 64, r, 10, false), mat);
  m.userData.noEdges = true;
  parent.add(m);
  return m;
}

/* ------------------------------------------------------------------ PCB builder
 * Builds a board in local coords: x ∈ [0,w], y ∈ [0,h], copper top at z = t.
 * spec.comps: list of { a:[x,y], b:[x,y], h, mat, r, z0 } boxes (z measured from board top)
 * spec.draw(g, w, h): silkscreen / copper painter (canvas units = mm, y down).
 */
function buildBoard(spec, M) {
  const { w, h, t = 1.6 } = spec;
  const group = new THREE.Group();
  const { c, g } = makeCanvas(w, h, 16);
  g.fillStyle = spec.mask || '#0f1113'; g.fillRect(0, 0, w, h);
  const Y = (y) => h - y; // model y-up → canvas y-down

  // copper traces under the mask
  const rand = rng(spec.seed || 7);
  g.lineCap = 'round'; g.lineJoin = 'round';
  for (let i = 0; i < (spec.traces || 70); i++) {
    let x = rand() * w, y = rand() * h;
    g.beginPath(); g.moveTo(x, Y(y));
    const segs = 2 + Math.floor(rand() * 4);
    for (let s = 0; s < segs; s++) {
      const dir = Math.floor(rand() * 8) * 45 * DEG, len = 2 + rand() * 9;
      x = Math.min(w - 1, Math.max(1, x + Math.cos(dir) * len));
      y = Math.min(h - 1, Math.max(1, y + Math.sin(dir) * len));
      g.lineTo(x, Y(y));
    }
    g.strokeStyle = 'rgba(52,64,60,0.55)'; g.lineWidth = rand() < 0.2 ? 0.6 : 0.25; g.stroke();
    if (rand() < 0.5) { g.fillStyle = '#7d6a3e'; g.beginPath(); g.arc(x, Y(y), 0.35, 0, Math.PI * 2); g.fill(); }
  }
  if (spec.vias) { // stitching vias on a sparse board
    g.fillStyle = '#6e6440';
    for (let x = 3; x < w - 1; x += 4.2) for (let y = 3; y < h - 1; y += 4.2) { g.beginPath(); g.arc(x + (rand() - 0.5) * 0.6, Y(y), 0.28, 0, Math.PI * 2); g.fill(); }
  }
  // pads under parts
  for (const p of spec.comps) {
    if (p.noPad) continue;
    g.fillStyle = '#a7843f';
    g.fillRect(p.a[0] - 0.4, Y(p.b[1] + 0.4), p.b[0] - p.a[0] + 0.8, p.b[1] - p.a[1] + 0.8);
  }
  // mounting holes
  for (const [x, y] of spec.holes || []) {
    g.fillStyle = '#c69a48'; g.beginPath(); g.arc(x, Y(y), 2.4, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#050505'; g.beginPath(); g.arc(x, Y(y), 1.6, 0, Math.PI * 2); g.fill();
  }

  // scattered passives (instanced meshes + pads)
  const avoid = spec.comps.map((p) => [p.a[0] - 1.2, p.a[1] - 1.2, p.b[0] + 1.2, p.b[1] + 1.2])
    .concat((spec.holes || []).map(([x, y]) => [x - 3, y - 3, x + 3, y + 3]));
  const smd = [];
  for (let i = 0, tries = 0; i < (spec.smd || 60) && tries < 4000; tries++) {
    const big = rand() < 0.35, rot = rand() < 0.5;
    const L = big ? 2.0 : 1.6, W = big ? 1.25 : 0.8;
    const sx = rot ? W : L, sy = rot ? L : W;
    const x = 1.5 + rand() * (w - 3), y = 1.5 + rand() * (h - 3);
    const bb = [x - sx / 2 - 0.4, y - sy / 2 - 0.4, x + sx / 2 + 0.4, y + sy / 2 + 0.4];
    const hit = avoid.some((r) => bb[0] < r[2] && bb[2] > r[0] && bb[1] < r[3] && bb[3] > r[1]);
    if (hit) continue;
    avoid.push(bb);
    smd.push({ x, y, sx, sy, cap: rand() < 0.45, hz: big ? 0.8 : 0.5 });
    g.fillStyle = '#b48d43';
    if (rot) { g.fillRect(x - sx / 2, Y(y + sy / 2), sx, 0.45); g.fillRect(x - sx / 2, Y(y - sy / 2) - 0.45, sx, 0.45); }
    else { g.fillRect(x - sx / 2, Y(y + sy / 2), 0.45, sy); g.fillRect(x + sx / 2 - 0.45, Y(y + sy / 2), 0.45, sy); }
    if (rand() < 0.3) {
      g.fillStyle = 'rgba(235,235,230,0.85)'; g.font = '500 1.1px Roboto, Arial';
      g.fillText((rand() < 0.5 ? 'R' : 'C') + (1 + Math.floor(rand() * 30)), x + sx / 2 + 0.3, Y(y) + 0.4);
    }
    i++;
  }
  // silkscreen
  g.fillStyle = '#ecebe6'; g.strokeStyle = '#ecebe6';
  if (spec.draw) spec.draw(g, Y);

  const tex = canvasTexture(c);
  const top = new THREE.MeshPhysicalMaterial({ map: tex, roughness: 0.55, clearcoat: 0.18, clearcoatRoughness: 0.45, specularIntensity: 0.6 });
  const pcb = new THREE.Mesh(new THREE.BoxGeometry(w, h, t), [M.fr4, M.fr4, M.fr4, M.fr4, top, M.fr4]);
  pcb.position.set(w / 2, h / 2, t / 2);
  group.add(pcb);

  for (const p of spec.comps) {
    if (!p.mat) continue;
    const z0 = t + (p.z0 || 0);
    const m = box(group, p.mat, [p.a[0], p.a[1], z0], [p.b[0], p.b[1], z0 + p.h], p.r || 0);
    if (p.matArr) m.material = p.matArr;
  }

  for (const [kind, mat] of [[false, M.smdR], [true, M.smdC]]) {
    const list = smd.filter((s) => s.cap === kind);
    if (!list.length) continue;
    const inst = new THREE.InstancedMesh(new THREE.BoxGeometry(1, 1, 1), mat, list.length);
    const o = new THREE.Object3D();
    list.forEach((s, i) => {
      o.position.set(s.x, s.y, t + s.hz / 2); o.scale.set(s.sx, s.sy, s.hz); o.updateMatrix();
      inst.setMatrixAt(i, o.matrix);
    });
    inst.castShadow = true; inst.receiveShadow = true;
    group.add(inst);
  }
  return group;
}

/* 2.54 mm headers. Main board ↔ expansion board are joined by four 2×5 blocks (EasyEDA U1–U4 / H1–H4):
 * male pins on the underside of the main board plug into female sockets on the expansion board. */
const PITCH = 2.54;
// All board coordinates below are taken from the EasyEDA project (ProPrj_AIoT-NTU-Prototype_2026-03-28.epro):
// both boards are 65 × 60 mm, origin at the lower-left corner, x right, y toward the module edge.
const BOARD_W = 65, BOARD_H = 60;
const STACK_HEADERS = [[7, 43], [58, 43], [7, 17], [58, 17]]; // block centres (board-local mm)
const EXT_HOLES = [[4, 5], [61, 5], [4, 55], [61, 55]];       // M2.5 holes on the enclosure standoffs
const BOARD_ORIGIN = [30, 7.5];                                // board-local (0,0) in the enclosure frame
const FEMALE_H = 8.5, SPACER_H = 2.5;
const headerRect = ([cx, cy], cols = 2, rows = 5) => ({ a: [cx - cols * PITCH / 2, cy - rows * PITCH / 2], b: [cx + cols * PITCH / 2, cy + rows * PITCH / 2] });
function pinGrid([cx, cy], cols = 2, rows = 5) {
  const out = [];
  for (let i = 0; i < cols; i++) for (let j = 0; j < rows; j++) out.push([cx + (i - (cols - 1) / 2) * PITCH, cy + (j - (rows - 1) / 2) * PITCH]);
  return out;
}
function instancedAt(parent, geom, mat, pts, z, scale) {
  const inst = new THREE.InstancedMesh(geom, mat, pts.length);
  const o = new THREE.Object3D();
  pts.forEach(([x, y], i) => { o.position.set(x, y, z); if (scale) o.scale.set(...scale); o.updateMatrix(); inst.setMatrixAt(i, o.matrix); });
  inst.castShadow = true; inst.receiveShadow = true;
  parent.add(inst);
  return inst;
}

/* Bare PCB whose top face carries the EasyEDA 3D-view render of the real layout (copper, silkscreen,
 * pads and small passives); larger parts are added as 3D bodies at their EasyEDA placements. */
function texturedPcb(M, tex) {
  const g = new THREE.Group();
  const top = new THREE.MeshPhysicalMaterial({ map: tex, roughness: 0.55, clearcoat: 0.22, clearcoatRoughness: 0.42, specularIntensity: 0.55 });
  const pcb = new THREE.Mesh(new THREE.BoxGeometry(BOARD_W, BOARD_H, 1.6), [M.fr4, M.fr4, M.fr4, M.fr4, top, M.pcbBottom]);
  pcb.position.set(BOARD_W / 2, BOARD_H / 2, 0.8);
  g.add(pcb);
  return g;
}

/* Main board (top of the stack). Placements (mm, board-local) from the EasyEDA PCB:
 * MCU-ESP32S3N16R8 (42, 49.5); P1 VIN terminal KF301-2P (3.68, 30.0), pin 1 = VIN at y 27.46, pin 2 = GND at y 32.54;
 * USB-POWER (18.35, 5); USB-USART (50.62, 5); TF_CARD (34.42, 9.58); ON/OFF (61.34, 29.77); BOOT (63, 42.5); RST (63, 10);
 * LDO SOT-223 (51.43, 28.0); RGBLED 5050 (23.11, 28.76); VM 2-pin (13.97, 43.74); U1–U4 headers on the bottom side. */
function buildMainboard(M) {
  const board = texturedPcb(M, M.mainTop);
  const T = 1.6;

  // ESP32-S3-WROOM-1U: module PCB + RF shield (notched for the U.FL connector) with laser marking
  const lab = makeCanvas(15.6, 13.0, 40);
  lab.g.fillStyle = '#e8eaee'; lab.g.fillRect(0, 0, 15.6, 13.0);
  lab.g.fillStyle = '#80868e';
  lab.g.font = '700 1.2px Roboto, Arial'; lab.g.fillText('ESPRESSIF', 1.0, 2.2);
  lab.g.font = '600 1.15px Roboto, Arial'; lab.g.fillText('ESP32-S3-WROOM-1U', 1.0, 4.0);
  lab.g.font = '400 0.85px Roboto Mono, monospace';
  lab.g.fillText('N16R8', 1.0, 5.4); lab.g.fillText('2AC7Z-ESPS3WROOM1', 1.0, 6.6);
  const qr = rng(3);
  for (let i = 0; i < 10; i++) for (let j = 0; j < 10; j++) if (qr() < 0.5 || (i < 2 && j < 2)) lab.g.fillRect(10.2 + i * 0.42, 7.6 + j * 0.42, 0.42, 0.42);
  const shieldTop = new THREE.MeshPhysicalMaterial({ map: canvasTexture(lab.c), metalness: 1, roughness: 0.34 });
  box(board, M.black, [33.0, 40.3, T], [51.0, 59.5, T + 0.8]);
  const sA = box(board, M.steel, [34.3, 41.6, T + 0.8], [49.9, 54.6, T + 3.2]);
  sA.material = [M.steel, M.steel, M.steel, M.steel, shieldTop, M.steel];
  box(board, M.steel, [34.3, 54.6, T + 0.8], [45.0, 58.8, T + 3.2]);
  box(board, M.gold, [46.9, 56.0, T + 0.8], [49.1, 58.2, T + 1.0]);
  cyl(board, M.gold, [48.0, 57.1, T + 1.4], 1.05, 0.9, 'z', 24);                 // U.FL

  // USB-C receptacles and TF socket on the front edge
  for (const cx of [18.35, 50.62]) {
    box(board, M.steel, [cx - 4.47, -0.4, T], [cx + 4.47, 7.9, T + 3.2], 1.2);
    box(board, M.black, [cx - 3.4, -0.45, T + 0.9], [cx + 3.4, -0.35, T + 2.3], 0.4);
  }
  box(board, M.steel, [26.3, -0.3, T], [42.7, 15.0, T + 1.9]);
  box(board, M.black, [27.2, -0.4, T + 0.3], [41.8, -0.2, T + 1.4]);

  // P1 VIN screw terminal (KF301-2P, blue): wire entries face the board edge (−x), screws on top
  box(board, M.blue, [0.2, 24.92, T], [7.6, 35.08, T + 9.4], 0.4);
  for (const y of [27.46, 32.54]) {
    cyl(board, M.steel, [3.9, y, T + 9.45], 1.55, 0.3, 'z', 24);
    box(board, M.black, [3.2, y - 1.3, T + 9.6], [4.6, y + 1.3, T + 9.75]);
    box(board, M.socket, [0.12, y - 1.6, T + 2.2], [0.22, y + 1.6, T + 5.4]);   // wire entry
  }

  // ON/OFF slide switch (lever toward the right-hand edge), BOOT / RST side buttons
  box(board, M.steel, [57.8, 25.3, T], [64.9, 34.2, T + 3.6], 0.3);
  box(board, M.black, [64.9, 27.9, T + 1.6], [66.5, 29.8, T + 3.0], 0.2);
  for (const [y0, y1] of [[40.1, 44.7], [7.7, 12.3]]) {
    box(board, M.black, [61.2, y0, T], [65.0, y1, T + 3.5], 0.3);
    box(board, M.steel, [65.0, (y0 + y1) / 2 - 0.9, T + 1.2], [66.0, (y0 + y1) / 2 + 0.9, T + 2.6], 0.3);
  }

  // LDO (SOT-223), RGB LED (5050), VM jumper header
  box(board, M.black, [49.6, 24.8, T], [53.1, 31.2, T + 1.6], 0.15);
  box(board, M.steel, [47.4, 26.3, T], [49.6, 29.7, T + 0.25]);
  for (const y of [25.7, 28.0, 30.3]) box(board, M.steel, [53.1, y - 0.35, T], [54.9, y + 0.35, T + 0.25]);
  box(board, M.white, [20.6, 26.3, T], [25.6, 31.3, T + 1.6], 0.2);
  cyl(board, M.led, [23.11, 28.76, T + 1.62], 1.8, 0.06, 'z', 32);
  box(board, M.black, [12.7, 41.2, T], [15.25, 46.3, T + 2.5], 0.15);
  instancedAt(board, new THREE.BoxGeometry(0.64, 0.64, 6), M.gold, [[13.97, 42.47], [13.97, 45.01]], T + 2.5 + 3 - 0.5);

  // stacking headers on the underside: plastic spacer + male pins; solder fillets on the top pads
  const pins = STACK_HEADERS.flatMap((c) => pinGrid(c));
  for (const c of STACK_HEADERS) { const r = headerRect(c); box(board, M.black, [r.a[0], r.a[1], -SPACER_H], [r.b[0], r.b[1], 0], 0.2); }
  instancedAt(board, new THREE.BoxGeometry(0.64, 0.64, 8.5), M.gold, pins, -SPACER_H - 8.5 / 2 + 2.5);
  instancedAt(board, new THREE.SphereGeometry(0.85, 12, 6, 0, Math.PI * 2, 0, Math.PI / 2), M.nickel, pins, T, [1, 1, 0.6]);

  board.position.set(BOARD_ORIGIN[0], BOARD_ORIGIN[1], 7 + 1.6 + FEMALE_H + SPACER_H);
  return board;
}

/* Expansion board (bottom of the stack): ADXL355 (32.5, 45.0) and ADXL367 (32.13, 16.95), female sockets
 * H1–H4 under the main-board headers; screwed to the enclosure standoffs. */
function buildExtBoard(M) {
  const board = texturedPcb(M, M.extTop);
  const T = 1.6;
  box(board, M.ceramic, [29.5, 42.2, T], [35.5, 47.8, T + 2.1], 0.25);          // ADXL355 ceramic LCC
  box(board, M.black, [31.0, 15.8, T], [33.25, 18.1, T + 0.87], 0.1);           // ADXL367 LGA
  for (const c of STACK_HEADERS) { const r = headerRect(c); box(board, M.black, [r.a[0], r.a[1], T], [r.b[0], r.b[1], T + FEMALE_H], 0.15); }
  instancedAt(board, new THREE.BoxGeometry(1.0, 1.0, 0.06), M.socket, STACK_HEADERS.flatMap((c) => pinGrid(c)), T + FEMALE_H + 0.03);
  for (const [x, y] of EXT_HOLES) cyl(board, M.steel, [x, y, T + 0.7], 2.25, 1.4, 'z', 24);
  board.position.set(BOARD_ORIGIN[0], BOARD_ORIGIN[1], 7);
  return board;
}

/* BMS: the real board model (DFRobot Solar Power Manager, STEP → glTF, simplified + quantized).
 * glTF frame (mm): PCB top at z = 0 (bottom −1.6), parts on +z, port edge (USB-A, DC) at x ≈ −54.
 * Mounted on the outer face of the BMS frame (x = 16.5) with its ports facing the front-wall slot
 * (x 17.5–26.5, z 13.5–62.5), so: glTF x → model y, glTF y → model z, glTF z → model x. */
function buildBMS(gltfScene, M) {
  const board = new THREE.Group();
  gltfScene.traverse((o) => {
    if (!o.isMesh) return;
    o.userData.noEdges = true;
    const n = o.material && o.material.name || '';
    if (n.startsWith('Opaque(0,153,0)')) o.material = M.bmsMask;        // PCB: black solder mask as on the real board
    else if (n.startsWith('Opaque(0,255,255)')) o.material = M.bmsCopper; // exported copper layer → dark copper under mask
  });
  board.add(gltfScene);
  const basis = new THREE.Matrix4().makeBasis(new THREE.Vector3(0, 1, 0), new THREE.Vector3(0, 0, 1), new THREE.Vector3(1, 0, 0));
  board.quaternion.setFromRotationMatrix(basis);
  board.position.set(19.9, 66.3, 7.5);
  return board;
}

function buildBattery(M) {
  const g = new THREE.Group();
  const x0 = 31.5, x1 = 96.5, L = x1 - x0, zc = 52 + 9.3, r = 9.35;
  const ys = [18.9, 37.5, 56.1];
  for (const y of ys) {
    cyl(g, M.cell, [(x0 + x1) / 2, y, zc], r, L, 'x', 48);
    // shrink-wrap ends: slightly rounded rims
    for (const x of [x0, x1]) {
      const rim = new THREE.Mesh(new THREE.TorusGeometry(r - 1.1, 1.1, 12, 48), M.cell);
      rim.rotation.y = Math.PI / 2; rim.position.set(x, y, zc); g.add(rim);
    }
  }
  box(g, M.cell, [x0, ys[0], zc - 8.2], [x1, ys[2], zc + 8.2]);
  // wrap label
  const lw = 50, lh = 30;
  const { c, g: ctx } = makeCanvas(lw, lh, 20);
  ctx.fillStyle = '#f4f5f2'; ctx.fillRect(0, 0, lw, lh);
  ctx.fillStyle = '#7cc04b'; ctx.fillRect(0, 0, lw, 8.5);
  ctx.fillStyle = '#ffffff'; ctx.font = 'italic 700 4.6px Roboto, Arial'; ctx.fillText('Li-ion 18650', 2, 6.2);
  ctx.fillStyle = '#f08a24'; roundRect(ctx, 37.2, 2.2, 11, 4.6, 2.3); ctx.fill();
  ctx.fillStyle = '#fff'; ctx.font = '700 2.6px Roboto, Arial'; ctx.fillText('3P · XH2.54', 37.9, 5.4);
  ctx.fillStyle = '#2a2d30'; ctx.font = '500 2.1px Roboto, Arial';
  ctx.fillText('Nominal voltage  3.7 V', 2, 13); ctx.fillText('Rated capacity  10500 mAh', 2, 16.4);
  ctx.fillText('Rated energy  38.85 Wh', 2, 19.8); ctx.fillText('Charge limit  4.2 V', 2, 23.2);
  const bar = rng(9);
  for (let x = 34; x < 48; x += 0.35) if (bar() < 0.55) ctx.fillRect(x, 20, 0.22 + bar() * 0.25, 7);
  const label = new THREE.Mesh(new THREE.PlaneGeometry(lw, lh),
    new THREE.MeshPhysicalMaterial({ map: canvasTexture(c), roughness: 0.35, clearcoat: 0.8, clearcoatRoughness: 0.2 }));
  label.position.set(64, 37.5, zc + r + 0.06);
  label.userData.noEdges = true;
  g.add(label);
  return g;
}
function roundRect(g, x, y, w, h, r) {
  g.beginPath(); g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r);
  g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath();
}

function buildAntenna(M) {
  const g = new THREE.Group();
  const y = 37.5, z = 40;
  cyl(g, M.gold, [96.3, y, z], 4.2, 2.2, 'x', 6);       // inner nut
  cyl(g, M.gold, [104.2, y, z], 3.1, 8.4, 'x', 32);     // threaded barrel
  cyl(g, M.gold, [101.3, y, z], 4.4, 2.4, 'x', 6);      // outer nut
  cyl(g, M.rubber, [111, y, z], 4.4, 5.2, 'x', 32);     // base
  cyl(g, M.rubber, [114.2, y, z + 1.2], 4.9, 9.6, 'y', 32); // hinge knuckle
  const whip = cyl(g, M.rubber, [114.2, y, z + 56], 4.7, 100, 'z', 40, 3.3);
  const tip = new THREE.Mesh(new THREE.SphereGeometry(3.3, 32, 16), M.rubber);
  tip.scale.set(1, 1, 1.5); tip.position.set(114.2, y, z + 106); g.add(tip);
  for (const dz of [12, 14.2]) cyl(g, M.black, [114.2, y, z + dz], 4.75 - dz * 0.0135, 0.6, 'z', 40);
  whip.userData.noEdges = true; tip.userData.noEdges = true;
  return g;
}

function buildMagnets(M, x) {
  const g = new THREE.Group();
  for (const y of [11.1, 63.9]) {
    cyl(g, M.nickel, [x, y, -2.5], 8, 5, 'z', 48);
    cyl(g, M.black, [x, y, -5.02], 6.2, 0.1, 'z', 48);
    cyl(g, M.steel, [x, y, 3.5], 1.9, 9, 'z', 20);
    cyl(g, M.steel, [x, y, 6.4], 3.6, 3, 'z', 6);
  }
  return g;
}

function buildWires(M) {
  const g = new THREE.Group();
  // BMS output → mainboard screw terminal
  tube(g, M.wireRed, [[25.5, 38.5, 66], [28.4, 36.5, 56], [29.4, 35.2, 36], [30.2, 34.96, 25]], 0.9);
  tube(g, M.wireBlue, [[25.5, 42, 66], [28.4, 41.4, 56], [29.4, 40.2, 36], [30.2, 40.04, 25]], 0.9);
  // battery leads → BMS battery connector
  tube(g, M.wireRed, [[31.5, 59, 66], [29.5, 62, 68], [27.5, 58, 68], [25.5, 56.5, 64]], 0.8);
  tube(g, M.wireBlack, [[31.5, 55, 66], [29.5, 58, 69], [27.5, 55, 68.5], [25.5, 54, 64]], 0.8);
  // IPEX pigtail → SMA bulkhead
  tube(g, M.coax, [[78, 64.6, 23.2], [85, 68, 31], [92, 52, 38], [95.2, 37.5, 40]], 0.65);
  return g;
}

/* ------------------------------------------------------------------ CSS / template */
const ICON = {
  play: '<svg viewBox="0 0 24 24" width="14" height="14"><path fill="currentColor" d="M8 5.5v13l11-6.5z"/></svg>',
  back: '<svg viewBox="0 0 24 24" width="14" height="14"><path fill="currentColor" d="M16 5.5v13L5 12z"/></svg>',
  rotate: '<svg viewBox="0 0 24 24" width="15" height="15" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"><path d="M20 12a8 8 0 1 1-2.34-5.66"/><path d="M20 4v4.5h-4.5"/></svg>',
  reset: '<svg viewBox="0 0 24 24" width="15" height="15" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"><circle cx="12" cy="12" r="3"/><path d="M12 2v4M12 18v4M2 12h4M18 12h4"/></svg>',
  fs: '<svg viewBox="0 0 24 24" width="15" height="15" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"><path d="M4 9V4h5M20 9V4h-5M4 15v5h5M20 15v5h-5"/></svg>',
};

const CSS = `
:host { display:block; position:relative; height:560px; border-radius:14px; overflow:hidden; isolation:isolate;
        font-family: var(--md-text-font-family, Roboto), Roboto, system-ui, -apple-system, "Segoe UI", sans-serif; }
:host(:fullscreen) { border-radius:0; height:100vh; width:100vw; }
* { box-sizing:border-box; }
.stage { container-type:size; position:absolute; inset:0; background:var(--bg); color:var(--fg); transition:background-color .5s; user-select:none; -webkit-user-select:none; }
.stage[data-theme=dark] { --bg:radial-gradient(120% 95% at 50% 32%, #283244 0%, #131925 46%, #07090d 100%);
  --fg:#eef2f8; --muted:rgba(238,242,248,.56); --glass:rgba(16,21,30,.58); --line:rgba(255,255,255,.12);
  --hover:rgba(255,255,255,.08); --accent:#8fb4ff; --seg:rgba(255,255,255,.05); --solid:#0b0e14; --vig:rgba(0,0,0,.42); }
.stage[data-theme=light] { --bg:radial-gradient(120% 95% at 50% 32%, #ffffff 0%, #eef1f6 48%, #d3d9e3 100%);
  --fg:#131821; --muted:rgba(19,24,33,.55); --glass:rgba(255,255,255,.68); --line:rgba(16,24,40,.11);
  --hover:rgba(16,24,40,.06); --accent:#3565e8; --seg:rgba(16,24,40,.045); --solid:#ffffff; --vig:rgba(40,50,70,.12); }
.stage[data-mode=blueprint] { --fg:#e6f1ff; --muted:rgba(214,232,255,.62); --glass:rgba(8,32,62,.62); --line:rgba(160,205,255,.22);
  --hover:rgba(160,205,255,.1); --accent:#9fd0ff; --seg:rgba(160,205,255,.06); --solid:#0a2747; --vig:rgba(0,10,30,.35);
  background-color:#0a2747;
  background-image:linear-gradient(rgba(150,200,255,.12) 1px, transparent 1px), linear-gradient(90deg, rgba(150,200,255,.12) 1px, transparent 1px),
    linear-gradient(rgba(150,200,255,.05) 1px, transparent 1px), linear-gradient(90deg, rgba(150,200,255,.05) 1px, transparent 1px);
  background-size:100px 100px, 100px 100px, 20px 20px, 20px 20px; background-position:-1px -1px; }
canvas { position:absolute; inset:0; width:100%; height:100%; display:block; touch-action:none; outline:none; z-index:0; }
.vig { position:absolute; inset:0; pointer-events:none; z-index:1; background:radial-gradient(130% 100% at 50% 45%, transparent 55%, var(--vig) 100%); }
svg.lead { position:absolute; inset:0; width:100%; height:100%; pointer-events:none; z-index:2; overflow:visible; }
svg.lead polyline { fill:none; stroke:var(--fg); stroke-opacity:.5; stroke-width:1; }
svg.lead circle.d { fill:var(--fg); } svg.lead circle.r { fill:none; stroke:var(--fg); stroke-opacity:.35; }
svg.lead g.hot polyline, svg.lead g.hot circle.r { stroke:var(--accent); stroke-opacity:1; } svg.lead g.hot circle.d { fill:var(--accent); }
.labels { position:absolute; inset:0; pointer-events:none; z-index:3; }
.label { position:absolute; left:0; top:0; padding:6px 10px 7px; border-radius:8px; background:var(--glass);
  -webkit-backdrop-filter:blur(10px) saturate(140%); backdrop-filter:blur(10px) saturate(140%);
  border:1px solid var(--line); white-space:nowrap; will-change:transform,opacity; transition:border-color .2s; }
.label.hot { border-color:var(--accent); }
.label .t { display:flex; gap:8px; align-items:baseline; font-size:12.5px; font-weight:600; letter-spacing:.01em; line-height:1.25; }
.label .n { font:500 10px/1 "Roboto Mono", ui-monospace, monospace; color:var(--accent); letter-spacing:.04em; }
.label .s { margin-top:2px; font-size:11px; color:var(--muted); line-height:1.25; }
.brand { position:absolute; left:20px; top:18px; z-index:4; pointer-events:none; }
.brand b { display:block; font-size:12px; font-weight:700; letter-spacing:.28em; }
.brand span { display:block; margin-top:4px; font-size:11px; color:var(--muted); letter-spacing:.04em; }
.hint { position:absolute; right:20px; top:20px; z-index:4; font-size:11px; color:var(--muted); pointer-events:none; transition:opacity .6s; }
.readout { position:absolute; left:20px; bottom:22px; z-index:4; font:500 10.5px/1.5 "Roboto Mono", ui-monospace, monospace;
  color:var(--muted); letter-spacing:.06em; pointer-events:none; text-transform:uppercase; }
.readout i { font-style:normal; color:var(--fg); }
.toolbar { position:absolute; left:0; right:0; margin:0 auto; width:max-content; bottom:16px; z-index:5; display:flex; flex-wrap:wrap; justify-content:center;
  align-items:center; gap:4px; padding:5px; border-radius:999px; background:var(--glass); border:1px solid var(--line);
  -webkit-backdrop-filter:blur(14px) saturate(150%); backdrop-filter:blur(14px) saturate(150%);
  box-shadow:0 12px 32px rgba(0,0,0,.22); max-width:calc(100% - 24px); }
button { all:unset; cursor:pointer; display:inline-flex; align-items:center; gap:6px; padding:7px 11px; border-radius:999px;
  font-size:12px; font-weight:500; line-height:1; color:var(--fg); transition:background .2s, color .2s; }
button:hover { background:var(--hover); }
button:focus-visible { outline:2px solid var(--accent); outline-offset:1px; }
button[aria-pressed=true] { background:var(--fg); color:var(--solid); }
button.icon { padding:7px 9px; }
button.primary { padding:7px 14px 7px 11px; }
.seg { display:inline-flex; gap:2px; padding:2px; border-radius:999px; background:var(--seg); }
.sep { width:1px; height:18px; background:var(--line); margin:0 4px; }
input[type=range] { -webkit-appearance:none; appearance:none; width:110px; height:22px; background:transparent; margin:0 6px; cursor:pointer; }
input[type=range]::-webkit-slider-runnable-track { height:3px; border-radius:3px; background:linear-gradient(90deg, var(--fg) var(--p,0%), var(--line) var(--p,0%)); }
input[type=range]::-moz-range-track { height:3px; border-radius:3px; background:var(--line); }
input[type=range]::-moz-range-progress { height:3px; border-radius:3px; background:var(--fg); }
input[type=range]::-webkit-slider-thumb { -webkit-appearance:none; width:14px; height:14px; margin-top:-5.5px; border-radius:50%; background:var(--fg); border:3px solid var(--solid); box-shadow:0 0 0 1px var(--line); }
input[type=range]::-moz-range-thumb { width:10px; height:10px; border-radius:50%; background:var(--fg); border:3px solid var(--solid); }
.loading { position:absolute; inset:0; z-index:6; display:grid; place-items:center; pointer-events:none; color:var(--muted);
  font-size:11px; letter-spacing:.22em; text-transform:uppercase; transition:opacity .6s; }
.loading.done { opacity:0; }
.capture .toolbar, .capture .hint { display:none; }
.stage.figure { --bg:#ffffff; background:#ffffff; }
.stage.figure.transparent { background:transparent; }
.figure .brand, .figure .readout, .figure .vig, .figure .labels, .figure svg.lead, .figure .hint, .figure .toolbar { display:none; }
@container (max-width: 980px) { .stage:not(.capture) .readout { display:none; } }
@container (max-width: 760px) { .hint { display:none; } input[type=range] { width:84px; } .sep { margin:0 1px; } button { padding:7px 9px; } }
.capture .brand { left:56px; top:48px; } .capture .brand b { font-size:20px; } .capture .brand span { font-size:15px; margin-top:8px; }
.capture .readout { left:56px; bottom:48px; font-size:13px; }
.capture .label .t { font-size:15px; } .capture .label .s { font-size:13px; } .capture .label { padding:8px 13px 9px; }
@media (max-width: 640px) {
  :host { height:440px; }
  .hint, .readout { display:none; } .label .s { display:none; } .label { padding:4px 8px; }
  input[type=range] { width:76px; } button { padding:7px 9px; } .brand span { display:none; }
}
`;

/* ------------------------------------------------------------------ mount */
export async function mount(host) {
  const langAttr = (host.getAttribute('lang') || document.documentElement.lang || 'en').toLowerCase();
  const lang = langAttr.startsWith('zh') ? 'zh' : 'en';
  const T = TEXT[lang];
  const capture = host.hasAttribute('capture');
  const brand = host.getAttribute('brand') || T.brand;           // e.g. brand="NEXNODE" on sister projects
  const tagline = host.getAttribute('tagline') || T.tagline;
  const modelsBase = host.getAttribute('models') || new URL('../../HARDWARE-ESP/FUSION/', import.meta.url).href;

  const root = host.shadowRoot || host.attachShadow({ mode: 'open' });
  root.innerHTML = `<style>${CSS}</style>
  <div class="stage${capture ? ' capture' : ''}" data-theme="dark" data-mode="studio">
    <canvas tabindex="0" aria-label="LiftNode 3D model"></canvas>
    <div class="vig"></div>
    <svg class="lead"></svg>
    <div class="labels"></div>
    <div class="brand"><b>${brand}</b><span>${tagline}</span></div>
    <div class="hint">${T.hint}</div>
    <div class="readout"></div>
    <div class="toolbar" role="toolbar">
      <button class="primary" data-act="toggle">${ICON.play}<span>${T.explode}</span></button>
      <input type="range" min="0" max="1000" value="0" aria-label="${T.explodeAmt}">
      <span class="sep"></span>
      <div class="seg" role="group">
        <button data-mode="studio" aria-pressed="true">${T.studio}</button>
        <button data-mode="xray" aria-pressed="false">${T.xray}</button>
        <button data-mode="blueprint" aria-pressed="false">${T.blueprint}</button>
      </div>
      <span class="sep"></span>
      <button class="icon" data-act="rotate" aria-pressed="true" title="${T.rotate}" aria-label="${T.rotate}">${ICON.rotate}</button>
      <button class="icon" data-act="reset" title="${T.reset}" aria-label="${T.reset}">${ICON.reset}</button>
      <button class="icon" data-act="fs" title="${T.fullscreen}" aria-label="${T.fullscreen}">${ICON.fs}</button>
    </div>
    <div class="loading">${T.loading}</div>
  </div>`;
  const $ = (s) => root.querySelector(s);
  const stage = $('.stage'), canvas = $('canvas'), svg = $('svg.lead'), labelsEl = $('.labels');
  const slider = $('input[type=range]'), toggleBtn = $('[data-act=toggle]'), readout = $('.readout');

  /* ---------- renderer / scene */
  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true, preserveDrawingBuffer: capture, powerPreference: 'high-performance' });
  renderer.setClearColor(0x000000, 0);
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.05;
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.VSMShadowMap;

  const scene = new THREE.Scene();
  const pmrem = new THREE.PMREMGenerator(renderer);
  const envTex = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;
  scene.environment = envTex;
  scene.environmentIntensity = 0.55;

  const camera = new THREE.PerspectiveCamera(28, 1, 5, 6000);
  const controls = new OrbitControls(camera, canvas);
  controls.enableDamping = true; controls.dampingFactor = 0.08;
  controls.enablePan = false; controls.rotateSpeed = 0.7; controls.zoomSpeed = 0.8;
  controls.minPolarAngle = 8 * DEG; controls.maxPolarAngle = 100 * DEG;
  controls.autoRotateSpeed = 0.55;
  const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
  controls.autoRotate = !capture && !reducedMotion;
  if (!controls.autoRotate) $('[data-act=rotate]').setAttribute('aria-pressed', 'false');

  const key = new THREE.DirectionalLight(0xfff6ea, 2.8);
  key.position.set(110, 420, 160);
  key.castShadow = true;
  key.shadow.mapSize.set(2048, 2048);
  Object.assign(key.shadow.camera, { left: -230, right: 230, top: 230, bottom: -230, near: 50, far: 1200 });
  key.shadow.bias = -0.0006; key.shadow.normalBias = 0.35; key.shadow.radius = 14; key.shadow.blurSamples = 24;
  scene.add(key);
  const rim = new THREE.DirectionalLight(0xa8c2ff, 1.8);
  rim.position.set(-280, 170, -260);
  scene.add(rim);
  const fill = new THREE.DirectionalLight(0xffffff, 0.6);
  fill.position.set(-120, 60, 260);
  scene.add(fill);

  /* post: MSAA render → GTAO (ground-truth ambient occlusion, gives the CAD parts contact depth) → tone map */
  const aoEnabled = !host.hasAttribute('no-ao');
  const composer = new EffectComposer(renderer, new THREE.WebGLRenderTarget(1, 1, { type: THREE.HalfFloatType, samples: 4 }));
  composer.addPass(new RenderPass(scene, camera));
  const gtao = new GTAOPass(scene, camera, 1, 1);
  gtao.blendIntensity = 0.9;
  gtao.updateGtaoMaterial({ radius: 10, distanceExponent: 1.5, thickness: 5, scale: 1.15, samples: 16, distanceFallOff: 1, screenSpaceRadius: false });
  gtao.updatePdMaterial({ lumaPhi: 10, depthPhi: 2, normalPhi: 3, radius: 6, radiusExponent: 1, rings: 2, samples: 16 });
  const baseOverride = gtao.overrideVisibility.bind(gtao);
  gtao.overrideVisibility = () => {
    baseOverride();
    // in X-ray the translucent shells must not occlude the parts seen through them
    if (state.xray > 0.5) for (const id of SHELL) partGroups[id].visible = false;
    for (const o of [blob, stageRing]) o.visible = false;
  };
  gtao.enabled = aoEnabled;
  composer.addPass(gtao);
  composer.addPass(new OutputPass());

  const GROUND_Y = -34;
  const ground = new THREE.Mesh(new THREE.PlaneGeometry(4000, 4000), new THREE.ShadowMaterial({ opacity: 0.4 }));
  ground.rotation.x = -Math.PI / 2; ground.position.y = GROUND_Y; ground.receiveShadow = true;
  scene.add(ground);
  const blob = (() => {
    const c = document.createElement('canvas'); c.width = c.height = 256;
    const g = c.getContext('2d'); const gr = g.createRadialGradient(128, 128, 0, 128, 128, 128);
    gr.addColorStop(0, 'rgba(0,0,0,0.55)'); gr.addColorStop(0.5, 'rgba(0,0,0,0.18)'); gr.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = gr; g.fillRect(0, 0, 256, 256);
    const m = new THREE.Mesh(new THREE.PlaneGeometry(1, 1), new THREE.MeshBasicMaterial({ map: new THREE.CanvasTexture(c), transparent: true, depthWrite: false }));
    m.rotation.x = -Math.PI / 2; m.position.set(0, GROUND_Y + 0.1, 0); m.scale.set(300, 220, 1);
    scene.add(m); return m;
  })();
  // turntable "stage": fine concentric rings + degree ticks fading out, drawn on the ground
  const stageRing = (() => {
    const N = 1024, c = document.createElement('canvas'); c.width = c.height = N;
    const g = c.getContext('2d'); const C = N / 2;
    g.strokeStyle = '#fff'; g.fillStyle = '#fff';
    for (const [r, a, w] of [[0.46, 0.55, 2], [0.43, 0.25, 1], [0.30, 0.18, 1], [0.18, 0.12, 1]]) {
      g.globalAlpha = a; g.lineWidth = w; g.beginPath(); g.arc(C, C, r * N, 0, Math.PI * 2); g.stroke();
    }
    for (let i = 0; i < 180; i++) {
      const t = (i / 180) * Math.PI * 2, major = i % 15 === 0, r0 = 0.43 * N, r1 = (major ? 0.475 : 0.455) * N;
      g.globalAlpha = major ? 0.6 : 0.28; g.lineWidth = major ? 2.2 : 1.2;
      g.beginPath(); g.moveTo(C + Math.cos(t) * r0, C + Math.sin(t) * r0); g.lineTo(C + Math.cos(t) * r1, C + Math.sin(t) * r1); g.stroke();
    }
    g.globalAlpha = 1; g.globalCompositeOperation = 'destination-in';
    const fade = g.createRadialGradient(C, C, 0, C, C, C);
    fade.addColorStop(0, 'rgba(0,0,0,1)'); fade.addColorStop(0.85, 'rgba(0,0,0,0.9)'); fade.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = fade; g.fillRect(0, 0, N, N);
    const tex = new THREE.CanvasTexture(c); tex.anisotropy = 8;
    const m = new THREE.Mesh(new THREE.PlaneGeometry(1, 1), new THREE.MeshBasicMaterial({ map: tex, transparent: true, depthWrite: false, opacity: 0.16, color: 0xffffff }));
    m.rotation.x = -Math.PI / 2; m.position.set(0, GROUND_Y + 0.2, 0); m.scale.set(520, 520, 1);
    m.renderOrder = -1;
    scene.add(m); return m;
  })();

  /* ---------- assembly (model frame: mm, Z-up → world Y-up) */
  const assembly = new THREE.Group();
  assembly.rotation.x = -Math.PI / 2;
  assembly.position.set(-50, 0, 37.5);
  scene.add(assembly);

  if (document.fonts && document.fonts.ready) { try { await document.fonts.ready; } catch { /* ignore */ } }
  const M = makeMaterials();
  const loader = new STLLoader();
  const localModels = new URL('./models/', import.meta.url).href;
  const bmsPromise = new GLTFLoader().loadAsync(new URL('bms.glb', localModels).href);
  const texLoader = new THREE.TextureLoader();
  const [mainTex, extTex] = await Promise.all(['mainboard-top.jpg', 'extboard-top.jpg'].map((f) => texLoader.loadAsync(new URL(f, localModels).href)));
  for (const t of [mainTex, extTex]) { t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = renderer.capabilities.getMaxAnisotropy(); }
  M.mainTop = mainTex; M.extTop = extTex;
  const stlGeoms = await Promise.all(STL_FILES.map(async ([id, file, where]) => {
    const geom = await loader.loadAsync(new URL(file, where === 'local' ? localModels : modelsBase).href);
    geom.deleteAttribute('color');
    const off = STL_OFFSETS[id]; if (off) geom.translate(off[0], off[1], off[2]);
    return [id, toCreasedNormals(geom, 32 * DEG)];
  }));
  const stl = Object.fromEntries(stlGeoms);
  const figureMode = host.hasAttribute('figure');
  let bodyFullGeom = null;
  if (figureMode) {
    const g = await loader.loadAsync(new URL('ENCLOSURE-3D-BODY.stl', modelsBase).href);
    g.deleteAttribute('color');
    bodyFullGeom = toCreasedNormals(g, 32 * DEG);
  }
  const bmsGltf = await bmsPromise;

  const partGroups = {};
  const mk = (id, child) => { const g = new THREE.Group(); g.add(child); g.userData.partId = id; assembly.add(g); partGroups[id] = g; return g; };
  for (const id of ['base', 'wallFront', 'wallBack', 'wallLeft', 'wallRight']) mk(id, new THREE.Mesh(stl[id], M.shellBody));
  mk('lid', new THREE.Mesh(stl.lid, M.shellLid));
  mk('plate', new THREE.Mesh(stl.plate, M.frame));
  mk('bmsFrame', new THREE.Mesh(stl.bmsFrame, M.frame));
  mk('board', buildMainboard(M));
  mk('extBoard', buildExtBoard(M));
  mk('bms', buildBMS(bmsGltf.scene, M));
  mk('battery', buildBattery(M));
  mk('antenna', buildAntenna(M));
  mk('magnetsL', buildMagnets(M, -6.2));
  mk('magnetsR', buildMagnets(M, 106.2));
  const wires = buildWires(M); assembly.add(wires);
  const wireMats = [M.wireRed, M.wireBlue, M.wireBlack, M.coax];

  /* edges + bookkeeping */
  const meshes = [];
  const edgeMats = {};
  for (const p of PARTS) {
    const shell = SHELL.has(p.id);
    const em = new THREE.LineBasicMaterial({ color: 0x1d1c18, transparent: true, opacity: shell ? 0.22 : 0.1, depthWrite: false });
    edgeMats[p.id] = em;
    const grp = partGroups[p.id];
    grp.traverse((o) => {
      if (!o.isMesh) return;
      o.castShadow = true; o.receiveShadow = true;
      o.userData.partId = p.id; o.userData.mat = o.material;
      meshes.push(o);
      if (o.isInstancedMesh || o.userData.noEdges) return;
      const thr = shell ? 22 : 35;
      const pos = o.geometry.attributes.position;
      if (pos && pos.count > 6000 && !shell && p.id !== 'plate' && p.id !== 'bmsFrame') return;
      const lines = new THREE.LineSegments(new THREE.EdgesGeometry(o.geometry, thr), em);
      lines.raycast = () => {};
      lines.userData.isEdge = true;
      o.add(lines);
    });
  }
  wires.traverse((o) => { if (o.isMesh) { o.castShadow = true; o.userData.mat = o.material; o.userData.wire = true; meshes.push(o); } });
  const basePos = Object.fromEntries(PARTS.map((p) => [p.id, partGroups[p.id].position.clone()]));
  let bodyFull = null;
  if (bodyFullGeom) {
    const mesh = new THREE.Mesh(bodyFullGeom, M.shellBody);
    mesh.castShadow = true; mesh.receiveShadow = true;
    const em = new THREE.LineBasicMaterial({ color: 0x111111, transparent: true, opacity: 0.5, depthWrite: false });
    edgeMats.bodyFull = em;
    mesh.add(new THREE.LineSegments(new THREE.EdgesGeometry(bodyFullGeom, 22), em));
    bodyFull = new THREE.Group(); bodyFull.add(mesh); bodyFull.visible = false;
    assembly.add(bodyFull);
  }

  /* dashed travel lines (classic exploded-view leaders): assembled point → current point */
  const guideList = PARTS.flatMap((p) => (p.guides || []).map((pt) => ({ p, pt })));
  const guideGeom = new THREE.BufferGeometry();
  guideGeom.setAttribute('position', new THREE.BufferAttribute(new Float32Array(guideList.length * 6), 3));
  const guideMat = new THREE.LineDashedMaterial({ color: 0x9fb3d1, dashSize: 2.4, gapSize: 2.2, transparent: true, opacity: 0, depthWrite: false });
  const guides = new THREE.LineSegments(guideGeom, guideMat);
  guides.frustumCulled = false; guides.raycast = () => {};
  assembly.add(guides);

  /* ---------- state */
  const state = {
    explode: 0, tween: null, xray: 0, xrayTarget: 0, mode: 'studio', theme: 'dark',
    hover: null, labelAlpha: 0, zoom: 1, lastD: 0, w: 1, h: 1, visible: true, running: false,
    idleUntil: 0, autoRotate: controls.autoRotate,
  };

  const _q = new THREE.Quaternion(), _axis = new THREE.Vector3(), _piv = new THREE.Vector3();
  function applyExplode(e) {
    for (const p of PARTS) {
      const k = easeInOut(clamp01((e - p.start) / STAGGER_SPAN));
      const g = partGroups[p.id];
      g.position.set(basePos[p.id].x + p.dir[0] * k, basePos[p.id].y + p.dir[1] * k, basePos[p.id].z + p.dir[2] * k);
      if (p.rot) {
        // hinge about the wall's outer bottom edge: R about pivot = translate(pivot − R·pivot) · R
        _q.setFromAxisAngle(_axis.set(...p.rot.axis), p.rot.angle * DEG * k);
        g.quaternion.copy(_q);
        _piv.set(...p.rot.pivot);
        g.position.add(_piv).sub(_piv.applyQuaternion(_q));
      }
      g.updateMatrix();
    }
    const gp = guideGeom.attributes.position.array;
    guideList.forEach(({ p, pt }, i) => {
      _piv.set(...pt).applyMatrix4(partGroups[p.id].matrix);
      gp.set([pt[0], pt[1], pt[2], _piv.x, _piv.y, _piv.z], i * 6);
    });
    guideGeom.attributes.position.needsUpdate = true;
    guides.computeLineDistances();
    guideMat.opacity = smooth(0.04, 0.3, e) * (state.mode === 'blueprint' ? 0.8 : 0.55);
    const wa = 1 - smooth(0, 0.06, e);
    for (const m of wireMats) { m.opacity = wa; m.depthWrite = wa > 0.98; }
    wires.visible = wa > 0.002;
  }

  /* framing: bounding spheres at e=0 and e=1 */
  const sphere0 = new THREE.Sphere(), sphere1 = new THREE.Sphere();
  const fitBox = new THREE.Box3();
  const fitObjects = () => { fitBox.makeEmpty(); for (const id in partGroups) fitBox.expandByObject(partGroups[id]); return fitBox; };
  applyExplode(0); scene.updateMatrixWorld(true); fitObjects().getBoundingSphere(sphere0);
  applyExplode(1); scene.updateMatrixWorld(true); fitObjects().getBoundingSphere(sphere1);
  applyExplode(0);

  function fitDistance(e) {
    const R = lerp(sphere0.radius, sphere1.radius, e);
    const vf = camera.fov * DEG;
    const hf = 2 * Math.atan(Math.tan(vf / 2) * camera.aspect);
    const f = Math.min(vf, hf);
    return (R / Math.sin(f / 2)) * lerp(1.0, 0.8, e); // bounding sphere over-estimates the flat exploded layout
  }
  const fitCenter = (e, out) => out.copy(sphere0.center).lerp(sphere1.center, e);

  const HOME = { theta: 34 * DEG, phi: 70 * DEG };
  function placeCamera(theta, phi, e, zoom = 1) {
    fitCenter(e, controls.target);
    const d = fitDistance(e) * zoom;
    camera.position.set(
      controls.target.x + d * Math.sin(phi) * Math.sin(theta),
      controls.target.y + d * Math.cos(phi),
      controls.target.z + d * Math.sin(phi) * Math.cos(theta));
    camera.lookAt(controls.target);
    state.lastD = d;
  }

  /* ---------- modes & theme */
  const bpFill = new THREE.MeshBasicMaterial({ color: 0x3d8bff, transparent: true, opacity: 0.055, depthWrite: false, side: THREE.DoubleSide });
  function setMode(mode) {
    state.mode = mode;
    stage.dataset.mode = mode;
    root.querySelectorAll('[data-mode]').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.mode === mode)));
    state.xrayTarget = mode === 'xray' ? 1 : 0;
    const bp = mode === 'blueprint';
    for (const m of meshes) m.material = bp ? bpFill : m.userData.mat;
    ground.visible = !bp; blob.visible = !bp; stageRing.visible = !bp; gtao.enabled = !bp && aoEnabled;
    renderer.shadowMap.enabled = !bp;
    applyEdgeStyle();
  }
  function applyEdgeStyle() {
    const bp = state.mode === 'blueprint';
    for (const p of PARTS) {
      const em = edgeMats[p.id];
      const shell = SHELL.has(p.id);
      const hot = state.hover === (ALIAS[p.id] || p.id);
      if (bp) { em.color.set(hot ? 0xffffff : 0xbfe0ff); em.opacity = hot ? 1 : shell ? 0.75 : 0.5; }
      else if (hot) { em.color.set(state.theme === 'dark' ? 0x8fb4ff : 0x3565e8); em.opacity = 0.95; }
      else {
        em.color.set(state.xray > 0.5 && shell ? (state.theme === 'dark' ? 0xb9c8e6 : 0x50607a) : 0x1d1c18);
        em.opacity = shell ? lerp(0.22, 0.5, state.xray) : 0.1;
      }
    }
    guideMat.color.set(bp ? 0xcfe8ff : state.theme === 'dark' ? 0x9fb3d1 : 0x55647d);
  }
  function applyXray(x) {
    for (const m of [M.shellBody, M.shellLid]) {
      m.opacity = lerp(1, 0.12, x);
      m.depthWrite = x < 0.02;
    }
  }
  function detectTheme() {
    const a = host.getAttribute('theme');
    if (a === 'dark' || a === 'light') return a;
    const s = document.body && document.body.getAttribute('data-md-color-scheme');
    if (s) return s === 'slate' ? 'dark' : 'light';
    return matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
  }
  function applyTheme() {
    state.theme = detectTheme();
    stage.dataset.theme = state.theme;
    ground.material.opacity = state.theme === 'dark' ? 0.45 : 0.22;
    blob.material.opacity = state.theme === 'dark' ? 1 : 0.55;
    stageRing.material.color.set(state.theme === 'dark' ? 0xffffff : 0x1b2433);
    stageRing.material.opacity = state.theme === 'dark' ? 0.16 : 0.2;
    renderer.toneMappingExposure = state.theme === 'dark' ? 1.05 : 1.0;
    applyEdgeStyle();
  }
  applyTheme();
  const themeObs = new MutationObserver(applyTheme);
  if (document.body) themeObs.observe(document.body, { attributes: true, attributeFilter: ['data-md-color-scheme'] });
  const mql = matchMedia('(prefers-color-scheme: dark)');
  mql.addEventListener('change', applyTheme);

  /* ---------- labels */
  const labelNodes = {};
  const LABELED = PARTS.filter((p) => p.label !== false);
  LABELED.forEach((p, i) => {
    const el = document.createElement('div');
    el.className = 'label';
    el.innerHTML = `<div class="t"><span class="n">${String(i + 1).padStart(2, '0')}</span><span>${p.name[lang]}</span></div><div class="s">${p.sub[lang]}</div>`;
    el.style.opacity = '0';
    labelsEl.appendChild(el);
    const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    g.innerHTML = '<polyline/><circle class="r" r="6"/><circle class="d" r="2.6"/>';
    g.style.opacity = '0';
    svg.appendChild(g);
    labelNodes[p.id] = { el, g, line: g.querySelector('polyline'), r: g.querySelector('.r'), d: g.querySelector('.d'), w: 0, h: 0 };
  });
  const tmpV = new THREE.Vector3();
  function layoutLabels(alphaAll) {
    const W = state.w, H = state.h, cx = W / 2;
    const items = [];
    for (const p of LABELED) {
      const n = labelNodes[p.id];
      const a = Math.max(alphaAll, state.hover === p.id ? 1 : 0);
      if (a < 0.01) { n.el.style.opacity = '0'; n.g.style.opacity = '0'; continue; }
      tmpV.set(...p.anchor);
      partGroups[p.id].localToWorld(tmpV);
      tmpV.project(camera);
      if (tmpV.z > 1) continue;
      const sx = (tmpV.x * 0.5 + 0.5) * W, sy = (-tmpV.y * 0.5 + 0.5) * H;
      if (!n.w) { n.w = n.el.offsetWidth; n.h = n.el.offsetHeight; }
      items.push({ p, n, a, sx, sy, left: sx < cx });
    }
    const gap = W < 640 ? 34 : 70;
    const bottomLimit = H - (capture ? 40 : 76), topLimit = capture ? 120 : 64;
    for (const side of [true, false]) {
      const list = items.filter((i) => i.left === side).sort((a, b) => a.sy - b.sy);
      let y = -Infinity;
      for (const it of list) { it.y = Math.max(it.sy - it.n.h / 2, y, topLimit); y = it.y + it.n.h + 8; }
      const over = list.length ? list[list.length - 1].y + list[list.length - 1].n.h - bottomLimit : 0;
      if (over > 0) for (let i = list.length - 1, push = over; i >= 0; i--) {
        list[i].y -= push;
        if (i > 0) push = Math.max(0, list[i - 1].y + list[i - 1].n.h + 8 - list[i].y);
      }
      for (const it of list) {
        const { n } = it;
        const x = side ? Math.max(12, it.sx - gap - n.w) : Math.min(W - n.w - 12, it.sx + gap);
        const yc = it.y + n.h / 2;
        const edgeX = side ? x + n.w : x;
        const elbowX = side ? edgeX + 14 : edgeX - 14;
        n.el.style.transform = `translate(${x.toFixed(1)}px, ${it.y.toFixed(1)}px)`;
        n.el.style.opacity = it.a.toFixed(3);
        n.el.classList.toggle('hot', state.hover === it.p.id);
        n.line.setAttribute('points', `${it.sx.toFixed(1)},${it.sy.toFixed(1)} ${elbowX.toFixed(1)},${yc.toFixed(1)} ${edgeX.toFixed(1)},${yc.toFixed(1)}`);
        n.r.setAttribute('cx', it.sx.toFixed(1)); n.r.setAttribute('cy', it.sy.toFixed(1));
        n.d.setAttribute('cx', it.sx.toFixed(1)); n.d.setAttribute('cy', it.sy.toFixed(1));
        n.g.style.opacity = it.a.toFixed(3);
        n.g.classList.toggle('hot', state.hover === it.p.id);
      }
    }
  }

  /* ---------- sizing */
  function resize(dpr) {
    const r = host.getBoundingClientRect();
    state.w = Math.max(1, Math.round(r.width)); state.h = Math.max(1, Math.round(r.height));
    const pr = dpr || (capture ? window.devicePixelRatio || 1 : Math.min(window.devicePixelRatio || 1, 1.75));
    renderer.setPixelRatio(pr);
    renderer.setSize(state.w, state.h, false);
    composer.setPixelRatio(pr);
    composer.setSize(state.w, state.h);
    camera.aspect = state.w / state.h;
    const lift = capture ? 0 : Math.min(34, state.h * 0.05);
    camera.setViewOffset(state.w, state.h, 0, lift, state.w, state.h);
    camera.updateProjectionMatrix();
    for (const id in labelNodes) labelNodes[id].w = 0;
  }
  resize();
  const ro = new ResizeObserver(() => { resize(); if (!state.running) renderOnce(); });
  ro.observe(host);

  /* ---------- interaction */
  const raycaster = new THREE.Raycaster();
  const pointer = new THREE.Vector2();
  let pointerDirty = false, pointerInside = false;
  canvas.addEventListener('pointermove', (ev) => {
    const r = canvas.getBoundingClientRect();
    pointer.set(((ev.clientX - r.left) / r.width) * 2 - 1, -((ev.clientY - r.top) / r.height) * 2 + 1);
    pointerDirty = true; pointerInside = true;
  });
  canvas.addEventListener('pointerleave', () => { pointerInside = false; setHover(null); });
  function setHover(id) {
    id = id && (ALIAS[id] || id);
    if (state.hover === id) return;
    state.hover = id; applyEdgeStyle();
    canvas.style.cursor = id ? 'pointer' : '';
  }
  function pick() {
    raycaster.setFromCamera(pointer, camera);
    const targets = meshes.filter((m) => !m.userData.wire && !(state.xray > 0.5 && SHELL.has(m.userData.partId)));
    const hit = raycaster.intersectObjects(targets, false)[0];
    setHover(hit ? hit.object.userData.partId : null);
  }

  function playTo(target) {
    state.tween = { from: state.explode, to: target, t0: performance.now(), dur: 2400 * Math.abs(target - state.explode) + 200 };
    updateToggle(target);
    kick();
  }
  function updateToggle(target) {
    const exploding = target > 0.5;
    toggleBtn.innerHTML = `${exploding ? ICON.back : ICON.play}<span>${exploding ? T.assemble : T.explode}</span>`;
  }
  toggleBtn.addEventListener('click', () => playTo(state.explode > 0.5 || (state.tween && state.tween.to > 0.5) ? 0 : 1));
  canvas.addEventListener('dblclick', () => playTo(state.explode > 0.5 ? 0 : 1));
  slider.addEventListener('input', () => {
    state.tween = null; state.explode = slider.value / 1000;
    updateToggle(state.explode > 0.5 ? 1 : 0); kick();
  });
  root.querySelectorAll('[data-mode]').forEach((b) => b.addEventListener('click', () => { setMode(b.dataset.mode); kick(); }));
  const rotBtn = $('[data-act=rotate]');
  rotBtn.addEventListener('click', () => {
    state.autoRotate = !state.autoRotate; controls.autoRotate = state.autoRotate;
    rotBtn.setAttribute('aria-pressed', String(state.autoRotate)); kick();
  });
  $('[data-act=reset]').addEventListener('click', () => {
    state.zoom = 1; placeCamera(HOME.theta, HOME.phi, state.explode); controls.update(); kick();
  });
  $('[data-act=fs]').addEventListener('click', () => {
    if (document.fullscreenElement) document.exitFullscreen(); else host.requestFullscreen && host.requestFullscreen();
  });
  // Inside a docs page the wheel scrolls the page until the viewer is clicked (or fullscreen).
  const armZoom = (on) => {
    controls.enableZoom = on || !!document.fullscreenElement;
    canvas.style.touchAction = document.fullscreenElement ? 'none' : 'pan-y'; // let phones scroll the page past the viewer
  };
  armZoom(false);
  canvas.addEventListener('pointerdown', () => armZoom(true));
  host.addEventListener('pointerleave', () => armZoom(false));
  const onFs = () => { armZoom(false); resize(); kick(); };
  document.addEventListener('fullscreenchange', onFs);
  controls.addEventListener('start', () => { controls.autoRotate = false; state.idleUntil = Infinity; $('.hint').style.opacity = '0'; });
  controls.addEventListener('end', () => { state.idleUntil = performance.now() + 5000; });
  controls.addEventListener('change', kick);

  /* ---------- frame */
  function updateReadout() {
    const pct = Math.round(state.explode * 100);
    const m = state.mode === 'studio' && state.xray > 0.5 ? 'xray' : state.mode;
    const modeName = { studio: T.studio, xray: T.xray, blueprint: T.blueprint }[m];
    readout.innerHTML = `${T.explodeAmt} <i>${String(pct).padStart(3, ' ')}%</i> &nbsp;·&nbsp; <i>${modeName}</i> &nbsp;·&nbsp; ${COMPONENT_COUNT} ${T.parts}`;
  }
  function renderOnce() {
    composer.render();
    layoutLabels(state.labelAlpha);
  }

  let raf = 0, lastT = performance.now(), quietFrames = 0;
  function frame(now) {
    raf = 0;
    const dt = Math.min(0.05, (now - lastT) / 1000); lastT = now;
    let active = false;
    if (state.tween) {
      const k = clamp01((now - state.tween.t0) / state.tween.dur);
      state.explode = lerp(state.tween.from, state.tween.to, easeInOut(k));
      if (k >= 1) state.tween = null;
      active = true;
    }
    slider.value = String(Math.round(state.explode * 1000));
    slider.style.setProperty('--p', `${(state.explode * 100).toFixed(1)}%`);
    if (Math.abs(state.xray - state.xrayTarget) > 0.001) {
      state.xray += (state.xrayTarget - state.xray) * Math.min(1, dt * 6); active = true;
      applyXray(state.xray); applyEdgeStyle();
    } else if (state.xray !== state.xrayTarget) { state.xray = state.xrayTarget; applyXray(state.xray); applyEdgeStyle(); }
    applyExplode(state.explode);

    if (!controls.autoRotate && state.autoRotate && now > state.idleUntil) controls.autoRotate = true;
    controls.update(dt);
    // zoom is relative to the auto-framed distance so the stack stays in frame while exploding
    const r = camera.position.distanceTo(controls.target);
    if (state.lastD && Math.abs(r - state.lastD) > 1e-3) state.zoom = Math.min(2.6, Math.max(0.42, state.zoom * (r / state.lastD)));
    const dir = tmpV.copy(camera.position).sub(controls.target).normalize();
    fitCenter(state.explode, controls.target);
    const d = fitDistance(state.explode) * state.zoom;
    camera.position.copy(controls.target).addScaledVector(dir, d);
    camera.lookAt(controls.target);
    state.lastD = d;

    if (pointerDirty && pointerInside) { pick(); pointerDirty = false; }
    const la = smooth(0.62, 0.95, state.explode);
    if (Math.abs(la - state.labelAlpha) > 1e-3) active = true;
    state.labelAlpha = la;
    updateReadout();
    renderOnce();

    quietFrames = active || controls.autoRotate || state.tween ? 0 : quietFrames + 1;
    if (state.visible && quietFrames < 90) raf = requestAnimationFrame(frame);
    else state.running = false;
  }
  function kick() {
    if (capture || raf || !state.visible) return;
    state.running = true; quietFrames = 0; lastT = performance.now();
    raf = requestAnimationFrame(frame);
  }
  const io = new IntersectionObserver((es) => {
    state.visible = es.some((e) => e.isIntersecting) && !document.hidden;
    if (state.visible) kick();
  });
  io.observe(host);
  const onVis = () => { state.visible = !document.hidden; if (state.visible) kick(); };
  document.addEventListener('visibilitychange', onVis);

  /* ---------- start */
  setMode('studio');
  placeCamera(HOME.theta, HOME.phi, 0);
  controls.update();
  updateReadout();
  { const ld = $('.loading'); if (capture) ld.remove(); else { ld.classList.add('done'); setTimeout(() => ld.remove(), 700); } }
  if (!capture) kick(); else renderOnce();

  /* ---------- academic figure (thesis): white background, near-orthographic camera, print styling */
  function figure(opts = {}) {
    if (!bodyFull) throw new Error('figure() needs the figure attribute on <liftnode-viewer>');
    const layoutName = opts.layout || 'axial';
    const LAYOUT = FIGURE_LAYOUTS[layoutName];
    const theta = (opts.theta ?? 36) * DEG, elev = (opts.elev ?? 24) * DEG;
    const marginX = opts.marginX ?? 0.2, marginY = opts.marginY ?? 0.05;
    stage.classList.add('figure');
    stage.classList.toggle('transparent', !!opts.transparent);
    state.mode = 'studio'; state.xray = 0; state.xrayTarget = 0; applyXray(0);
    for (const m of meshes) m.material = m.userData.mat;
    ground.visible = false; blob.visible = false; stageRing.visible = false;
    renderer.shadowMap.enabled = opts.shadows ?? true;
    renderer.toneMappingExposure = opts.exposure ?? 1.0;
    scene.environmentIntensity = opts.env ?? 0.85;
    gtao.enabled = opts.ao ?? true;
    rim.color.set(0xffffff); rim.intensity = 1.1;   // neutral light for print (no blue cast on the PCBs)

    // layout
    const walls = ['base', 'wallFront', 'wallBack', 'wallLeft', 'wallRight'];
    if (layoutName === 'unfold') {
      applyExplode(1);
      for (const id of walls) partGroups[id].visible = true;
      bodyFull.visible = false;
    } else {
      for (const id of walls) partGroups[id].visible = false;
      bodyFull.visible = true;
      for (const p of PARTS) {
        const L = LAYOUT[p.id]; if (!L) continue;
        const g = partGroups[p.id];
        g.quaternion.identity();
        g.position.set(basePos[p.id].x + L.dir[0], basePos[p.id].y + L.dir[1], basePos[p.id].z + L.dir[2]);
        g.updateMatrix();
      }
      const gp = guideGeom.attributes.position.array;
      guideList.forEach(({ p, pt }, i) => {
        _piv.set(...pt).applyMatrix4(partGroups[p.id].matrix);
        gp.set([pt[0], pt[1], pt[2], _piv.x, _piv.y, _piv.z], i * 6);
      });
      guideGeom.attributes.position.needsUpdate = true;
      guides.computeLineDistances();
    }
    if (opts.only) {
      for (const p of PARTS) partGroups[p.id].visible = opts.only.includes(p.id);
      bodyFull.visible = opts.only.includes('bodyFull');
    }
    wires.visible = false;
    guideMat.color.set(0x6b7280); guideMat.opacity = opts.guides === false || layoutName === 'assembled' ? 0 : 0.7;
    for (const id in edgeMats) {
      const shell = SHELL.has(id) || id === 'bodyFull';
      edgeMats[id].color.set(0x111111); edgeMats[id].opacity = shell ? 0.55 : 0.16;
    }
    scene.updateMatrixWorld(true);

    // near-orthographic camera (axonometric look), framed on the projected bounding box
    const roots = [];
    if (bodyFull.visible) roots.push(bodyFull);
    for (const p of PARTS) if (partGroups[p.id].visible && (LAYOUT[p.id] || layoutName === 'unfold')) roots.push(partGroups[p.id]);
    const box = new THREE.Box3(); for (const o of roots) box.expandByObject(o);
    const center = box.getCenter(new THREE.Vector3());
    const dir = new THREE.Vector3(Math.cos(elev) * Math.sin(theta), Math.sin(elev), Math.cos(elev) * Math.cos(theta));
    const D = 6000;
    camera.clearViewOffset();
    camera.near = D - 1500; camera.far = D + 1500;
    camera.position.copy(center).addScaledVector(dir, D); camera.lookAt(center); camera.updateMatrixWorld();
    const view = new THREE.Box3(), v = new THREE.Vector3();
    for (const o of roots) o.traverse((m) => {
      if (!m.isMesh || !m.geometry || m.isInstancedMesh) return;
      if (!m.geometry.boundingBox) m.geometry.computeBoundingBox();
      const bb = m.geometry.boundingBox;
      for (let i = 0; i < 8; i++) {
        v.set(i & 1 ? bb.max.x : bb.min.x, i & 2 ? bb.max.y : bb.min.y, i & 4 ? bb.max.z : bb.min.z)
          .applyMatrix4(m.matrixWorld).applyMatrix4(camera.matrixWorldInverse);
        view.expandByPoint(v);
      }
    });
    const vc = view.getCenter(new THREE.Vector3()), vs = view.getSize(new THREE.Vector3());
    const right = new THREE.Vector3().setFromMatrixColumn(camera.matrixWorld, 0);
    const up = new THREE.Vector3().setFromMatrixColumn(camera.matrixWorld, 1);
    const target = center.clone().addScaledVector(right, vc.x).addScaledVector(up, vc.y);
    camera.position.copy(target).addScaledVector(dir, D); camera.lookAt(target);
    const aspect = state.w / state.h;
    const halfH = Math.max(vs.y / 2 / (1 - 2 * marginY), vs.x / 2 / aspect / (1 - 2 * marginX));
    camera.fov = 2 * Math.atan(halfH / D) / DEG;
    camera.aspect = aspect;
    camera.updateProjectionMatrix(); camera.updateMatrixWorld();
    composer.render();

    // callout anchors in CSS px
    const anchors = [];
    for (const [id, L] of Object.entries(LAYOUT)) {
      if (!L.n) continue;
      const p = PARTS.find((q) => q.id === id) || {};
      const a = new THREE.Vector3(...(L.anchor || p.anchor));
      (id === 'bodyFull' ? bodyFull : partGroups[id]).localToWorld(a);
      a.project(camera);
      anchors.push({ n: L.n, id, x: (a.x * 0.5 + 0.5) * state.w, y: (-a.y * 0.5 + 0.5) * state.h,
        name: (L.name || p.name)[lang], sub: (L.sub || p.sub)[lang], names: L.name || p.name, side: L.side });
    }
    anchors.sort((a, b) => a.n - b.n);
    const points = (opts.points || []).map(([x, y, z]) => {
      const v = assembly.localToWorld(new THREE.Vector3(x, y, z)).project(camera);
      return [(v.x * 0.5 + 0.5) * state.w, (-v.y * 0.5 + 0.5) * state.h];
    });
    const halfWpx = (vs.x / 2) / (halfH * aspect) * (state.w / 2), halfHpx = (vs.y / 2) / halfH * (state.h / 2);
    const modelBox = { left: state.w / 2 - halfWpx, right: state.w / 2 + halfWpx, top: state.h / 2 - halfHpx, bottom: state.h / 2 + halfHpx };
    return { width: state.w, height: state.h, anchors, modelBox, points };
  }

  /* ---------- cinematic timeline (used by capture mode and app.renderAt) */
  const LOOP = 16;
  function timeline(t) {
    t = ((t % LOOP) + LOOP) % LOOP;
    const explode = smooth(1.8, 5.2, t) * (1 - smooth(9.0, 12.4, t));
    const xray = smooth(11.4, 12.8, t) * (1 - smooth(14.4, 15.8, t));
    const theta = HOME.theta + (2 * Math.PI * t) / LOOP;
    const phi = HOME.phi - 5 * DEG * Math.sin((2 * Math.PI * t) / LOOP);
    return { explode, xray, theta, phi };
  }

  const app = {
    LOOP,
    figure,
    setExplode(v) { state.tween = null; state.explode = clamp01(v); updateToggle(state.explode > 0.5 ? 1 : 0); kick(); },
    explode() { playTo(1); }, assemble() { playTo(0); },
    setMode(m) { setMode(m); kick(); },
    /** Deterministic frame for video capture. t in seconds along the 16 s loop. */
    renderAt(t, opts = {}) {
      const k = timeline(t);
      state.explode = k.explode; state.xray = k.xray; state.xrayTarget = k.xray;
      applyExplode(k.explode); applyXray(k.xray); applyEdgeStyle();
      state.labelAlpha = smooth(0.8, 0.98, k.explode);
      placeCamera(k.theta, k.phi, k.explode, opts.zoom || 1);
      updateReadout();
      renderOnce();
    },
    resize(dpr) { resize(dpr); renderOnce(); },
    /** Inspection hook (devtools / capture scripts): raw three.js objects. */
    debug: { THREE, scene, camera, controls, partGroups, renderOnce, state },
    dispose() {
      cancelAnimationFrame(raf); io.disconnect(); ro.disconnect(); themeObs.disconnect();
      mql.removeEventListener('change', applyTheme); document.removeEventListener('visibilitychange', onVis); document.removeEventListener('fullscreenchange', onFs);
      controls.dispose(); composer.dispose(); renderer.dispose(); pmrem.dispose(); envTex.dispose();
      scene.traverse((o) => { if (o.geometry) o.geometry.dispose(); });
    },
  };
  return app;
}
