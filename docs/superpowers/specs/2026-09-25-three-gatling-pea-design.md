# 三线机枪射手（Three Gatling Pea）设计

**合成态**植物（`SeedType::SEED_THREE_GATLING_PEA`，隐藏种子，**没有自己的种子卡**）：
把**三线射手**卡（325 阳光）种在已种的**机枪射手**上升级即得 —— 与**寒冰 / 火焰机枪射手**
同一条路径（`Plant::IsUpgradableTo` + `Board::MouseDownWithPlant` 改写实际种下的种子）。

- **普攻**：与机枪射手同一轮 **4 连发**（`mShootingCounter` 18/35/51/68），
  但**每一发都向每一行各打一颗**（三线射手的"全场开火"行规则）→ 一轮最多 6 行 × 4 发 = 24 颗。
- **大招**：不再随机角度散射，而是**每一行每 0.03 秒稳定一发**、每颗高度上下浮动 **±15 px**、持续 **3 秒**
  （每行 100 波 × 1 发 = **100 颗**）；伤害沿用机枪散射的 `mDamageOverride = 200`。

## 数值与节奏

| 项 | 值 | 位置 |
| --- | --- | --- |
| 阳光 / 冷却 | 325 / 750 厘秒（= 三线射手卡，合成不额外收费） | `gPlantDefs`（`src/Lawn/Plant.cpp`） |
| 生命 | 300（引擎统一值） | `Plant::PlantInitialize` |
| 发射节奏 | `mLaunchRate = 100`（与机枪射手完全一致） | `gPlantDefs` |
| 普攻连发 | 一轮 4 发 × 每一行，`mShootingCounter` = 18 / 35 / 51 / 68 | `Plant::UpdateShooting` |
| 普攻弹种 | `PROJECTILE_PEA`（**不做**机枪那 3% 电能豌豆掷骰） | `Plant::Fire` |
| 大招触发 | 每轮起手 `Rand(100) < mGatlingScatterChance` → `mGatlingScatterCountdown = 300` | `Plant::LaunchThreeGatling` |
| 大招弹幕 | 每 3 帧（0.03 s）向每一行各 1 颗、直飞、`mDamageOverride = 200` | `Plant::UpdateShooting` + `Plant::Fire` |
| 大招高度浮动 | 每颗出生时各自 `RandRangeInt(-15, +15)` 加在出膛高度上 | `THREE_GATLING_HEIGHT_JITTER` |
| 大招时长 | `THREE_GATLING_ULTIMATE_TICKS = 300`（3 秒 → 100 波、每行 100 颗） | `src/GameConstants.h` |
| 弹丸池闸门 | 池子（4096）加上"这一波要占的槽位"超过 `THREE_GATLING_PROJECTILE_POOL_GUARD = 3072` 就整波不发 | `Plant::FireThreeGatlingVolley` |
| 概率成长 | 非大招期每轮 4 次判定：50% 概率 `mGatlingScatterChance++`（上限 100，大招结束重置 3） | `Plant::UpdateShooting` |
| 合成返阳光 | 扣款后原样返还 `THREE_GATLING_SYNTHESIS_REFUND = 325`（净花费 0） | `Board::MouseDownWithPlant` |

阳光与冷却取自**被消耗的那张卡**（三线射手，325 / 750），所以合成不需要额外规则：
`Board::MouseDownWithPlant` 照常按卡价扣 325 阳光、走三线射手的卡槽冷却，随后原样返还 325。
**只有真的扣过款才返**（`aPaysWithSun`）：传送带关卡与免费种植本来就不扣钱，无条件返还会白送阳光。

## 与「射击节奏」有关的三条口径（实现时最容易踩的地方）

### 1. 大招是"按时间开火"，不是"按轮次开火"

机枪家族的散射大招寄生在 4 连发的 `mShootingCounter` 上（非 0 才开火），所以它天然会在
两次连发之间的空闲期停火。三线机枪射手要求"稳定每 0.03 秒一波、持续 3 秒"，于是大招分支被放在
`UpdateShooting` **最前面**、位于 `if (mShootingCounter == 0) return;` **之前**，并用散射计时自己算年龄：

```cpp
if (mSeedType == SeedType::SEED_THREE_GATLING_PEA && mGatlingScatterCountdown > 0)
{
    const int aUltimateAge = THREE_GATLING_ULTIMATE_TICKS - mGatlingScatterCountdown;
    if (aUltimateAge % THREE_GATLING_ULTIMATE_INTERVAL == 0) { ...FireThreeGatlingVolley(true); mShootingCounter = 1; }
    return;   // 大招期间不参与 4 连发计数
}
```

- `LaunchThreeGatling()` 在大招期间直接 `return`，所以新一轮 4 连发不会插进来打断这 3 秒。
- `mShootingCounter = 1` 是留给**收招**用的：`UpdateAbilities` 在 `UpdateShooting` 之后递减散射计时，
  等它归零时大招分支不再命中，这个 `1` 就会走到 `UpdateShooting` 尾部的收招逻辑，把三个头混回
  `anim_head_idle1/2/3`。计数口径因此是 **age = 0, 3, 6, …, 297 共 100 波**（age = 300 时计数已经归零）。

### 1b. 弹幕量：为什么是 0.03 秒一发（以及为什么不能更密）

**0.02 秒两发那一档会卡死闪退**（用户实测）。算一下就明白：

| 档位 | 每波 | 每帧新增 | 弹丸寿命 ≈200 帧 | 单株峰值同时在场 |
| --- | --- | --- | --- | --- |
| 机枪射手散射（基准） | 2 颗 / 2 帧 / 1 行 | 1 颗 | 约 200 颗 | 引擎本来就扛得住 |
| **现在（0.03 s × 每行 1 发）** | 6 颗 / 3 帧 / 6 行 | 2 颗 | 约 400 颗 | 与基准同量级 ✓ |
| 曾经（0.02 s × 每行 2 发） | 12 颗 / 2 帧 / 6 行 | **6 颗** | 约 1200 颗 | 几株齐开就顶满弹丸池 ✗ |

`Board::mProjectiles` 是定长 `DataArray`（上限 4096），而 **Release 版的 `TOD_ASSERT` 是空宏**：
池子满了不会报错，`DataArrayAlloc` 会直接越界写内存 → 卡死闪退。所以除了把频率降回 0.03 秒，
`FireThreeGatlingVolley` 开头还有一道**按整波预留槽位**的闸门：

```cpp
const int aVolleySlots = aBulletsPerRow * MAX_GRID_SIZE_Y;      // 这一波要占几个槽位
if (mBoard->mProjectiles.mSize + aVolleySlots > THREE_GATLING_PROJECTILE_POOL_GUARD) return;
```

（整波要么都发、要么都不发，不会出现"发了一半才发现池子满了"。闸门 3072 = 上限的 75%。）
`scripts/check-three-gatling-pea.ps1` 里另有一条**弹幕预算自检**：每帧新增 > 2 颗、
或单株峰值 > 512 颗就直接报错，免得以后有人再把档位调密。

### 2. 三个头的开火动画：普通一轮播一遍，大招期间循环

`PlayThreeGatlingShootAnim(bool theLoop)` 同时管两件事：普通 4 连发用
`REANIM_PLAY_ONCE_AND_HOLD`（与三线射手一致，播完停住，由尾部收招），大招第一发时切成
`REANIM_LOOP`（3 秒里三个头持续开火）。收招复用 `UpdateShooting` 尾部原本只服务
`SEED_THREEPEATER` 的那一段（条件里加上本种子即可）。

### 3. 音效：只让"本行"那一次出声

`Fire()` 里 `FOLEY_THROW` 是按调用次数播的。本植物一次开火要打满全场每一行（一轮最多 6 次
`Fire`），逐行放会叠成 24 声/轮。所以加了一道门控：

```cpp
if (mSeedType != SeedType::SEED_THREE_GATLING_PEA || theRow == mRow) { mApp->PlayFoley(FOLEY_THROW); }
```

→ 每轮 4 声（与机枪射手同量级），大招里每 0.2 秒一声（正是"稳定连发"的听感）。

## 机制一：大招子弹的高度浮动（±15 px）

浮动加在**出膛高度**上，通过 `Plant::Fire` 已有的 `theYOffset` 参数传进去：

```cpp
// FireThreeGatlingVolley
int aYOffset = theJitterY ? RandRangeInt(-THREE_GATLING_HEIGHT_JITTER, THREE_GATLING_HEIGHT_JITTER) : 0;
Fire(nullptr, aRow, PlantWeapon::WEAPON_PRIMARY, aYOffset);

// Fire：三线射手那条出膛点分支
aOriginX = mX + 45;
aOriginY = mY + 10 + theYOffset;
```

同一行的两颗各自掷一次（所以一对子弹是上下错开的，而不是完全重叠）。

**为什么只需要挪出膛高度就够**：弹丸的影子在 `ProjectileInitialize` 里按目标行摆好
（`mShadowY = GridToPixelY(...) + 67`），而引擎判定"能不能碰/该不该消失"看的是**影子间距**
`mShadowY - mPosY`：

- 本行直飞的弹丸：间距全程恒为 `57 - 浮动`；
- 跨行弹丸：`Fire` 里已有 `mShadowY -= aLaneDeltaY` 校正（三线射手那套），间距同样是 `57 - 浮动`。

而 `Projectile::CheckForCollision` 的 `> 90` 会整帧跳过碰撞（弹丸穿过僵尸飞出屏幕）、
`Projectile::CheckForHighGround` 的 `< 28` 会当场消散弹丸 —— 两者都表现为"子弹莫名其妙消失"。
±15 px 下间距落在 `42..72`（草坪）与 `30..60`（行距 85 的泳池/雾夜/屋顶），都在 `(28, 90)` 内。
`scripts/check-three-gatling-pea.ps1` 用 `THREE_GATLING_HEIGHT_JITTER` 的**当前值**跑一遍这个窗口自检，
并断言它不超过最紧的一档（45 - 28 = 17）。

## 机制二：贴图（独立 reanim 文件，不换图、不染色）

两张玩家贴图是按 **reanim 的贴图命名规则**给的（`IMAGE_REANIM_<NAME>` → `reanim/<NAME>`）：

| 贴图 | 尺寸 | 角色（对照机枪射手） |
| --- | --- | --- |
| `res/main/reanim/ThreeGaling_helmet.png` | 1000×1000（带透明通道） | 头盔；三个头各一顶（对应 `GatlingPea_helmet`） |
| `res/main/reanim/ThreeGaling_mouth_overlay.png` | 19×43 | **嘴覆层**（唇形）；对应 `GatlingPea_mouth_overlay` |
| `ThreePeater_mouth.png`（原版，**不改**） | 19×43 | **嘴洞**；对应 `GatlingPea_mouth` |
| `reanim/GatlingPea_barrel`（原版素材，无需新图） | 43×27 | 枪管；三个头各一根，四段式（照抄机枪射手的四段几何） |

接入方式是**新建一个 reanim 文件** `res/main/reanim/ThreeGaling.reanim` + 独立定义槽
`ReanimationType::REANIM_THREE_GATLINGPEA`（`REANIM_NO_ATLAS`）：

- 骨架 = `reanim/ThreePeater.reanim` 的**副本**（149 帧 × 36 条轨道）；
- **原版嘴保持不动**（`IMAGE_REANIM_THREEPEATER_MOUTH` 仍在三条嘴轨道上）——
  它在机枪射手那边对应的就是 `GatlingPea_mouth`（嘴洞），是底层；
- 追加三条嘴覆层轨道 `ThreeGaling_mouth_overlay1/2/3`，引用玩家给的
  `IMAGE_REANIM_THREEGALING_MOUTH_OVERLAY`；两张图同为 19×43，所以**逐帧沿用嘴轨道的变换**，
  唇形正好落在嘴洞上；
- 追加十二条枪管轨道 `ThreeGaling_head{1,2,3}_barrel{3,4,2,1}`（引用机枪射手自己的
  `IMAGE_REANIM_GATLINGPEA_BARREL`）；
- 追加三条头盔轨道 `ThreeGaling_helmet1/2/3`，引用 `IMAGE_REANIM_THREEGALING_HELMET`。

**轨道顺序就是绘制顺序**，与机枪射手逐层对齐
（机枪射手：`anim_face → GatlingPea_mouth → barrel3/4/2/1 → GatlingPea_mouth_overlay → … → helmet`）：

```
[三线射手原有 36 条（含三条嘴）] → [12 条枪管] → [3 条嘴覆层] → [3 条头盔]
```

于是：脸 → 嘴洞 → 枪管（从嘴里伸出来）→ 唇形压在枪管根部 → 头盔压在最上面。

### 离线预览（调美术不用起游戏）

`tools/reanim-preview.py` 把 reanim 的某一帧按引擎的变换语义渲染成 PNG，
用来核对落位与层次（近邻采样 + 不处理 attach 父子矩阵，属于近似预览）：

```bash
python tools/reanim-preview.py res/main/reanim/ThreeGaling.reanim plant.png 124,4,45,86 4   # 整株
python tools/reanim-preview.py res/main/reanim/ThreeGaling.reanim head1.png 4 8              # 单个头放大
```

本设计提交了两张预览（`docs/superpowers/specs/three-gatling-pea-preview/plant.png`、`head1.png`）：
三个头各一顶头盔、一根四段式枪管从唇形中间伸出来，与机枪射手的观感一致。
枪管整体偏小 / 想更靠前，就改生成器顶部的 `BARREL_SCALE` / `BARREL_OFFSET_X` 后重跑。

## 机制三：编译缓存陷阱（本植物踩过，必须留在 check 脚本里）

**症状**：改完 `ThreeGaling.reanim`（加枪管、换成嘴覆层）之后重新打包 pak、重启游戏，
**场上和图鉴都还是旧外观** —— 而 pak 里明明已经是新数据（对比 `dist/main.pak` 里的 XML 可以确认）。

**根因**：`DefinitionCompileAndLoad`（`src/Sexy.TodLib/Definition.cpp`）原本写成

```cpp
#ifdef _PVZ_DEBUG
    const bool aRequireCompiledUpToDate = true;
#else
    const bool aRequireCompiledUpToDate = false;     // ← Release
#endif
    const bool aShouldTryCompiled = !aRequireCompiledUpToDate || DefinitionIsCompiled(theXMLFilePath);
```

Release 下 `aShouldTryCompiled` 恒为真，于是 `DefinitionReadCompiledFile` **无条件**先读

```
<AppData>/io.github.wszqkzqk/PvZPortable/cache64/compiled/reanim/ThreeGaling.reanim.compiled
```

（读不到才退回 pak 里的同名文件）。而这个缓存正是**游戏第一次运行**时编译 XML 写下来的。
于是第一次跑的是哪个版本，之后就一直用哪个版本：改 XML、重打 pak、重启游戏统统无效。

**修法**（已改）：一律 `const bool aShouldTryCompiled = DefinitionIsCompiled(theXMLFilePath);`

- 原版定义：pak 里本来就随包带了预编译版本 → `IsFileInPakFile` 立即为真，行为与以前完全一致；
- 真实文件形式的资源（`-resdir` 指向目录）：按 mtime 比较，源文件更新就重编；
- 模组新增、没有预编译版本的定义（XML 只在 pak 里，无从 stat）：判为不可信 → 按 XML 现编
  （几毫秒、每次启动一次），换来"改了就一定生效"。

`scripts/check-three-gatling-pea.ps1` 里为此加了两条断言（必须走 `DefinitionIsCompiled`，
且 `aRequireCompiledUpToDate` 那个开关不许回来）。

**已经中招的机器**：删掉那一个缓存文件即可（或直接换上修好的 exe —— 修好后它自己就不会再信这个缓存）：

```
%APPDATA%\io.github.wszqkzqk\PvZPortable\cache64\compiled\reanim\ThreeGaling.reanim.compiled
```

**为什么不直接改 `ThreePeater.reanim`**：该文件被 `REANIM_THREEPEATER` 共享（普通三线射手、
图鉴卡面、过场动画都在用），加轨道/换嘴会让原版三线射手也长出头盔、换掉嘴。独立文件 + 独立槽位
让原版零影响，而且贴图名**直接写在文件里** —— 于是**不需要**任何 `ApplyReanimArtSwaps`
逐帧换图、也不需要"缺图就整体回退成染色"那套安全阀（`DefinitionLoadImage` 找不到图会直接
报 `Failed to load reanim`，不会静默画白块）。

### 头盔的帧区间与落位公式

三线射手的三个头各有三个窗口（绝对帧号）：`anim_head_idleN`（25 帧）、`anim_shootingN`（13 帧）、
`anim_blinkN`（3 帧）。头盔轨道的内容帧区间必须**恰好等于对应脸轨道**（`anim_faceN`）的区间，
这样它才会在 idle 与 shooting 两种动作里都出现，而**不会**出现在眨眼实例的窗口里
（眨眼是另一个临时 reanim 实例，窗口只有 3 帧；若头盔在那里也有内容就会重复画一顶）：

| 头 | 脸轨道 | 眨眼窗口 | 内容帧区间（= 头盔区间） |
| --- | --- | --- | --- |
| 1 | `anim_face1` | 1..3 | 4..41（idle 4..28 + shooting 29..41） |
| 3 | `anim_face3` | 42..44 | 45..82（idle 45..69 + shooting 70..82） |
| 2 | `anim_face2` | 83..85 | 86..123（idle 86..110 + shooting 111..123） |

落位公式照抄**原版机枪射手**的比例（实测 `GatlingPea.reanim` 第 44 帧：脸 70×65 @0.555 →
38.85×36.08；头盔 82×87 @(0.611,0.583) → 50.10×50.72；**头盔绘制宽 / 头绘制宽 = 1.288**，
**头盔中心 − 头中心 = (−3.7, −4.7) px**）：

```
r = (30 * face_sx) / 38.85                       # 三线射手的头图是 30x30
helmet_w = 50.10 * r ;  helmet_h = 50.72 * r
helmet_center = (face_x + 30*face_sx/2 - 3.7*r, face_y + 30*face_sy/2 - 4.7*r)
sx = helmet_w / 1000 ;  sy = helmet_h / 1000     # 1000x1000 的方图
kx/ky = 与脸轨道同帧相同                          # 帽子跟着头一起倾斜
x = helmet_center.x - 500*sx ;  y = helmet_center.y - 500*sy
```

生成与调参都走 `scripts/gen-three-galing-reanim.py`（顶部 4 个常量，改完重跑即可；
`--check` 只做校验、不写文件，会被 `scripts/check-three-gatling-pea.ps1` 调用）。
**`ThreePeater.reanim` 若以后被改动，必须重跑生成器**（生成器会先核对三条脸轨道的内容区间，
对不上就直接报错而不是悄悄生成错位的头盔）。

### 枪管的落位公式

枪管也用**同一条内容帧区间**规则（= 脸轨道区间，避开眨眼窗口），三个头各四段，共 12 条轨道。
枪管在 `GatlingPea.reanim` 里与头刚性绑定（同一帧内只随头动，外加一点开火抖动），
所以生成器把它当成"头局部坐标里的一个偏移"搬到每个头上：

```
r     = (30 * face_sx) / 38.85                    # 本植物的头 vs 机枪射手的头
θ     = kx（MatrixFromTransform 的线性部分 = Rot(kx)·diag(sx,sy)）
d     = (机枪射手枪管 x/y − 机枪射手脸 x/y) * r      # 偏移按头部大小等比缩放
枪管位置 = (face_x, face_y) + Rot(θ) · d            # 头部倾斜多少，枪就跟着转多少
kx/ky = 脸.kx/ky + 枪管.kx/ky                       # 枪自己的抖动叠在头的倾斜上
sx/sy = 枪管.sx/sy * r
```

帧号做**相位映射**：idle 段（25 帧）1:1，shooting 段（本植物 13 帧 → 机枪射手 39 帧）按比例拉伸，
于是枪管的开火抖动跟着本植物自己的开火动作走。枪管段号与绘制顺序与源文件一致（3、4、2、1）。

## 接线清单

| 文件 | 变更 |
| --- | --- |
| `src/ConstEnums.h` | 追加 `SEED_THREE_GATLING_PEA`、`REANIM_THREE_GATLINGPEA`（都在 `NUM_*` 之前，既有存档枚举值不动） |
| `src/Sexy.TodLib/Reanimator.cpp` | `gLawnReanimationArray` 末尾追加 `"reanim/ThreeGaling.reanim"` + `REANIM_NO_ATLAS` |
| `src/GameConstants.h` | `THREE_GATLING_SYNTHESIS_REFUND / ULTIMATE_TICKS / ULTIMATE_INTERVAL / BULLETS_PER_ROW / HEIGHT_JITTER / PROJECTILE_POOL_GUARD` |
| `src/Lawn/Plant.h` / `Plant.cpp` | 定义行；`LaunchThreeGatling` / `PlayThreeGatlingShootAnim` / `FireThreeGatlingVolley`；`PlantInitialize`（3 头）、`AttachBlinkAnim`、`IsUpgradableTo`、`UpdateShooter`、`UpdateShooting`（大招 + 4 连发 + 收招）、`Fire`（弹种/出膛点/大招子弹/跨行弹道/音效/孤狼名单） |
| `src/Lawn/Board.cpp` | 合成改写 + 返 325（金额提成变量 `aGatlingSynthesisRefund`）、HP 悬浮提示、`mPeaShooterUsed`、冰冻关沙盒旅行页 |
| `src/Lawn/Widget/AlmanacDialog.h` / `.cpp` | `NUM_ALMANAC_EXTRA_SEEDS` 8 → 9、追加图鉴植物页第 2 页条目 |
| `src/Lawn/System/ReanimationLawn.cpp` | `MakeCachedPlantFrame` 的 3 头绘制分支加入本种子（卡面/图鉴/光标预览） |
| `src/Lawn/SeedPacket.cpp` | 卡面缩放 switch 与 `SEED_THREEPEATER` 同组（0.5 / 5 / 10） |
| `res/properties/pvzp-strings.xml`、`pvzp-strings.zh-CN.xml` | `THREE_GATLING_PEA` / `_TOOLTIP` / `_DESCRIPTION` |
| `res/main/reanim/ThreeGaling.reanim` | 新数据文件（生成器产物） |
| `scripts/gen-three-galing-reanim.py` | 生成器 + `--check` |
| `scripts/check-three-gatling-pea.ps1` | 新增源级断言 + 浮动窗口数值自检 + 编译缓存陷阱断言 |
| `src/Sexy.TodLib/Definition.cpp` | 编译缓存改为"必须是最新的"（Release 不再盲目相信用户缓存） |
| `tools/reanim-preview.py` | 新增离线预览工具（配合上面的调参） |

**无需改动**的地方（容易多此一举）：`SaveGame.cpp`（只追加枚举值、无新存档字段）、
`Plant::PreloadPlantResources`（定义里的 reanim 类型就是实际类型）、
`LawnApp::HasSeedType` / 种子选择器（没有自己的卡）、`Plant::IsUpgrade` / `IsRedCard`
（不是升级卡、卡包用普通底色）、斗蛐蛐植物池（`SetupCricketFight` 自动收录本区间内所有种子）。

### 顺带维护的旧检查脚本

新种子插在 `SEED_FIRE_GATLING_PEA` / `REANIM_FIRE_GATLINGPEA` 之后，按仓库既有做法
（"窗口放宽 + 说明"）同步了四处断言：

- `check-fire-gatling-pea.ps1`：两条枚举窗口放宽到 2000；`AddSunMoney(GATLING_SYNTHESIS_REFUND)`
  改成 `AddSunMoney(aGatlingSynthesisRefund)`（金额提成了变量）；`NUM_ALMANAC_EXTRA_SEEDS` 8 → 9。
- `check-fire-pea-shooter.ps1`：两条枚举窗口放宽到 2000。
- `check-peater-1-5.ps1`：**未改**——本种子的 `case` 紧挨着 `SEED_THREEPEATER` 插入，
  `SEED_PEATER_1_5 → PROJECTILE_PEA` 的间隔断言因此仍然成立。
- `check-threepeater-lanes.ps1`：`Fire` 里那两处分支条件现在写作
  `SEED_THREEPEATER || SEED_THREE_GATLING_PEA`，正则相应放宽（仍只取含 `MOTION_THREEPEATER` 的那一处）。

## 明确不做

- 不改 `ThreePeater.reanim` / `REANIM_THREEPEATER`（原版三线射手零影响）。
- 不新增弹丸类型、不新增存档字段、不改 `SaveGame.cpp`。
- 不做独立种子卡、不做旅行专属限定（哪里有卡就能合，与 `SEED_FIRE_GATLING_PEA` 一致）。
- 不加"点击升级"之类新机制；不动两张素材文件本身。

## 测试

静态（本仓库无自动化测试框架，与其它 `check-*.ps1` 一样是源级断言）：

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check-three-gatling-pea.ps1
# 以及全部既有检查脚本（新种子会挤动几条枚举窗口断言）
```

构建（Windows）：`build.bat`（或 `cmake --build build`）；构建会把 `res/main` 重新打包成
`dist/main.pak`，新 reanim、两张贴图随之进入资源包。

**改过 reanim / particle 这类 XML 定义之后**：如果游戏里看不到变化，先确认不是**编译缓存陷阱**
（见「机制三」）—— 修好之后 Release 版也会按 XML 现编，不需要手动删缓存；
旧版本（或换回旧 exe）才需要删掉那个 `cache64/compiled/reanim/ThreeGaling.reanim.compiled`。

进游戏手动验收：

1. 三线射手卡拖到已种的机枪射手上 → 变成本植物、字幕"合成完成，返还 325 阳光！"、
   卡进冷却、阳光净变化 0；空地上拖三线射手卡仍正常种植。
2. 外观：三个头各一顶头盔 + 一根四段式机枪枪管，都跟随各自头部的摆动与倾斜；
   嘴是「原版嘴洞 + 新唇形覆层」，枪管从嘴里伸出来、唇形压在枪管根部（与机枪射手同一套层级）；
   **这些的位置与缩放只能进游戏看**（不满意就改 `scripts/gen-three-galing-reanim.py`
   顶部常量后重跑 —— 枪管几何完全由机枪射手的轨道推出来，改 `GATLING_*` 常量即可整体调）。
3. 普攻：僵尸分散在多行时，每行每轮各吃 4 发；穿过火炬树桩会被点燃（普通豌豆语义）。
4. 大招：多打几轮等到开大，确认"每行每 0.03 秒 1 发、±15 px 上下浮动、共 3 秒、无随机角度"；
   悬浮提示显示剩余秒数；结束后三个头回 idle；**帧率正常、不卡死不闪退**（这是从 0.02 s 两发降下来的原因）。
5. 眨眼：不出现"两顶头盔 / 两根枪管"的重影（靠内容帧区间与眨眼窗口不重叠）。
6. 冰冻关沙盒旅行页可种下；图鉴植物页第 2 页有卡面与描述。
7. 存档：战斗中存档 → 读档继续（植物、大招计时、卡冷却）。

### 强度与性能（当前档位）

- 每行 100 颗、每颗 `mDamageOverride = 200` → 单株一次大招的理论伤害是 6 行 × 20000，
  比机枪射手散射（单行 300 颗 × 200）"多打 5 行"。要不要调低单发伤害属于平衡问题，
  改 `Plant::Fire` 大招分支里的那一个 `200` 即可。
- 单株峰值同时在场 ≈ 400 颗弹丸（与机枪射手散射同量级）；多株齐开时若逼近
  `THREE_GATLING_PROJECTILE_POOL_GUARD`，那几波会**整波不发**（设计如此，不是崩溃）。
