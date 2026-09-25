// Normalise the hand-drawn source icon and export every platform size.
//
// Source: <repo>/icon1.png  (RGBA, arbitrary size, may carry transparent margin)
//
// What it does:
//   1. finds the alpha bounding box and crops the empty margin away
//   2. re-centres the art on a square canvas with a consistent 8% padding, so
//      the subject occupies ~84% of the frame like a normal app icon
//   3. writes:  root icon.png, Windows icon.ico, Android mipmaps,
//               iOS AppIcon.png (flattened, no alpha), Linux pixmaps/hicolor
const fs = require("fs");
const path = require("path");
const zlib = require("zlib");
const { createCanvas, loadImage } = require("@napi-rs/canvas");

const ROOT = path.resolve(__dirname, "..", "..");
const SRC = path.join(ROOT, "icon1.png");
const PADDING = 0.08; // share of the canvas left empty on each side

function loadRaw(img) {
  const c = createCanvas(img.width, img.height);
  const ctx = c.getContext("2d");
  ctx.drawImage(img, 0, 0);
  return ctx.getImageData(0, 0, img.width, img.height);
}

function alphaBBox(data, w, h, threshold = 8) {
  let minX = w, minY = h, maxX = -1, maxY = -1;
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      if (data[(y * w + x) * 4 + 3] > threshold) {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
      }
    }
  }
  if (maxX < 0) throw new Error("source image is fully transparent");
  return { minX, minY, maxX, maxY };
}

// Draw the source art, cropped to its content box, into a square of `size`
// with a fixed relative padding. Returns a canvas.
function normalize(img, size, padding = PADDING, background = null) {
  const { minX, minY, maxX, maxY } = alphaBBox(loadRaw(img).data, img.width, img.height);
  const cw = maxX - minX + 1;
  const ch = maxY - minY + 1;
  const box = size * (1 - padding * 2); // available box for the art
  const scale = Math.min(box / cw, box / ch);
  const dw = cw * scale;
  const dh = ch * scale;

  const c = createCanvas(size, size);
  const ctx = c.getContext("2d");
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = "high";
  if (background) {
    ctx.fillStyle = background;
    ctx.fillRect(0, 0, size, size);
  }
  ctx.drawImage(img, minX, minY, cw, ch, (size - dw) / 2, (size - dh) / 2, dw, dh);
  return c;
}

// ---------------------------------------------------------------- ICO writer

function encodePng(canvas) {
  return canvas.toBuffer("image/png");
}

// classic BMP (BITMAPINFOHEADER + 32bpp XOR + 1bpp AND mask) for small ICO slots
function encodeBmpEntry(canvas) {
  const size = canvas.width;
  const data = canvas.getContext("2d").getImageData(0, 0, size, size).data;
  const maskRow = Math.ceil(size / 32) * 4; // AND mask rows are dword aligned
  const xorLen = size * size * 4;
  const andLen = maskRow * size;
  const buf = Buffer.alloc(40 + xorLen + andLen);

  buf.writeUInt32LE(40, 0); // biSize
  buf.writeInt32LE(size, 4); // biWidth
  buf.writeInt32LE(size * 2, 8); // biHeight (XOR + AND)
  buf.writeUInt16LE(1, 12); // biPlanes
  buf.writeUInt16LE(32, 14); // biBitCount
  buf.writeUInt32LE(0, 16); // BI_RGB
  buf.writeUInt32LE(xorLen, 20);
  buf.writeInt32LE(2835, 24);
  buf.writeInt32LE(2835, 28);

  // XOR: bottom-up BGRA
  let p = 40;
  for (let y = size - 1; y >= 0; y--) {
    for (let x = 0; x < size; x++) {
      const i = (y * size + x) * 4;
      buf[p++] = data[i + 2];
      buf[p++] = data[i + 1];
      buf[p++] = data[i];
      buf[p++] = data[i + 3];
    }
  }
  // AND mask: fully transparent pixels set to 1
  for (let y = size - 1; y >= 0; y--) {
    const rowStart = p;
    for (let x = 0; x < size; x++) {
      if (data[(y * size + x) * 4 + 3] <= 8) {
        buf[rowStart + (x >> 3)] |= 0x80 >> (x & 7);
      }
    }
    p += maskRow;
  }
  return buf;
}

function writeIco(entries, outPath) {
  const header = Buffer.alloc(6);
  header.writeUInt16LE(0, 0); // reserved
  header.writeUInt16LE(1, 2); // type 1 = icon
  header.writeUInt16LE(entries.length, 4);

  const dir = Buffer.alloc(16 * entries.length);
  let offset = 6 + dir.length;
  entries.forEach((e, i) => {
    const o = i * 16;
    dir.writeUInt8(e.size >= 256 ? 0 : e.size, o); // width (0 means 256)
    dir.writeUInt8(e.size >= 256 ? 0 : e.size, o + 1);
    dir.writeUInt8(0, o + 2); // palette
    dir.writeUInt8(0, o + 3); // reserved
    dir.writeUInt16LE(1, o + 4); // planes
    dir.writeUInt16LE(32, o + 6); // bit count
    dir.writeUInt32LE(e.buf.length, o + 8);
    dir.writeUInt32LE(offset, o + 12);
    offset += e.buf.length;
  });

  fs.writeFileSync(outPath, Buffer.concat([header, dir, ...entries.map((e) => e.buf)]));
  console.log("wrote " + outPath + "  (" + entries.map((e) => e.size).join(", ") + ")");
}

// ------------------------------------------------------------------- helpers

function write(canvas, outPath) {
  fs.mkdirSync(path.dirname(outPath), { recursive: true });
  fs.writeFileSync(outPath, canvas.toBuffer("image/png"));
  const kb = (fs.statSync(outPath).size / 1024).toFixed(0);
  console.log("wrote " + outPath + "  " + canvas.width + "x" + canvas.height + "  " + kb + "KB");
}

(async () => {
  if (!fs.existsSync(SRC)) throw new Error("missing source icon: " + SRC);
  const img = await loadImage(SRC);
  console.log("source " + SRC + "  " + img.width + "x" + img.height);

  // master art: 1024 square, transparent, trimmed + padded
  const master = normalize(img, 1024);
  write(master, path.join(ROOT, "icon.png"));

  // ---- Windows: multi-size .ico -----------------------------------------
  const icoSizes = [16, 24, 32, 48, 64, 128, 256];
  const entries = icoSizes.map((size) => {
    const c = normalize(img, size);
    return { size, buf: size >= 128 ? encodePng(c) : encodeBmpEntry(c) };
  });
  writeIco(entries, path.join(ROOT, "icon.ico"));

  // ---- Android: launcher mipmaps ----------------------------------------
  const android = {
    "mipmap-mdpi": 48,
    "mipmap-hdpi": 72,
    "mipmap-xhdpi": 96,
    "mipmap-xxhdpi": 144,
    "mipmap-xxxhdpi": 192,
  };
  for (const [dir, size] of Object.entries(android)) {
    write(normalize(img, size), path.join(ROOT, "android/app/src/main/res", dir, "ic_launcher.png"));
  }

  // ---- iOS: single 1024 icon, flattened (iOS rejects alpha) -------------
  write(normalize(img, 1024, PADDING, "#ffffff"),
    path.join(ROOT, "ios/Assets.xcassets/AppIcon.appiconset/AppIcon.png"));

  // ---- Linux: pixmap + hicolor theme sizes ------------------------------
  write(normalize(img, 512), path.join(ROOT, "archlinux/icons/512x512/io.github.wszqkzqk.pvz-portable.png"));
  for (const size of [16, 32, 48, 64, 128, 256]) {
    write(normalize(img, size),
      path.join(ROOT, `archlinux/icons/${size}x${size}/io.github.wszqkzqk.pvz-portable.png`));
  }

  console.log("done");
})();
