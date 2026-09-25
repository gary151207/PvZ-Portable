// Build all final icon deliverables into docs/icons/.
// Run: node build.js   (requires @napi-rs/canvas; see package.json)
const fs = require("fs");
const path = require("path");
const L = require("./lib");
const { appIcon } = require("./export");
const { createCanvas, loadImage } = require("@napi-rs/canvas");

const OUT = path.resolve(__dirname, "..", "..", "docs", "icons");
const S = 1024;

const CONCEPTS = [
  { key: "a-journey", mod: "./A-scene" },
  { key: "b-elements", mod: "./B-fusion" },
  { key: "c-redcard", mod: "./C-redcard" },
];

// the concepts render at 2x supersampling; bring them back down to 1024
async function downsample(canvas) {
  const img = await loadImage(canvas.toBuffer("image/png"));
  const c = createCanvas(S, S);
  const ctx = c.getContext("2d");
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = "high";
  ctx.drawImage(img, 0, 0, S, S);
  return c;
}

(async () => {
  fs.mkdirSync(OUT, { recursive: true });
  for (const c of CONCEPTS) {
    const { compose } = require(c.mod);
    const final = await downsample(compose());
    const square = path.join(OUT, `${c.key}-1024.png`);
    fs.writeFileSync(square, final.toBuffer("image/png"));
    console.log("wrote " + square);
    await appIcon(final, path.join(OUT, `${c.key}-appicon-1024.png`));
  }
})();
