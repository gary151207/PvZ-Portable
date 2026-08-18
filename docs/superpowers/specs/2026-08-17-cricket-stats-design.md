# 斗蛐蛐胜率统计设计

**日期:** 2026-08-17
**状态:** 待实现
**参考:** 无尽斗蛐蛐（`docs/superpowers/specs/2026-08-17-cricket-fight-design.md` 的胜负/重开流程）

## 目标

为无尽斗蛐蛐添加植物与僵尸的胜率统计：每场战斗结算时按类型聚合胜/负，支持跨会话持久化（文件）与游戏内查看（Tab 面板）。

## 数据模型

LawnApp 级成员（跨 Board 生命周期持久）：

| 成员 | 尺寸 |
|------|------|
| `int mCricketPlantWins[SeedType::NUM_SEED_TYPES]` | 53 |
| `int mCricketPlantLosses[SeedType::NUM_SEED_TYPES]` | 53 |
| `int mCricketZombieWins[ZombieType::NUM_ZOMBIE_TYPES]` | 33 |
| `int mCricketZombieLosses[ZombieType::NUM_ZOMBIE_TYPES]` | 33 |

- 出场数 = 胜 + 负；胜率 = 胜 / 出场数（整数百分比，向下取整）
- 构造时清零；启动时从文件加载；每次结算后写文件

## 记录时机（复用无尽制框架）

- **开战时**：
  - `Board::SetupCricketFight()` 把 5 个植物类型存入 `Board::mCricketBattlePlants[5]`
  - `Board::PickZombieWaves()` 斗蛐蛐覆盖块把 6 个僵尸类型存入 `Board::mCricketBattleZombies[6]`
- **结算时**：`Board::RecordCricketMatchResult(bool thePlantsWon)`：
  - `thePlantsWon == true`（胜，`TrySpawnLevelAward` 斗蛐蛐分支调用）→ 5 植物各 +1 胜，6 僵尸各 +1 负
  - `false`（负，`ZombiesWon` 斗蛐蛐分支调用）→ 6 僵尸各 +1 胜，5 植物各 +1 负
- 记录后立即 `LawnApp::SaveCricketStats()` 写文件

## 持久化

- 文件路径：`GetAppDataPath("userdata/cricket_stats.txt")`（与存档同目录）
- 格式：每行 `类型名 胜 负`，按类型序号顺序写入全部 53+33 项（含零值，保证全量）
  - 植物名：`GetPlantDefinition(SeedType).mPlantName`（如 `PEASHOOTER`）
  - 僵尸名：`GetZombieDefinition(ZombieType).mZombieName`（如 `ZOMBIE_NORMAL`）
- 加载：逐行解析，按名称匹配到类型；未知行跳过（容错手改）
- I/O：`std::ifstream` / `std::ofstream`（项目已在多处使用）
- 加载入口：`LawnApp::Init()` 或构造函数（`LoadCricketStats()`）

## 游戏内面板（Tab 循环切换）

- **Tab 键**（`Board::KeyDown`，仅斗蛐蛐模式）循环三态：`关闭 → 植物列表 → 僵尸列表 → 关闭`
- 列表内容：只显示出场数 > 0 的类型，按胜率降序（同胜率按类型序号）；每行 `名称 胜率% (胜/出场)`
- 标题：`植物胜率 (N 场)` / `僵尸胜率 (N 场)`，N = 当前列表记录的胜场+负场总数
- 滚动：`↑/↓` 方向键（`Board::KeyDown`）+ 鼠标滚轮（`Board::MouseWheel` 新增重载）
- 绘制：`Board::Draw` 末尾半透明面板浮层（不暂停游戏，纯 overlay）
- 状态成员：`Board::mCricketStatsPanel`（0=关, 1=植物, 2=僵尸）、`Board::mCricketStatsScroll`

## 不做的事（YAGNI）

- 不暂停游戏（纯 overlay）
- 不做图表/趋势/逐场明细
- 不做按玩家/存档隔离（全局累计）

## 验证

无自动化测试。手动验证：
- 构建通过（cmake + Ninja Release）
- 无尽斗蛐蛐打若干场（含胜负），确认：
  - 结算后 `userdata/cricket_stats.txt` 生成且数值与场次一致（胜场植物 +1 胜、僵尸 +1 负等）
  - Tab 面板在 关/植物/僵尸 三态间循环，列表只显示有出场类型、胜率降序、滚动正常
  - 重启游戏后文件数据重新载入（跨会话累计）
  - 非斗蛐蛐模式 Tab 无影响（回归）
