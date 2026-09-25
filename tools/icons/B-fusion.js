// Concept B: one gatling pea, one barrel per fusion element.
// The mod's whole gatling family - ice, electric and fire - in a single clean
// silhouette: three barrels, three elements. Reads down to 32px.
const L = require("./lib");
const { paint, lin, rad, ellipsePath, roundRectPath, starPath, C } = L;

const S = 1024;

function rays(ctx, cx, cy, r, n, colors) {
  ctx.save();
  for (let i = 0; i < n; i++) {
    const a0 = (i / n) * Math.PI * 2;
    const a1 = a0 + Math.PI / n;
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(cx + Math.cos(a0) * r, cy + Math.sin(a0) * r);
    ctx.lineTo(cx + Math.cos(a1) * r, cy + Math.sin(a1) * r);
    ctx.closePath();
    ctx.fillStyle = i % 2 ? colors[0] : colors[1];
    ctx.fill();
  }
  ctx.restore();
}

// one barrel pointing right, with a themed element badge on its tip
function barrel(ctx, s, y, len, r, pal, element) {
  const x = 0.40 * s;
  paint(ctx, (c) => {
    c.beginPath();
    c.moveTo(x, y - r);
    c.lineTo(x + len, y - r * 0.9);
    c.ellipse(x + len, y, r * 0.5, r * 0.9, 0, -Math.PI / 2, Math.PI / 2);
    c.lineTo(x, y + r);
    c.ellipse(x, y, r * 0.5, r, 0, Math.PI / 2, -Math.PI / 2, true);
    c.closePath();
  }, {
    fill: pal[1],
    grad: lin(0, y - r, 0, y + r, [
      [0, "#FFFFFF"], [0.14, pal[0]], [0.5, pal[1]], [1, pal[2]],
    ]),
    lw: 0.03 * s * 0.7,
  });

  // muzzle braking slots
  ctx.save();
  ctx.strokeStyle = "rgba(25,30,35,0.45)";
  ctx.lineWidth = 0.03 * s * 0.34;
  for (let k = 0; k < 2; k++) {
    const vx = x + len - 0.05 * s - k * 0.036 * s;
    ctx.beginPath();
    ctx.moveTo(vx, y - r * 0.7);
    ctx.lineTo(vx, y + r * 0.7);
    ctx.stroke();
  }
  ctx.restore();

  // bore
  paint(ctx, (c) => ellipsePath(c, x + len, y, r * 0.4, r * 0.72, 0), {
    fill: "#242A2F", lw: 0.03 * s * 0.45,
  });

  // ---- muzzle sparks / trail in the element's colour -------------------
  const bx = x + len + r * 0.5;
  const streaks = element === "fire" ? 2 : 3;
  ctx.save();
  ctx.lineCap = "round";
  for (let i = 0; i < streaks; i++) {
    const a = (i - (streaks - 1) / 2) * 0.42;
    const r0 = r * 0.75, r1 = r * (1.9 + (i % 2) * 0.55);
    const x0 = bx + Math.cos(a) * r0, y0 = y + Math.sin(a) * r0;
    const x1 = bx + Math.cos(a) * r1, y1 = y + Math.sin(a) * r1;
    const xm = bx + Math.cos(a + 0.22) * ((r0 + r1) / 2), ym = y + Math.sin(a + 0.22) * ((r0 + r1) / 2);
    ctx.strokeStyle = "rgba(18,24,28,0.5)";
    ctx.lineWidth = 0.03 * s * 0.62;
    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.quadraticCurveTo(xm, ym, x1, y1);
    ctx.stroke();
    ctx.strokeStyle = element === "fire" ? "#FFE9A8" : element === "ice" ? "#EAFBFF" : "#D6F2FF";
    ctx.lineWidth = 0.03 * s * 0.34;
    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.quadraticCurveTo(xm, ym, x1, y1);
    ctx.stroke();
  }
  paint(ctx, (c) => ellipsePath(c, bx + r * 2.0, y, r * 0.32, r * 0.32, 0), {
    fill: element === "fire" ? "#FFD24A" : element === "ice" ? "#EAFBFF" : "#D6F2FF",
    lw: 0.03 * s * 0.28,
  });
  ctx.restore();
}

function compose() {
  const { c, ctx } = L.newCanvas(S, S);

  const bg = ctx.createRadialGradient(500, 380, 60, 512, 520, 880);
  bg.addColorStop(0, "#4A9626");
  bg.addColorStop(0.48, "#24601C");
  bg.addColorStop(1, "#0A240C");
  ctx.fillStyle = bg;
  ctx.fillRect(0, 0, S, S);

  ctx.save();
  ctx.globalAlpha = 0.09;
  rays(ctx, 512, 420, 940, 24, ["#EAF7C0", "#08200A"]);
  ctx.restore();

  // aura behind the barrel cluster: cool above, warm below
  const g = ctx.createRadialGradient(700, 470, 20, 700, 470, 460);
  g.addColorStop(0, "rgba(200,245,255,0.5)");
  g.addColorStop(0.45, "rgba(255,220,150,0.28)");
  g.addColorStop(1, "rgba(255,220,150,0)");
  ctx.save();
  ctx.fillStyle = g;
  ctx.beginPath();
  ctx.arc(700, 470, 460, 0, Math.PI * 2);
  ctx.fill();
  ctx.restore();

  // ---- the plant --------------------------------------------------------
  const s = 580;
  ctx.save();
  ctx.translate(330, 546);
  ctx.rotate(-0.03);

  // mouth ring the cluster plugs into
  paint(ctx, (c) => ellipsePath(c, 0.245 * s, 0.06 * s, 0.16 * s, 0.155 * s, 0), {
    fill: "#3A1810", lw: 0.03 * s * 0.8,
  });
  paint(ctx, (c) => {
    c.beginPath();
    c.ellipse(0.245 * s, 0.072 * s, 0.09 * s, 0.078 * s, 0, Math.PI, Math.PI * 2);
    c.closePath();
  }, { fill: "#7A3324", lw: 0 });

  // collar
  const colX0 = 0.30 * s, colX1 = 0.45 * s;
  const colY0 = -0.20 * s, colY1 = 0.32 * s;
  paint(ctx, (c) => {
    c.beginPath();
    c.moveTo(colX0, colY0 + 0.018 * s);
    c.lineTo(colX1, colY0);
    c.lineTo(colX1, colY1);
    c.lineTo(colX0, colY1 - 0.018 * s);
    c.closePath();
  }, {
    fill: "#C9C9BC",
    grad: lin(colX0, 0, colX1, 0, [
      [0, "#F7F7EE"], [0.34, "#DCDCD0"], [0.78, "#ABAB9C"], [1, "#6E6E62"],
    ]),
    lw: 0.03 * s * 0.85,
  });

  // three barrels, one per element
  const br = 0.082 * s;
  barrel(ctx, s, -0.12 * s, 0.32 * s, br * 0.94, ["#D9F6FF", "#45C2DE", "#146880"], "ice");
  barrel(ctx, s, 0.06 * s, 0.40 * s, br, ["#D6F2FF", "#3FA0F0", "#0F3F96"], "electric");
  barrel(ctx, s, 0.262 * s, 0.34 * s, br * 0.96, ["#FFE7A8", "#F08A1E", "#9E3A0C"], "fire");

  // ---- the pea head ----------------------------------------------------
  const v = {
    head: C.leafLight, headMid: C.leaf, headDark: C.leafMid,
    helmet: C.helmet, helmetDark: C.helmetDark,
  };
  for (const lf of [
    { a: 174, len: 0.62, w: 0.23 },
    { a: 210, len: 0.54, w: 0.20 },
    { a: 144, len: 0.38, w: 0.16 },
  ]) {
    const a = (lf.a * Math.PI) / 180;
    const cx = -0.3 * s, cy = 0.12 * s;
    const x0 = cx + Math.cos(a) * 0.12 * s, y0 = cy + Math.sin(a) * 0.12 * s;
    const x1 = cx + Math.cos(a) * (0.12 + lf.len) * s, y1 = cy + Math.sin(a) * (0.12 + lf.len) * s;
    paint(ctx, (cc) => {
      const ang = Math.atan2(y1 - y0, x1 - x0);
      const nx = Math.cos(ang + Math.PI / 2), ny = Math.sin(ang + Math.PI / 2);
      const mxp = (x0 + x1) / 2, myp = (y0 + y1) / 2;
      const w0 = lf.w * s, w1 = lf.w * 0.2 * s;
      const wm = ((w0 + w1) / 2) * 1.15;
      cc.beginPath();
      cc.moveTo(x0 + nx * w0, y0 + ny * w0);
      cc.quadraticCurveTo(mxp + nx * wm, myp + ny * wm, x1 + nx * w1, y1 + ny * w1);
      cc.lineTo(x1 - nx * w1, y1 - ny * w1);
      cc.quadraticCurveTo(mxp - nx * wm, myp - ny * wm, x0 - nx * w0, y0 - ny * w0);
      cc.closePath();
    }, {
      fill: v.headDark,
      grad: lin(x0, y0, x1, y1, [[0, v.headMid], [1, v.headDark]]),
      lw: 0.03 * s * 0.95,
    });
  }

  paint(ctx, (c) => {
    c.beginPath();
    c.moveTo(-0.5 * s, 0.0 * s);
    c.bezierCurveTo(-0.51 * s, -0.36 * s, -0.32 * s, -0.52 * s, 0.0 * s, -0.53 * s);
    c.bezierCurveTo(0.34 * s, -0.54 * s, 0.52 * s, -0.34 * s, 0.52 * s, 0.0 * s);
    c.bezierCurveTo(0.52 * s, 0.33 * s, 0.32 * s, 0.52 * s, 0.0 * s, 0.52 * s);
    c.bezierCurveTo(-0.32 * s, 0.52 * s, -0.5 * s, 0.34 * s, -0.5 * s, 0.0 * s);
    c.closePath();
  }, {
    fill: v.head,
    grad: rad(-0.22 * s, -0.26 * s, 0.05 * s, 0.78 * s, [
      [0, "#F6FF9A"], [0.45, v.head], [1, v.headMid],
    ]),
    lw: 0.03 * s,
  });

  const eyeX = 0.165 * s, eyeY = -0.10 * s, eyeRx = 0.115 * s, eyeRy = 0.147 * s;
  for (const sx of [-1, 1]) {
    paint(ctx, (c) => ellipsePath(c, sx * eyeX, eyeY, eyeRx, eyeRy, sx * 0.07), {
      fill: C.white, lw: 0.03 * s * 0.8,
    });
    paint(ctx, (c) => ellipsePath(c, sx * eyeX + sx * 0.014 * s, eyeY + 0.03 * s, eyeRx * 0.6, eyeRy * 0.6, sx * 0.05), {
      fill: "#16210C", lw: 0.03 * s * 0.55,
    });
    ctx.save();
    ctx.fillStyle = "rgba(255,255,255,0.95)";
    ctx.beginPath();
    ctx.ellipse(sx * eyeX - sx * 0.038 * s, eyeY - 0.062 * s, eyeRx * 0.2, eyeRy * 0.22, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();
  }

  const hw = 0.45 * s, brimY = -0.33 * s, topY = -0.60 * s;
  paint(ctx, (c) => {
    c.beginPath();
    c.moveTo(-hw, brimY);
    c.bezierCurveTo(-hw * 0.99, topY + 0.02 * s, hw * 0.99, topY + 0.02 * s, hw, brimY);
    c.quadraticCurveTo(0, brimY + 0.1 * s, -hw, brimY);
    c.closePath();
  }, {
    fill: v.helmet,
    grad: lin(-hw, topY, hw * 0.6, brimY, [[0, "#96A562"], [0.42, v.helmet], [1, v.helmetDark]]),
    lw: 0.03 * s * 0.95,
  });
  paint(ctx, (c) => {
    c.beginPath();
    c.moveTo(-hw * 1.08, brimY - 0.005 * s);
    c.quadraticCurveTo(0, brimY + 0.13 * s, hw * 1.08, brimY - 0.005 * s);
    c.quadraticCurveTo(0, brimY + 0.185 * s, -hw * 1.08, brimY - 0.005 * s);
    c.closePath();
  }, { fill: v.helmetDark, lw: 0.03 * s * 0.6 });
  paint(ctx, (c) => ellipsePath(c, 0, topY + 0.09 * s, 0.038 * s, 0.034 * s, 0), {
    fill: "#C3CB84", lw: 0.03 * s * 0.45,
  });

  ctx.restore();

  // ---- inner rim --------------------------------------------------------
  ctx.save();
  roundRectPath(ctx, 26, 26, S - 52, S - 52, 96);
  ctx.strokeStyle = "rgba(10,26,8,0.62)";
  ctx.lineWidth = 16;
  ctx.stroke();
  roundRectPath(ctx, 40, 40, S - 80, S - 80, 88);
  ctx.strokeStyle = "rgba(240,255,190,0.4)";
  ctx.lineWidth = 7;
  ctx.stroke();
  ctx.restore();

  return c;
}

module.exports = { compose };

if (require.main === module) {
  (async () => { await L.save(compose(), "out/B-fusion.png"); })();
}
