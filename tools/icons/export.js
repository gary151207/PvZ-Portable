// Export the three concepts as final deliverables:
//   1. *-1024.png         full square badge (already produced by each concept)
//   2. *-appicon-1024.png rounded-squircle transparent-background variant
const fs = require("fs");
const path = require("path");
const L = require("./lib");
const { roundRectPath } = L;
const { createCanvas, loadImage } = require("@napi-rs/canvas");

const S = 1024;
const OUT = path.resolve(__dirname, "..", "..", "docs", "icons");

function squirclePath(ctx, cx, cy, r) {
  // superellipse-ish squircle, matching the iOS icon silhouette
  const n = 5;
  ctx.beginPath();
  for (let i = 0; i <= 720; i++) {
    const t = (i / 720) * Math.PI * 2;
    const ct = Math.cos(t), st = Math.sin(t);
    const x = cx + Math.sign(ct) * Math.pow(Math.abs(ct), 2 / n) * r;
    const y = cy + Math.sign(st) * Math.pow(Math.abs(st), 2 / n) * r;
    i ? ctx.lineTo(x, y) : ctx.moveTo(x, y);
  }
  ctx.closePath();
}

async function appIcon(canvas, outPath) {
  const src = await loadImage(canvas.toBuffer("image/png"));
  const c = createCanvas(S, S);
  const ctx = c.getContext("2d");

  // Apple's icon grid: superellipse approximated by a rounded rect whose
  // corner radius is ~22.4% of the side, leaving a small transparent margin.
  const pad = 22;
  const size = S - pad * 2;
  const radius = size * 0.2237;

  ctx.save();
  roundRectPath(ctx, pad, pad, size, size, radius);
  ctx.clip();

  // inset the art a hair so the rounded mask never bites into the subject
  const scale = 1.03;
  ctx.translate(S / 2, S / 2);
  ctx.scale(scale, scale);
  ctx.translate(-S / 2, -S / 2);
  ctx.drawImage(src, 0, 0, S, S);
  ctx.restore();

  // inner rim following the mask
  ctx.save();
  roundRectPath(ctx, pad + 4, pad + 4, size - 8, size - 8, radius - 4);
  ctx.strokeStyle = "rgba(12,20,8,0.45)";
  ctx.lineWidth = 12;
  ctx.stroke();
  ctx.restore();

  fs.mkdirSync(path.dirname(outPath), { recursive: true });
  fs.writeFileSync(outPath, c.toBuffer("image/png"));
  console.log("wrote " + outPath);
}

module.exports = { appIcon, squirclePath };
