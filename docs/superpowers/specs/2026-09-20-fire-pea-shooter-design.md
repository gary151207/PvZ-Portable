# 火豌豆射手（Fire Pea Shooter）设计

旅行专属**红卡**植物（`SeedType::SEED_FIRE_PEASHOOTER`）：175 阳光、普通短冷却（`mRefreshTime=750`）、
300 生命、**可直接种下**。发射**特制紫火豌豆**：**群伤** —— 直接命中的僵尸吃满 **65 伤害**，
命中点周围（`FIRE_PEA_SPLASH_WIDTH` = 100px 宽、上下各 1 行）的其它僵尸各吃 **1/3 ≈ 21 伤害**；
**命中与溅射到的**僵尸都**变红 4 秒**，这 4 秒内它们**受到的任何伤害 +40%**；
4 秒后僵尸的样子与伤害倍率一起恢复原样。

## 数值与节奏

| 项 | 值 | 位置 |
| --- | --- | --- |
| 阳光 / 冷却 | 175 / 750 厘秒 | `gPlantDefs`（`src/Lawn/Plant.cpp`） |
| 发射间隔 | `mLaunchRate = 113`（1.125 秒；实际 `mLaunchRate - Rand(15)`） | `FIRE_PEASHOOTER_LAUNCH_RATE` |
| 弹丸伤害（直接命中） | 65 | `gProjectileDefinition[PROJECTILE_PURPLE_FIRE_PEA]` |
| 溅射伤害 | 伤害 / 3（引擎既有 splash 公式）≈ 21 | `Projectile::DoSplashDamage` |
| 溅射范围 | 宽 `FIRE_PEA_SPLASH_WIDTH` = 100px、上下各 1 行 | `Projectile::IsZombieHitBySplash` |
| 易伤时长 | 400 逻辑帧（4 秒，100 帧/秒） | `FIRE_PEA_VULN_TICKS` |
| 易伤倍率 | +40%（`伤害 × 140 / 100`，整数向下取整） | `FIRE_PEA_VULN_PERCENT` |
| 变红强度 | 与"冰冻/减速"同一套：覆写色 + 同色加色 = `(FIRE_PEA_VULN_R/G/B) = (255,75,75)` | `FIRE_PEA_VULN_*` |
| 火焰切换 | 每 8 帧在 `fire1` / `fire2` 之间切 | `FIRE_PEASHOOTER_FIRE_FLIP_TICKS` |
| 火焰位置微调 | +10px（右），走轨道实例 `mShakeX/mShakeY` | `FIRE_PEASHOOTER_FIRE_OFFSET_X/Y` |

**这一发本身吃不到自己的易伤**：`Projectile::DoImpact` 先结算伤害，再挂易伤标记
（群伤时由 `DoSplashDamage` 在伤害循环之后给每个被溅射到的僵尸挂）。

## 机制一：脑袋左上后方那撮小叶子的火 = 覆盖 headleaf 锚点轨道

"背后的火"指的是豌豆射手**脑袋左上后方那一小块叶子**（`idle_headleaf_*` 那撮，共 6 支轨道），
**不是**底下的大叶子（`frontleaf` / `backleaf`）。锚点选 `idle_headleaf_tip_top`（这撮里最上面的一支）：
描点中心 `(16.7, 21.5)`、缩放 `0.555`，于是 128×128 火焰的可见部分（≈33×24）落在
`(-5.8, 12.9)-(27.0, 37.3)` —— 正好是脑袋左上后方那一块。
`Reanimation::DrawTrack` 取**原图**（`PeaShooter_headleaf_tip_top.png` 14×12）算描点，
再用 `mImageOverride` 的尺寸绘制，所以火焰白拿这撮小叶子的摇摆动画。

> 定位口径：把截图里"要放火的位置"换算成 reanim 坐标后，`tip_top` 的火焰中心是 `(10.6, 25.1)`，
> 与目标 `(11.0, 22.0)` 差 ~3 单位；其余候选（`nearest` (15.4,28.2)、`2ndfarthest` (9.7,38.1)、
> `farthest` (11.7,45.0)…）都差 7~23 单位，`2ndfarthest`/`farthest` 会明显偏低（贴头下缘）。
> 对照图 `fire-pea-shooter-preview/2-fire-on-headleaf.png` 里画了目标圈，可以直接核对。

**为什么必须挂在 head 实例上**：headleaf 轨道只在头部层（帧 `29..53`）有数据，
body 实例（帧 `4..28`）这一层是空白帧 —— 挂在 body 上根本不会画出来。

**层级刚好合适**：head 实例是挂在身体 `anim_stem` 轨道上的附件，在 body 的
`stalk_*` / `frontleaf`（第 8~11 条）**之后**绘制，而 headleaf（第 13~18 条）又排在
`anim_face`（第 19 条，脸）**之前** —— 所以火焰是"脑袋后面烧、脸挡着前面"，
既不会被底下的大叶子挡住，也不会糊在脸上。

- 场上植物：`Plant::PlantInitialize` 先给 head 实例铺第 0 张，`Plant::Update` 每帧按
  `(mMainCounter / FIRE_PEASHOOTER_FIRE_FLIP_TICKS + mPlantCol * 3 + mRow) & 1` 覆盖（相位按格子错开）。
- 卡面 / 图鉴 / 光标预览：`ReanimatorCache::DrawReanimatorFrame` 里静态铺第 0 张
  （缓存帧不会每帧重画，所以不做交替）。临时实例同时要画 body 层与 head 层，一个实例就够了。

## 机制一之二：没有眉毛、没有多余叶尖

`FirePeaShooterHideTracks()` 把六条"原版豌豆射手才有"的装饰轨道设为 `RENDER_GROUP_HIDDEN`：

- `PeaShooter_eyebrow`：`PeaShooter_eyebrow.png` 是**独立的眉毛贴图**（画在头上，帧 29..103），
  新头（`FirePeaShooter_head.png`）本身没画眉毛，留着就会多出一条原版的绿眉毛
  —— 与 1.5 发射手同款处理。
- 那撮小叶子里**除锚点（`idle_headleaf_tip_top`）以外**的五支（`idle_headleaf_farthest` /
  `_3rdfarthest` / `_2ndfarthest` / `_tip_bottom` / `_nearest`）：整撮都被火取代了。
  它们有的排在锚点之前（会被火盖住）、有的排在锚点之后（会盖在火上面），留着就是火里戳出来的绿叶。

调用点：场上植物的 body 实例与 head 实例（眉毛在两个实例上都隐藏，统一处理）、
以及 `DrawReanimatorFrame` 里卡面/图鉴/光标预览的临时实例。

## 机制二：专用贴图用独立 reanim 槽位 + 天然兜底

`ReanimationType::REANIM_FIRE_PEASHOOTER` 指向**同一个** `reanim/PeaShooter.reanim`，但用
`REANIM_NO_ATLAS`（定义里始终是真实贴图指针，换图不挑时机，也不与豌豆射手共用 Atlas）。
`FirePeaShooterHasCustomArt()` 复用公共助手 `ApplyReanimArtSwaps()` 把
`PeaShooter_head/mouth/blink1/blink2` 换成 `FirePeaShooter_*`（四张与原图**同尺寸**、必须带透明通道），
失败即整体回退 —— 此时该槽位保持原版豌豆射手贴图，**植物看起来就是普通豌豆射手**，不会画白块。

`gPlantDefs` 里**直接**写 `REANIM_FIRE_PEASHOOTER`（不像究极电能系列那样再套一层
`XxxReanimType()` 运行时切换），因为兜底物就是"这个槽位里原本的贴图"，不需要换类型；
`PlantInitialize` / `AttachBlinkAnim` / `MakeCachedPlantFrame` 读的都是它。

> `PeaShooter.reanim` 并不引用 `PeaShooter_Lips`，所以 `FirePeaShooter_lips.png` 没有可替换的目标，
> 与原版 `PeaShooter_Lips.png` 一样是闲置素材。

## 机制三：紫火弹丸（缓存滤镜换色 + 群伤）

弹丸 `PROJECTILE_PURPLE_FIRE_PEA` 不画贴图，视觉复用原版"被火炬树桩点燃的豌豆"那套
`REANIM_FIRE_PEA`（旋转火球 + 三条火舌 + 火花），整株挂
`FilterEffect::FILTER_EFFECT_FIREPEA_PURPLE` —— 新增的 HSL 缓存滤镜，把色相直接设成
`FIRE_PEA_PURPLE_HUE = 5.03`（0..6 扇区 → 301.8°，紫）、饱和度 `× FIRE_PEA_PURPLE_SAT = 1.47`
（两个数取自仓库根目录「火豌豆射手的颜色.txt」）。

选它而不是平涂叠加：`mExtraOverlayColor` 是"白色剪影 × 颜色"，会把火球的明暗与亮斑压平；
换色保留了亮度层次，紫得干净又有火的样子。滤镜结果按 (原图, 滤镜) 全局缓存，每颗弹丸共用。

**群伤**：直接把本类型加进 `IsSplashDamage()`，复用引擎既有的 splash 通道
（`DoImpact` → `DoSplashDamage`）：直接目标吃满 65，`IsZombieHitBySplash` 里给本类型把矩形加宽到
`FIRE_PEA_SPLASH_WIDTH`（100px）并沿用"上下各 1 行"的通用规则，其它僵尸各吃 `65/3 ≈ 21`。
易伤标记在**伤害结算之后**补挂到每个被溅射到的僵尸身上（见 `DoSplashDamage` 末尾的循环）。

**命中不炸火**：曾经挂过 `PARTICLE_FIREBALL_DEATH`，但那份粒子是**僵尸博士火球的爆燃**
（`IMAGE_ZOMBIE_BOSS_FIREBALL_PARTICLES`，20 颗粒子、`ParticleScale` 最高 2 倍、`LaunchSpeed 200`），
命中时会在场上糊出一大团火，所以去掉了，只保留 `FOLEY_IGNITE` 的"呼"声。

行为上它是"打着火的豌豆"而不是火球：**不穿透**，穿过**火炬树桩不转化**
（`PeaAboutToHitTorchwood` 只认 PEA / SNOWPEA）。

## 机制四：僵尸变红 + 易伤

- **数值**：`Zombie::TakeDamage` 在路障射手 2000 上限之后、Boss 光环之前统一乘 `140/100`，
  因此护盾 / 头盔 / 本体、以及**所有伤害来源**（其它植物、爆炸、碾压、小推车）一视同仁。
  `DAMAGE_HITS_SHIELD_AND_BODY`（火球/西瓜这类"同时打盾与本体"）会把伤害重置回原始值，
  那里补乘一次，否则本体那份会白丢 40%。
- **计时**：`Zombie::mFireVulnCounter`（新字段，`ZombieInitialize` 清零，`Zombie::Update` 每帧自减）。
- **变红**：`Zombie::DrawReanim` 用的是**和"冰冻/减速"完全同一套画法** ——
  `mColorOverride`（正片叠底）+ `mExtraAdditiveColor`（同色加色）+ `mEnableExtraAdditiveDraw`，
  只是把冰冻那支 `Color(75, 75, 255)` 的蓝色通道搬到红色通道上 → `Color(255, 75, 75)`，
  看起来就是"中了红色的减速"。想调浓淡只动 `FIRE_PEA_VULN_G/B`（越接近 255 越灰白）。
  优先级：焦黑 > Boss 濒死灰 > 魅惑紫 > **火豌豆易伤红** > 冰冻蓝 > 水族馆绿
  （易伤标记必须可见，所以压过冰蓝；速度仍按已施加的冰冻/减速走）。
  护盾 / 铁门等 render group 会一并变红，头盔等独立 reanim 不变色。
  注意 reanim 是池化复用的，但 `DataArrayAlloc` 会 placement-new 重建对象（构造函数把颜色与
  两个 enable 标志复位），所以不需要额外"每帧写回 false"来防残留。
- **存档**：`SyncZombieTailPortable` **末尾**追加 `mFireVulnCounter`（长度前缀字段，旧档读为 0），
  不提升 `.v4` 版本、不动既有字段顺序。

## 机制五：火焰位置微调（+10px 向右）

火焰挂在轨道上，位置由轨道变换决定；要再挪一点就改
`FIRE_PEASHOOTER_FIRE_OFFSET_X/Y`（单位 = 屏幕像素，植物 1:1 绘制）。
实现方式是把偏移写到**轨道实例**的 `mShakeX/mShakeY` 上：`Reanimation::DrawTrack` 会把它作为
这一条轨道独有的平移加进矩阵，所以整株（头/茎/叶）不动，只有火挪；又因为这条轨道的
`mShakeOverride` 是 0，`Reanimation::Update` 不会把 `mShakeX/mShakeY` 随机覆盖掉。

## 接入点

| 文件 | 改动 |
| --- | --- |
| `src/ConstEnums.h` | `SEED_FIRE_PEASHOOTER` / `REANIM_FIRE_PEASHOOTER` / `PROJECTILE_PURPLE_FIRE_PEA = 16`（全部前插在 `NUM_*` 之前） |
| `src/GameConstants.h` | `FIRE_PEASHOOTER_*` / `FIRE_PEA_*` 常量 |
| `src/Sexy.TodLib/FilterEffect.h/.cpp` | `FILTER_EFFECT_FIREPEA_PURPLE` + `FilterEffectDoHueSat()` |
| `src/Sexy.TodLib/Reanimator.cpp` | `REANIM_FIRE_PEASHOOTER` → `reanim/PeaShooter.reanim`（`REANIM_NO_ATLAS`） |
| `src/Lawn/Plant.h/.cpp` | 专用贴图 + 火焰覆盖 + 隐藏眉毛/叶尖的助手；`gPlantDefs` 行；豌豆组 case + 眨眼组 + 枪口偏移 + `Fire()` 弹丸映射 + `IsRedCard` + `PreloadPlantResources` |
| `src/Lawn/System/ReanimationLawn.cpp` | `DrawReanimatorFrame` 换贴图 + 静态火焰 + 隐藏眉毛/叶尖；`MakeCachedPlantFrame` 头部列表并入该种子 |
| `src/Lawn/Projectile.cpp` | `gProjectileDefinition` 65 伤害；初始化挂紫色火球 reanim；**群伤**（`IsSplashDamage` / `IsZombieHitBySplash` / `DoSplashDamage` 的溅射易伤）；命中不炸火；`Update/Draw/DrawShadow/GetProjectileRect/CantHitHighGround/CheckForHighGround/PlayImpactSound/DoImpact` 逐处登记 |
| `src/Lawn/Zombie.h/.cpp` | `mFireVulnCounter` + `ApplyFireVulnerability()` + `TakeDamage` 加成 + `DrawReanim` 染红 |
| `src/Lawn/System/SaveGame.cpp` | 僵尸 tail 末尾追加易伤剩余刻 |
| `src/Lawn/Travel.cpp` | `gTravelPlantDefs` 登记（页 1 第 6 格） |
| `src/LawnApp.cpp` | `HasSeedType` 旅行特判 |
| `src/Lawn/Board.cpp` | 冰关沙盒旅行页 |
| `src/Lawn/Widget/AlmanacDialog.h/.cpp` | 图鉴植物页第 2 页（第 7 格） |
| `res/properties/pvzp-strings.xml`、`pvzp-strings.zh-CN.xml` | `FIRE_PEASHOOTER` / `_TOOLTIP` / `_DESCRIPTION` |

## 手动验收

离线对照图（`fire-pea-shooter-preview/`，按 `PeaShooter.reanim` 的 base pose + 真实贴图合成的诊断图，
只看布局不看引擎）：

| 图 | 内容 | 说明 |
| --- | --- | --- |
| `1-peashooter-original.png` | 原版豌豆射手（基准） | 脑袋左后方可见那撮小叶子 |
| `2-fire-on-headleaf.png` | **本次改法**：火覆盖 `idle_headleaf_tip_top`，隐藏眉毛与多余叶尖 | 红圈 = 截图里圈的目标位置，火焰落在圈内 |
| `3-fire-on-backleaf.png` | 更早一版：火覆盖底下的小叶子（`backleaf`） | 火落在植株底部，不是"脑袋后面那一块" |

1. 图鉴植物页翻到第 2 页：头/嘴/眨眼是橙红色、**没有眉毛**，**脑袋左上后方**是火焰
   且两张图来回闪、火焰右侧被脸挡住；底下的大叶子保持原样。
   卡面与光标预览同样带火（无交替；预览不应用 +10px 偏移不敏感，缩放后会按比例一起缩）。
2. 单只普通僵尸 270 HP：首发 → 205；4 秒内第二发 → **114**（65 × 1.4 = 91）；4 秒后再一发 → 49。
   期间僵尸**淡淡发红**（不是刷一层大红），4 秒后恢复；易伤期间用豌豆射手（20）打同一只 → 28。
3. **群伤**：一行 3 只僵尸挤在一起时，一发只让直射那只掉 65，旁边两只各掉 ≈21；
   相邻行的僵尸（上下各 1 行、水平方向在 100px 内）也会各掉 ≈21 并一起变红。
4. **命中不炸火**：命中时**不再**出现那团铺满屏幕的大火（原先误用了僵尸博士火球的爆燃粒子），
   只剩"呼"的一声。
5. 存档 / 读档后外观、攻击、易伤均正常；破坏性测试：删掉任一 `FirePeaShooter_*.png`
   重新打包 → 应回退成普通豌豆射手外观且不崩。
