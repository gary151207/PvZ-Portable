// Concept C: the mod's signature red travel card. A chunky seed-packet tile in
// the game's card idiom, carrying the ultimate electric gatling pea, a red
// RARE ribbon, and a winding path motif for the 11-round journey.
const L = require("./lib");
const { paint, lin, rad, ellipsePath, roundRectPath, starPath, starPath: sp, limb, C } = L;
const { drawHead } = require("./head");

const S = 1024;

// a picture of the travel road, clipped into the card window
function miniRoad(ctx, x, y, w, h) {
  ctx.save();
  roundRectPath(ctx, x, y, w, h, 26);
  ctx.clip();

  const lg = ctx.createLinearGradient(0, y, 0, y + h);
  lg.addColorStop(0, "#8ED04A");
  lg.addColorStop(0.42, "#4E9A28");
  lg.addColorStop(1, "#255E14");
  ctx.fillStyle = lg;
  ctx.fillRect(x, y, w, h);

  // mow stripes
  ctx.save();
  for (let i = -8; i <= 8; i++) {
    ctx.beginPath();
    const x0 = x + w * 0.5 + i * w * 0.09;
    ctx.moveTo(x + w * 0.5 + i * w * 0.05, y);
    ctx.lineTo(x + w * 0.5 + i * w * 0.22, y + h);
    ctx.lineTo(x + w * 0.5 + (i + 1) * w * 0.22, y + h);
    ctx.lineTo(x + w * 0.5 + (i + 1) * w * 0.05, y);
    ctx.closePath();
    ctx.fillStyle = i % 2 ? "rgba(255,255,255,0.06)" : "rgba(0,40,0,0.05)";
    ctx.fill();
  }
  ctx.restore();

  // winding road with stepping stones
  const N = 30;
  const pts = [];
  for (let i = 0; i <= N; i++) {
    const t = i / N;
    pts.push([
      x + w * (0.08 + t * 0.84),
      y + h * (0.88 - t * 0.62) + Math.sin(t * Math.PI * 1.2) * h * 0.14,
    ]);
  }
  const wide = (t) => (0.035 + 0.035 * (1 - t)) * w;
  for (const [color, scale] of [["#7E5F2E", 1.18], [C.path, 1.0]]) {
    ctx.beginPath();
    pts.forEach(([px, py], i) => {
      const t = i / N;
      const a = Math.atan2(
        pts[Math.min(i + 1, N)][1] - pts[Math.max(i - 1, 0)][1],
        pts[Math.min(i + 1, N)][0] - pts[Math.max(i - 1, 0)][0]
      );
      const nx = Math.cos(a + Math.PI / 2), ny = Math.sin(a + Math.PI / 2);
      const ww = wide(t) * scale;
      i ? ctx.lineTo(px + nx * ww, py + ny * ww) : ctx.moveTo(px + nx * ww, py + ny * ww);
    });
    for (let i = N; i >= 0; i--) {
      const t = i / N;
      const [px, py] = pts[i];
      const a = Math.atan2(
        pts[Math.min(i + 1, N)][1] - pts[Math.max(i - 1, 0)][1],
        pts[Math.min(i + 1, N)][0] - pts[Math.max(i - 1, 0)][0]
      );
      const nx = Math.cos(a + Math.PI / 2), ny = Math.sin(a + Math.PI / 2);
      const ww = wide(t) * scale;
      ctx.lineTo(px - nx * ww, py - ny * ww);
    }
    ctx.closePath();
    ctx.fillStyle = color;
    ctx.globalAlpha = color === C.path ? 0.96 : 0.45;
    ctx.fill();
    ctx.globalAlpha = 1;
  }

  // 11 stones, last one is the goal marker
  for (let i = 0; i < 11; i++) {
    const t = (i + 0.5) / 11;
    const px = x + w * (0.08 + t * 0.84);
    const py = y + h * (0.88 - t * 0.62) + Math.sin(t * Math.PI * 1.2) * h * 0.14;
    const r = w * (0.014 + 0.008 * (1 - t));
    paint(ctx, (c) => ellipsePath(c, px, py, r * 1.2, r * 0.7, 0), {
      fill: i === 10 ? "#FFF0C0" : "#EBD9AC", lw: 4, line: "rgba(60,40,10,0.5)",
    });
  }
  ctx.restore();
}

function compose() {
  const { c, ctx } = L.newCanvas(S, S);

  // ---- backdrop: dark wood-ish plate so the red card pops ---------------
  const bg = ctx.createRadialGradient(512, 420, 80, 512, 520, 860);
  bg.addColorStop(0, "#5A2114");
  bg.addColorStop(0.5, "#2E0F09");
  bg.addColorStop(1, "#100503");
  ctx.fillStyle = bg;
  ctx.fillRect(0, 0, S, S);

  // faint sunburst
  ctx.save();
  ctx.globalAlpha = 0.055;
  for (let i = 0; i < 26; i++) {
    const a0 = (i / 26) * Math.PI * 2;
    const a1 = a0 + Math.PI / 26;
    ctx.beginPath();
    ctx.moveTo(512, 470);
    ctx.lineTo(512 + Math.cos(a0) * 980, 470 + Math.sin(a0) * 980);
    ctx.lineTo(512 + Math.cos(a1) * 980, 470 + Math.sin(a1) * 980);
    ctx.closePath();
    ctx.fillStyle = i % 2 ? "#FFE9B0" : "#2A0804";
    ctx.fill();
  }
  ctx.restore();

  // ---- card plate -------------------------------------------------------
  const cw = 700, ch = 828;
  const cx0 = (S - cw) / 2, cy0 = (S - ch) / 2 + 8;

  // outer glow
  ctx.save();
  ctx.shadowColor = "rgba(255,180,90,0.5)";
  ctx.shadowBlur = 60;
  paint(ctx, (c) => roundRectPath(c, cx0, cy0, cw, ch, 46), {
    fill: "#B3271E",
    grad: lin(0, cy0, 0, cy0 + ch, [
      [0, "#F0655A"], [0.28, "#D8342A"], [0.7, "#A8231B"], [1, "#6E140F"],
    ]),
    lw: 20,
    line: "#2A0906",
  });
  ctx.restore();

  // gold frame
  paint(ctx, (c) => roundRectPath(c, cx0 + 17, cy0 + 17, cw - 34, ch - 34, 34), {
    fill: "rgba(255,255,255,0.06)",
    grad: lin(cx0, cy0, cx0 + cw, cy0 + ch, [
      [0, "#FFD98A"], [0.35, "#E2A32E"], [0.62, "#B2700F"], [1, "#FFD98A"],
    ]),
    lw: 12,
    line: "#5A2A06",
  });

  // inner panel: the plant art fills it, cropped by the card frame
  const px0 = cx0 + 34, py0 = cy0 + 34, pw = cw - 68, ph = ch - 68;
  paint(ctx, (c) => roundRectPath(c, px0, py0, pw, ph, 26), {
    fill: "#5EA02C",
    grad: lin(0, py0, 0, py0 + ph, [[0, "#7CBE3C"], [0.55, "#4E8E24"], [1, "#2C5E14"]]),
    lw: 12,
    line: "#8C4A12",
  });

  // picture window inside the panel: the travel road
  const wx = px0 + 22, wy = py0 + 22, ww = pw - 44, wh = ph * 0.42;
  miniRoad(ctx, wx, wy, ww, wh);
  ctx.save();
  roundRectPath(ctx, wx, wy, ww, wh, 24);
  ctx.strokeStyle = "#8C4A12";
  ctx.lineWidth = 10;
  ctx.stroke();
  ctx.restore();

  // ---- hero art: fits the panel and overlaps the picture window --------
  ctx.save();
  ctx.translate(430, py0 + ph * 0.50);
  ctx.rotate(-0.05);
  drawHead(ctx, 560, { electric: true });
  ctx.restore();

  // ---- name plate -------------------------------------------------------
  const nx = px0 + 30, nw = pw - 60, nh = 132;
  const ny = py0 + ph - nh - 30;
  paint(ctx, (c) => roundRectPath(c, nx, ny, nw, nh, 22), {
    fill: "#2E0B08",
    grad: lin(0, ny, 0, ny + nh, [[0, "#5A1710"], [1, "#2A0906"]]),
    lw: 10,
    line: "#8C4A12",
  });
  // title bars: three chunky plates reading as the plant's name
  const barW = [0.66, 0.48, 0.32];
  barW.forEach((frac, i) => {
    const bw = nw * frac;
    const bx = nx + (nw - bw) / 2;
    const by = ny + 22 + i * 34;
    paint(ctx, (c) => roundRectPath(c, bx, by, bw, 20, 10), {
      fill: "#FFD98A",
      grad: lin(0, by, 0, by + 20, [[0, "#FFF3C8"], [0.5, "#FFD98A"], [1, "#E0A33A"]]),
      lw: 6,
      line: "#5A2A06",
    });
  });

  // ---- sun cost chip ----------------------------------------------------
  const sx = cx0 + 6, sy = cy0 + 6, sr = 108;
  paint(ctx, (c) => ellipsePath(c, sx, sy, sr, sr, 0), {
    fill: "#3A2A08",
    grad: rad(sx - 30, sy - 34, 10, sr * 1.25, [[0, "#FFF6C0"], [0.45, C.gold], [1, "#B87407"]]),
    lw: 16,
  });
  // sun rays inside the chip
  ctx.save();
  ctx.beginPath();
  ctx.arc(sx, sy, sr * 0.8, 0, Math.PI * 2);
  ctx.clip();
  ctx.globalAlpha = 0.35;
  for (let i = 0; i < 12; i++) {
    const a0 = (i / 12) * Math.PI * 2;
    const a1 = a0 + Math.PI / 12;
    ctx.beginPath();
    ctx.moveTo(sx, sy);
    ctx.lineTo(sx + Math.cos(a0) * sr, sy + Math.sin(a0) * sr);
    ctx.lineTo(sx + Math.cos(a1) * sr, sy + Math.sin(a1) * sr);
    ctx.closePath();
    ctx.fillStyle = i % 2 ? "#FFFFFF" : "#B87407";
    ctx.fill();
  }
  ctx.restore();
  // cost numerals via hand-drawn bars: 1 7 5
  ctx.save();
  ctx.translate(sx, sy + 4);
  ctx.fillStyle = "#3A1A02";
  const bar = (bx, by, bw, bh) => {
    ctx.beginPath();
    ctx.roundRect(bx, by, bw, bh, 5);
    ctx.fill();
  };
  // "1"
  bar(-52, -26, 12, 52);
  bar(-60, -26, 26, 11);
  // "7"
  bar(-14, -26, 40, 12);
  bar(0, -14, 12, 40);
  // "5"
  bar(38, -26, 38, 11);
  bar(38, -18, 11, 20);
  bar(38, -2, 36, 11);
  bar(68, 0, 11, 16);
  bar(38, 16, 38, 11);
  ctx.restore();

  // ---- RARE ribbon ------------------------------------------------------
  ctx.save();
  ctx.translate(cx0 + cw - 18, cy0 + ch * 0.3);
  ctx.rotate(Math.PI / 2);
  paint(ctx, (c) => roundRectPath(c, -128, -40, 256, 80, 18), {
    fill: "#FFD24A",
    grad: lin(0, -40, 0, 40, [[0, "#FFF3C8"], [0.45, "#FFCB3A"], [1, "#D98A0F"]]),
    lw: 10,
    line: "#5A2A06",
  });
  ctx.fillStyle = "#5A2A06";
  for (let i = 0; i < 4; i++) {
    const bx = -84 + i * 40;
    ctx.beginPath();
    ctx.roundRect(bx, -14, 26, 28, 4);
    ctx.fill();
  }
  // star to read as "rare"
  starPath(ctx, 96, 0, 5, 26, 11);
  ctx.fill();
  ctx.restore();

  return c;
}

module.exports = { compose };

if (require.main === module) {
  (async () => { await L.save(compose(), "out/C-redcard.png"); })();
}
