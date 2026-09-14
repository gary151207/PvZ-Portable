# 计划：旅行模式（11 轮完整版）

对应设计：`docs/superpowers/specs/2026-09-13-travel-journey-design.md`

## 任务

1. **枚举**：`ConstEnums.h` 在 `GAMEMODE_CHALLENGE_TRAVEL_2` 后插入 `GAMEMODE_CHALLENGE_TRAVEL_JOURNEY`。
2. **数据表**：`Travel.h/.cpp` 新增 11 轮旅行常量、`TravelJourneyMap` 枚举与轮次查询 API；
   `gTravelLevelDefs` 追加入口（10 波 / 2 旗帜 / 自由选卡 / 可翻页 / 非传送带）。
3. **背景**：`Board::PickBackground` 本模式按轮次取泳池或迷雾；新增 `Board::InitTravelJourneyRound()`
   负责换轮时释放旧背景资源并加载新背景、重置迷雾与天降阳光计时。
4. **出怪**：`Board::PickZombieWaves` 增加本模式的基础点数（无尽式 `wave*2/5+1`）与每轮翻倍；
   第 11 轮第一大波/第二大波固定塞入 BOSS 路障射手僵尸。
5. **出怪池**：`Challenge::InitZombieWaves` 增加本模式分支（随轮次解锁的出怪池）；
   `InitZombieWavesSurvival` 排除 BOSS（顺带修无尽模式随机刷 BOSS 的旧问题）。
6. **换关保留**：`Board::IsSurvivalStageWithRepick` / `IsFinalSurvivalStage` / `GetNumWavesPerSurvivalStage` /
   `UpdateZombieSpawning` / `FadeOutLevel` / `StartLevel`、`CutScene::IsSurvivalRepick` /
   `IsScrolledLeftAtStart`、`SeedPacket::WasPlanted` 全部纳入本模式。
7. **推进与结算**：`LawnApp::CheckForGameEnd` 本模式分支（1-10 轮 → 轮次 +1 + `InitSurvivalStage()`；
   第 11 轮 → 正常结算）；`UpdatePlayerProfileForFinishingLevel` 只在第 11 轮计入通关记录。
8. **入口与文案**：`ChallengeScreen::gChallengeDefs` (4,4) 入口 + `MoreTrophiesNeeded` 永久可用；
   `properties/pvzp-strings.xml` / `.zh-CN.xml` 新增 4 个键。
9. **文档**：`CONTEXT.md` 旅行模式子系统章节补充 11 轮旅行模式词条。
10. **构建验证**：`cmake --build build`，并做代码级走查（无自动化测试框架）。

## 验收

见设计文档「手动测试清单」1-8 条。
