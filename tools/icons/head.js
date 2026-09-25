// PvZ-style shooter head renderer. Local unit space: the head is ~1.0 wide,
// centred on (0,0). The gun cluster extends to +x, the foliage to -x.
const L = require("./lib");
const { paint, lin, rad, ellipsePath, limb, C } = L;

const VARIANTS = {
  gatling: {
    head: C.leafLight, headMid: C.leaf, headDark: C.leafMid,
    helmet: C.helmet, helmetDark: C.helmetDark,
    barrel: C.metal, barrelMid: C.metalMid, barrelDark: C.metalDark,
    gun: true, helmetOn: true, eyebrow: false, fire: false, electric: false,
  },
  fire: {
    head: C.leafLight, headMid: C.leaf, headDark: C.leafMid,
    helmet: C.helmet, helmetDark: C.helmetDark,
    barrel: "#FFC65A", barrelMid: "#E2711C", barrelDark: "#8E2E12",
    gun: true, helmetOn: false, eyebrow: false, fire: true, electric: false,
  },
  electric: {
    head: C.leafLight, headMid: C.leaf, headDark: C.leafMid,
    helmet: "#5E7FA8", helmetDark: "#3B5478",
    barrel: "#7BD8FF", barrelMid: "#1E86DC", barrelDark: "#0B3E86",
    gun: true, helmetOn: true, eyebrow: false, fire: false, electric: true,
  },
  three: {
    head: "#C6E82F", headMid: "#9CCF1E", headDark: "#6E9C13",
    helmet: "#8A8A72", helmetDark: "#5C5C48",
    barrel: C.metal, barrelMid: C.metalMid, barrelDark: C.metalDark,
    gun: false, helmetOn: false, eyebrow: false, fire: false, electric: false, muzzle: false,
  },
  plain: {
    head: C.leafLight, headMid: C.leaf, headDark: C.leafMid,
    helmet: C.helmet, helmetDark: C.helmetDark,
    barrel: C.metal, barrelMid: C.metalMid, barrelDark: C.metalDark,
    gun: true, helmetOn: false, eyebrow: true, fire: false, electric: false,
  },
};

// ---------------------------------------------------------------------------
// Foliage: the chunky leaves sprouting from the back of the head.
// ---------------------------------------------------------------------------
function leavesBack(ctx, s, v, outline) {
  const leaves = [
    { a: 200, len: 0.46, w: 0.20 }, // lower-left, longest + broadest
    { a: 196, len: 0.30, w: 0.13 }, // tucked under it
    { a: 168, len: 0.32, w: 0.17 }, // left
    { a: 140, len: 0.24, w: 0.14 }, // upper-left
  ];
  for (const lf of leaves) {
    const a = (lf.a * Math.PI) / 180;
    const cx = -0.3 * s, cy = 0.1 * s;
    const x0 = cx + Math.cos(a) * 0.12 * s;
    const y0 = cy + Math.sin(a) * 0.12 * s;
    const x1 = cx + Math.cos(a) * (0.12 + lf.len) * s;
    const y1 = cy + Math.sin(a) * (0.12 + lf.len) * s;
    paint(ctx, (c) => limb(c, x0, y0, x1, y1, lf.w * s, lf.w * 0.22 * s), {
      fill: v.headDark,
      grad: lin(x0, y0, x1, y1, [[0, v.headMid], [1, v.headDark]]),
      lw: outline * 0.95,
    });
  }
}

// ---------------------------------------------------------------------------
// Gatling turret out of the mouth: housing collar + 2x2 barrel cluster.
// ---------------------------------------------------------------------------
function drawGun(ctx, s, v, outline) {
  const bx = 0.365 * s, by = 0.185 * s; // mouth anchor
  const br = 0.076 * s; // barrel radius
  const gap = 0.018 * s; // spacing between barrels

  // muzzle housing: a chunky collar straddling the mouth, taller than it is deep
  const collarX0 = 0.235 * s, collarX1 = 0.425 * s;
  const collarY0 = by - 0.175 * s, collarY1 = by + 0.19 * s;
  paint(ctx, (c) => {
    c.beginPath();
    c.moveTo(collarX0, collarY0 + 0.018 * s);
    c.lineTo(collarX1, collarY0);
    c.lineTo(collarX1, collarY1);
    c.lineTo(collarX0, collarY1 - 0.018 * s);
    c.closePath();
  }, {
    fill: v.barrelMid,
    grad: lin(collarX0, 0, collarX1, 0, [
      [0, "#F7F7EE"], [0.32, v.barrel], [0.78, v.barrelMid], [1, v.barrelDark],
    ]),
    lw: outline * 0.85,
  });

  // collar underside shade so it reads as a solid block
  ctx.save();
  ctx.beginPath();
  ctx.moveTo(collarX0, collarY0 + 0.018 * s);
  ctx.lineTo(collarX1, collarY0);
  ctx.lineTo(collarX1, collarY1);
  ctx.lineTo(collarX0, collarY1 - 0.018 * s);
  ctx.closePath();
  ctx.clip();
  ctx.fillStyle = "rgba(20,26,30,0.22)";
  ctx.fillRect(collarX0, by + 0.075 * s, collarX1 - collarX0, 0.2 * s);
  ctx.restore();

  // barrel cluster: 3 tight rows, tapering slightly outward, back row first
  const rows = [
    { y: by - 0.112 * s, x: bx + 0.03 * s, len: 0.245 * s, r: br * 0.85 },
    { y: by + 0.012 * s, x: bx + 0.045 * s, len: 0.30 * s, r: br },
    { y: by + 0.128 * s, x: bx + 0.038 * s, len: 0.265 * s, r: br * 0.88 },
  ];
  for (const row of rows) {
    const { y, r, x, len } = row;
    const rf = r * 0.9; // muzzle end slightly narrower
    paint(ctx, (c) => {
      c.beginPath();
      c.moveTo(x, y - r);
      c.lineTo(x + len, y - rf * 0.88);
      c.ellipse(x + len, y, r * 0.5, rf, 0, -Math.PI / 2, Math.PI / 2);
      c.lineTo(x, y + r);
      c.ellipse(x, y, r * 0.5, r, 0, Math.PI / 2, -Math.PI / 2, true);
      c.closePath();
    }, {
      fill: v.barrel,
      grad: v.electric
        ? lin(0, y - r, 0, y + r, [
            [0, "#EAF9FF"], [0.1, "#9FE0FA"], [0.32, "#3E9EE8"], [0.68, "#1A66B8"], [1, "#0C356F"],
          ])
        : lin(0, y - r, 0, y + r, [
            [0, "#FFFFFF"], [0.1, v.barrel], [0.3, v.barrel], [0.62, v.barrelMid], [1, v.barrelDark],
          ]),
      lw: outline * 0.68,
    });
    // cooling vent lines near the tip
    ctx.save();
    ctx.strokeStyle = "rgba(30,36,40,0.42)";
    ctx.lineWidth = outline * 0.36;
    for (let k = 0; k < 2; k++) {
      const vx = x + len - 0.045 * s - k * 0.032 * s;
      ctx.beginPath();
      ctx.moveTo(vx, y - r * 0.66);
      ctx.lineTo(vx, y + r * 0.66);
      ctx.stroke();
    }
    ctx.restore();
    // bore
    paint(ctx, (c) => ellipsePath(c, x + len, y, r * 0.38, rf * 0.7, 0), {
      fill: "#262C31", lw: outline * 0.42,
    });
  }

  if (v.electric) {
    const cx = rows[1].x + rows[1].len + 0.015 * s;
    for (let i = 0; i < 7; i++) {
      const a = -1.15 + i * 0.36;
      const r0 = br * 1.6, r1 = br * (2.7 + (i % 2) * 0.9);
      ctx.save();
      ctx.strokeStyle = "rgba(20,30,40,0.5)";
      ctx.lineWidth = outline * 0.72;
      ctx.lineCap = "round";
      ctx.beginPath();
      const x0 = cx + Math.cos(a) * r0, y0 = by + Math.sin(a) * r0;
      const xm = cx + Math.cos(a + 0.3) * ((r0 + r1) / 2), ym = by + Math.sin(a + 0.3) * ((r0 + r1) / 2);
      const x1 = cx + Math.cos(a) * r1, y1 = by + Math.sin(a) * r1;
      ctx.moveTo(x0, y0); ctx.quadraticCurveTo(xm, ym, x1, y1);
      ctx.stroke();
      ctx.strokeStyle = "#E4FAFF";
      ctx.lineWidth = outline * 0.38;
      ctx.beginPath();
      ctx.moveTo(x0, y0); ctx.quadraticCurveTo(xm, ym, x1, y1);
      ctx.stroke();
      ctx.restore();
    }
  }
}

function drawHead(ctx, s, spec = {}) {
  const v = { ...VARIANTS.gatling, ...spec };
  const outline = 0.03 * s;

  if (v.heads > 1) {
    const heads = v.heads;
    const sub = s * (heads === 3 ? 0.5 : 0.6);
    const dx = heads === 3 ? 0.62 : 0.36;
    const order = heads === 3 ? [-1, 1, 0] : [-1, 1];
    for (const k of order) {
      ctx.save();
      const back = k !== 0;
      ctx.translate(k * dx * s, (back ? 0.12 : -0.04) * s);
      ctx.rotate(k * (heads === 3 ? 0.09 : 0.06));
      drawHead(ctx, sub, {
        ...v, heads: 1, gun: true, muzzle: false,
        helmetOn: false, helmet: "#6E7C46", helmetDark: "#4A552E",
      });
      ctx.restore();
    }
    return;
  }

  leavesBack(ctx, s, v, outline);

  // ---- skull ------------------------------------------------------------
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
    lw: outline,
  });

  // ---- eyes (high on the face, clear of the helmet brim) ---------------
  const eyeX = 0.165 * s, eyeY = -0.13 * s, eyeRx = 0.115 * s, eyeRy = 0.147 * s;
  for (const sx of [-1, 1]) {
    paint(ctx, (c) => ellipsePath(c, sx * eyeX, eyeY, eyeRx, eyeRy, sx * 0.07), {
      fill: C.white, lw: outline * 0.8,
    });
    paint(ctx, (c) => ellipsePath(c, sx * eyeX + sx * 0.014 * s, eyeY + 0.03 * s, eyeRx * 0.6, eyeRy * 0.6, sx * 0.05), {
      fill: "#16210C", lw: outline * 0.55,
    });
    ctx.save();
    ctx.fillStyle = "rgba(255,255,255,0.95)";
    ctx.beginPath();
    ctx.ellipse(sx * eyeX - sx * 0.038 * s, eyeY - 0.062 * s, eyeRx * 0.2, eyeRy * 0.22, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();
  }

  // ---- brows: a slim bar in the gap between helmet and eyes ------------
  if (v.eyebrow) {
    for (const sx of [-1, 1]) {
      paint(ctx, (c) => {
        c.beginPath();
        c.moveTo(sx * 0.055 * s, -0.305 * s);
        c.quadraticCurveTo(sx * 0.17 * s, -0.35 * s, sx * 0.285 * s, -0.315 * s);
        c.quadraticCurveTo(sx * 0.17 * s, -0.312 * s, sx * 0.055 * s, -0.275 * s);
        c.closePath();
      }, { fill: "#2E4A0C", lw: outline * 0.4 });
    }
  }

  // ---- mouth: a dark lip ring that the gun collar plugs into ------------
  const mz = v.muzzle === false;
  const mx = (mz ? 0.0 : 0.245) * s;
  const my = (mz ? 0.26 : 0.185) * s;
  const mr = mz ? 0.17 : 0.155;
  paint(ctx, (c) => ellipsePath(c, mx, my, mr * s, mr * 0.95 * s, 0), {
    fill: "#3A1810", lw: outline * 0.8,
  });
  paint(ctx, (c) => {
    c.beginPath();
    c.ellipse(mx, my + 0.012 * s, mr * 0.56 * s, mr * 0.5 * s, 0, Math.PI, Math.PI * 2);
    c.closePath();
  }, { fill: "#7A3324", lw: 0 });

  // ---- gun --------------------------------------------------------------
  if (v.gun) {
    if (v.electric) {
      // charged halo around the whole turret
      ctx.save();
      const gx = 0.68 * s, gy = 0.19 * s;
      const g = ctx.createRadialGradient(gx, gy, 0.02 * s, gx, gy, 0.56 * s);
      g.addColorStop(0, "rgba(225,252,255,0.95)");
      g.addColorStop(0.34, "rgba(95,200,255,0.6)");
      g.addColorStop(1, "rgba(60,140,255,0)");
      ctx.fillStyle = g;
      ctx.beginPath();
      ctx.arc(gx, gy, 0.56 * s, 0, Math.PI * 2);
      ctx.fill();
      ctx.restore();
    }
    drawGun(ctx, s, v, outline);
  }

  // ---- helmet: a shallow bowl on the crown, brim well above the brows --
  if (v.helmetOn) {
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
      lw: outline * 0.95,
    });
    paint(ctx, (c) => {
      c.beginPath();
      c.moveTo(-hw * 1.08, brimY - 0.005 * s);
      c.quadraticCurveTo(0, brimY + 0.13 * s, hw * 1.08, brimY - 0.005 * s);
      c.quadraticCurveTo(0, brimY + 0.185 * s, -hw * 1.08, brimY - 0.005 * s);
      c.closePath();
    }, { fill: v.helmetDark, lw: outline * 0.6 });
    paint(ctx, (c) => ellipsePath(c, 0, topY + 0.09 * s, 0.038 * s, 0.034 * s, 0), {
      fill: "#C3CB84", lw: outline * 0.45,
    });
  }

  // ---- fire tuft (fire variant: flames replace the helmet) -------------
  if (v.fire) {
    const flame = (cx, cy, sc, rot) => {
      ctx.save();
      ctx.translate(cx, cy);
      ctx.rotate(rot);
      paint(ctx, (c) => {
        c.beginPath();
        c.moveTo(0, 0.24 * sc);
        c.bezierCurveTo(-0.3 * sc, 0.06 * sc, -0.24 * sc, -0.26 * sc, 0.0 * sc, -0.5 * sc);
        c.bezierCurveTo(0.2 * sc, -0.26 * sc, 0.3 * sc, 0.06 * sc, 0, 0.24 * sc);
        c.closePath();
      }, {
        fill: C.fire1,
        grad: lin(0, 0.24 * sc, 0, -0.5 * sc, [[0, C.fire3], [0.3, C.fire2], [0.68, C.fire1], [1, "#FFF6BC"]]),
        lw: outline * 0.6,
      });
      ctx.restore();
    };
    flame(-0.36 * s, -0.34 * s, 0.62 * s, -0.3);
    flame(-0.16 * s, -0.42 * s, 0.44 * s, -0.12);
    flame(0.06 * s, -0.5 * s, 0.34 * s, 0.06);
  }
}

module.exports = { drawHead, drawGun, VARIANTS };
