// Renders docs/icons/preview.png: the three concepts at full size plus their
// rounded app-icon variants over a checkerboard, so transparency is visible.
const fs = require("fs");
const path = require("path");
const { createCanvas, loadImage } = require("@napi-rs/canvas");

const OUT = path.resolve(__dirname, "..", "..", "docs", "icons");
const NAMES = ["a-journey", "b-elements", "c-redcard"];

(async () => {
  const c = createCanvas(1728, 750);
  const ctx = c.getContext("2d");
  for (let y = 0; y < 750; y += 36) {
    for (let x = 0; x < 1728; x += 36) {
      ctx.fillStyle = ((x / 36) + (y / 36)) % 2 ? "#ececec" : "#d4d4d4";
      ctx.fillRect(x, y, 36, 36);
    }
  }
  for (let i = 0; i < NAMES.length; i++) {
    const big = await loadImage(path.join(OUT, `${NAMES[i]}-1024.png`));
    const app = await loadImage(path.join(OUT, `${NAMES[i]}-appicon-1024.png`));
    const x = 24 + i * 560;
    ctx.drawImage(big, x, 24, 480, 480);
    ctx.drawImage(app, x + 300, 516, 180, 180);
    ctx.drawImage(big, x, 516, 0, 0); // keep column spacing explicit
  }
  fs.writeFileSync(path.join(OUT, "preview.png"), c.toBuffer("image/png"));
  console.log("wrote " + path.join(OUT, "preview.png"));
})();
