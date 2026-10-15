# 字体替换（MOD-FONT）

这个分支把游戏**所有原版位图字体**（`FONT_BRIANNETOD*`、`FONT_DWARVENTODCRAFT*`、`FONT_HOUSEOFTERROR*`、
`FONT_CONTINUUMBOLD14*`、`FONT_PICO129`、`FONT_TINYBOLD` 等，来自 `main.pak`）替换成同一个 TrueType 字体
（默认 `fzcq.ttf`，方正少儿）。加载界面、主界面同样使用替换后的字体。

## 放置字体文件

把字体文件放到游戏数据目录（与 `main.pak` 同级）下的任一位置，按以下顺序查找，取第一个存在的：

1. 命令行参数 `-modfont="<路径>"` 指定的文件；
2. `properties/fzcq.ttf`（推荐；`res\properties\` 会被 `build.bat` 复制到 `dist\properties\`）；
3. `fonts/fzcq.ttf`；
4. `fzcq.ttf`。

字体文件不存在、无法读取或不是 TrueType 字体时，游戏不做任何替换，表现与原来完全一致。

> `res/` 是本仓库跟踪的可编辑资源树（`res/main/` 由 `main.pak` 解包而来，`res/properties/` 放外部配置），
> 打包产物 `dist/main.pak` 不进版本控制。字体文件放在 `res/properties/fzcq.ttf` 即可。

## 命令行开关

| 参数 | 说明 |
| --- | --- |
| `-modfont="<路径>"` | 指定用于替换的字体文件 |
| `-nomodfont` | 关闭字体替换，保留原版位图字体（便于对比排查） |

示例：

```bat
run-pvz.bat -modfont="D:\fonts\fzcq.ttf"
run-pvz.bat -nomodfont
```

## 实现方式

引擎原本只有位图字体（`ImageFont`），没有任何 TrueType 渲染能力，因此新增了：

- `src/SexyAppFramework/graphics/stb_truetype.h`：vendored 的 [stb_truetype](https://github.com/nothings/stb)（public domain / MIT）；
- `src/SexyAppFramework/graphics/TrueTypeFont.{h,cpp}`：`_Font` 实现。按像素高把字形光栅化进一张运行时创建的
  `MemoryImage` 图集（白色 RGB + 覆盖度放在 alpha 通道），再用与 `ImageFont` 相同的方式绘制
  （`SetColorizeImages(true)` + `SetColor` + `DrawImage`）。图集 512² 起步，写满自动放大到 2048²；
- `src/SexyAppFramework/graphics/ModFont.{h,cpp}`：包装类 + 字体文件加载。在 `ResourceManager::DoLoadFont()`
  里把刚创建的位图字体包起来，因此全局 `FONT_*`、`LoadFont`、`GetFontThrow`、reanimator 的按名加载等
  **所有**取字体的路径都会拿到替换后的字体；
- `TodCommon::TodDrawStringMatrix()`：reanimator/局内消息用的矩阵文本路径，改为对替换字体逐字形
  `TodBltMatrix` 绘制（原先对 `_Font*` 的 `reinterpret_cast<ImageFont*>` 也一并改成 `dynamic_cast`，避免对
  非位图字体解引用）。

## 行为与调参

- **度量沿用原字体**：`ascent/height/line spacing` 一律取自被替换掉的原版位图字体，所以每处文字的基线和行距
  与替换前一致（例如种子卡片的费用、阳光数都按原字体的 ascent 定位，不会往下掉）。
- **基线约定与原版一致**：`DrawString(g, x, y, ...)` 里的 `y` 就是**基线**（字形墨迹顶端在 `y - GetAscent()`
  附近），与 `ImageFont` 的语义完全相同；这是文字不会整体下移的关键。
- **字号推导**：像素高按「让替换字体的 ascent 等于原字体 ascent」计算（`TrueTypeFont::PixelSizeForAscent`），
  这样字形的视觉高度和基线同时对齐原版。
- **多层字体**：原字体可能用多层叠加做阴影/描边（例如 `FONT_BRIANNETOD32` 是一层 `ColorMult=黑` + 一层正常）。
  替换字体会按层重复绘制，并套用每层自己的 `ColorMult/ColorAdd` 与偏移，尽量保留原来的描边/阴影观感。
- **笔画实心度**：抗锯齿会让小字号发灰，因此对字形覆盖度做了 gamma 调整（`TrueTypeFont.cpp` 里的
  `kCoverageGamma`，默认 0.75）；调大更实、调小更细。
- **缺字回退**：替换字体没有的码位（生僻字、特殊符号等）按连续片段回退到原版位图字体绘制，测量与绘制
  使用同一套回退规则。
- **描边字体**：资源名含 `OUTLINE` 的字体（`FONT_CONTINUUMBOLD14OUTLINE`）用膨胀（3×3 最大值滤波）
  后的字形，保证 `MessageWidget`「先画黑描边、再画白字」的写法仍有描边。
- **尺寸微调**：`src/SexyAppFramework/graphics/ModFont.cpp` 里的 `kFontSizeTable` 可以按资源名覆盖像素高，
  例如 `{ "FONT_PICO129", 10 }`。默认值已按原字体 ascent 推导，一般不需要改。
- **图集上限**：同尺寸字体共享一张图集；图集写满且已到 2048² 时，新字形回退原版位图字体。
