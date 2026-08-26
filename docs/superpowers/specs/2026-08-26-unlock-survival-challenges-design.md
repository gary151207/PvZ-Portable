# 生存模式页"解锁全部生存关卡"按钮 — 设计文档

日期：2026-08-26
状态：已确认

## 背景与目标

生存模式（Survival）的关卡锁定是由 `ChallengeScreen::MoreTrophiesNeeded()` 根据
玩家**已通关数量**（`mChallengeRecords` + 奖杯数）动态计算的，并无独立的"锁定"标志。
玩家希望在不判定通关、不改写存档的前提下，一键解开生存模式页上所有被锁的关卡按钮，
便于快速体验各生存关卡。

**目标**：在挑战模式选择页的生存模式页（`CHALLENGE_PAGE_SURVIVAL`）左下角新增一个
"解锁生存关卡"按钮，点击后内存态解锁该页全部 11 个关卡按钮（Normal 5 + Hard 5 +
泳池无尽 1）。不写存档、不算通关、不影响成就，重启后恢复锁定。

**非目标**：
- 不做通关判定、不显示奖杯、不改 `HasBeatenChallenge`
- 不写 `mChallengeRecords` 或任何玩家存档字段
- 不处理 Limbo 页（其余 4 个无尽模式入口不在此页，不在本次范围内）
- 不做持久化

## 现状调研

### 锁定计算

- `ChallengeScreen::MoreTrophiesNeeded(int)`（ChallengeScreen.cpp:230）：按挑战模式与
  已获奖杯数计算还差几个奖杯才能解锁，返回 0 表示已解锁。
- `ChallengeScreen::AccomplishmentsNeeded(int)`（ChallengeScreen.cpp:407）：
  `return mCheatEnableChallenges ? 0 : aTrophiesNeeded;`
- 类成员 `mCheatEnableChallenges` 已存在（ChallengeScreen.h:51），默认 `false`，
  当前代码中无任何地方将其置为 `true`，可视为预留的作弊开关。但它作用于**所有页面**，
  与"只解锁生存页"的诉求不符，故不复用它，新增专用成员。

### 生存页实际显示的按钮

`gChallengeDefs` 中 `mPage == CHALLENGE_PAGE_SURVIVAL` 的条目共 11 个：

| 关卡 | 行/列 |
|---|---|
| SURVIVAL_NORMAL_STAGE_1..5 | row 0-1, col 0-4 |
| SURVIVAL_HARD_STAGE_1..5 | row 1, col 0-4（注：Hard 5 个在 row 1） |
| SURVIVAL_ENDLESS_STAGE_3（泳池无尽） | row 2, col 2 |

> 注：Normal 与 Hard 实际排布为 row 0（5 个 Normal）+ row 1（5 个 Hard），
> 泳池无尽在 row 2 col 2。其余 4 个无尽入口在 Limbo 页，不在此页。

### 解锁动画与音效

- 现有解锁流程：`SetUnlockChallengeIndex()` 触发 `UNLOCK_SHAKING` → `FOLEY_PAPER` →
  `UNLOCK_FADING` 动画。本次不复制整套动画，仅播放 `FOLEY_PAPER` 作为反馈。

## 设计

### 改动文件

仅两处：
- `src/Lawn/Widget/ChallengeScreen.h`
- `src/Lawn/Widget/ChallengeScreen.cpp`

### 1. 新增按钮

- 枚举新增 id：`ChallengeScreen_UnlockSurvival = 500`（避开现有 100/200/300 区间）。
- 新增成员：
  - `NewLawnButton* mUnlockSurvivalButton;`
  - `bool mCheatUnlockSurvival;`（默认 `false`）
- 构造函数中用 `MakeNewButton()` 创建，复用 Back 按钮同款外观：
  - 图：`Sexy::IMAGE_SEEDCHOOSER_BUTTON2` / `_GLOW`（与 `mBackButton` 一致）
  - 文字：`"解锁生存关卡"`
  - 位置：`(140, 568, 111, 26)`，紧挨 Back 按钮右侧
  - `mTextDownOffsetX/Y = 1`、`COLOR_LABEL = Color(42, 42, 90)`（照抄 Back 按钮设置）

### 2. 解锁逻辑（纯内存）

- `AccomplishmentsNeeded()` 开头追加：

```cpp
if (mCheatUnlockSurvival && GetChallengeDefinition(theChallengeIndex).mPage == CHALLENGE_PAGE_SURVIVAL)
    return 0;
```

- 仅对生存页关卡生效；其他页面（Challenge/Puzzle/Limbo）不受影响。
- 不动 `mChallengeRecords`，不触发 `HasBeatenChallenge`，不写存档。

### 3. 交互

- `ButtonDepress()` 中处理 `ChallengeScreen_UnlockSurvival`：
  - 置 `mCheatUnlockSurvival = true`
  - 按钮 `mDisabled = true`（灰显"已解锁"）
  - `mApp->PlayFoley(FOLEY_PAPER)` 播放解锁音效
  - 不离开当前页面
- `UpdateButtons()` 中：
  - 按钮仅 `mPageIndex == CHALLENGE_PAGE_SURVIVAL` 时可见
  - 若 `mCheatUnlockSurvival` 为 true，按钮置灰禁用

### 4. 生命周期

沿用 Back 按钮的现有模式：
- 构造创建按钮
- `AddedToManager()` 中 `AddWidget(mUnlockSurvivalButton)`
- `RemovedFromManager()` 中 `RemoveWidget(mUnlockSurvivalButton)`
- 析构 `delete mUnlockSurvivalButton`

## 错误处理与边界

- 重复点击：按钮已禁用，不会重复触发。
- 非生存页：按钮隐藏，逻辑分支不执行（`AccomplishmentsNeeded` 的页面判断兜底）。
- 已全部解锁的存档：点击无害，只是按钮变灰。
- 重启：`mCheatUnlockSurvival` 不持久化，自然恢复锁定状态。

## 测试

手动测试（项目无自动化测试）：
1. 新建存档 → 进入生存模式页 → 确认按钮可见，锁定的关卡仍为锁定。
2. 点击按钮 → 确认 11 个按钮全部可点（不再显示锁图标），按钮灰显，有音效。
3. 点击任一刚解锁的生存关卡 → 能正常进入关卡。
4. 切换到小游戏页 / 益智页 → 确认这些页面的锁定状态不受影响，且本按钮隐藏。
5. 退出到主菜单再进入 → 确认锁定恢复（内存态不持久化）。
6. 通关记录：确认 `mChallengeRecords` 未变、无奖杯显示、`HasBeatenChallenge` 为 false。
