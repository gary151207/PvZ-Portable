# 设计：路障射手僵尸（BOSS Conehead Pea Zombie）

> 对应用户请求：加入新僵尸 `BOSS_Conehead_Pea_Zombie`（路障射手僵尸）。

## 需求要点（来自用户）

1. 枚举名沿用 `ZOMBIE_` 风格（用 `ZOMBIE_BOSS_CONHEAD_PEA`，而非 `BOSS_Conehead_Pea_Zombie`）。
2. **血量**：本体 5400 + 一类防具（路障盔）7400。
3. **体型**：贴图 = 带路障的豌豆射手僵尸，体型放大 2 倍（`mScaleZombie = 2.0f`）。
4. **巡逻**：从右侧（第 8 列）向左走到第 2 列；到第 2 列折返时随机换行，再往回走到第 8 列；到第 8 列后再折返向左，循环往复（第 2 列 ↔ 第 8 列往返）。
5. **回血**：
   - 啃咬植物 = 与路障僵尸一致的回血（`Animate` 中每次 `Heal(1)`，优先本体、本体满血回头盔）。
   - 发射一颗豌豆回血 40 点。
6. **击败植物 → 转化**：啃死一棵植物时，在该植物原地生成一只新的路障射手僵尸（BOSS）。
7. **出现方式**：不在常规出怪池自然生成；加入游戏，可在**冰冻关卡（CHALLENGE_ICE）沙盒**中放出。

## 实现方案

复用现有 `ZOMBIE_PEA_HEAD`（豌豆射手头）的行喷射逻辑与 `LoadPlainZombieReanim`（普通僵尸身体），叠加路障盔、2 倍体型、高血量、巡逻与转化。

- **身体动画**：`REANIM_ZOMBIE`（`LoadPlainZombieReanim`），隐藏 `anim_hair` / `anim_head2`，隐藏 `anim_head1` 原图后挂载 `REANIM_PEASHOOTER` 豌豆头（`anim_head_idle` / `anim_shooting`），同 `ZOMBIE_PEA_HEAD`。
- **路障盔**：`ReanimShowPrefix("anim_cone")`，`mHelmType=HELMTYPE_TRAFFIC_CONE`，`mHelmHealth=7400`。
- **BOSS 属性**：`mScaleZombie=2.0f`，`mBodyHealth=5400`，加宽 `mZombieRect` / `mZombieAttackRect`。
- **巡逻**：新增字段 `mConheadPatrolState`（0=向左朝第 2 列，1=向右朝第 8 列）。`IsWalkingBackwards()` 在向右巡逻时返回 true，从而让 `UpdateZombieWalking` 移动方向翻转并翻转身形；`UpdateZombieBossConheadPea()` 每帧检测是否到达 `GridToPixelX(2)` / `GridToPixelX(8)`，到第 2 列折返时随机换行，并限制 `mPosX` 在巡逻范围内。**换行只用当前地图有效行**：从 `0..MAX_GRID_SIZE_Y-1` 中随机挑一个 `RowCanHaveZombies()`（非 `PLANTROW_DIRT`）且与当前行不同的行，避免在只有 5 路的地图上误切到第 6 路（DIRT / 草地外）。
- **喷射 + 回血**：`UpdateZombieBossConheadPea()` 周期（`mPhaseCounter` 每次 150 帧）向本行喷射一枚 `PROJECTILE_ZOMBIE_PEA`（`MOTION_BACKWARDS`）；魅惑时改为对僵尸有效的 `PROJECTILE_PEA`。发射后回血 40（优先本体、本体满血回头盔）。
- **啃咬回血**：`Animate()` 的路障僵尸回血分支加入该类型（`Heal(1)`，优先本体再头盔）。
- **击败植物 → 转化**：`EatPlant()` 中植物血量耗尽时，`AddZombieInRow(ZOMBIE_BOSS_CONHEAD_PEA)` 在该植物格生成新 BOSS，并设置其 `mPosX/mPosY` 为该格位置。
- **BOSS 守卫**：`CanLoseBodyParts()` 排除该类型（不掉头 / 不掉身体部件）；`MowDown()` 排除（压路基无法碾死）；`ApplyBurn()` 走 BOSS 分支（只掉 3600 不烧死）。
- **免疫魅惑**：`StartMindControlled()` 对该类型直接返回（不吃魅惑菇 / 火炬召唤变身）。
- **免疫吞噬**：`UpdateChomper()` 中该类型按 Gargantuar/BOSS 处理（只咬掉 40 血，不整只吞下）；`UpdateTanglekelp()` 不会把该类型拖入水下。
- **单次受伤上限 2000**：`TakeDamage()` 对该类型单次伤害钳制到 2000（所有来源，含压扁 / 大招 / 爆炸；并处理 `DAMAGE_HITS_SHIELD_AND_BODY` 重置回原始伤害的绕过路径）。

## 变更文件

- `src/ConstEnums.h` — `ZombieType` 新增 `ZOMBIE_BOSS_CONHEAD_PEA`（置于 `NUM_ZOMBIE_TYPES` 前，不重排既有值）。
- `src/Lawn/Zombie.h` — 新增 `UpdateZombieBossConheadPea()` 声明；新增 `mConheadPatrolState` 字段。
- `src/Lawn/Zombie.cpp` — `gZombieDefs` 条目；`ZombieInitialize` 新 case（血量 5400+7400、2 倍体型、巡逻状态初始化）；重写 `UpdateZombieBossConheadPea()`（巡逻 + 喷射 + 回血 40）；`Animate()` 啃咬回血分支加入该类型；`EatPlant()` 植物→BOSS 转化；`IsWalkingBackwards()` 向右巡逻时返回 true；`StartMindControlled()` 免疫魅惑；`TakeDamage()` 单次受伤上限 2000；`CanLoseBodyParts` / `MowDown` / `ApplyBurn` 排除该类型。
- `src/Lawn/Plant.cpp` — `UpdateChomper()` 该类型按 Gargantuar/BOSS 处理（只咬不吞）；`UpdateTanglekelp()` 不抓取该类型（免疫吞噬）。
- `src/Lawn/Challenge.cpp` — `gZombieAllowedLevels` 新条目（全 0，沙盒 / 调试外不自然生成）。
- `src/Lawn/Board.cpp` — `gIceSandboxZombieTypes` 收录；调试召唤键 `P`。
- `properties/pvzp-strings.xml` / `pvzp-strings.zh-CN.xml` — `BOSS_CONHEAD_PEA_ZOMBIE` 名称（英文 "Conehead Pea Zombie"，中文 "路障射手僵尸"）。

## 访问 / 测试

- **冰冻关卡沙盒**：进入 `CHALLENGE_ICE`，打开背包 → 僵尸页 → 选中「路障射手僵尸」，点到任意格释放。
- **调试召唤**：游戏中按 `P` 直接召唤。
- 生成时走 `ZOMBIE_WAVE_DEBUG`，不会触发 20% 路障→豌豆头等转换，稳定召出本体。

## 边界

- 存档序列化：新增枚举值位于 `NUM_ZOMBIE_TYPES` 前，既有枚举值未重排，旧存档兼容；新类型暂不写入常规出怪池。
- `mConheadPatrolState` 不参与存档序列化（新增字段，默认 0）；玩家手动死亡/重载后不会丢失（重载后重新初始化）。
- 豌豆射手头动画随 `DieNoLoot()` 通过 `mSpecialHeadReanimID` 一并清除，无泄漏。
- 图鉴（Almanac）等外围展示暂未接入，后续按需补。
