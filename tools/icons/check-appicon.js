// Renders a verification sheet of the generated app icons, including the ICO
// frames as the OS would show them (large -> downscaled -> tiny).
const fs = require("fs");
const path = require("path");
const { createCanvas, loadImage, Image } = require("@napi-rs/canvas");

const ROOT = path.resolve(__dirname, "..", "..");
const OUT = path.join(ROOT, "docs", "icons", "appicon-check.png");

function readIco(buf) {
  const count = buf.readUInt16LE(4);
  const frames = [];
  for (let i = 0; i < count; i++) {
    const o = 6 + i * 16;
    const size = buf.readUInt8(o) || 256;
    const len = buf.readUInt32LE(o + 8);
    const off = buf.readUInt32LE(o + 12);
    frames.push({ size, data: buf.subarray(off, off + len), png: buf.readUInt32LE(off) === 0x474e5089 });
  }
  return frames;
}

(async () => {
  const W = 1500, H = 760;
  const c = createCanvas(W, H);
  const ctx = c.getContext("2d");
  for (let y = 0; y < H; y += 30) {
    for (let x = 0; x < W; x += 30) {
      ctx.fillStyle = ((x / 30) + (y / 30)) % 2 ? "#efefef" : "#d8d8d8";
      ctx.fillRect(x, y, 30, 30);
    }
  }

  // master + a real downscale ladder (like a launcher would render)
  const master = await loadImage(path.join(ROOT, "icon.png"));
  ctx.drawImage(master, 24, 24, 420, 420);
  ctx.fillStyle = "#222";
  ctx.font = "22px sans-serif";
  ctx.fillText("icon.png 1024 (master)", 24, 470);

  const ladder = [128, 64, 48, 32, 16];
  let x = 480;
  for (const s of ladder) {
    ctx.drawImage(master, x, 24 + (128 - s), s, s);
    ctx.fillText(s + "px", x, 220);
    x += 150;
  }

  // ICO frames straight out of the file, at their native size
  const frames = readIco(fs.readFileSync(path.join(ROOT, "icon.ico")));
  x = 24;
  ctx.fillText("icon.ico frames (native size)", 24, 540);
  for (const f of frames) {
    const img = new Image();
    img.src = f.data;
    const cols = Math.ceil(Math.sqrt(f.size));
    const cell = 128 / cols;
    ctx.drawImage(img, x, 560, cols * cell, cols * cell);
    ctx.fillText(f.size + (f.png ? " png" : " bmp"), x, 740);
    x += cols * cell + 26;
  }

  // android + ios + linux samples
  const samples = [
    ["android xxxhdpi 192", "android/app/src/main/res/mipmap-xxxhdpi/ic_launcher.png", 192],
    ["ios 1024 (flattened)", "ios/Assets.xcassets/AppIcon.appiconset/AppIcon.png", 192],
    ["linux hicolor 256", "archlinux/icons/256x256/io.github.wszqkzqk.pvz-portable.png", 192],
  ];
  let sx = 900;
  for (const [label, rel, size] of samples) {
    const img = await loadImage(path.join(ROOT, rel));
    ctx.drawImage(img, sx, 330, size, size);
    ctx.fillText(label, sx, 545);
    sx += 200;
  }

  fs.writeFileSync(OUT, c.toBuffer("image/png"));
  console.log("wrote " + OUT);
})();
