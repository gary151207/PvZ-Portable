# 毁灭菇头小丑僵尸 — Design

Date: 2026-08-26
Status: Approved by user (brainstorming session)

## Goal

小丑僵尸（`ZOMBIE_JACK_IN_THE_BOX`）生成时有 **10% 概率**变为"毁灭菇头小丑僵尸"
（新类型 `ZOMBIE_DOOMSHROOM_HEAD`）：头上不是音乐盒，而是一颗毁灭菇。
血量结构为 **500（小丑本体）+ 370（一类防具，毁灭菇头）**。
特性：毁灭菇头带计时，到点会**盒子式爆炸一次**（本体存活）；防具被打掉后
盒子回来，再过一段时间**开盒再爆一次**——理想情况下该僵尸能"炸两次"。

## 与用户的约定（brainstorming 已确认）

1. 概率：**10%**（`!Rand(10)`）。
2. 机制：毁灭菇头是防具（370 血、一类=路障级），先于本体承伤（标准头盔管线）。
3. "炸两次"的时序：
   - 阶段一：有毁灭菇头 → 头部自爆计时到 → **盒子式爆炸**（本体存活）→ 防具被炸掉；
   - 阶段二：盒子回来 → 开盒计时到 → 普通小丑开盒爆炸（本体死亡）。
   - 非理想情况：玩家在计时结束前打掉头（370 血）→ 头直接掉落、无爆炸 → 盒子提前回来，
     只剩一次爆炸。
4. 头部爆炸效果：**盒子式**（与开盒爆炸同款：`KillAllZombiesInRadius` 115 半径 + 
   `KillAllPlantsInRadius` 90 半径 + `PARTICLE_JACKEXPLODE` + 震屏 + 爆炸音效），本体存活。
   - 注：现行代码里小丑爆炸对**敌方僵尸**实际不生效（`EffectedByDamage(255)` 只命中魅惑僵尸），
     该行为原样保留；头部爆炸同样如此，且借助该机制自然实现"本体不被自己的爆炸伤害"。
5. 头部自爆前有 **~110 帧引信动画**（毁灭菇 `anim_explode` + 嘶嘶声），给玩家预警。
6. 磁力菇对毁灭菇头**无效**（阶段一不会被吸走盒子、不破坏状态机）。

## 实现方案（方案 1：新僵尸类型）

镜像现有植物头僵尸模式（`ZOMBIE_PEA_HEAD` / `ZOMBIE_WALLNUT_HEAD` 等）。

### 1. 枚举（ConstEnums.h）

- `ZombieType` 增加 `ZOMBIE_DOOMSHROOM_HEAD`，插在 `ZOMBIE_REDEYE_GARGANTUAR` 之后、
  `NUM_ZOMBIE_TYPES` 之前（**不重排任何现有值**，序列化安全）。
- `HelmType` 末尾追加 `HELMTYPE_DOOMSHROOM = 10`（无哨兵枚举，末尾追加安全）。

### 2. 定义表（Zombie.cpp）

- `gZombieDefs` 追加：
  `{ ZOMBIE_DOOMSHROOM_HEAD, REANIM_JACKINTHEBOX, 3, 31, 10, 1000, "DOOMSHROOM_HEAD_ZOMBIE" }`
  （与 JITB 同值；mPickWeight=1000 但允许波次表全 0，不会自然出怪）。
- `Challenge.cpp` 的 `gZombieAllowedLevels[NUM_ZOMBIE_TYPES]` 末尾追加
  `{ ZOMBIE_DOOMSHROOM_HEAD, {0} }`（对齐枚举位置，数组顺序不变）。

### 3. 生成概率（Board::AddZombieInRow）

```cpp
// 小丑僵尸有 10% 概率变为毁灭菇头小丑僵尸（调试召唤与我是僵尸玩法除外）
if (theZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX &&
    theFromWave != Zombie::ZOMBIE_WAVE_DEBUG &&
    !mApp->IsIZombieLevel() &&
    !Rand(10))
{
    theZombieType = ZombieType::ZOMBIE_DOOMSHROOM_HEAD;
}
```

### 4. 阶段一初始化（ZombieInitialize 新 case）

- `mBodyHealth = 500`（同 JITB），`mAnimTicksPerFrame = 6`，攻击矩形同 JITB。
- 隐藏盒子轨道：`ReanimShowPrefix("Zombie_jackbox_box", RENDER_GROUP_HIDDEN)`、
  `ReanimShowPrefix("Zombie_jackbox_handle", RENDER_GROUP_HIDDEN)`；
  外臂换空手图：`SetImageOverride("Zombie_jackbox_outerarm_lower", IMAGE_REANIM_ZOMBIE_JACKBOX_OUTERARM_LOWER2)`。
- 在盒子轨道位置挂载毁灭菇头：`mApp->AddReanimation(... REANIM_DOOMSHROOM)` →
  `AttachReanim(aTrackInstance->mAttachmentID, aHeadReanim, 0, 0)`（仿坚果头，轨道取
  `Zombie_jackbox_box`，偏移/缩放实现时微调），`mSpecialHeadReanimID` 记录。
- 防具：`mHelmType = HelmType::HELMTYPE_DOOMSHROOM; mHelmHealth = 370;`
  （`mHelmMaxHealth` 由现有代码自动同步，HP 倍率 CLI 同样生效）。
- 头部自爆计时：与 JITB 开盒计时公式一致：
  `aDistance = 450 + Rand(300)`，`Rand(20) == 0` 时 `/= 3`（早爆），
  `mPhaseCounter = (int)(aDistance / mVelX) * ZOMBIE_LIMP_SPEED_FACTOR`；
  `IsScaryPotterLevel()` 时 `mPhaseCounter = 10`。
- `mZombiePhase = PHASE_JACK_IN_THE_BOX_RUNNING`（`IsOnBoard()` 时）。

### 5. 阶段一状态机（扩展 UpdateZombieJackInTheBox，DOOMSHROOM_HEAD 也分发进来）

- `PHASE_JACK_IN_THE_BOX_RUNNING` 计时到（`mHasHead`）：
  - `mPhaseCounter = 110; mZombiePhase = PHASE_JACK_IN_THE_BOX_POPPING;`
  - 播 `anim_pop` 身体动画（同 JITB）；
  - 若是 DOOMSHROOM_HEAD：毁灭菇头播放 `anim_explode`（引信）+ 
    `SetShakeOverride("DoomShroom_head1/2/3", ...)` + `PlayFoley(FOLEY_REVERSE_EXPLOSION)`。
- `PHASE_JACK_IN_THE_BOX_POPPING` 计时到：
  - 盒子式爆炸（与 JITB 完全相同）：
    `KillAllZombiesInRadius(mRow, x, y, 115, 1, true, mMindControlled ? 127 : 255)` +
    `KillAllPlantsInRadius(x, y, 90)`（仅非魅惑分支）+ `PARTICLE_JACKEXPLODE` + `ShakeBoard(4,-6)` +
    `PlayFoley(FOLEY_EXPLOSION)`；
  - 若是 DOOMSHROOM_HEAD：**不** `DieNoLoot()`，改调 `RevertToJackInTheBox()`；
  - 否则（普通 JITB）：原有 `DieNoLoot()` + ScaryPotter 逻辑不变。

### 6. 防具被打掉（伤害路径，TakeHelmDamage）

- `DropHelm` 增加 `HELMTYPE_DOOMSHROOM` 分支（不生成任何粒子，直接 `mHelmType = NONE`）。
- `TakeHelmDamage` 中 `mHelmHealth == 0` 且类型为 DOOMSHROOM_HEAD 时，
  在 `DropHelm` 之后调用 `RevertToJackInTheBox()`（头被击毁 → 无爆炸 → 直接变回小丑）。

### 7. RevertToJackInTheBox()

1. 移除毁灭菇头：`mApp->RemoveReanimation(mSpecialHeadReanimID)`，ID 置 NULL。
2. 恢复盒子：`ReanimShowPrefix("Zombie_jackbox_box"/"Zombie_jackbox_handle", RENDER_GROUP_NORMAL)`，
   外臂恢复默认图像（`SetImageOverride(..., nullptr)` 或重载身体动画）。
3. 类型切换：`mZombieType = ZombieType::ZOMBIE_JACK_IN_THE_BOX;`
   （之后开盒/爆炸/音乐/磁力菇全部走现有 JITB 逻辑）。
4. 状态重置：`mZombiePhase = PHASE_JACK_IN_THE_BOX_RUNNING`，重置开盒计时
   （同一 `450+Rand(300)` 公式，`1/20` 早爆），`StartZombieSound()`。

### 8. 死亡/状态清理

- `DropHead`、`ApplyBurn`、`Die`：仿 `ZOMBIE_SQUASH_HEAD` 模式，DOOMSHROOM_HEAD
  掉头/死亡时清理 `mSpecialHeadReanimID`（防泄漏）。
- `RemoveButter`：DOOMSHROOM_HEAD 时同步恢复/暂停毁灭菇头动画速率（黄油冻结一致性）。

### 9. 磁力菇

- `Plant.cpp` 磁力菇的 JITB 分支条件加 `mZombieType != ZOMBIE_DOOMSHROOM_HEAD`：
  阶段一不被吸走盒子（不破坏状态机）。阶段二类型已切为 JITB，行为不变。

### 10. 调试召唤（可选，便于测试）

- `Board.cpp` 调试召唤列表（现有植物头僵尸条目旁）追加
  `ZombieType::ZOMBIE_DOOMSHROOM_HEAD`。

## 存档/读档

零改动。`mZombieType`（新枚举值）、`mHelmType`、`mHelmHealth`、`mPhaseCounter`、
`mSpecialHeadReanimID`、附件系统（`AttachReanim`）均已序列化；阶段一/阶段二都能正确还原。
旧存档不受影响（现有枚举值未重排）。

## 测试

无自动化测试。手动验证：

1. 冒险模式：小丑僵尸 10% 出毁灭菇头；打掉头（370 血）→ 无爆炸、盒子回来、开盒爆炸一次。
2. 理想情况：不动它 → 头引信 → 头爆炸（植物 90 半径内全灭、敌方僵尸不受影响、本体存活）→
   盒子回来 → 开盒爆炸 → 死亡。
3. "我是僵尸"模式：不出毁灭菇头小丑（排除逻辑）。
4. 调试召唤：F11 直接召出，方便复测。
5. 存档读档：阶段一/阶段二各存一次、读档验证（头还在/盒已回）。
6. 魅惑：双重爆炸都只杀敌方僵尸、不炸植物、本体存活。
7. 磁力菇：对毁灭菇头无效；变回小丑后可正常吸盒。
8. 黄油/冰冻：毁灭菇头动画同步冻结。
9. 毁灭菇头 HP 与 `--zombie-hp-multiplier` 联动。

## Out of scope

- 不给毁灭菇头做"被打爆"爆炸（只有自爆才爆；被伤害打掉 = 无爆炸，这是"理想情况才炸两次"的来源）。
- 不加入蟋蟀模式（cricket）名单、不加图鉴条目。
- 头部爆炸不生成毁灭菇弹坑（按约定用盒子式爆炸）。
