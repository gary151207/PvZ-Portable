# 无尽模式多存档槽系统 — 设计文档

日期：2026-08-28
状态：已确认

## 背景与目标

当前每个游戏模式只有**一个**存档槽：`userdata/game{playerId}_{gameMode}.v4`（`LawnCommon.cpp::GetSavedGameName`）。
Board 会将整局状态（僵尸、植物、波次、`mSurvivalStage` 等）用 TLV `.v4` 分块序列化（`System/SaveGame.cpp`）。
点击无尽模式入口时（`Widget/ChallengeScreen.cpp:708` → `PreNewGame(mode, true)` → `TryLoadGame`）
会自动读取那个唯一存档并弹出 ContinueDialog（继续/新游戏）。

**目标**：给无尽模式加**多存档槽**系统，让玩家可以同时保留多局无尽模式，并在进入时选择读取哪一局，
支持给每局**命名**与**删除**。

**适用范围**（用户确认）：
- 生存无尽 5 个阶段（白天/夜晚/泳池/浓雾/屋顶）
- 我是僵尸无尽（`GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS`）
- 惊吓盒无尽（`GAMEMODE_SCARY_POTTER_ENDLESS`）

每个模式各自拥有独立的 5 个槽位池。

**非目标**：
- 不动冒险模式、普通挑战/谜题关卡的单槽存档行为
- 不做云端同步、不做槽位排序/加密
- 不解析 `.v4` 本体来显示进度（元数据走独立的 `.meta` 文件）

## 现状调研

### 存档基础设施

- 存档文件：`GetSavedGameName(GameMode, int)` → `userdata/game{pid}_{mode}.v4`（`LawnCommon.cpp:134`）
- 序列化：`LawnSaveGame`/`LawnLoadGame`（`System/SaveGame.cpp`），Board 整局状态 + `mChallenge->mSurvivalStage` 均在其中
- 自动存档触发点（无需改动）：
  - 生存模式旗帜波结束、下一阶段开始前（`Board.cpp:2022` 附近 `mNextSurvivalStageCounter == 1` → `TryToSaveGame`）
  - 暂停时（`Board.cpp:5217`，`NeedSaveGame()` 为真时）
- 存档删除：`LawnApp::KillBoard()`（`LawnApp.cpp:339`）在 `BOARDRESULT_WON/LOST/RESTART/CHEAT` 时删除单槽文件
- 入口：`ChallengeScreen::ButtonDepress`（`ChallengeScreen.cpp:708`）对所有挑战模式 `PreNewGame((GameMode)(aChallengeMode + 1), true)`

### 无尽模式清单

`ChallengeScreen.cpp:47-51, 97, 107` 的 `gChallengeDefs`：

| 模式 | 页面 | 说明 |
|---|---|---|
| SURVIVAL_ENDLESS_STAGE_1..5 | LIMBO ×4 + SURVIVAL ×1 | 生存无尽，进度 = `GetSurvivalFlagsCompleted()`（旗帜数）+ `mCurrentWave` |
| SCARY_POTTER_ENDLESS | PUZZLE | 阶段数 = `mChallenge->mSurvivalStage` |
| PUZZLE_I_ZOMBIE_ENDLESS | PUZZLE | 阶段数 = `mChallenge->mSurvivalStage` |

谜题无尽阶段推进：`Challenge::PuzzleNextStageClear()`（`Challenge.cpp:4237`）→ `mSurvivalStage++`（`Challenge.cpp:4286`），已随 Board 序列化。

### 可复用 UI 控件

- `LawnDialog` + `ListWidget` + 底部按钮布局：`UserDialog`（玩家管理）正是"列表 + 改名/删除/确认"的现成模式
- 文本输入：`NewUserDialog` 的 `EditWidget`（`CreateEditWidget`，`mMaxChars`、回车确认）

## 设计

### 1. 存储模型

每个无尽模式（7 个：生存 5 + 谜题 2）各有 **5 个槽位**，每个槽位两个文件：

| 文件 | 路径 | 内容 |
|---|---|---|
| 存档 | `userdata/game{pid}_{modeId}_s{slot}.v4` | 现有 TLV 存档，原样复用（slot = 0..4） |
| 元数据 | `userdata/game{pid}_{modeId}_s{slot}.meta` | 纯文本键值行：`name`、`flags`（已过旗帜）、`wave`（当前波）、`stage`（阶段数）、`timestamp` |

- 元数据在每次成功存档（`LawnSaveGame` 返回 true）后同步更新，记录当时进度；改名只改 `.meta`，不碰 `.v4`。
- 槽位对话框**只读 `.meta` 显示**，不解析 `.v4` 本体——快、稳、与游戏逻辑解耦。
- 当前运行中的槽位记录在 `LawnApp::mEndlessSlotId`（int，-1 = 非无尽模式/未选）。
  现有存档读写路径（`GetSavedGameName`、`TryToSaveGame`、`KillBoard` 删除）据此路由到具体槽位文件。
- 存档文件名带 `_s{slot}` 后缀；旧的单槽文件（无后缀）一次性迁移到槽位 1（见 §4）。

### 2. 槽位选择对话框 `EndlessSlotDialog`

点击无尽模式入口（ChallengeScreen）时弹出，**替代**现在的"直接读档 + ContinueDialog"流程。

对话框内部**一律使用显式槽位路径**（`GetEndlessSaveName(mode, pid, slot)`），不依赖 `mEndlessSlotId`；`mEndlessSlotId` 在槽位被确认的瞬间（`PreNewGame`/`LoadGame` 之前）才设置，之后自动存档才路由到该槽。

- **列表 5 行**，每行显示：
  - 占用槽：`"{名称} — 已过 {N} 旗 · 第 {M} 波"`（生存无尽）/ `"{名称} — 第 {N} 阶段"`（谜题无尽），可附时间戳
  - 空槽：`"空槽位"`
- **自动名** = `TodStringTranslate(挑战名)` + `" 存档{slot+1}"`（如"生存-白天 存档1"）；改名后不再自动变
- **底部按钮**（复用 UserDialog 的"列表 + 按钮"布局）：`开始/继续`、`改名`、`删除`、`取消`
  - **双击某行** = 开始/继续
  - `开始/继续`：**空槽** → 弹命名输入框（默认自动名，如"生存-白天 存档1"）→ 开新局；**占用槽** → 直接读档进入游戏（等价原"继续"）
  - `改名`：占用槽 → 弹 `EditWidget` 命名框（预填当前名，回车确认）
  - `删除`：占用槽 → 确认对话框 → 删 `.v4` + `.meta`，槽位变空
- 想"覆盖重开"一局 = 删除该槽 → 空槽 → 新建命名，流程自然且不会误覆盖。

交互总结：**选占用槽 = 继续该局；选空槽 = 开新局；删除 = 腾槽。**（无 ContinueDialog 叠层）

### 3. 流程改动

**入口**：`ChallengeScreen::ButtonDepress` 对无尽模式不再 `PreNewGame(mode, true)`，改为弹出 `EndlessSlotDialog`；非无尽模式逻辑不变。

**确定开始后**：
- 空槽 → 命名对话框 → `PreNewGame(mode, false)` + 记录 `mEndlessSlotId` → 新局
- 占用槽 → 直接 `LoadGame(槽位.v4)` 进入游戏（复用 `TryLoadGame` 路径，指向槽位文件）；加载失败则提示并留在对话框

**LawnApp 新增**：
- `int mEndlessSlotId`（-1 = 非无尽/未选）
- 辅助函数：`GetEndlessSaveName(mode, pid, slot)` / `GetEndlessMetaName(mode, pid, slot)` / `EraseEndlessSlot(mode, pid, slot)`（删 .v4 + .meta）
- `GetSavedGameName` 对无尽模式且 `mEndlessSlotId >= 0` 时返回槽位文件 → 现有自动存档（旗帜波结束、暂停、退出）**自动路由到当前槽位，无需改动触发点**

**槽位生命周期规则**（`.v4` 与 `.meta` 同生共死，RESTART 例外）：

| 事件 | 行为 |
|---|---|
| 失败 / 通关 / 作弊（LOST/WON/CHEAT） | 删 `.v4` + `.meta` → 槽位变空（`KillBoard` 现有删除分支，改为删槽位文件） |
| 暂停菜单"重启本关"（RESTART） | 只删 `.v4`，**保留名字**；同槽重开，进度清零（.meta 刷新 flags/wave/stage=0） |
| 新局开档 | 写/刷新 `.meta`：已有名字则保留，否则用自动名；进度清零 |
| 自动存档 | 写 `.v4` + 刷新 `.meta`（flags/wave/stage/timestamp） |

**元数据写入**：挂在现有 `TryToSaveGame` 成功路径之后；改名只写 `.meta`。

`SurvivalSaveScore`（写入 `mChallengeRecords` 最高旗帜记录）与槽位无关，保持不变。

### 4. 迁移、错误处理、测试

**迁移**：无尽模式旧单槽文件 `game{pid}_{mode}.v4`（无 `_s` 后缀）存在且 `_s0` 不存在时 → 迁移到槽位 1，生成默认 `.meta`，删旧文件。迁移时点：打开槽位对话框前。

**错误处理**：
- `.meta` 损坏/缺失但 `.v4` 存在 → 显示"有存档"、补默认名
- `.v4` 加载失败 → 提示并可删除该槽重建
- 文件读写失败 → 弹提示，不崩溃

**测试（手动）**：
- 3 类无尽模式各开 2-3 个槽，互相切换验证进度独立
- 暂停存档 → 退出重进；杀进程强退后重进，确认旗帜波存档生效
- 失败清槽、RESTART 保留名字重开、改名、删除（含确认）
- 旧单槽存档迁移
- 槽位 `.v4` 用 `scripts/pvzp-v4-converter.py` 验证可解析
- 非无尽模式回归：冒险、普通挑战/谜题不受影响

## 改动文件清单（预估）

- 新增：`src/Lawn/Widget/EndlessSlotDialog.h/.cpp`
- 新增：无尽元数据读写（可放 `LawnCommon` 或新建 `System/EndlessSlot.cpp`）
- 修改：`src/LawnApp.h/.cpp`（`mEndlessSlotId`、辅助函数、`KillBoard` 删除分支、`GetSavedGameName` 路由）
- 修改：`src/Lawn/Widget/ChallengeScreen.cpp`（无尽入口改为弹槽位对话框）
- 修改：`src/Lawn/Board.cpp`（`TryToSaveGame` 成功后写 `.meta`）
- 修改：`CMakeLists.txt` 或现有源文件列表加入新文件
