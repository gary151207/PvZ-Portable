// PvZ-style icon drawing toolkit (dev scratch, not part of the game build).
const { createCanvas, loadImage } = require("@napi-rs/canvas");
const fs = require("fs");
const path = require("path");

const S = 1024; // final canvas size
const SS = 2; // supersample factor

function newCanvas(w = S, h = S) {
  const c = createCanvas(w * SS, h * SS);
  const ctx = c.getContext("2d");
  ctx.scale(SS, SS);
  return { c, ctx };
}

async function save(c, out) {
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, c.toBuffer("image/png"));
  console.log("wrote " + out);
}

// ---------- geometry ----------

function ellipsePath(ctx, x, y, rx, ry, rot = 0) {
  ctx.beginPath();
  ctx.ellipse(x, y, Math.abs(rx), Math.abs(ry), rot, 0, Math.PI * 2);
}

// blob: closed smooth outline through points, drawn with quadratic midpoints
function blobPath(ctx, pts, tension = 0.5) {
  const n = pts.length;
  ctx.beginPath();
  const mid = (a, b) => [(a[0] + b[0]) / 2, (a[1] + b[1]) / 2];
  let m0 = mid(pts[n - 1], pts[0]);
  ctx.moveTo(m0[0], m0[1]);
  for (let i = 0; i < n; i++) {
    const cur = pts[i];
    const next = pts[(i + 1) % n];
    const m = mid(cur, next);
    // control point pulled outward from cur for a rounder blob
    const cx = cur[0] + (1 - tension) * 0;
    const cy = cur[1] + (1 - tension) * 0;
    ctx.quadraticCurveTo(cx, cy, m[0], m[1]);
  }
  ctx.closePath();
}

// star / asterisk with n spikes
function starPath(ctx, x, y, spikes, outer, inner, rot = -Math.PI / 2) {
  ctx.beginPath();
  for (let i = 0; i < spikes * 2; i++) {
    const r = i % 2 ? inner : outer;
    const a = rot + (i * Math.PI) / spikes;
    const px = x + Math.cos(a) * r;
    const py = y + Math.sin(a) * r;
    i ? ctx.lineTo(px, py) : ctx.moveTo(px, py);
  }
  ctx.closePath();
}

// rounded rect path
function roundRectPath(ctx, x, y, w, h, r) {
  const rr = Math.min(r, w / 2, h / 2);
  ctx.beginPath();
  ctx.moveTo(x + rr, y);
  ctx.arcTo(x + w, y, x + w, y + h, rr);
  ctx.arcTo(x + w, y + h, x, y + h, rr);
  ctx.arcTo(x, y + h, x, y, rr);
  ctx.arcTo(x, y, x + w, y, rr);
  ctx.closePath();
}

function polyPath(ctx, pts) {
  ctx.beginPath();
  pts.forEach((p, i) => (i ? ctx.lineTo(p[0], p[1]) : ctx.moveTo(p[0], p[1])));
  ctx.closePath();
}

// ---------- painting ----------

const INK = "#0d1408";

/**
 * Core cartoon primitive: flat fill + thick dark outline.
 * opts: { fill, grad, line, lw, shadow, shade }
 */
function paint(ctx, pathFn, opts = {}) {
  const {
    fill = "#888",
    grad = null,
    line = INK,
    lw = 9,
    shadow = 0, // drop shadow offset (+x,+y) drawn under the shape
    shadowColor = "rgba(20,30,10,0.28)",
    shade = 0, // inner bottom shading strength 0..1
    shadeColor = "rgba(0,0,0,0.18)",
    highlight = 0,
  } = opts;

  if (shadow) {
    ctx.save();
    ctx.translate(shadow, shadow * 0.85);
    ctx.fillStyle = shadowColor;
    pathFn(ctx);
    ctx.fill();
    ctx.restore();
  }

  ctx.save();
  pathFn(ctx);
  if (grad) {
    ctx.fillStyle = grad(ctx);
  } else {
    ctx.fillStyle = fill;
  }
  ctx.fill();

  if (shade > 0) {
    ctx.save();
    ctx.clip();
    const bb = ctx.getTransform();
    ctx.fillStyle = shadeColor;
    ctx.globalAlpha = shade;
    pathFn(ctx);
    ctx.fill();
    ctx.globalAlpha = 1;
    ctx.restore();
  }
  ctx.restore();

  if (lw > 0) {
    ctx.save();
    ctx.lineJoin = "round";
    ctx.lineCap = "round";
    ctx.strokeStyle = line;
    ctx.lineWidth = lw;
    pathFn(ctx);
    ctx.stroke();
    ctx.restore();
  }
}

// linear gradient helper
function lin(x0, y0, x1, y1, stops) {
  return (ctx) => {
    const g = ctx.createLinearGradient(x0, y0, x1, y1);
    stops.forEach(([o, c]) => g.addColorStop(o, c));
    return g;
  };
}

function rad(x, y, r0, r1, stops) {
  return (ctx) => {
    const g = ctx.createRadialGradient(x, y, r0, x, y, r1);
    stops.forEach(([o, c]) => g.addColorStop(o, c));
    return g;
  };
}

// organic leaf / gun barrel: tapered lens shape between two points
function limb(ctx, x0, y0, x1, y1, w0, w1) {
  const a = Math.atan2(y1 - y0, x1 - x0);
  const nx = Math.cos(a + Math.PI / 2);
  const ny = Math.sin(a + Math.PI / 2);
  const mx = (x0 + x1) / 2;
  const my = (y0 + y1) / 2;
  const wm = (w0 + w1) / 2;
  ctx.beginPath();
  ctx.moveTo(x0 + nx * w0, y0 + ny * w0);
  ctx.quadraticCurveTo(mx + nx * wm * 1.15, my + ny * wm * 1.15, x1 + nx * w1, y1 + ny * w1);
  ctx.lineTo(x1 - nx * w1, y1 - ny * w1);
  ctx.quadraticCurveTo(mx - nx * wm * 1.15, my - ny * wm * 1.15, x0 - nx * w0, y0 - ny * w0);
  ctx.closePath();
}

// ---------- palette (sampled from the game's own sprites) ----------

const C = {
  ink: INK,
  leafLight: "#DDEE44",
  leaf: "#B8E02A",
  leafMid: "#8FC61A",
  leafDark: "#5E8F12",
  leafDeep: "#3F6410",
  stem: "#7FB017",
  stemDark: "#4F7A0E",
  metal: "#DDDDD2",
  metalMid: "#ABAB9C",
  metalDark: "#6E6E62",
  helmet: "#6E7C46",
  helmetDark: "#4A552E",
  electric: "#4FC8FF",
  electricDeep: "#1B6FE0",
  purple: "#B14BE8",
  fire1: "#FFD23A",
  fire2: "#FF7A18",
  fire3: "#D6231E",
  lawn1: "#63A81F",
  lawn2: "#55961A",
  path: "#D9B26A",
  pathDark: "#A98342",
  flagRed: "#E23B2E",
  flagDark: "#A3251C",
  gold: "#FFCC33",
  cardRed1: "#D8342A",
  cardRed2: "#7E140F",
  white: "#FFFFFF",
};

module.exports = {
  S, SS, newCanvas, save, paint, lin, rad, ellipsePath, blobPath, starPath,
  roundRectPath, polyPath, limb, C, INK, loadImage, createCanvas,
};
