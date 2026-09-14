# 设计：旅行模式（11 轮完整版）— Travel Journey

日期：2026-09-13
状态：已确认

> 对应用户请求：加入新模式「旅行」。该模式共 11 轮；第 11 轮第一大波与第二大波各固定刷新一支 BOSS 路障射手僵尸；
> 出怪参考无尽模式；前 5 轮地图为泳池、第 6-10 轮为迷雾、第 11 轮回到泳池；前一轮的植物保留到后一轮；每过一轮出怪量翻倍。

## 背景

`src/Lawn/Travel.h/.cpp` 已经是一套数据驱动的旅行模式框架（`TravelLevelDef` / `TravelPlantDef` 表 + 查询 API），
当前表内只有两条**体验关**（`GAMEMODE_CHALLENGE_TRAVEL_1/2`，传送带小关）。本次落地该框架预留的
「完整旅行模式」：一条 **11 轮**的连续推进路线。

## 需求拆解

| 需求 | 落点 |
|---|---|
| 新模式「旅行」，11 轮 | 新 `GameMode::GAMEMODE_CHALLENGE_TRAVEL_JOURNEY`；轮次存在 `mChallenge->mSurvivalStage`（0..10 → 第 1..11 轮） |
| 第 11 轮第一/第二大波各固定刷新 1 支 BOSS 路障射手僵尸 | `Board::PickZombieWaves` 固定出怪（绕过点数与出怪池） |
| 出怪参考无尽模式 | 出怪池随轮次扩张（无尽模式式）+ 出怪点数走无尽模式的波内增长曲线 |
| 前 5 轮泳池 / 第 6-10 轮迷雾 / 第 11 轮泳池 | `PickBackground()` 按轮次取 `BACKGROUND_3_POOL`（白天泳池）/ `BACKGROUND_4_FOG`（夜晚迷雾泳池） |
| 前一轮植物保留到后一轮 | 复用生存模式的**换关保留**（repick）机制：`IsSurvivalStageWithRepick()` 纳入本模式 |
| 每过一轮出怪量翻倍 | 出怪点数 × `2^(轮次-1)` |

## 现状调研（复用点）

- **生存模式的换关保留**：`Board::IsSurvivalStageWithRepick()` 为真时，关卡结束不结算，
  `FadeOutLevel()` → `mNextSurvivalStageCounter = 500` → `mLevelComplete = true` → `LawnApp::CheckForGameEnd()`
  → `mChallenge->mSurvivalStage++` + `Board::InitSurvivalStage()`（重掷出怪、回选卡界面、过场、**植物/阳光/小推车全部保留**）。
  这正是「前一轮的植物保留到后一轮」。
- **`CutScene::IsSurvivalRepick()`**：换关过场里避免重复铺小推车/花盆/耙子。轮次 > 1 必须为真，否则小推车会翻倍。
- **`Challenge::InitZombieWavesSurvival()`**：无尽模式的出怪池随 `mSurvivalStage` 扩张。
- **泳池与迷雾的地形完全一致**（`BACKGROUND_3_POOL` 与 `BACKGROUND_4_FOG` 都是 `NORMAL/NORMAL/POOL/POOL/NORMAL/NORMAL` 六行），
  所以换轮只换背景图 + 迷雾 + 昼夜 + 音乐，**不需要重建 Board**，保留的植物不会掉落。

### 顺带修复

`Challenge::InitZombieWavesSurvival()` 的随机出怪池未排除 `ZOMBIE_BOSS_CONHEAD_PEA`（BOSS 的
`mPickWeight = 3000`），无尽模式第 2 阶段起有概率把它随机刷成常规僵尸。本次一并排除（旅行模式必须排除，否则
「固定出怪」会变成随机出怪）。

## 设计

### 一、数据模型（`Travel.h` / `Travel.cpp`）

```cpp
constexpr const int TRAVEL_JOURNEY_ROUNDS           = 11;   // 总轮数
constexpr const int TRAVEL_JOURNEY_WAVES_PER_ROUND  = 10;   // 每轮波数
constexpr const int TRAVEL_JOURNEY_FLAGS_PER_ROUND  = 2;    // 每轮旗帜波（大波）数
constexpr const int TRAVEL_JOURNEY_BOSSES_PER_WAVE  = 1;    // 每个 BOSS 波固定刷出的 BOSS 数

enum TravelJourneyMap : int32_t
{
    TRAVEL_JOURNEY_MAP_POOL,   // 白天泳池
    TRAVEL_JOURNEY_MAP_FOG,    // 夜晚迷雾（泳池）
};

bool             IsTravelJourneyLevel(GameMode theGameMode);
int              TravelJourneyRound(int theSurvivalStage);      // 1..11（越界钳制）
int              TravelJourneyMapForRound(int theRound);        // 泳池/迷雾
int              TravelJourneySpawnMultiplier(int theRound);    // 2^(轮次-1)
bool             TravelJourneyIsBossRound(int theRound);        // 第 11 轮
bool             TravelJourneyIsBossFlagWave(int theWaveIndex, int theWavesPerFlag, int theNumWaves);
```

`gTravelLevelDefs` 追加本模式一行：`{ 3, GAMEMODE_CHALLENGE_TRAVEL_JOURNEY, 2, 10, true, true, false }`
—— 自由选卡、选卡器可翻页（旅行专属植物可用）、非传送带。于是 `IsTravelLevel()` / `HasConveyorBeltSeedBank()` /
`GetNumWavesPerFlag()` / `SeedChooserScreen` 翻页 / 旅行专属植物全部自动生效。

**地图分配**：第 1-5 轮 `MAP_POOL`，第 6-10 轮 `MAP_FOG`，第 11 轮 `MAP_POOL`。

**出怪点数（参考无尽模式 + 每轮翻倍）**：基础点数走无尽模式的波内增长项
`aWave * 2 / 5 + 1`（无尽模式为 `(阶段*20 + 波)*2/5 + 1`，本模式的「阶段增长」改由显式翻倍承担），
旗帜波仍由引擎统一 ×2.5，最后整体乘 `2^(轮次-1)`。

**出怪池（参考无尽模式）**：随轮次扩张的受控解锁表（无尽模式的池也是随阶段扩张的，这里改成确定性解锁，
避免第 1 轮就随机出伽刚特尔）：

| 轮次 | 新解锁 |
|---|---|
| 1 | 普通、路障 |
| 2 | 铁桶 |
| 3 | 读报 |
| 4 | 海豚骑手 |
| 5 | 潜水、橄榄球 |
| 6 | 玩偶匣 |
| 7 | 扶梯 |
| 8 | 冰车 |
| 9 | 投石车、蹦极（旗帜波限定） |
| 10 | 伽刚特尔、舞王 |
| 11 | 红眼伽刚特尔 |

**BOSS 固定出怪**：第 11 轮的**第一大波**（旗帜波序号 0）与**第二大波**（旗帜波序号 1）各固定 `PutZombieInWave`
一支 `ZOMBIE_BOSS_CONHEAD_PEA`。本模式每轮 2 面旗帜、10 波 → 旗帜波落在波号 4 与 9（0 起）。

### 二、接线点（最小侵入，全部走 `IsTravelJourneyLevel()` 查询）

| 文件 | 改动 |
|---|---|
| `Board::PickBackground` | 本模式按轮次取背景（泳池/迷雾） |
| `Board::InitTravelJourneyRound` | 换轮时释放旧背景资源、按新轮次重取背景、重置迷雾/天降阳光计时 |
| `Board::PickZombieWaves` | 基础点数（无尽式）+ 每轮翻倍；第 11 轮两个大波固定出 BOSS |
| `Board::GetNumWavesPerSurvivalStage` | 本模式返回 10（供 `GetSurvivalFlagsCompleted` 用） |
| `Board::IsFinalSurvivalStage` | 本模式第 11 轮为最终轮（→ 不 repick，正常结算） |
| `Board::IsSurvivalStageWithRepick` | 纳入本模式（→ 植物保留） |
| `Board::UpdateZombieSpawning` | 纳入本模式（末波后正常淡出换轮） |
| `Board::FadeOutLevel` | 放宽 `mApp->IsSurvivalMode()` 断言 |
| `Board::StartLevel` | 轮次 > 1 时解除暂停/冻结（换轮后必须解冻） |
| `Board::StageHasFog` | 本模式的迷雾轮**要**开雾（体验关仍关雾） |
| `Board::CanZombieSpawnOnLevel` | 本模式出怪池分支（含 BOSS；BOSS 不参与随机） |
| `Challenge::InitZombieWaves` | 本模式走随轮次扩张的出怪池 |
| `Challenge::InitZombieWavesSurvival` | 排除 BOSS |
| `Challenge::StartLevel` | 每轮开始时提示「第 X / 11 轮 · 地图」 |
| `LawnApp::CheckForGameEnd` | 第 1-10 轮清空 → 轮次 +1 并 `InitSurvivalStage()`；第 11 轮清空 → 正常结算 |
| `LawnApp::UpdatePlayerProfileForFinishingLevel` | 只有打穿第 11 轮才计入通关记录（避免第 1 轮就发奖杯） |
| `CutScene::IsSurvivalRepick` / `IsScrolledLeftAtStart` | 纳入本模式 |
| `SeedPacket::WasPlanted` | 换轮后卡牌不冷却（同生存模式） |
| `ChallengeScreen` `gChallengeDefs` | 挑战页 (4,4) 入口；`MoreTrophiesNeeded` 返回 0（永久可用） |

### 三、文案

| 键 | 英文 | 中文 |
|---|---|---|
| `TRAVEL_JOURNEY` | Travel: Endless Journey | 旅行：无尽旅程 |
| `TRAVEL_JOURNEY_ROUND` | Travel Round {ROUND}/11 - {MAP} | 旅行第 {ROUND}/11 轮 - {MAP} |
| `TRAVEL_MAP_POOL` | Pool | 泳池 |
| `TRAVEL_MAP_FOG` | Fog | 迷雾 |

### 四、存取与兼容

- 新 `GameMode` 值插在 `GAMEMODE_CHALLENGE_TRAVEL_2` 之后、`GAMEMODE_UPSELL` 之前
  （与既有 `CRICKET`/`TRAVEL_1/2` 的插入位置同一套路），不动既有值语义；`mChallengeRecords[100]` 足够。
- 中途存档：`mSurvivalStage`、`mBackground`、`mZombiesInWave` 均在 `.v4` 存档内，
  `Board::LoadGame` 会按恢复后的 `mBackground` 重新加载背景图，因此换轮后读档可继续。
- 不新增美术资源（泳池/迷雾背景、BOSS 均为既有资源）。

### 五、已知边界

- `MAX_ZOMBIES_IN_WAVE = 300` 是引擎硬上限。翻倍到第 6 轮以后，**每波出怪列表**已经吃满 300 只，
  再翻倍只体现在点数上（与既有「巨大坚果体验关 100 倍出怪」同样的饱和行为）。
- 僵尸池 `DataArray<Zombie>` 上限 1024；波次推进受 `mZombieCountDown`（2500 帧）与「上一波血量降到
  50%~65% 才提前推进」双重约束，正常不会溢出。

## 手动测试清单

1. 挑战页出现「旅行：无尽旅程」入口；点击进入第 1 轮，背景为**白天泳池**，6 行、中间两行水池。
2. 第 1 轮清空 → 出现「更多僵尸」→ 回选卡界面，**上一轮的植物仍在原地**、小推车没有翻倍、阳光保留。
3. 第 2 轮出怪明显多于第 1 轮；第 3/4/5 轮继续翻倍。
4. 第 6 轮背景变为**夜晚迷雾**（有雾、蘑菇不需要咖啡豆也能打），音乐切到迷雾曲；第 10 轮仍为迷雾。
5. 第 11 轮背景回到**白天泳池**，第一大波与第二大波各出现一支**路障射手僵尸**（2 倍体型）。
6. 第 11 轮清空 → 正常结算，挑战页出现奖杯；第 1-10 轮清空不会提前发奖杯。
7. 换轮后读档（暂停 → 保存退出 → 继续）能回到同一轮次与同一背景。
8. 回归：两条旅行体验关行为不变；普通生存/冒险/其它小游戏不受影响；无尽模式不再随机刷出路障射手僵尸。
