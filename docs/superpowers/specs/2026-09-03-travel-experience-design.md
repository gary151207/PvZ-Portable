# 旅行模式（数据驱动子系统）+ 体验关 "Travel Experience: Puff-shroom Group!" — 设计文档

日期：2026-09-03
状态：已确认

## 背景与目标

用户希望为 PvZ-Portable 加入"旅行模式"（参考社区 mod 融合版的旅行模式，作为长期方向）。
本次不实现完整旅行模式，只落地**第一阶段**：

1. **数据驱动的旅行模式子系统**（`TravelLevelDef` / `TravelPlantDef` 表 + 查询 API），
   为将来完整旅行模式（路线推进、多关、进度存取）预留扩展点，本次表内只放 1 关 + 1 植物。
2. **一个旅行体验关**（入口在小游戏挑战页）：夜间泳池、2 面旗帜、教学友好难度、
   自由选卡、选卡器可翻页。标题：
   - 英文资源包：**Travel Experience: Puff-shroom Group!**
   - 中文资源包：**旅行体验：大喷菇群！**
3. **新植物"大喷菇群"**：一张 0 阳光、30 秒冷却的紫卡（升级卡），
   只能用在已种的大喷菇上；升级后该格 = 中间一只大喷菇 + 左右各一只小喷菇（三头挤一格）。

**非目标**：
- 不做完整旅行模式（路线图、多关推进、旅行进度存档）——只预留 `mTravelId` 字段
- 不做任何新美术素材（全部复用现有 Reanimation / Particle / 图像资源）
- 不改动既有模式行为；翻页、升级卡、旅行专属植物只作用于旅行关卡
- 不做自动化测试（仓库无测试框架），以手动试玩清单验证

## 现状调研

### 自定义模式先例：GAMEMODE_CHALLENGE_CRICKET（斗蛐蛐）

- `ConstEnums.h` `GameMode` 枚举前插新值；`gChallengeDefs`（ChallengeScreen.cpp）加行：
  `{ GameMode, level, ChallengePage, row, col, 名称 }`，名称既有 `[XXX]` 翻译键
  （如 `[BEGHOULED_TWIST]`）也有直接字符串（`"斗蛐蛐"`、`"Squirrel"`、`"Snowy Day"`）。
- Board/LawnApp 中以 `switch (mGameMode)` 分支处理模式差异——项目既定风格。

### 自定义植物先例

- `ConstEnums.h` `SeedType` 枚举在 `NUM_SEED_TYPES` 前已加过
  `SEED_EXPLODE_O_NUT`、`SEED_GIANT_WALLNUT`、`SEED_SPROUT`、`SEED_LEFTPEATER`。
- `Plant.cpp` `gPlantDefs[]` 注册，图像 `nullptr` + 复用现有 `ReanimationType`
  （如 LEFTPEATER 复用 `REANIM_REPEATER`）。
- 项目禁止对植物/僵尸做类继承，一律枚举分支（AGENTS.md）。

### 弹道先例（两侧孢子斜飞的关键依据）

豌豆散射系统（CONTEXT.md 有专章）：散射子弹**沿角度飞行、碰撞检测不锁定原行、
命中飞行路径上第一个僵尸即消失**——本设计的"非锁行斜飞单体孢子"直接复用该判定路径。

### 翻译机制（文案落点）

- 代码用 `TodStringTranslate("[KEY]")` 取值（TodStringFile.cpp）。
- `LoadingThreadProc`（LawnApp.cpp:1905 附近）加载顺序：
  `TodStringListLoad("Properties/LawnStrings.txt")`（pak 内）→
  `LoadProperties("properties/default.xml", false, false)` →
  `LoadProperties("properties/Layout.xml", false, false)`；后加载覆盖先加载。
- `PakInterface::FOpen`（paklib/PakInterface.cpp:190）：**先查 main.pak 记录，
  找不到再 fallback 到资源文件夹（`-resdir`）磁盘**。因此仓库新增一个 pak 内不存在的
  properties 文件名、启动时以 `LoadProperties(..., required=false)` 追加加载，
  即可从 `-resdir/properties/` 磁盘读入且文件缺失时静默跳过。
- properties XML 格式（PropertiesParser.cpp）：`<String id="KEY">value</String>`。
- 用户资源包为英文（AIO GOTY EN Origin）；需同时支持中文资源包文案 → 仓库同时维护
  英文默认文件与中文备用文件。

## 设计

### 一、总体架构与数据模型

新文件（`src/Lawn/`，与 Challenge 同级，不新建目录层级）：

```
src/Lawn/Travel.h     — TravelLevelDef / TravelPlantDef 结构 + 查询 API
src/Lawn/Travel.cpp   — 数据表（本版写死常量，将来可换配置文件）+ 查询实现
```

```cpp
struct TravelLevelDef
{
    int        mTravelId;        // 旅行内部关卡号（将来 1..N 沿路线推进）
    GameMode   mGameMode;        // 映射的 GameMode（用于 Board/存档）
    BoardType  mBoardType;       // BOARD_NIGHT_POOL
    int        mNumFlags;        // 旗帜波数 = 2
    int        mTotalWaves;      // 总波数 = 6
    bool       mFreeChooser;     // 自由选卡 = true
    bool       mChooserPaged;    // 选卡器翻页 = true（仅旅行关）
    // 出怪阵容（教学友好档）参数随表配置
};

struct TravelPlantDef
{
    SeedType   mSeedType;        // 旅行专属植物（本版：大喷菇群卡）
    bool       mUpgradeCard;     // 是紫卡升级卡 = true
};
```

查询 API（Board/LawnApp 只调 API、不碰表内部）：
- `bool IsTravelLevel(GameMode)` —— 全项目唯一"是否旅行关"判断
- `const TravelLevelDef& GetTravelLevelDef(GameMode)` —— 关卡配置
- `bool IsTravelOnlySeed(SeedType)` —— 是否旅行专属（决定是否进选卡器第 2 页）

GameMode 接线：`GAMEMODE_CHALLENGE_TRAVEL_1` 前插；`gChallengeDefs` 加
挑战页空位网格项。Board/LawnApp 中凡是需要旅行关参数或旅行专属行为的判断点，
一律走 `IsTravelLevel(...)` / `GetTravelLevelDef(...)` 查询取参，
不在判断点散落模式专属常量（后续完整旅行模式加关只改表）。

### 二、入口与选卡翻页

- `gChallengeDefs`：`GAMEMODE_CHALLENGE_TRAVEL_1`，挑战页空位，按钮文本
  `"[TRAVEL_EXPERIENCE]"`。
- SeedChooserScreen 新增"页"概念：**页 0 = 原版 49 卡（现状）**、**页 1 = 旅行专属**
  （本版 1 张）。页 1 槽位复用现有网格布局（`GetSeedPositionInChooser` 坐标体系）。
- 底部**翻页按钮**（左右箭头 + 页码），仅 `IsTravelLevel(mGameMode)` 可见；
  页 1 无卡时不显示（规则先立）。
- 页 1 内容由 `TravelPlantDef` 驱动：`IsTravelOnlySeed` 为真的卡上页 1。
- **选卡上限：跨页合计仍 10 张**（沿用 `mNumSeedsToChoose`）；换页不重置已选卡与倒计时。

### 三、大喷菇群卡片与升级机制

- `SeedType` 前插 `SEED_FUMESHROOM_GROUP`（卡片类型 = 场上升级形态类型，
  同双子向日葵：一个类型，拖放后替换该格 `mSeedType`）。
- `PlantDefinition`：`mSunCost = 0`、冷却 30 秒（同其它紫卡）、
  "升级目标 = SEED_FUMESHROOM"；`TravelPlantDef` 标为旅行专属 → 选卡器页 1。
- 拖拽交互：悬停已种大喷菇格子 → 高亮放行；悬停空地/其它植物 → 红禁
  （沿用现有种植拦截 `CanPlaceAt` 路径）。释放 → 替换外观、攻击计时重置、
  **血量重置为满血**。
- 被吃/被铲 → **整格消失，不还原**。
- 不被模仿者模仿、不进图鉴新条目（沿用 LEFTPEATER 等自定义植物的处理）。
- **同选规则**：选了紫卡但卡组无普通大喷菇 → 点开始被拦并提示
  （键 `TRAVEL_NEED_FUMESHROOM`）。
- 渲染：中间 `REANIM_FUMESHROOM` 正常比例 + 左右各一缩小 `REANIM_PUFFSHROOM`，
  位置"左小-中大-右小"。

### 四、三头攻击行为

三头独立攻击、独立计时（互不等待），启动相位错开避免同时抬手（视觉优化）：

| 头 | 动画 | 节奏 | 效果 | 判定 |
|---|---|---|---|---|
| 中间大喷菇 | REANIM_FUMESHROOM | 大喷菇原版 | 前方 3×3 穿透烟雾 | 本行（原版逻辑） |
| 左小喷菇 | REANIM_PUFFSHROOM(小) | 小喷菇原版 | 单体孢子 PROJECTILE_PUFF | 微上斜，非锁行 |
| 右小喷菇 | REANIM_PUFFSHROOM(小) | 小喷菇原版 | 单体孢子 PROJECTILE_PUFF | 微下斜，非锁行 |

- 两侧孢子复用豌豆散射系统的**非锁行斜飞**判定路径（弹型换 PROJECTILE_PUFF，
  带拖尾粒子/溅射）。
- **射程一致**：孢子水平飞行上限 = 大喷菇烟雾有效射程（实现时从现有大喷菇攻击代码取值），
  到点消散。
- **数值全部沿用原版**（孢子 20 伤害等），不做平衡调整。

### 五、关卡数据（体验关）

- 场地：夜间泳池 `BOARD_NIGHT_POOL`（6 行，2/3 行泳池；蘑菇可裸种，无睡眠问题）。
- 波次：**6 波**，第 1 波、第 6 波为旗帜波；首旗弱（普通+路障+旗帜僵尸，宽裕前奏），
  中间小波普通/路障为主、中期掺铁桶；末旗铁桶 + 少量潜水员/海豚。
- 阳光：标准；无墓碑/花盆等特殊地形；纯自由选卡标准局。
- 结算：按小游戏挑战标准结算；不做解锁链/进度存档（`mTravelId` 预留）。

### 六、文案、错误处理、存档兼容与测试

文案（全部走仓库翻译文件，键名带前缀防冲突）：

| 键 | 英文（默认） | 中文（备用文件） |
|---|---|---|
| TRAVEL_EXPERIENCE | Travel Experience: Puff-shroom Group! | 旅行体验：大喷菇群！ |
| TRAVEL_NEED_FUMESHROOM | You need Fume-shroom in your seed pack to upgrade it. | 你的卡组需要包含大喷菇才能升级。 |

文件与加载：
- 仓库新增 `properties/pvzp-strings.xml`（英文默认）与
  `properties/pvzp-strings.zh-CN.xml`（中文备用，注释说明中文资源包用户以之替换前者）。
- `LoadingThreadProc` 在 default.xml/Layout.xml 之后追加
  `LoadProperties("properties/pvzp-strings.xml", false, false)`。
- `package.sh` 打包时把仓库 properties 拷入 dist 资源目录；本地开发把文件复制到
  `run-pvz.sh` 的 `-resdir/properties/`（实现阶段顺手放好，保证直接可跑）。

错误处理：紫卡拖拽红禁走既有拦截；同选校验弹提示；文件缺失静默（显示
`<Missing TRAVEL_EXPERIENCE>`，不弹错，分发保证就位）。

存档/兼容：新枚举值前插于 `NUM_*` 哨兵前，不动既有值；挑战模式无中场存档；
旅行专属逻辑全部收敛在查询 API 之后，普通模式路径零改动。

**手动测试清单**
1. 挑战页出现新入口；其它页面/关卡无变化
2. 旅行关选卡器有翻页按钮，页 1 只有大喷菇群；普通关无翻页按钮
3. 只选紫卡不选大喷菇 → 开始被拦 + 提示；补选后可开始
4. 种大喷菇 → 拖紫卡升级 → 三头外观、独立节奏齐射
5. 中间烟雾穿透本行；两侧孢子单体、斜飞、射程与大喷菇一致、命中消散/到点消散
6. 铲掉或僵尸吃掉大喷菇群 → 整格消失
7. 夜间泳池 6 波/2 旗帜完整通关、结算正常、可重试
8. 回归：冒险/生存/其它小游戏选卡器、图鉴、模仿者不受影响
9. 英文与中文两套文案各自正确

## 风险与取舍

- 三头同格渲染与"各喷各的"独立计时需要给 Plant 增加子状态字段/渲染子部件，
  是本实现中改动最深的点；隔离在升级形态的枚举分支内，不影响普通植物路径。
- 两侧孢子非锁行斜飞复用散射系统判定；若命中表现需微调，收敛在
  SEED_FUMESHROOM_GROUP 分支内。
- 翻译文件运行时读取的是 `-resdir/properties/` 磁盘副本：改英文文案后需同步拷贝
  才生效（本地已放好；打包脚本自动处理）。
