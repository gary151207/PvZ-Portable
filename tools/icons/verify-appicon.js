const fs = require("fs");
const { createCanvas, loadImage } = require("@napi-rs/canvas");

const buf = fs.readFileSync("../../icon.ico");
console.log("ICO header: reserved=" + buf.readUInt16LE(0) + " type=" + buf.readUInt16LE(2) + " count=" + buf.readUInt16LE(4));
for (let i = 0; i < buf.readUInt16LE(4); i++) {
  const o = 6 + i * 16;
  const w = buf.readUInt8(o) || 256, h = buf.readUInt8(o + 1) || 256;
  const len = buf.readUInt32LE(o + 8), off = buf.readUInt32LE(o + 12);
  const sig = buf.readUInt32LE(off);
  const isPng = sig === 0x474e5089;
  let note = isPng ? "PNG " + buf.readUInt32BE(off + 16) + "x" + buf.readUInt32BE(off + 20) : "BMP";
  if (!isPng) {
    const biW = buf.readInt32LE(off), biH = buf.readInt32LE(off + 4), bpp = buf.readUInt16LE(off + 14);
    const expect = 40 + w * h * 4 + Math.ceil(w / 32) * 4 * h;
    note = `BMP ${biW}x${biH / 2} ${bpp}bpp  entryLen=${len} expected=${expect} ${len === expect ? "OK" : "MISMATCH"}`;
  }
  console.log(`  frame ${i}: dirEntry=${w}x${h} bytes=${len} off=${off} -> ${note}`);
}

(async () => {
  // iOS icon must be opaque
  const ios = await loadImage("../../ios/Assets.xcassets/AppIcon.appiconset/AppIcon.png");
  const c = createCanvas(ios.width, ios.height);
  const ctx = c.getContext("2d");
  ctx.drawImage(ios, 0, 0);
  const d = ctx.getImageData(0, 0, ios.width, ios.height).data;
  let minA = 255;
  for (let i = 3; i < d.length; i += 4) if (d[i] < minA) minA = d[i];
  console.log("iOS AppIcon " + ios.width + "x" + ios.height + " minAlpha=" + minA + (minA === 255 ? "  OK (no transparency)" : "  WARNING"));

  // master icon padding check
  const m = await loadImage("../../icon.png");
  const c2 = createCanvas(m.width, m.height);
  const x2 = c2.getContext("2d");
  x2.drawImage(m, 0, 0);
  const dd = x2.getImageData(0, 0, m.width, m.height).data;
  let minX = m.width, minY = m.height, maxX = -1, maxY = -1;
  for (let y = 0; y < m.height; y++) for (let x = 0; x < m.width; x++) {
    if (dd[(y * m.width + x) * 4 + 3] > 8) { if (x < minX) minX = x; if (x > maxX) maxX = x; if (y < minY) minY = y; if (y > maxY) maxY = y; }
  }
  console.log(`icon.png content ${maxX - minX + 1}x${maxY - minY + 1} in ${m.width}x${m.height}; margins L${minX} R${m.width - 1 - maxX} T${minY} B${m.height - 1 - maxY}`);
  const padPct = (100 * minX / m.width).toFixed(1);
  console.log("effective padding " + padPct + "% per side; content occupies " + (100 * (maxX - minX + 1) / m.width).toFixed(1) + "% of width");
})();
