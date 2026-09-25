// Concept A: "Ultimate Gatling + the 11-round journey"
// A bold badge: lawn + winding travel road with 11 stepping stones and a red
// flag, a sun coin, and the ultimate electric gatling pea as the hero.
const L = require("./lib");
const { paint, lin, rad, ellipsePath, starPath, roundRectPath, limb, C } = L;
const { drawHead } = require("./head");

const S = 1024;

function drawSunCoin(ctx, x, y, r) {
  // PvZ sun: a golden disc with a lighter inner ring and a soft halo
  const halo = ctx.createRadialGradient(x, y, r * 0.8, x, y, r * 1.9);
  halo.addColorStop(0, "rgba(255,238,140,0.55)");
  halo.addColorStop(1, "rgba(255,238,140,0)");
  ctx.save();
  ctx.fillStyle = halo;
  ctx.beginPath();
  ctx.arc(x, y, r * 1.9, 0, Math.PI * 2);
  ctx.fill();
  ctx.restore();

  paint(ctx, (c) => starPath(c, x, y, 12, r * 1.06, r * 0.92), {
    fill: C.gold,
    grad: rad(x - r * 0.3, y - r * 0.35, r * 0.1, r * 1.4, [
      [0, "#FFF3B0"], [0.5, C.gold], [1, "#E39A12"],
    ]),
    lw: r * 0.16,
  });
  paint(ctx, (c) => ellipsePath(c, x - r * 0.18, y - r * 0.2, r * 0.34, r * 0.26, -0.5), {
    fill: "rgba(255,255,255,0.7)", lw: 0,
  });
}

// stylised lawn with mowed stripes, receding toward the horizon
function drawLawn(ctx) {
  const horizon = 505;
  const g = ctx.createLinearGradient(0, horizon, 0, S);
  g.addColorStop(0, "#7FC22C");
  g.addColorStop(0.45, C.lawn1);
  g.addColorStop(1, "#3E7213");
  ctx.fillStyle = g;
  ctx.fillRect(0, horizon, S, S - horizon);

  // mow stripes: vertical bands converging on a vanishing point
  ctx.save();
  ctx.beginPath();
  ctx.rect(0, horizon, S, S - horizon);
  ctx.clip();
  const vpx = S * 0.5, vpy = -260;
  for (let i = -14; i <= 14; i++) {
    const t0 = i / 9;
    ctx.beginPath();
    ctx.moveTo(vpx + t0 * S * 0.24, horizon);
    ctx.lineTo(vpx + t0 * S * 1.5, S + 40);
    ctx.lineTo(vpx + (t0 + 0.055) * S * 1.5, S + 40);
    ctx.lineTo(vpx + (t0 + 0.055) * S * 0.24, horizon);
    ctx.closePath();
    ctx.fillStyle = i % 2 ? "rgba(255,255,255,0.075)" : "rgba(0,40,0,0.06)";
    ctx.fill();
  }
  ctx.restore();

  // horizon haze
  const hz = ctx.createLinearGradient(0, horizon - 40, 0, horizon + 90);
  hz.addColorStop(0, "rgba(214,240,255,0.85)");
  hz.addColorStop(1, "rgba(214,240,255,0)");
  ctx.save();
  ctx.fillStyle = hz;
  ctx.fillRect(0, horizon - 40, S, 130);
  ctx.restore();
}

// winding road with 11 stepping stones, ending at a red flag
function drawRoad(ctx) {
  // road ribbon as a tapered band following a sine-ish curve
  const pts = [];
  const N = 26;
  for (let i = 0; i <= N; i++) {
    const t = i / N;
    const x = 104 + t * 668;
    const y = 926 - t * 220 + Math.sin(t * Math.PI * 1.2) * 96;
    pts.push([x, y]);
  }
  const wide = (t) => 26 + 30 * (1 - t);

  // outer (darker) then inner (sand) pass
  for (const [color, scale] of [["#8A6A33", 1.14], [C.path, 1.0]]) {
    ctx.save();
    ctx.beginPath();
    pts.forEach(([x, y], i) => {
      const t = i / N;
      const w = wide(t) * scale;
      const a = Math.atan2(
        (pts[Math.min(i + 1, N)][1] - pts[Math.max(i - 1, 0)][1]),
        (pts[Math.min(i + 1, N)][0] - pts[Math.max(i - 1, 0)][0])
      );
      const nx = Math.cos(a + Math.PI / 2), ny = Math.sin(a + Math.PI / 2);
      const px = x + nx * w, py = y + ny * w;
      i ? ctx.lineTo(px, py) : ctx.moveTo(px, py);
    });
    for (let i = N; i >= 0; i--) {
      const t = i / N;
      const [x, y] = pts[i];
      const w = wide(t) * scale;
      const a = Math.atan2(
        (pts[Math.min(i + 1, N)][1] - pts[Math.max(i - 1, 0)][1]),
        (pts[Math.min(i + 1, N)][0] - pts[Math.max(i - 1, 0)][0])
      );
      const nx = Math.cos(a + Math.PI / 2), ny = Math.sin(a + Math.PI / 2);
      ctx.lineTo(x - nx * w, y - ny * w);
    }
    ctx.closePath();
    ctx.fillStyle = color;
    ctx.globalAlpha = color === C.path ? 0.95 : 0.5;
    ctx.fill();
    ctx.globalAlpha = 1;
    ctx.restore();
  }

  // 11 stepping stones: 10 small round markers, the 11th is the goal
  for (let i = 0; i < 11; i++) {
    const t = (i + 0.5) / 11;
    const x = 104 + t * 668;
    const y = 926 - t * 220 + Math.sin(t * Math.PI * 1.2) * 96;
    const r = 16 + 6 * (1 - t);
    paint(ctx, (c) => ellipsePath(c, x, y, r * 1.25, r * 0.72, 0), {
      fill: i === 10 ? "#F2E2B4" : "#E8D6A8",
      lw: 5,
      line: "rgba(60,40,10,0.55)",
    });
  }

  // goal flag on the last marker
  const tf = 10.45 / 11;
  const fx = 104 + tf * 668 + 52;
  const fy = 926 - tf * 220 + Math.sin(tf * Math.PI * 1.2) * 96 - 26;

  ctx.save();
  // pole
  ctx.strokeStyle = "#4A3A22";
  ctx.lineWidth = 13;
  ctx.lineCap = "round";
  ctx.beginPath();
  ctx.moveTo(fx, fy + 6);
  ctx.lineTo(fx, fy - 210);
  ctx.stroke();
  ctx.strokeStyle = "#B9A67C";
  ctx.lineWidth = 6;
  ctx.beginPath();
  ctx.moveTo(fx - 2, fy + 2);
  ctx.lineTo(fx - 2, fy - 204);
  ctx.stroke();
  ctx.restore();

  // flag cloth (waving pennant)
  paint(ctx, (c) => {
    c.beginPath();
    c.moveTo(fx + 4, fy - 173);
    c.bezierCurveTo(fx + 74, fy - 196, fx + 120, fy - 148, fx + 160, fy - 165);
    c.bezierCurveTo(fx + 116, fy - 124, fx + 88, fy - 106, fx + 4, fy - 110);
    c.closePath();
  }, {
    fill: C.flagRed,
    grad: lin(fx, fy - 196, fx, fy - 106, [[0, "#F0564A"], [0.55, C.flagRed], [1, C.flagDark]]),
    lw: 11,
  });
  // flag highlight
  ctx.save();
  ctx.beginPath();
  ctx.moveTo(fx + 20, fy - 196);
  ctx.bezierCurveTo(fx + 80, fy - 214, fx + 118, fy - 176, fx + 156, fy - 186);
  ctx.bezierCurveTo(fx + 116, fy - 166, fx + 74, fy - 176, fx + 20, fy - 172);
  ctx.closePath();
  ctx.fillStyle = "rgba(255,255,255,0.24)";
  ctx.fill();
  ctx.restore();
}

function drawSky(ctx) {
  // Hero backdrop: deep night blue so the yellow-green hero and the electric
  // blue glow both pop, with a hint of a PvZ neighbourhood at the horizon.
  const g = ctx.createLinearGradient(0, 0, 0, 560);
  g.addColorStop(0, "#0E2140");
  g.addColorStop(0.5, "#1B4C86");
  g.addColorStop(1, "#3E86BE");
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, S, 560);

  // stars
  ctx.save();
  for (let i = 0; i < 46; i++) {
    const x = ((i * 7919) % 1024);
    const y = ((i * 4523) % 400) + 20;
    const r = 1.6 + ((i * 37) % 10) / 6;
    ctx.fillStyle = "rgba(255,255,255," + (0.18 + ((i * 13) % 10) / 22) + ")";
    ctx.beginPath();
    ctx.arc(x, y, r, 0, Math.PI * 2);
    ctx.fill();
  }
  ctx.restore();

  // distant hedge silhouette
  ctx.save();
  ctx.fillStyle = "#1E3A16";
  ctx.beginPath();
  ctx.moveTo(0, 500);
  for (let i = 0; i <= 32; i++) {
    const x = (i / 32) * S;
    const y = 505 + Math.sin(i * 0.9) * 13 + Math.cos(i * 2.1) * 7;
    ctx.lineTo(x, y);
  }
  ctx.lineTo(S, 560);
  ctx.lineTo(0, 560);
  ctx.closePath();
  ctx.fill();
  ctx.restore();
}

function drawRoundBadge(ctx) {
  // electric aura behind the hero
  const g = ctx.createRadialGradient(430, 560, 40, 430, 560, 460);
  g.addColorStop(0, "rgba(150,240,255,0.85)");
  g.addColorStop(0.35, "rgba(90,190,255,0.5)");
  g.addColorStop(0.68, "rgba(40,120,230,0.24)");
  g.addColorStop(1, "rgba(40,120,230,0)");
  ctx.save();
  ctx.fillStyle = g;
  ctx.beginPath();
  ctx.arc(430, 560, 460, 0, Math.PI * 2);
  ctx.fill();
  ctx.restore();

  // concentric electric rings for a "charged" look
  ctx.save();
  for (let i = 0; i < 3; i++) {
    ctx.beginPath();
    ctx.arc(430, 560, 300 + i * 52, 0, Math.PI * 2);
    ctx.strokeStyle = "rgba(170,245,255," + (0.3 - i * 0.08) + ")";
    ctx.lineWidth = 7 - i * 1.6;
    ctx.stroke();
  }
  ctx.restore();
}

function compose() {
  const { c, ctx } = L.newCanvas(S, S);

  // ---- background plate -------------------------------------------------
  ctx.save();
  roundRectPath(ctx, 0, 0, S, S, 0);
  ctx.clip();
  drawSky(ctx);
  drawLawn(ctx);
  drawRoad(ctx);
  drawRoundBadge(ctx);
  ctx.restore();

  // ---- hero -------------------------------------------------------------
  ctx.save();
  ctx.translate(422, 548);
  ctx.rotate(-0.05);
  drawHead(ctx, 545, { electric: true });

  // electric contact arcs on the ground under the hero
  ctx.save();
  ctx.strokeStyle = "rgba(200,250,255,0.75)";
  ctx.lineWidth = 7;
  ctx.lineCap = "round";
  for (let i = 0; i < 5; i++) {
    const x = 180 + i * 118;
    ctx.beginPath();
    ctx.moveTo(x, 905 + (i % 2) * 60);
    ctx.lineTo(x + 34, 866 + (i % 2) * 60);
    ctx.lineTo(x + 10, 838 + (i % 2) * 60);
    ctx.stroke();
  }
  ctx.restore();
  ctx.restore();

  // ---- sun coin --------------------------------------------------------
  drawSunCoin(ctx, 812, 198, 74);

  // ---- inner rim so the badge reads as an icon -------------------------
  ctx.save();
  roundRectPath(ctx, 26, 26, S - 52, S - 52, 96);
  ctx.strokeStyle = "rgba(12,20,8,0.55)";
  ctx.lineWidth = 16;
  ctx.stroke();
  roundRectPath(ctx, 40, 40, S - 80, S - 80, 88);
  ctx.strokeStyle = "rgba(255,255,255,0.35)";
  ctx.lineWidth = 7;
  ctx.stroke();
  ctx.restore();

  return c;
}

module.exports = { compose };

if (require.main === module) {
  (async () => {
    await L.save(compose(), "out/A-badge.png");
  })();
}
