# 旅行模式专属红卡植物：1.5 发射手（1.5 Peater）— 设计文档

日期：2026-09-11
状态：已确认（用户需求：150 阳光 / 红卡 / 仅旅行模式 / 可直接种下 / 每次攻击 50% 一发、50% 两发 / 贴图 = 去掉眉毛的双发射手；
后续追加：**种下 15 秒后可手动点击免费升级为双发射手**）

## 背景与目标

旅行模式子系统已落地（`src/Lawn/Travel.h/.cpp` 数据表 + 选卡器翻页 + 传送带 + 旅行专属植物
`SEED_FUMESHROOM_GROUP` 紫卡、`SEED_GIANT_WALLNUT` 红卡先例）。本次新增第三只旅行专属植物：

1. **纯新增枚举** `SeedType::SEED_PEATER_1_5`（前插在 `NUM_SEED_TYPES` 之前、既有自定义植物之后，
   不动任何既有枚举值 → 存档稳定）。
2. **数值**：150 阳光、冷却 750（普通短冷却）、生命 300（射手通用）、普通射手攻击节奏 75 帧。
3. **红卡卡面**：`Plant::IsRedCard` 扩展为"巨大坚果 + 1.5 发射手"，卡包底色沿用红色着色渲染
   （不新增美术）。
4. **仅旅行模式**：登记进 `gTravelPlantDefs`（→ `IsTravelOnlySeed` → 选卡器页 1），
   `LawnApp::HasSeedType` 仅 `IsTravelLevel(mGameMode)` 返回真 → 普通模式不可选/不可拥有。
5. **可直接种下**：与巨大坚果（需双坚果底座）、大喷菇群（需大喷菇升级）不同，它是普通射手，
   走通用种植路径，不需要任何前提。
6. **攻击特性**：每轮攻击开始时掷骰，**50% 本轮一发 / 50% 本轮两发**（平均 1.5 发/轮 → 得名 1.5 发射手）。
7. **贴图**：复用 `REANIM_REPEATER`（双发射手）但**隐藏眉毛轨道** `PeaShooter_eyebrow`。
8. **点击升级（追加需求）**：种下满 **15 秒**后，**点击该植物**即可**免费**升级为**双发射手**
   （不消耗阳光、不需要升级卡、不占卡槽、无冷却）。未到时间时点击给出"还差几秒"的提示。

**非目标**：
- 不做新美术/新 Reanimation（眉毛是双发射手 reanim 里的独立轨道，隐藏即可）。
- 不改双发射手本体（`SEED_REPEATER`）行为，不影响机枪射手/左向双发射手/精英机制。
- 不做自动化测试（仓库无测试框架），以编译 + 源级检查 + 手动试玩验证。

## 现状调研

### 双发射手的"两发"是怎么来的（关键）

双发射手不是"一次 Fire 出两颗豌豆"，而是**两次独立的攻击判定**：

- `Plant::UpdateShooter()`（Plant.cpp）在 `mLaunchCounter <= 0` 时打出**本轮主发**
  （重置 `mLaunchCounter = mLaunchRate - Rand(15)`），并在 `mLaunchCounter == 25` 时**补射一发**
  （`SEED_REPEATER` / `SEED_LEFTPEATER` 分支）。
- 两次都走 `FindTargetAndFire()` → 头部 reanim 播 `anim_shooting`（`mShootingCounter = 26`）
  → `Plant::UpdateShooting()` 在 `mShootingCounter` 归零前调用 `Plant::Fire()` 生成豌豆。

因此"1 发还是 2 发"只需在**每轮开始时掷一次骰**，再在既有补射点按掷骰结果决定是否补射。

### 眉毛在哪里

从 `dist/pvz-portable/main.pak` 解出 `reanim/PeaShooter.reanim`（双发射手使用的 reanim，
单发豌豆射手用的是 `PeaShooterSingle.reanim`）后可见轨道 `PeaShooter_eyebrow`，
其活动帧落在 `anim_head_idle` 区间内 —— 眉毛由**头部 reanim**绘制。因此两处都要隐藏：

1. 场上植物：`Plant::PlantInitialize()` 里 body + head 两个 reanim。
2. 卡面/图鉴/光标预览：`ReanimatorCache::MakeCachedPlantFrame()` 走 `DrawReanimatorFrame()`
   （临时 reanim，故给该函数加一个 `SeedType` 参数以识别 1.5 发射手）。

`AssignRenderGroupToTrack()` 只设置轨道实例的渲染组，不会被 `StartBlend()`/`SetFramesForLayer()`
重置，故 `PlantInitialize()` 设置一次即可长期生效。

## 改动清单

| 文件 | 改动 |
| --- | --- |
| `src/ConstEnums.h` | `SEED_PEATER_1_5,` 插在 `SEED_FUMESHROOM_GROUP` 之后、`NUM_SEED_TYPES` 之前 |
| `src/GameConstants.h` | `PEATER_1_5_UPGRADE_SECONDS`(15) / `PEATER_1_5_UPGRADE_FPS`(100) / `PEATER_1_5_UPGRADE_DELAY`(1500 帧) |
| `src/Lawn/Plant.cpp` | `gPlantDefs` 新增行（`REANIM_REPEATER` / 150 阳光 / 750 / SHOOTER / 75 / `"PEATER_1_5"`）；`PlantInitialize` 加入豌豆家族头部 reanim 分支并隐藏 `PeaShooter_eyebrow`、启动升级倒计时；`Update` 递减倒计时并在到点时提示；`MouseDown` 处理点击升级；`UpdateReanimColor` 可升级时闪白；`UpdateShooter` 掷骰 + 补射；`FindTargetAndFire` 头部动画节奏（与双发射手同组）；`Fire` 弹丸类型 switch + 枪口位置（**必须加，见下**）；`FindTargetZombie` 传送门判定列表；`AttachBlinkAnim` 眨眼列表；`PlantFlowerPotHeightOffset` 高度修正；`IsRedCard` |
| `src/Lawn/Plant.h` | 新成员 `mPeater15DoubleShot` / `mPeater15UpgradeCountdown`；`IsRedCard` 注释 |
| `src/Lawn/System/ReanimationLawn.h/.cpp` | `DrawReanimatorFrame` 增加 `SeedType`（默认 `SEED_NONE`）→ 卡面同样去眉毛；`MakeCachedPlantFrame` 传入种子类型 |
| `src/Lawn/Travel.cpp` | `gTravelPlantDefs` 登记 `{ SEED_PEATER_1_5, false }`（非升级卡） |
| `src/LawnApp.cpp` | `HasSeedType` 新增 `case SEED_PEATER_1_5: return IsTravelLevel(mGameMode);` |
| `src/Lawn/Board.cpp` | 冰冻沙盒"旅行页"加入 `SEED_PEATER_1_5` |
| `src/Lawn/Challenge.cpp` | 巨大坚果体验关（TRAVEL_2）传送带加入 1.5 发射手（权重 18） |
| `properties/pvzp-strings*.xml`、`dist/pvz-portable/properties/pvzp-strings*.xml` | `[PEATER_1_5]` / `[PEATER_1_5_TOOLTIP]`（中英） |
| `scripts/check-peater-1-5.ps1` | 源级检查（对齐既有 `check-*.ps1` 惯例） |

## 行为细节

- **掷骰时机**：`mLaunchCounter <= 0`（本轮主发那一帧）`mPeater15DoubleShot = (Sexy::Rand(2) == 0)`。
  初始化时该标志为 `false`，因此**首次主发之前不会凭空补射**。
- **补射时机**：与双发射手共用 `mLaunchCounter == 25` 这一个补射点（因 `mLaunchRate = 75`、
  每轮重置为 `75 - Rand(15)`，补射落在主发后约 35~50 帧）。无目标时 `FindTargetAndFire()`
  直接返回 false，不播动画、不出豌豆（与双发射手一致）。
- **平均值**：每轮期望 1.5 发豌豆，正好是双发射手（2 发）与豌豆射手（1 发）的中点。
- **冷却/花费**：`mRefreshTime = 750` → 普通短冷却（`SeedPacket` 的 3000/5000 升级卡冷却特判不命中）；
  `mSeedCost = 150` 由 `Plant::GetCost()` 默认分支从 `gPlantDefs` 读取。
- **眼睛/眉毛**：只隐藏 `PeaShooter_eyebrow`；面部、嘴、眨眼（`anim_blink` / `idle_shoot_blink`）
  等其它轨道保持原样，因此它仍会眨眼、开枪。

## 点击升级为双发射手（种下 15 秒后，免费）

### 为什么不用"升级卡"那套

引擎自带的升级（双发射手 → 机枪射手等）需要玩家**在卡槽里持有升级卡**，走
`Board::MouseDownWithPlant`：`CanPlantAt` → 扣阳光/走冷却 → `IsUpgradableTo` 成立则
`原植物->Die()` → `AddPlant(新植物)`。本需求是**没有卡、不花阳光**的"原地变身"，
所以不复用卡槽路径，而是直接把**点击**接到 `Plant::MouseDown`：

```
Board::MouseDown (鼠标左键、命中 OBJECT_TYPE_PLANT)
  → Plant::MouseDown
      → 倒计时归零 ⇒ Die() + AddPlant(同格, SEED_REPEATER)
```

- **点击路由**：`Board::MouseDown` 只在**没有**手持植物/工具时才把点击转发给场上植物
  （`Board.cpp` 的 `OBJECT_TYPE_PLANT` 分支），所以拿着铲子/卡片点击不会触发升级。
- **只认左键**：函数首行沿用 `if (theClickCount < 0) return;`（右键为负值）。
- **替换顺序 = 引擎升级顺序**：先 `Die()` 再 `AddPlant()`（与 `MouseDownWithPlant` 中
  `aNormalPlant->Die()` 后再 `AddPlant` 完全一致）。同格的下层植物（睡莲/花盆/南瓜）不受影响。
- **不碰阳光与卡槽**：全程不访问 `mSeedBank`/`GetCost`，因此天然免费且不触发冷却。
- **守卫**：已死/被压扁/血量为 0 不响应；**被蹦极僵尸抓着时不响应**（与 `CanPlantAt` 对
  升级卡的限制一致，避免"偷走已升级植物"）。

### 反馈（让玩家知道能点了）

| 时机 | 反馈 | 实现 |
| --- | --- | --- |
| 倒计时归零 | 底部提示 `[PEATER_1_5_UPGRADE_READY]` + 植物开始**闪白** | `Plant::Update`（只在归零那一帧提示一次）；`Plant::UpdateReanimColor` 用 `GetFlashingColor(mMainCounter, 90)`，与"手持升级卡时目标植物闪白"是同一套视觉语言 |
| 提前点击 | 提示 `[PEATER_1_5_UPGRADE_WAIT]`（含 `{TIME}` 剩余秒数，向上取整） | `Plant::MouseDown` + `TodReplaceNumberString` |
| 升级成功 | 与普通种植同样的种下音效/尘土粒子 | `AddPlant` → `DoPlantingEffects` |

倒计时在 `Plant::Update` 的 `doUpdate` 分支里按帧递减（与巨大坚果回血同一位置），
暂停/未开局时不走，所以 15 秒是**实际战斗时间**。常量集中在 `src/GameConstants.h`。

> 与既有 mod 计时器一致，`mPeater15UpgradeCountdown` **不写进存档**
> （`SyncPlantTailPortable` 同样不存 `mGiantRegenCountdown` / `mGatlingScatterCountdown`），
> 因此**读档后该植物立即可升级**（等价于读档时已过了 15 秒）。如需保留剩余时间，
> 需同时扩 TLV 字段与 `scripts/pvpz-v4-converter.py`。

## 踩过的坑：新射手必须同时接进 `Plant::Fire()`

`Plant::Fire()` 里是 `ProjectileType aProjectileType;`（**未初始化**）后 `switch (mSeedType)`：

```cpp
ProjectileType aProjectileType;          // 未初始化！
switch (mSeedType)
{
case SeedType::SEED_REPEATER: ... aProjectileType = ProjectileType::PROJECTILE_PEA; break;
...
default: TOD_ASSERT(false); break;       // 非 Debug 下是 no-op，变量保持未初始化
}
```

新种子若不加进这个 switch，就会带着**未初始化的弹丸类型**去 `AddProjectile()`：
豌豆查不到贴图 → **子弹完全看不见**（本特性首版即踩此坑，用户实测"没有子弹的贴图"）。
同一函数下面还有一段按种子选**枪口坐标**的 `else if`，漏掉会退化成兜底的 `(mX + 10, mY + 5)`。

**结论：新增任何发射类植物，必须同时检查 `Plant::Fire()` 的这两处**
（`scripts/check-peater-1-5.ps1` 已对 1.5 发射手加断言防回归）。
另外，若新植物的 reanim 是双发射手这一套（`PeaShooter.reanim`），头部
`anim_shooting` 的节奏也应与双发射手一致（`mShootingCounter = 26` / `mAnimRate = 45.0f`，
见 `FindTargetAndFire`），否则出膛帧与张口动画错位。

## 手动验证清单（无自动化测试框架）

1. 编译通过：`cmake --build build`。
2. 源级检查：`pwsh -File scripts/check-peater-1-5.ps1`（本机为 Windows PowerShell 时用 `&`）。
3. 进入旅行模式可获得它的入口：
   - 冰冻关卡沙盒 → 旅行页：应出现 1.5 发射手卡，且卡面为红卡、无眉毛。
   - 巨大坚果体验关（TRAVEL_2）传送带：应能刷出该卡。
4. 种植：普通草坪格可直接种下（无需底座/升级），花费 150 阳光、冷却为短冷却。
5. 战斗：连续观察多轮攻击，应有约一半轮次只出一发、一半轮次出两发（两发之间约 0.4~0.5 秒）；
   **豌豆必须能看见**（打出来是标准绿色豌豆贴图），且从嘴部飞出、不是从格子左下角飞出。
6. 观感：种下的植物与卡面都**没有眉毛**，其余与双发射手一致（两张大嘴、会眨眼）。
7. **点击升级**：
   - 刚种下就点 → 提示还差几秒，植物**不变**；
   - 种下满 15 秒 → 植物开始闪白并提示已就绪；**点击一次**该格变成双发射手（无眉毛消失、
     变成两发齐射的标准双发射手），**阳光数不变**、卡槽/冷却均无变化；
   - 睡莲/花盆/南瓜等下层植物在升级后仍在；铲掉升级后的双发射手行为正常。
8. 普通模式（冒险/生存/小游戏）：选卡器页 0 与图鉴中都不出现该植物，`HasSeedType` 为假。
8. 存档回归：进出一次关卡存档（`.v4`）读回正常（本次只前插枚举，未改动既有值）。
