# 斗蛐蛐 2（Cricket Fight 2）录制沙盒设计

**日期:** 2026-09-17
**状态:** 待实现
**类型:** 新 GameMode（小游戏挑战 / 录制沙盒）
**参考:** `2026-08-17-cricket-fight-design.md`（斗蛐蛐 1）、`docs/ice-sandbox.md`（冰冻沙盒背包）、坚不可摧（`GAMEMODE_CHALLENGE_LAST_STAND`）的开始战斗按钮

## 目标

新增小游戏「斗蛐蛐 2」：一个**为录制而生的自由对战沙盒**。

与斗蛐蛐 1（双方全随机、纯观战）的区别：

| 维度 | 斗蛐蛐 1 | 斗蛐蛐 2 |
|------|----------|----------|
| 战场 | 单行（第 3 行） | **完整 5 行草坪** |
| 植物 | 5 株随机自动摆放 | **自选卡自由布阵** |
| 阳光 | 无经济（`mSunMoney = 0`） | **无限阳光**（永不扣费） |
| 僵尸 | 6 只随机 | **自选种类 × 自选出怪倍率** |
| 开战 | 进关自动开战 | **坚不可摧式「开始战斗」按钮**，准备阶段不出怪 |
| 胜负 | 分胜负并自动重开 | **完全不判负**，纯沙盒 |
| 重开 | 自动重掷随机量 | 手动「结束战斗」清场回到准备阶段 |

## 核心玩法

### 阶段模型

`Board::mCricket2Prep` 表示准备阶段：

- **准备阶段（`true`）**：不出任何僵尸。玩家走正常选卡界面选卡 → 用无限阳光在 5 行草坪上自由布阵（可铲除、可重摆）。底部中央按钮 = 「开始战斗」。
- **战斗阶段（`false`）**：只有玩家点「开始战斗」才会刷出僵尸；僵尸按选定种类与倍率**一次性全部登场**。底部中央按钮 = 「结束战斗」，点击即清场（僵尸/子弹/粒子/金币）并回到准备阶段，植物保留。

录制循环：布阵 → 开始战斗 → 录 → 结束战斗 → 调整 → 再来一次。

### 出怪设置面板

顶部工具条之外的左下角按钮「出怪设置」开关一个面板（布局沿用冰冻沙盒背包的语言）：

- **僵尸卡网格**：7 列 × 3 行，收录 20 种可安全独立生成的常规僵尸（复用冰冻沙盒僵尸页的池，排除 BOSS/伴舞/潜水/海豚/雪橇/雪人/僵尸植物头等特殊生成类型）。
- **每张卡可点开关**：开 = 亮绿底 + 实心指示方块，关 = 暗灰底 + 空心方块。
- **出怪倍率**：`[-10] [-1] ×N [+1] [+10]`，范围 1–50。语义 = **每种已选僵尸各出 N 只**。
- **汇总行**：`已选 3 种 × 5 只 = 15 只（上限 300）`。
- **快捷按钮**：全选 / 全不选 / 关闭。

面板在准备阶段与战斗阶段都可打开；**面板里的改动只影响下一次「开始战斗」**，战斗中不即时生效。

### 一次性全出

点「开始战斗」时：

1. 按**轮转**（round-robin）方式枚举所有已选种类，每种各取 `倍率` 个，保证每种至少出场一次；
2. 总数截断在 `MAX_ZOMBIES_IN_WAVE`（300）；
3. 逐个 `Board::AddZombie(type, Zombie::ZOMBIE_WAVE_DEBUG)` —— 同一帧全部生成（不经过 `mZombiesInWave` / `SpawnZombieWave` 的波次系统）。

`ZOMBIE_WAVE_DEBUG` 同时关掉「路障 20% → 豌豆头」「铁桶 20% → 坚果头」「气球 20% → 豌豆头气球」「玩偶匣 10% → 毁灭菇头」等随机转化，保证**录出来的就是你选的**。

行位由 `Board::AddZombie` → `PickRowForNewZombie` 在 5 行内随机分配。

若一种僵尸都没选，点「开始战斗」只提示 `[ADVICE]` 风格的悬浮字幕，不消耗准备阶段。

### 纯沙盒（不判负）

- **无小推车**（`InitLawnMowers` 早退），僵尸走到左侧边界**安静退场**（同冰冻沙盒 / 我是僵尸的处理），不触发 `ZombiesWon`。
- **永不通关**：`Zombie::TrySpawnLevelAward` 对本模式直接 `return false`，僵尸全灭后不会有奖杯币、不会 `FadeOutLevel`、不会弹结算。
- **无波次**：`Challenge::UpdateZombieSpawning` 对本模式恒 `return true`（完全接管出怪），`mNumWaves = 1` 且波内列表清空；进度条 `HasProgressMeter` 对本模式返回 `false`。
- **无自然阳光/无向日葵产阳光**：`AddCoin` 丢弃 `COIN_SUN`、`UpdateSunSpawning` 早退 —— 无限阳光下阳光币只是录制噪音。金币/银币仍可掉（不影响）。

### 无限阳光

- `InitLevel` 起始 `mSunMoney = 9990`（显示上限）。
- `Board::TakeSunMoney` 在本模式下**直接返回 true 且不扣费** —— 这是阳光的真正消费入口（`AddSunMoney` 只负责加阳光，从没人用负数调它）。

### 选卡

- 走标准选卡界面（`ChooseSeedsOnCurrentLevel()` 为真，本模式**不**加入 `IsChallengeWithoutSeedBank`）。
- `LawnApp::HasSeedType` 对本模式的**全部常规植物（`SEED_PEASHOOTER`…`SEED_IMITATER`）返回真**，无需商店解锁 —— 录制沙盒不受存档进度限制。
- 旅行专属卡（`SEED_GIANT_WALLNUT` 起至 `SEED_ELECTRIC_STARFRUIT`）**不**在本关开放（需要专属底座/升级前提，放进自由布阵只会得到放不下去的废卡）。
- 卡槽数 `GetNumSeedsInBank()` 对本模式返回 10（满卡槽）。

## 技术结构

| 文件 | 改动 |
|------|------|
| `src/ConstEnums.h` | 新增 `GAMEMODE_CHALLENGE_CRICKET_2`，插在 `GAMEMODE_CHALLENGE_TRAVEL_JOURNEY` 之后、`GAMEMODE_UPSELL` 之前（遵守"不重排既有值"，新值一律排在 `NUM_GAME_MODES` 哨兵前） |
| `src/LawnApp.h` / `.cpp` | `IsCricketFight2Level()`；`HasSeedType` 全常规植物放行 |
| `src/Lawn/Widget/ChallengeScreen.cpp` | `gChallengeDefs` 新增挑战页 (row 5, col 0) 入口，名称为字面量 `"斗蛐蛐2"`（挑战页 5 行已满，第 6 行靠既有 `mScrollY` 滚动进入视野）；`MoreTrophiesNeeded` 对本模式恒返回 0 |
| `src/Lawn/Board.h` / `.cpp` | 见下 |
| `src/Lawn/Challenge.h` / `.cpp` | `CricketFight2Update()`（开始战斗按钮的显示与文案）；`UpdateZombieSpawning()` 恒真 |
| `src/Lawn/Zombie.cpp` | `CheckForBoardEdge` 安静退场分支；`TrySpawnLevelAward` 早期 `return false` |

### Board 改动细节

**状态成员**

```cpp
bool mCricket2Prep;                                     // true = 准备阶段（不出怪）
bool mCricket2ZombieEnabled[ZombieType::NUM_ZOMBIE_TYPES];
int  mCricket2ZombieMultiplier;                         // 1..50，每种已选僵尸各出几只
bool mCricket2PanelOpen;
int  mCricket2ButtonShownPrep;                           // -1 = 未初始化，给按钮文案用
```

**方法**

```cpp
/*inline*/ bool IsCricket2Level();
void CricketFight2StartBattle();   // 一次性全出
void CricketFight2EndBattle();     // 清场 → 回准备阶段
void CricketFight2UpdateButtons(); // 每帧刷新按钮文案（由 Challenge 调用）
void CricketFight2OpenPanel();
void CricketFight2DrawPanel(Graphics* g);
bool CricketFight2PanelMouseDown(int x, int y, int theClickCount);
bool CricketFight2HandleMouseDown(int x, int y, int theClickCount);
Rect GetCricket2PanelButtonRect();
```

**接入点**

1. `PickBackground`：`BACKGROUND_2_NIGHT`（沿用斗蛐蛐 1：夜战，蘑菇全部清醒，最贴近"全植物都能用"的沙盒诉求）
2. `PickZombieWaves`：`mNumWaves = 1`，并把 `mZombiesInWave[0][0]` 置 `ZOMBIE_INVALID`（本模式不用波次列表）
3. `InitLevel`：`mSunMoney = 9990`；初始化面板状态（默认只勾选普通僵尸、倍率 1）
4. `InitLawnMowers`：早退（无小推车）
5. `GetNumSeedsInBank`：返回 10
6. `HasProgressMeter`：返回 false
7. `TakeSunMoney`：直接返回 true，不扣费
8. `AddCoin`：丢弃 `COIN_SUN`
9. `UpdateSunSpawning`：早退
10. `StageHasGraveStones`：返回 false（夜战草坪默认会长墓碑，录制沙盒不要挡位）
11. `Board::Draw`：调用 `DrawCricket2UI`（左下按钮 + 底部开始/结束按钮 + 汇总行 + 面板）
12. `Board::MouseDown`：先交给 `CricketFight2HandleMouseDown`（面板与按钮优先消费点击，抢在种植之前）
13. `Board::KeyDown`：`Esc` 关面板

**按钮实现方式（与坚不可摧的关系）**

「开始战斗 / 结束战斗」不挂在 `Board::mStoreButton` 上，而是自己用**同一套 `DrawStoneButton()`** 画在坚不可摧那颗按钮的同位 `Rect(300, 546, 210, 46)`，点击由 `Board::MouseDown` 自己接管。原因：

- 坚不可摧那颗按钮靠 `MouseUp` 里的 `mStoreButton->IsMouseOver()` 触发，而 `MouseUp` 与种植不在同一条路径上 —— 手里拿着卡牌时点它会**既种下植物又开战**；
- 自己接管后点击在 `MouseDown` 就被消费，按钮区域永远不会误种植物，也能在"手里拿着卡"时直接点开始（`CanInteractWithBoardButtons()` 在手持卡牌时为假，所以不能直接复用它做门控）。

文案取 pak 的 `[START_ONSLAUGHT]`（与坚不可摧同一颗按钮文案，英文包自动跟随），缺失时回退字面量「开始战斗」。

### 按钮位置

- 「出怪设置」：`Rect(24, 546, 170, 44)`（左下）。**不能放顶部工具条**：本模式卡槽 10 格，卡槽背板 + 铲子已占满 `x∈[0,679]`，冰冻沙盒那个 `Rect(556,4,112,40)` 会压在最后两张卡上。
- 「开始战斗 / 结束战斗」：`Rect(300, 546, 210, 46)`（沿用坚不可摧的 `Resize(300, 559, 210, 46)` 同位）。
- 汇总行：底部居中 `y = 508 / 526`。

## 边界情况

- 一种僵尸都没选 → 提示字幕，不进入战斗阶段
- 选中总数超过 300 → 截断到 300，汇总行显示实际上限
- 战斗中点「结束战斗」→ 立即清场回准备阶段，植物/阳光/卡槽冷却保留
- 气球僵尸无仙人掌时无敌 → 仍在最左侧安静退场（本模式不判负），符合沙盒语义
- 蒜/挖掘等把僵尸赶到其它行 → 只是绕路，不影响任何判定
- 面板打开时不暂停游戏（纯 overlay），录制中可随时开关

## 不做的事（YAGNI）

- 不做战术/存档/排行榜：本关是录制工具，不写 `mChallengeRecords`、不给奖杯
- 不做按波次的分批出怪（用户明确选择"一次性全部刷出"）
- 不做旅行专属植物（需要底座/升级前提）
- 不做新的美术资源（复用现有僵尸卡框、石头按钮与黑夜草坪）
- 不做自动重开（清场由「结束战斗」手动触发）

## 验证

无自动化测试（项目无测试套件）。验证方式：

- 构建通过（`cmake --build build`，Release）
- 从小游戏页第 6 行（滚轮往下）进入「斗蛐蛐 2」→ 出现夜间 5 行草坪、无小推车、无进度条
- 选卡界面：所有常规植物可选（无需满存档），最多 10 张；旅行红卡/紫卡不出现
- 准备阶段：种任何植物都不掉阳光（恒 9990）；不点开始战斗时**永远不出僵尸**
- 左下「出怪设置」：勾选普通 + 铁桶、倍率 5 → 汇总显示 `已选 2 种 × 5 只 = 10 只`
- 点「开始战斗」：10 只僵尸同一瞬间从右侧登场、随机分布在 5 行；按钮变「结束战斗」
- 僵尸走到最左侧：安静消失，**不判负、不弹结算、不掉奖杯**
- 点「结束战斗」：场上僵尸清空，回到准备阶段，植物与已选设置保留
- 非本模式（如冒险、斗蛐蛐 1）：以上判定全部不受影响（回归）
