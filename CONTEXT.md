# PvZ-Portable 游戏核心系统

游戏核心战斗的子系统，包括僵尸生成和植物攻击行为。

## 僵尸出怪系统

控制每关每波出什么僵尸、出多少。

## 语言

**波 (Wave)**：
关卡的时间分段单位。僵尸按波次分批生成，波间有倒计时（`mZombieCountDown`），波内僵尸按 `mZombiesInWave` 列表逐一刷出。
_避免_：轮次、阶段

**旗帜波 (Flag Wave)**：
带进度条旗帜标记的波。点数预算 ×2.5，固定加入普通僵尸和旗帜僵尸，触发"大波僵尸"警告音效。旗帜波之间的小波数量取决于游戏模式。
_避免_：大波、红旗波

**僵尸点数 (Zombie Points)**：
每波可用的抽象预算值，驱动 `PickZombieType()` 随机选择僵尸类型。每种僵尸消耗其 `mZombieValue` 点数（普通僵尸=1，伽刚特尔=7）。点数用完则该波列表填满。
_避免_：出怪额度、僵尸配额

**僵尸价值 (Zombie Value)**：
单只僵尸消耗的点数。定义在 `ZombieDefinition::mZombieValue`。高价值僵尸比低价值僵尸更"贵"，占用更多点数预算。
_避免_：僵尸费用、僵尸成本

**出怪列表 (Wave Spawn List)**：
`mZombiesInWave[波号][位置]`，每波预先生成的僵尸类型序列。`PickZombieWaves()` 一次性填充全场所有波，生成后不变。
_避免_：僵尸阵容、刷怪表

**固定出怪 (Fixed Spawn)**：
绕过点数随机系统、直接塞入列表的僵尸。包括引导僵尸、旗帜波固定僵尸、关卡特殊僵尸（如 5-10 最终波的伽刚特尔、柱子关卡的梯子/玩偶匣）。固定出怪不受点数预算约束。
_避免_：必出僵尸、强制出怪

**僵尸倍率 (Zombie Multiplier)**：
作用于点数预算的 CLI 可配置乘数（`--zombie-multiplier=N`）。在模式乘数之后、固定出怪之后、随机填充之前施加。默认 6（仅放大随机填充部分，不影响固定出怪）。
_避免_：六倍模式、出怪倍率

## 关系

- 一关有多个**波**，其中若干为**旗帜波**
- 每个**波**有一个**出怪列表**，由**固定出怪**和随机填充组成
- 随机填充由**僵尸点数**预算驱动，选择**僵尸价值**不超过剩余点数的类型
- **僵尸倍率**放大随机填充的**僵尸点数**预算，间接增加僵尸数量
- 关卡自身模式乘数（列 ×6、小麻烦 ×4 等）与**僵尸倍率**叠加

## 示例对话

> **Dev：** "`--zombie-multiplier=6` 后，第 10 波的**旗帜波**会出现 48 只普通僵尸吗？"
> **领域专家：** "不会。旗帜波**固定出怪**（8 只普通+1 只旗帜）不受**僵尸倍率**影响。只有剩余的**僵尸点数**被放大 6 倍，用来随机填充更多僵尸。"
>
> **Dev：** "如果**僵尸点数**被放大到超过 `MAX_ZOMBIES_IN_WAVE=300` 的上限怎么办？"
> **领域专家：** "会被截断在 300 只。同时场上僵尸总数受 `DataArray<Zombie>` 池上限 1024 限制——如果多波重叠导致超过 1024 只同时在场，游戏崩溃。"

## 标记的歧义

- "出怪量" 可指**僵尸点数**预算或最终**出怪列表**长度——已解决：**僵尸倍率**操作的是**僵尸点数**，间接影响列表长度

## 豌豆散射系统

Peashooter 类植物的攻击模式修改，每次射击同时发射多颗豌豆呈扇形分布。

### 语言

**散射 (Scatter)**：
Peashooter 类植物每次攻击额外发射多颗豌豆，呈角度扇形分布。直射豌豆仍保留，散射豌豆在其两侧均匀展开。
_避免_：霰弹、扩散、分叉

**散射角度 (Spread Angle)**：
豌豆散射的扇形角度范围，以直射方向（0°）为中心对称分布。当前固定为 ±15°。
_避免_：扩散角、偏移角

**散射数量 (Scatter Count)**：
每次攻击额外发射的散射豌豆数量（不含直射那颗）。当前固定为 10，即每轮总计 11 颗。
_避免_：额外子弹数、附加豌豆

**散射子弹 (Scatter Pea)**：
扇形分布中非直射的豌豆。每颗散射子弹有独立的角度、飞行轨迹、碰撞检测，可跨行命中僵尸。
_避免_：侧向子弹、角度子弹

**适用植物 (Affected Plants)**：
散射适用于发射 `PROJECTILE_PEA`、`PROJECTILE_SNOWPEA` 或 `PROJECTILE_PUFF` 的植物：Peashooter、Snow Pea、Repeater、Gatling Pea、Split Pea、Leftpeater、Puff-shroom、Scaredy-shroom、Sea-shroom。Threepeater 不受影响。
_避免_：散射植物列表

### 关系

- 每次**散射**产生**散射数量**颗**散射子弹**，加上 1 颗直射豌豆，总计 11 颗
- **散射子弹**在 **−散射角度** 到 **+散射角度** 之间均匀分布，相邻子弹角度间隔 = 2 × 散射角度 / 散射数量 = 3°
- **散射子弹**沿角度飞行，碰撞检测不锁定原行——命中飞行路径上任何一行的第一个僵尸即消失
- **散射子弹**经过 Torchwood 时同样被点燃为火球，伤害翻倍
- **散射**是永久替换原版行为，无开关控制

### 示例对话

> **Dev：** "Repeater 一轮攻击两发，每发散射 11 颗——总共 22 颗豌豆同时在场？"
> **领域专家：** "对。Repeater 两发间隔 25 帧，第一发 11 颗飞出，25 帧后第二发 11 颗接上。Gatling Pea 更快，一轮 4 发共 44 颗。"
>
> **Dev：** "散射豌豆飞出地图上边界会怎样？"
> **领域专家：** "超出棋盘范围（`mPosX > WIDE_BOARD_WIDTH` 或 `mPosX + mWidth < 0`）即消失。向上飞出顶部同理，超出即销毁。"

### 标记的歧义

- （暂无）

## 旅行模式子系统

数据驱动的旅行模式框架：`src/Lawn/Travel.h/.cpp` 提供关卡表与旅行专属植物表及查询 API，
Board/LawnApp/SeedChooserScreen 只通过查询取参，不为每个旅行关卡堆叠模式分支。

### 语言

**旅行关卡 (Travel Level)**：
由 `TravelLevelDef` 表（`gTravelLevelDefs`）驱动的关卡；当前两条体验关：
- `GAMEMODE_CHALLENGE_TRAVEL_1`：夜间泳池、2 旗帜、6 波（大喷菇群主题，传送带）
- `GAMEMODE_CHALLENGE_TRAVEL_2`：**普通白天**、3 旗帜、6 波（巨大坚果主题，传送带；
  传送带=双发射手/1.5发射手/机枪射手/究极电能机枪射手/坚果/巨大坚果/樱桃炸弹；出怪=普通/路障/铁桶/小丑/巨人/冰车
  （**6 种僵尸任何波都可能出现**，随机不受波次限制；另第 5 波固定小丑+冰车、末波固定巨人保底；
  本关**自带 100 倍出怪**，不依赖 `--zombie-multiplier`）；
  使用传送带关音乐 `MUSIC_TUNE_CONVEYER`）
_避免_：旅关卡、旅游关

**旅行专属植物 (Travel-Only Plant)**：
只出现在旅行关卡选卡器**页 1** 的植物，由 `TravelPlantDef` 表（`gTravelPlantDefs`）登记。
普通模式不可见/不可拥有（`LawnApp::HasSeedType` 对旅行专属仅旅行关返回真）。
_避免_：旅行植物、页二植物

**翻页 (Paging)**：
旅行关选卡器的双页机制：页 0 = 原版 49 卡，页 1 = 旅行专属植物；右上角 "»/«" 按钮切换，
仅旅行关卡显示。跨页合计仍 10 张。
_避免_：分页、换页

**大喷菇群 (Puff-shroom Group)**：
旅行专属紫卡（`SEED_FUMESHROOM_GROUP`）：0 阳光、30 秒冷却（`mRefreshTime=3000`），
只能用在已种大喷菇上（`IsUpgrade` + `IsUpgradableTo`），升级后该格 = 中间大喷菇
（沿用大喷菇攻击管道：3×3 穿透烟雾、60 伤害/轮）+ 左右两只**原版大小**小喷菇，
各自独立 29 帧节奏发射**单体斜飞孢子**（`PROJECTILE_PUFF` + `MOTION_STAR`，行判定随
位置实时更新 → 可跨行命中邻行边缘），孢子飞行距离与大喷菇烟雾射程一致（约 340px 后消散）。
左小喷菇图层在大喷菇之上，右小喷菇在其之下。铲掉/被吃整格消失（不还原）。
_避免_：喷菇群、三头喷菇、Fume 群

**红卡 (Red Card)**：
卡包底色渲染为红色系的旅行高阶卡级（对标紫卡升级卡）。由 `Plant::IsRedCard` 判定；
当前四张 = **巨大坚果**（`SEED_GIANT_WALLNUT`）、**1.5 发射手**（`SEED_PEATER_1_5`）、
**究极电能机枪射手**（`SEED_ELECTRIC_GATLING_PEA`）与 **究极电能杨桃**（`SEED_ELECTRIC_STARFRUIT`）。
_避免_：红卡植物、稀有卡

**巨大坚果 (Giant Wall-nut)**：
旅行专属红卡（复用保龄球 2 的 `SEED_GIANT_WALLNUT`）：200 阳光、50 秒冷却、32000 生命、
被撑杆跳/跳跳/海豚骑手跳过时会被挡住（同高坚果拦截语义）。占**该行连续两格**，
由**双坚果底座**融合而成。附加机制：**每 5 秒回血 200**；**受到的任何伤害都全场分摊**
——啃食/碾压/小丑爆炸的单次伤害总额由场上所有巨大坚果平均承担（种得越多单只承伤越低）；
**免疫碾压/砸击与小丑爆炸秒杀**（碾压扣合计 500，车类僵尸（铲雪车/投石车）被向后击退一格；
小丑爆炸扣合计 2000）。此外**为周围植物承伤**：以巨大坚果两格为中心的 **3 行 × 4 列**区域内的
植物受到任何伤害时，伤害转由巨大坚果按全场分摊承担，**受保护植物不掉血**（范围在
`Board::FindGiantWallnutShield` 一处可调）。
_避免_：巨型坚果、大坚果

**双坚果底座 (Twin Wall-nut Base)**：
巨大坚果的种植前提——把红卡拖到已种的一颗普通坚果上，且**同一行左或右相邻格还有一颗坚果**
（均不能套南瓜、不能被蹦极抓取中）；落格后两颗坚果消耗，原地生成一只巨大坚果
（锚定两格中的左格，右格连带清空/禁种）。
_避免_：合体、坚果升级

**1.5 发射手 (1.5 Peater)**：
旅行专属红卡（`SEED_PEATER_1_5`）：150 阳光、普通短冷却（`mRefreshTime=750`）、300 生命。
**贴图 = 去掉眉毛的双发射手**（复用 `REANIM_REPEATER`，隐藏其 `PeaShooter_eyebrow` 轨道；
场上植物、卡面/图鉴/光标预览都去眉毛）。**可直接种下**（普通射手，无底座/升级前提）。
攻击特性：每轮攻击开始时掷骰，**50% 本轮一发 / 50% 本轮两发**（第二发复用双发射手的
`mLaunchCounter == 25` 补射点，见 `Plant::UpdateShooter`，平均 1.5 发/轮）。
**种下满 15 秒后可点击它免费升级为双发射手**（`Plant::MouseDown` → `Die()` + 同格 `AddPlant`，
不花阳光、不用升级卡、不走冷却；未到点点击会提示还差几秒）。
_避免_：一点五发射手、半发射手

**点击升级 (Click Upgrade)**：
1.5 发射手专属的原地升阶：种下满 `PEATER_1_5_UPGRADE_DELAY`（15 秒 = 1500 帧，常量在
`src/GameConstants.h`）后，**左键点击该植物**即免费换成双发射手。倒计时在 `Plant::Update`
里按帧递减（暂停/未开局不走），到点那一帧提示一次 `[PEATER_1_5_UPGRADE_READY]` 并让植物**闪白**
（与"手持升级卡时目标植物闪白"同一套视觉语言）。被蹦极抓取中不可升级。
_避免_：免费升级卡、自动升级

**究极电能机枪射手 (Electric Gatling Pea)**：
旅行专属**红卡 + 升级卡**（`SEED_ELECTRIC_GATLING_PEA`）：200 阳光、30.01 秒冷却（`mRefreshTime=3000`）、
300 生命。只能拖到已种的**机枪射手**上升级（`IsUpgrade` + `IsUpgradableTo`）。
**100% 发射电能豌豆** = 现有 `PROJECTILE_FIREPEA_RED`（**电能蓝**闪电豌豆：贴图过白色滤镜取剪影后按
共用的 `ELECTRIC_BLUE_R/G/B` 上色，与究极电能机枪射手同色；接触每 0.15 秒 30 点伤害、无限穿透、出屏才消失），
故不新增弹丸类型。其余机制与**机枪射手**完全一致：
每轮 4 连发、**开大（散射大招）**触发/时长/扇形/概率成长全部复用同一分支。
**贴图**：优先用玩家自备的**专用部位贴图**（`reanim/ElectricGatling_head/mouth/mouth_overlay/barrel`、
`reanim/EletricGatling_blink1/blink2`，注意 blink 两张在 pak 里拼作 `Eletric`）。
接入方式是新增 `ReanimationType::REANIM_ELECTRIC_GATLINGPEA`：与机枪射手**同一个 reanim 文件**、
**独立的定义槽**，装载后由 `ElectricGatlingHasCustomArt()` 把定义里的贴图**逐帧按原图指针**换成专用贴图
（`anim_blink` / `idle_shoot_blink` 各引用两张 blink 图，所以不能按轨道挂 `mImageOverride`）。
该类型用 **`REANIM_NO_ATLAS`**：不建自己的 Atlas，定义里始终是真实贴图指针，换图不必再抢在
Atlas 建立之前，也不与机枪射手共用同一批源贴图。专用贴图**必须自带透明通道、且尺寸与原贴图一致**
（reanim 按帧号索引贴图列），任何一张不合格就整体回退。开关见
`src/GameConstants.h` 的 `ELECTRIC_GATLING_USE_CUSTOM_ART`（当前 `true`）。
**兜底**：专用贴图缺失或不可用时，改为"机枪射手贴图 + 电能蓝染色"——只作用于 head 实例的
**白色滤镜叠加绘制**（`mExtraOverlayColor` + 共用的 `ELECTRIC_BLUE_R/G/B` 与叠加强度
`ELECTRIC_GATLING_TINT_A`，`src/GameConstants.h`）——**不能**用加色（`mExtraAdditiveColor`），
因为加色按原图像素成比例相加，绿贴图的绿色通道压不下去、永远调不出蓝/白；头盔轨道用
`mIgnoreExtraAdditiveColor` + 新增的 `mIgnoreExtraOverlayColor` 双重豁免，body 实例（叶/茎）完全不着色。
_避免_：电能机枪、雷电机枪、电豌豆射手

**究极电能杨桃 (Electric Starfruit)**：
旅行专属**红卡 + 升级卡**（`SEED_ELECTRIC_STARFRUIT`）：300 阳光、30 秒冷却（`mRefreshTime=3000`）、
300 生命。只能拖到已种的**杨桃**上升级（`IsUpgrade` + `IsUpgradableTo`）。
**发射 5 颗追踪的电能星星**（`PROJECTILE_ELECTRIC_STAR`）：米字方向与杨桃完全一致，
飞行约 1 格后复用杨桃那套"锁定最靠左僵尸并重定向速度"的追踪；**命中后星星不消失**，
而是**钉在该僵尸身上**，期间**每 0.15 秒造成 30 点电能伤害**（`ELECTRIC_STAR_HIT_DAMAGE = 30`，
节奏常量 `ELECTRIC_DAMAGE_INTERVAL_TICKS = 15` 刻，与电能豌豆同一口径：刚钉住先打一次，之后每
0.15 秒一次），僵尸走动时星星跟着走。**钉住的僵尸一死就自动改追下一个目标**
（`Projectile::RetargetElectricStar`：索敌规则同杨桃＝最靠左的可伤害僵尸；`Zombie::EffectedByDamage`
本身就排除死亡/濒死僵尸，所以一进死亡动画就立刻换目标），改追飞行期间**总寿命照常流逝**——
一颗星星从**首次命中**起总共只存活 `ELECTRIC_STAR_LINGER_TICKS = 250`（2.5 秒），时间到就消失
（想让"每个新目标各自重新算 2.5 秒"，去掉 `StartElectricStarLinger` 里 `if (mLingerCountdown <= 0)`
那层守卫即可）。其余机制与**杨桃**完全一致（攻击节奏、护盾穿透、无影子、+10 光照偏移）。
**贴图**：优先用玩家自备的**专用部位贴图**（`reanim/Electric_Starfruit_body/eyes1/eyes2`）。
接入方式是新增 `ReanimationType::REANIM_ELECTRIC_STARFRUIT`：与杨桃**同一个 reanim 文件**、
**独立的定义槽**，装载后由 `ElectricStarfruitHasCustomArt()` 与机枪射手**共用同一个**
`ApplyReanimArtSwaps()` 换图助手（逐帧按原图指针替换、必须带透明通道、尺寸必须与原图一致，
任何一张不合格就整体回退），并用 `REANIM_NO_ATLAS`。开关见
`src/GameConstants.h` 的 `ELECTRIC_STARFRUIT_USE_CUSTOM_ART`（当前 `true`）。
**兜底**：专用贴图不可用时，整株（杨桃只有一个 reanim 实例）叠加电能蓝（`mExtraOverlayColor` +
共用的 `ELECTRIC_BLUE_R/G/B` + `ELECTRIC_GATLING_TINT_A`）。
_避免_：电能杨桃、雷电杨桃

**究极形态互换 (Ultimate Switch)**：
两只究极植物可以**原地互相变身**，并**返还 `ELECTRIC_STARFRUIT_SWITCH_REFUND`（225）阳光**：

- **杨桃**卡（125）拖到 **究极电能机枪射手** → 变成 **究极电能杨桃**（净 +100 阳光）
- **机枪射手**卡（250）拖到 **究极电能杨桃** → 变成 **究极电能机枪射手**（净 −25 阳光）

实现：`Plant::IsUpgradableTo` 加两条互换规则（同时让手持卡片的目标高亮生效），
`Board::MouseDownWithPlant` 在"原植物已销毁"之后**改写要种的种子**（`aPlantSeedType`）并
`AddSunMoney(225)`；`Board::PlantingRequirementsMet` 里机枪射手放宽为"场上有双发射手**或**究极电能杨桃"，
否则反向互换的卡面会被判灰而拿不起来。
_避免_：转职、究极切换、形态转换

### 关系

- **旅行关卡** 的选卡器可**翻页**；**翻页**第 1 页放**旅行专属植物**（当前：**大喷菇群**、**巨大坚果**、
  **1.5 发射手**、**究极电能机枪射手**、**究极电能杨桃**）
- **大喷菇群** = 紫卡升级卡：拖到已种**大喷菇**格执行升级；选卡时必须同选**大喷菇**（否则开始被拦）
- **大喷菇群**三个头各喷各的：中间**大喷菇**烟雾（本行 3×3 穿透），两侧**小喷菇**孢子（单体、微斜、340px 内命中邻行边缘）
- **巨大坚果** = **红卡** = **双坚果底座**产物：只出现在旅行关（传送带/页 1）；占两格、挡跳跃、
  32000 生命；普通模式不可选/不可拥有（`HasSeedType` 旅行特判）
- **1.5 发射手** = **红卡**：只出现在旅行关（传送带/页 1/沙盒旅行页）；**可直接种下**、
  150 阳光；每轮 50% 一发 / 50% 两发（去眉毛双发射手贴图）；普通模式不可选/不可拥有（`HasSeedType` 旅行特判）
- **1.5 发射手** 种下满 15 秒后可通过**点击升级**免费变成**双发射手**（`SEED_REPEATER`）：
  升级后它不再是旅行专属、也不再掷骰，行为与普通双发射手完全一致
- **究极电能机枪射手** = **红卡 + 升级卡**：拖到已种的**机枪射手**上，花 **200 阳光**升级
  （走引擎既有升级卡路径，下层睡莲/花盆/南瓜不受影响）；升级后每发都是**电能豌豆**，
  其余（4 连发、**开大**散射大招）与**机枪射手**完全一致
- **究极电能杨桃** = **红卡 + 升级卡**：拖到已种的**杨桃**上，花 **300 阳光**升级；
  发射 5 颗**追踪**电能星星，命中后钉在僵尸身上 **2.5 秒**、每 **0.15 秒** **30 点**伤害，
  其余与**杨桃**完全一致
- **究极电能杨桃** 与 **究极电能机枪射手** 之间可通过**究极形态互换**互相变身（各返还 225 阳光）：
  **杨桃**卡 → 究极电能杨桃、**机枪射手**卡 → 究极电能机枪射手
- 翻译文案统一走 `properties/pvzp-strings.xml`（键带 `TRAVEL_` 前缀；植物名/图鉴走 `[FUMESHROOM_GROUP]`/`[GIANT_WALLNUT]`/`[PEATER_1_5]`/`[ELECTRIC_GATLING_PEA]`/`[ELECTRIC_STARFRUIT]` 标准键）
  - **必须把该文件复制到当前 `-resdir` 的 `properties/` 里**，否则所有 mod 字符串都显示成
    `<Missing [XXX]>`。`run-pvz.bat` 的 `RESDIR` 就是"当前资源目录"——它换一次，这里就要跟着装一次。
  - 加载顺序（`LawnApp::LoadingThreadProc`）：`TodStringListLoad(LawnStrings.txt)` → `LoadProperties(pvzp-strings.xml)`，
    但 `SexyAppBase::SetString` 用的是 `mStringProperties.insert()`（**不覆盖**已有键），
    所以 mod 键必须与 `LawnStrings.txt` **不重名**才生效（pak 里没有这些键，故均为新增）。
  - 只加载 `pvzp-strings.xml` 这一个名字，**没有**语言后缀机制；`pvzp-strings.zh-CN.xml` 只是译文备份。
  - `properties/pvzp-strings.xml` 是"游戏实际加载的那个"，改了它**无需重新编译**（纯数据文件）。
- 卡面观感：紫卡（`Plant::IsUpgrade`）/ 红卡（`Plant::IsRedCard`）都是**种子本身的属性**
  （保龄球 2 的巨型滚球卡同样显示红卡）；"是否旅行专属"由 `gTravelPlantDefs` + `HasSeedType` 决定

## 旅行模式（11 轮路线）

旅行模式的**完整形态**：一条 11 轮的连续路线，轮与轮之间**保留场上植物**。数据仍在
`src/Lawn/Travel.h/.cpp`，模式为 `GAMEMODE_CHALLENGE_TRAVEL_JOURNEY`。

### 语言

**旅行轮 (Travel Round)**：
11 轮路线中的一轮，**轮次存在 `Challenge::mSurvivalStage`**（0..10 ↔ 第 1..11 轮，查询走
`TravelJourneyRound()`）。每轮固定 **10 波 / 2 面旗帜**（`TRAVEL_JOURNEY_WAVES_PER_ROUND` /
`TRAVEL_JOURNEY_FLAGS_PER_ROUND`），大波落在波号 4 与 9（0 起）。
_避免_：旅行阶段、旅行关

**轮次地图 (Round Map)**：
每轮的场地，由 `TravelJourneyMapForRound()` 决定：第 **1-5 轮泳池**（`BACKGROUND_3_POOL`，白天）、
第 **6-10 轮迷雾**（`BACKGROUND_4_FOG`，夜晚 + 浓雾）、第 **11 轮回到泳池**。
两套地形完全相同（`NORMAL/NORMAL/POOL/POOL/NORMAL/NORMAL` 六行），换轮只换背景图/雾/昼夜/音乐，
所以**换轮不需要重建 Board**，已种植物原地保留。
_避免_：地图切换、场景轮换

**出怪翻倍 (Spawn Doubling)**：
每过一轮**出怪点数** ×2（`TravelJourneySpawnMultiplier()` = `2^(轮次-1)`，第 1 轮 ×1 … 第 11 轮 ×1024），
再叠加常规**僵尸倍率**。基准点数走**无尽模式**的波内增长项 `波号*2/5+1`（无尽模式本为
`(阶段*20+波)*2/5+1`，本模式的"阶段增长"由翻倍承担）。
_避免_：出怪指数、波数翻倍

**随轮解锁出怪池 (Per-Round Roster)**：
参考无尽模式"池随阶段扩张"的做法，改成**确定性解锁表**（`Challenge::InitZombieWavesTravelJourney()`）：
第 1 轮普通/路障 → 第 2 铁桶 → 第 3 读报 → 第 4 海豚 → 第 5 潜水+橄榄球 → 第 6 玩偶匣 → 第 7 扶梯 →
第 8 冰车 → 第 9 蹦极（旗帜波限定）→ 第 10 舞王+伽刚特尔 → 第 11 红眼伽刚特尔。
同时沿用无尽模式的**出怪权重曲线**与伽刚特尔/红眼每波上限。
_避免_：出怪表、僵尸池

**旅行禁出僵尸 (Travel-Forbidden Zombie)**：
**投篮车（投石车）僵尸在旅行模式中一概不出现**（`Board::IsTravelForbiddenZombie()`）：
它会隔着植物把投掷物砸到后排，而旅行的按轮解锁池没法像冒险关那样用"关卡 / 最早波次"把它关住。
三处一起生效：出怪池（`Challenge::InitZombieWavesTravelJourney()` 不置位）、
随机抽取（`Board::PickZombieType()` 跳过）、刷怪（`Board::SpawnZombieWave()` 跳过，
顺便兜住"旧存档的波内列表里已经烤进了投篮车"的情况）。
旅行体验关（TRAVEL_1/2）本来就走 `CanZombieSpawnOnLevel()` 的白名单，同样没有它。
_避免_：禁僵尸、黑名单

**换轮保留 (Round Carry-Over)**：
复用生存模式的 *repick* 流程（`Board::IsSurvivalStageWithRepick()` 纳入本模式）：

1. 本轮最后一波清空 → `FadeOutLevel()` 走"更多僵尸"分支（`mNextSurvivalStageCounter = 500`）；
2. 计数归零 → `mLevelComplete = true` → `LawnApp::CheckForGameEnd()`；
3. `mSurvivalStage++` → `Board::InitTravelJourneyRound()` 切到本轮地图 → `Board::InitSurvivalStage()`
   （重掷出怪、回选卡界面、过场）。

**植物、阳光、小推车、场上状态全部原样留到下一轮**；小推车不会重复摆放（`CutScene::IsSurvivalRepick()`）。
_避免_：继承植物、关卡继承

**BOSS 大波 (Boss Flag Wave)**：
第 **11 轮**的**第一大波**与**第二大波**各**固定出怪**一支**路障射手僵尸**
（`ZOMBIE_BOSS_CONHEAD_PEA`，见 `2026-09-10-boss-conhead-pea-design.md`）。
固定出怪绕过点数与出怪池，故 BOSS 不进 `mZombieAllowed`；`InitZombieWavesSurvival()` 同时排除该类型，
避免它在生存/旅行模式里被随机抽中。
_避免_：BOSS 波、首领波

### 关系

- **旅行轮** 共 11 轮；**轮次地图** 由轮次决定（1-5 泳池 / 6-10 迷雾 / 11 泳池）
- **出怪翻倍** 作用于**出怪点数**；因 `MAX_ZOMBIES_IN_WAVE = 300` 是引擎硬上限，
  第 6 轮以后每波**出怪列表**已吃满 300 只，继续翻倍只体现在点数上（与"巨大坚果体验关 100 倍出怪"同样的饱和行为）
- **随轮解锁出怪池** 与**出怪翻倍**共同决定难度：前者决定"出什么"，后者决定"出多少"
- **旅行禁出僵尸** 从**随轮解锁出怪池**里整体拿掉：它在**旅行轮**的解锁表上没有位置，
  也不受"关卡 / 最早波次"那套限制，所以三处（池 / 抽取 / 刷怪）一起拦
- **换轮保留** 让**旅行专属植物**（尤其需要底座的**巨大坚果**/**大蘑菇群**/**究极电能机枪射手**）
  可以跨轮复用，不必每轮重新种
- 只有打穿**第 11 轮**才计入通关记录（`LawnApp::UpdatePlayerProfileForFinishingLevel()` 特判），
  中途清空一轮不算通关；失败提示会告知坚持到第几轮（`[TRAVEL_JOURNEY_LOST]`）
- 中途存档沿用普通挑战档（`mSurvivalStage`/`mBackground`/`mZombiesInWave` 均在 `.v4` 内存档），
  换轮后读档可继续

## 关卡手套（搬植物道具）

把**已种下的植物**搬到本关**能种下它的位置**的关卡内道具，按钮画在**铲子旁边**。
与禅境花园里那套花园手套（拖盆栽、可送进独轮车）是**两套东西**，只在"买没买"和图标上共享。

### 语言

**关卡手套 (Level Glove)**：
铲子旁边的手套按钮（默认在铲子右边一格），点击后进入 `CURSOR_TYPE_GLOVE`，再点一株植物把它"拿在手上"
（`CURSOR_TYPE_PLANT_FROM_GLOVE` + `CursorObject::mGlovePlantID`），最后点目标格完成搬运。
判定入口 `Board::CanUseLevelGlove()`，落点判定 `Board::GloveCanMovePlantTo()`。
_避免_：园艺手套、花园手套

**手套热键 (Glove Hotkey)**：
**G 键**：拿手套；手套/植物已经在手上时再按一次 G 就是放下（植物留在原格，不算**搬运**）。
前置条件是 `CanUseLevelGlove()` + `IsGloveToolbarReady()` + `SCENE_PLAYING` + 戴夫没在说话；
**手里拿着卡牌（传送带关尤其常见）或铲子时，G 会先把它们放回去再拿手套**——否则玩家会觉得
"按 G 没反应"。**冷却中按 G 只会响一声**（不接手，也不放下别的工具）。
_避免_：手套快捷键、G 键手套

**工具条就绪 (Toolbar Ready)**：
`Board::IsGloveToolbarReady()` = `SCENE_PLAYING` 且（`mShowShovel` **或** `IsTravelLevel()`）。
正常关卡跟随铲子的显隐；**旅行模式下即使铲子被隐藏，手套也照常提供**——旅行体验关是传送带关、
无尽旅程是 10 卡槽，工具条挤到 `x≈679`，很容易让人以为"这关没有手套"。
_避免_：工具栏可见、按钮显隐

**可换手光标 (Swappable Cursor)**：
按 G 时允许"先放回手上东西"的光标集合（`IsGloveSwappableCursor()`）：`CURSOR_TYPE_NORMAL`、`SHOVEL`、
`PLANT_FROM_BANK`、`PLANT_FROM_USABLE_COIN`、`PLANT_FROM_DUPLICATOR`。
卡牌走 `RefreshSeedPacketFromCursor()` 归位（`SeedPacket::Activate()`，不产生冷却）；
其它特殊光标（玉米加农炮瞄准、锤子、独轮车）不动，免得打断它们自己的流程。
_避免_：手势切换、强制换手

**手套冷却 (Glove Cooldown)**：
一次**成功搬运**之后手套进入的等待期，记在 `Board::mGloveCooldown`（**厘秒**，与卡牌
`mRefreshTime` 同一时间单位）。时长由 `Board::GetGloveCooldownDuration()` 决定：
其他模式 **1000（10 秒）**，**所有旅行模式 0（无冷却）**。
冷却按游戏刻在 `UpdateGame()` 里递减（暂停/过场自然停摆），按钮变灰并显示剩余秒数。
_避免_：手套充能、手套CD

**搬运拒绝 (Move Rejected)**：
目标格不合法时**不消耗**冷却：响一声 `SOUND_BUZZER`、弹字幕 `[ADVICE_GLOVE_CANT_MOVE]`，
植物**继续拿在手上**（右键/点草地外放下）。"合法"由 `Board::GloveCanMovePlantTo()` 判定。
_避免_：搬运失败、放置失败

**可种植判定 (Plantable Destination)**：
`Board::GloveCanMovePlantTo()` 把目的地合法性**委托给 `CanPlantAt()`**，于是
"手套能搬过去的地方" = "这株植物本来就能种下去的地方"，关卡/地形限制一处生效。
只有两种"前提未满足"不算地形限制：`PLANTING_NEEDS_UPGRADE`（机枪射手/忧郁菇/香蒲这类紫卡）
与 `PLANTING_NEEDS_TWO_WALLNUTS`（巨大坚果）——搬的本来就是已经长成的成品。
_避免_：地形校验、种植检查

**载体格 (Carrier Cell)**：
目的地可以带**睡莲/花盆**或**南瓜壳**（手套只搬"植物位"，不铲别人的植物）：
把植物搬进**南瓜里**、或搬到**睡莲/花盆上**都允许；但对方若有**站着的植物/飞行植物**则**搬运拒绝**。
_避免_：底座格、容器格

**南瓜套壳 (Pumpkin Wrapping)**：
**南瓜是唯一的例外**：它本来就是套在植物外面的东西，所以**可以把南瓜搬到"已经有植物、还没套南瓜"
的格子上**（套上去就是）。不能做的只有两件：套第二层南瓜、把南瓜套到**玉米加农炮/巨大坚果**上
（后者由 `CanPlantAt` 的南瓜规则否决）。
_避免_：南瓜叠加、套南瓜

### 关系

- **关卡手套**的解锁条件就是禅境花园商店的**园艺手套**（`STORE_ITEM_GARDENING_GLOVE > 0`）——
  买一次，**任意关卡**（冒险/生存/小游戏/旅行）都能用；禅境花园与智慧树不重复提供（那里用原有花园手套）
- **手套冷却**只由**成功搬运**触发：拿起又原地放回、或**搬运拒绝**都不进冷却
- **旅行模式**（`IsTravelLevel()`：两个体验关 + 11 轮旅程）冷却恒为 0，可以连续挪植物
- 搬运只动**点中的那一株**：**睡莲/花盆/南瓜壳**都留在原格；但**睡莲/花盆**上还站着东西时不能单独搬走它（**搬运拒绝**）
- **可种植判定**决定"能搬去哪"：陆地植物进不了没睡莲的水面、睡莲上不了岸、屋顶没花盆放不下、
  弹坑/墓碑/罐子/冰面/画作格子/关卡界线外一律不行；反过来**南瓜里、睡莲/花盆上都能搬**
- **南瓜套壳**是"已有植物"的唯一开口：**南瓜可以搬到有植物、还没套南瓜的格子上**（其余植物仍然
  只能搬到植物位空着的格子，手套从不替换/铲除别人的植物）
- **载体格**只让"植物位"空着就行；**玉米加农炮**与**巨大坚果**占两格，要求右边那格连底座/南瓜都没有，
  且这两种也不能塞进南瓜
- **我是僵尸**关不提供**关卡手套**（那里的鼠标按下整体交给 `Challenge::IZombieMouseDownWithZombie`）；
  **老虎机关**没有植物可搬，也不提供
- **手套热键**只做"拿/放"这一件事，落点规则与鼠标完全共用 `Board::GloveCanMovePlantTo()`；
  它与铲子的 Shift 并列写在 `Board::KeyDown()` 里，过场/选卡界面不响应
- **工具条就绪**与**可换手光标**一起保证 G 在**旅行模式**里不至于"看着没反应"：
  旅行体验关手里永远有一张传送带卡、无尽旅程是 10 卡槽把按钮挤到暂停键边上，
  所以 G 允许**先把卡牌/铲子放回**再拿手套，且不要求铲子按钮可见
- 冷却写进 `.v4` 存档（`BOARD_FIELD_GLOVE_COOLDOWN`），读档后继续走

