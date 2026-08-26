# 毁灭菇头小丑僵尸 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 小丑僵尸 10% 概率变为毁灭菇头小丑僵尸（`ZOMBIE_DOOMSHROOM_HEAD`）：500 本体 + 370 一类防具（毁灭菇头）；毁灭菇头计时到会盒子式爆炸一次（本体存活），防具被打掉后盒子回来、开盒再爆一次。

**Architecture:** 新增僵尸类型 `ZOMBIE_DOOMSHROOM_HEAD`（镜像现有植物头僵尸模式）。阶段一复用 JITB 身体动画 + 挂载 `REANIM_DOOMSHROOM` 头 + 头盔管线（`HELMTYPE_DOOMSHROOM`，370 血）；头被击毁或自爆后 `RevertToJackInTheBox()` 把 `mZombieType` 切回 `ZOMBIE_JACK_IN_THE_BOX`，后续开盒/爆炸/音乐全部走现有逻辑。

**Tech Stack:** C++20, CMake/Ninja, SDL2 项目（PvZ-Portable，SexyAppFramework 风格，`mMember`/`theParam` 命名）。

**Global Constraints**

- 不重排 `ConstEnums.h` 现有枚举值（存档序列化）；新值加在哨兵 `NUM_ZOMBIE_TYPES` 前 / `HelmType` 末尾。
- 不新建植物/僵尸子类，只用枚举分支（AGENTS.md 规则）。
- 头僵尸模式参照 `ZOMBIE_PEA_HEAD`/`ZOMBIE_WALLNUT_HEAD` 的既有写法。
- 每任务结束必须 `cmake --build build` 编译通过后才提交；无自动化测试，验证以手动游戏为准。
- 小丑爆炸对敌方僵尸不生效的现有行为（`EffectedByDamage(255)` 只命中魅惑僵尸）原样保留。

---

### Task 1: 枚举与定义表

**Files:**
- Modify: `src/ConstEnums.h` — `ZombieType` 枚举（`ZOMBIE_REDEYE_GARGANTUAR` 之后、`NUM_ZOMBIE_TYPES` 之前）；`HelmType` 枚举末尾
- Modify: `src/Lawn/Zombie.cpp:60-78` — `gZombieDefs` 末尾追加一行
- Modify: `src/Lawn/Challenge.cpp:281` — `gZombieAllowedLevels` 末尾（`ZOMBIE_REDEYE_GARGANTUAR` 条目后）追加一行

**Interfaces:**
- Produces: `ZombieType::ZOMBIE_DOOMSHROOM_HEAD`（Task 2/3/4 使用）、`HelmType::HELMTYPE_DOOMSHROOM`（Task 3/5 使用）

- [ ] **Step 1: ConstEnums.h — ZombieType 新增值**

在 `src/ConstEnums.h` 的 `ZOMBIE_REDEYE_GARGANTUAR,` 与 `NUM_ZOMBIE_TYPES,` 之间插入：

```cpp
    ZOMBIE_REDEYE_GARGANTUAR,
    ZOMBIE_DOOMSHROOM_HEAD,              // 毁灭菇头小丑僵尸（小丑僵尸 10% 概率转化）
    NUM_ZOMBIE_TYPES,
```

- [ ] **Step 2: ConstEnums.h — HelmType 新增值**

把 `src/ConstEnums.h` 的 HelmType 枚举末尾改为：

```cpp
    HELMTYPE_WALLNUT = 8,
    HELMTYPE_TALLNUT = 9,
    HELMTYPE_DOOMSHROOM = 10            // 毁灭菇头（一类防具，370 血）
};
```

- [ ] **Step 3: Zombie.cpp — gZombieDefs 追加条目**

在 `src/Lawn/Zombie.cpp` 的 `gZombieDefs` 数组（`{ ZOMBIE_REDEYE_GARGANTUAR, ... }` 行之后、`};` 之前）追加：

```cpp
    { ZOMBIE_REDEYE_GARGANTUAR, REANIM_GARGANTUAR,          10,     48,     15,     6000,   "REDEYED_GARGANTUAR" },
    { ZOMBIE_DOOMSHROOM_HEAD,   REANIM_JACKINTHEBOX,        3,      31,     10,     1000,   "DOOMSHROOM_HEAD_ZOMBIE" },
};
```

- [ ] **Step 4: Challenge.cpp — gZombieAllowedLevels 追加条目**

在 `src/Lawn/Challenge.cpp` 的 `gZombieAllowedLevels` 数组中，`{ ZOMBIE_REDEYE_GARGANTUAR, {0} },` 之后追加：

```cpp
    { ZOMBIE_REDEYE_GARGANTUAR, {0} },
    { ZOMBIE_DOOMSHROOM_HEAD, {0} },
};
```

- [ ] **Step 5: 编译验证**

Run: `cmake --build build`
Expected: 编译通过，无报错。

- [ ] **Step 6: 提交**

```bash
git add src/ConstEnums.h src/Lawn/Zombie.cpp src/Lawn/Challenge.cpp
git commit -m "feat(zombie): add ZOMBIE_DOOMSHROOM_HEAD enum and def tables"
```

---

### Task 2: 生成概率（10% 转化）

**Files:**
- Modify: `src/Lawn/Board.cpp` — `Board::AddZombieInRow`（约 2988-3045 行，在"铁桶→坚果头"块之后）

**Interfaces:**
- Consumes: `ZombieType::ZOMBIE_DOOMSHROOM_HEAD`（Task 1）
- Produces: 游戏中以 `ZOMBIE_DOOMSHROOM_HEAD` 生成的僵尸实例（Task 3 的初始化代码必须已就绪）

- [ ] **Step 1: AddZombieInRow 添加 10% 转化**

在 `src/Lawn/Board.cpp` 的 `AddZombieInRow` 中、"铁桶僵尸 20% 概率变为坚果头僵尸" 块之后插入：

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

- [ ] **Step 2: 编译验证**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 3: 提交**

```bash
git add src/Lawn/Board.cpp
git commit -m "feat(zombie): 10% chance jack-in-the-box spawns as doom-shroom head"
```

---

### Task 3: 阶段一初始化（ZombieInitialize 新 case）

**Files:**
- Modify: `src/Lawn/Zombie.cpp` — `Zombie::ZombieInitialize`，在 `ZOMBIE_JACK_IN_THE_BOX` case 之后（约 490 行）新增 `ZOMBIE_DOOMSHROOM_HEAD` case

**Interfaces:**
- Consumes: `ZombieType::ZOMBIE_DOOMSHROOM_HEAD`、`HelmType::HELMTYPE_DOOMSHROOM`（Task 1）；`IMAGE_BLANK`、`IMAGE_REANIM_ZOMBIE_JACKBOX_OUTERARM_LOWER2`（Resources.h 已有）；`REANIM_DOOMSHROOM`（ConstEnums.h 已有）
- Produces: 阶段一僵尸状态：`mSpecialHeadReanimID` 指向毁灭菇头动画、`mHelmType=HELMTYPE_DOOMSHROOM`、`mHelmHealth=370`、`mPhaseCounter`=头部自爆计时、`mZombiePhase=PHASE_JACK_IN_THE_BOX_RUNNING`（Task 4 的状态机依赖）

- [ ] **Step 1: 编写 ZombieInitialize case**

在 `ZOMBIE_JACK_IN_THE_BOX` case 的 `break;` 之后、`ZOMBIE_BOBSLED` case 之前插入：

```cpp
    case ZombieType::ZOMBIE_DOOMSHROOM_HEAD:
    {
        mBodyHealth = 500;
        mAnimTicksPerFrame = 6;
        mZombieAttackRect = Rect(20, 0, 50, 115);

        Reanimation* aBodyReanim = mApp->ReanimationGet(mBodyReanimID);

        // 盒子本体与摇柄用空白图覆盖（轨道保持可见，用于挂载毁灭菇头）
        aBodyReanim->GetTrackInstanceByName("Zombie_jackbox_box")->mImageOverride = IMAGE_BLANK;
        aBodyReanim->GetTrackInstanceByName("Zombie_jackbox_handle")->mImageOverride = IMAGE_BLANK;
        // 外臂换成空手图
        aBodyReanim->SetImageOverride("Zombie_jackbox_outerarm_lower", IMAGE_REANIM_ZOMBIE_JACKBOX_OUTERARM_LOWER2);

        // 在盒子位置挂载毁灭菇头
        ReanimatorTrackInstance* aTrackInstance = aBodyReanim->GetTrackInstanceByName("Zombie_jackbox_box");
        Reanimation* aHeadReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_DOOMSHROOM);
        aHeadReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
        mSpecialHeadReanimID = mApp->ReanimationGetID(aHeadReanim);
        AttachEffect* aAttachEffect = AttachReanim(aTrackInstance->mAttachmentID, aHeadReanim, 0.0f, 0.0f);
        aBodyReanim->mFrameBasePose = 0;
        TodScaleRotateTransformMatrix(aAttachEffect->mOffset, 0.0f, -5.0f, 0.2f, -0.8f, 0.8f);

        // 一类防具：毁灭菇头 370 血（先于本体承伤；mHelmMaxHealth 与 HP 倍率由后续通用代码处理）
        mHelmType = HelmType::HELMTYPE_DOOMSHROOM;
        mHelmHealth = 370;

        // 头部自爆计时（与开盒计时公式一致，含 1/20 早爆）
        int aDistance = 450 + Rand(300);
        if (Rand(20) == 0)
        {
            aDistance /= 3;
        }
        mPhaseCounter = static_cast<int>(aDistance / mVelX) * ZOMBIE_LIMP_SPEED_FACTOR;
        if (mApp->IsScaryPotterLevel())
        {
            mPhaseCounter = 10;
        }
        if (IsOnBoard())
        {
            mZombiePhase = ZombiePhase::PHASE_JACK_IN_THE_BOX_RUNNING;
        }
        break;
    }
```

- [ ] **Step 2: 编译验证**

Run: `cmake --build build`
Expected: 编译通过。若报 `GetTrackInstanceByName`/`TodScaleRotateTransformMatrix`/`AttachReanim` 未声明，检查头文件包含（`Reanimator.h`/`Attachment.h` 已在 Zombie.cpp 顶部包含）。

- [ ] **Step 3: 提交**

```bash
git add src/Lawn/Zombie.cpp
git commit -m "feat(zombie): doom-shroom head stage-1 init (armor 370, head reanim, detonate timer)"
```

---

### Task 4: 阶段一状态机（头部自爆 → 变回小丑）

**Files:**
- Modify: `src/Lawn/Zombie.cpp` — `Zombie::Update()` 的分发处（约 4498 行，`ZOMBIE_JACK_IN_THE_BOX` 分支）
- Modify: `src/Lawn/Zombie.cpp` — `Zombie::UpdateZombieJackInTheBox()`（约 2058 行）
- Modify: `src/Lawn/Zombie.h` — 新增 `RevertToJackInTheBox()` 声明（`UpdateZombieJackInTheBox` 声明附近）
- Modify: `src/Lawn/Zombie.cpp` — 新增 `RevertToJackInTheBox()` 定义（`UpdateZombieJackInTheBox` 之后）

**Interfaces:**
- Consumes: Task 3 的初始化状态（`mSpecialHeadReanimID`、`mPhaseCounter`、`mZombiePhase=PHASE_JACK_IN_THE_BOX_RUNNING`）
- Produces: `Zombie::RevertToJackInTheBox()`（Task 5 复用）；阶段二状态：`mZombieType=ZOMBIE_JACK_IN_THE_BOX` + 盒子轨道恢复 + 开盒计时

- [ ] **Step 1: Update() 分发**

把 `src/Lawn/Zombie.cpp` 约 4498 行改为：

```cpp
    if (mZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX || mZombieType == ZombieType::ZOMBIE_DOOMSHROOM_HEAD)
    {
        UpdateZombieJackInTheBox();
    }
```

- [ ] **Step 2: UpdateZombieJackInTheBox 扩展**

把 `Zombie::UpdateZombieJackInTheBox()` 整体替换为（RUNNING 分支在进入 POPPING 时对毁灭菇头播引信动画；POPPING 分支在爆炸时区分类型）：

```cpp
void Zombie::UpdateZombieJackInTheBox()
{
    if (mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_RUNNING)
    {
        if (mPhaseCounter <= 0 && mHasHead)
        {
            mPhaseCounter = 110;
            mZombiePhase = ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING;

            StopZombieSound();
            mApp->PlaySample(SOUND_BOING);
            PlayZombieReanim("anim_pop", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 28.0f);

            if (mZombieType == ZombieType::ZOMBIE_DOOMSHROOM_HEAD)
            {
                // 毁灭菇头引信：播放爆炸前摇动画 + 嘶嘶声
                Reanimation* aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
                if (aHeadReanim)
                {
                    aHeadReanim->PlayReanim("anim_explode", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 23.0f);
                    aHeadReanim->SetShakeOverride("DoomShroom_head1", 1.0f);
                    aHeadReanim->SetShakeOverride("DoomShroom_head2", 2.0f);
                    aHeadReanim->SetShakeOverride("DoomShroom_head3", 2.0f);
                }
                mApp->PlayFoley(FoleyType::FOLEY_REVERSE_EXPLOSION);
            }
        }
    }
    else if (mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING)
    {
        if (mPhaseCounter == 80)
        {
            mApp->PlayFoley(FoleyType::FOLEY_JACK_SURPRISE);
        }

        if (mPhaseCounter <= 0)
        {
            mApp->PlayFoley(FoleyType::FOLEY_EXPLOSION);

            int aPosX = mX + mWidth / 2;
            int aPosY = mY + mHeight / 2;
            if (mMindControlled)
            {
                mBoard->KillAllZombiesInRadius(mRow, aPosX, aPosY, JackInTheBoxZombieRadius, 1, true, 127);
            }
            else
            {
                mBoard->KillAllZombiesInRadius(mRow, aPosX, aPosY, JackInTheBoxZombieRadius, 1, true, 255);
                mBoard->KillAllPlantsInRadius(aPosX, aPosY, JackInTheBoxPlantRadius);
            }

            mApp->AddTodParticle(aPosX, aPosY, Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 0), ParticleEffect::PARTICLE_JACKEXPLODE);
            mBoard->ShakeBoard(4, -6);

            if (mZombieType == ZombieType::ZOMBIE_DOOMSHROOM_HEAD)
            {
                // 毁灭菇头爆炸：本体存活，变回普通小丑僵尸（盒子回来再爆一次）
                RevertToJackInTheBox();
            }
            else
            {
                DieNoLoot();

                if (mApp->IsScaryPotterLevel())
                {
                    mBoard->mChallenge->ScaryPotterJackExplode(aPosX, aPosY);
                }
            }
        }
    }
}
```

- [ ] **Step 3: Zombie.h 声明**

在 `src/Lawn/Zombie.h` 的 `UpdateZombieJackInTheBox` 声明旁新增：

```cpp
    void                            UpdateZombieJackInTheBox();
    void                            RevertToJackInTheBox();
```

- [ ] **Step 4: RevertToJackInTheBox 定义**

在 `src/Lawn/Zombie.cpp` 中 `UpdateZombieJackInTheBox()` 函数之后新增：

```cpp
void Zombie::RevertToJackInTheBox()
{
    // 移除毁灭菇头
    if (mSpecialHeadReanimID != ReanimationID::REANIMATIONID_NULL)
    {
        mApp->RemoveReanimation(mSpecialHeadReanimID);
        mSpecialHeadReanimID = ReanimationID::REANIMATIONID_NULL;
    }

    // 恢复盒子（清除空白图覆盖）
    Reanimation* aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim)
    {
        aBodyReanim->GetTrackInstanceByName("Zombie_jackbox_box")->mImageOverride = nullptr;
        aBodyReanim->GetTrackInstanceByName("Zombie_jackbox_handle")->mImageOverride = nullptr;
        aBodyReanim->SetImageOverride("Zombie_jackbox_outerarm_lower", nullptr);
    }

    // 变回普通小丑僵尸
    mZombieType = ZombieType::ZOMBIE_JACK_IN_THE_BOX;
    mZombiePhase = ZombiePhase::PHASE_JACK_IN_THE_BOX_RUNNING;
    int aDistance = 450 + Rand(300);
    if (Rand(20) == 0)
    {
        aDistance /= 3;
    }
    mPhaseCounter = static_cast<int>(aDistance / mVelX) * ZOMBIE_LIMP_SPEED_FACTOR;
    StartZombieSound();
}
```

- [ ] **Step 5: 编译验证**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 6: 手动验证（如需可先在 Task 7 的调试召唤就绪后复测）**

Run: 游戏 → 任意有小丑僵尸的关卡（或调试召唤）
Expected: 毁灭菇头小丑走到计时终点 → 头播引信动画 + 嘶嘶声 → 爆炸粒子 + 震屏 → 本体存活、盒子回来、音乐盒音效响起 → 开盒 → 再爆一次 → 死亡。

- [ ] **Step 7: 提交**

```bash
git add src/Lawn/Zombie.cpp src/Lawn/Zombie.h
git commit -m "feat(zombie): doom-shroom head detonates box-style then reverts to jack-in-the-box"
```

---

### Task 5: 防具击毁路径（伤害打掉头 → 直接变回小丑）

**Files:**
- Modify: `src/Lawn/Zombie.cpp` — `Zombie::DropHelm()`（约 7827 行，`HELMTYPE_BOBSLED` 分支后）
- Modify: `src/Lawn/Zombie.cpp` — `Zombie::TakeHelmDamage()`（约 7886 行，`mHelmHealth == 0` 块）

**Interfaces:**
- Consumes: `HelmType::HELMTYPE_DOOMSHROOM`（Task 1）、`RevertToJackInTheBox()`（Task 4）
- Produces: 伤害击毁毁灭菇头 → 无爆炸直接变回小丑的行为

- [ ] **Step 1: DropHelm 增加分支**

在 `src/Lawn/Zombie.cpp` 的 `DropHelm()` 中、`HELMTYPE_BOBSLED` 分支之后插入：

```cpp
    else if (mHelmType == HelmType::HELMTYPE_DOOMSHROOM)
    {
        // 毁灭菇头：不生成掉落粒子；头动画与盒子恢复由调用方 RevertToJackInTheBox() 处理
    }
```

- [ ] **Step 2: TakeHelmDamage 击毁后触发变身**

把 `src/Lawn/Zombie.cpp` 的 `TakeHelmDamage()` 中这段：

```cpp
    if (mHelmHealth == 0)
    {
        DropHelm(theDamageFlags);
        return aDamageRemaining;
    }
```

改为：

```cpp
    if (mHelmHealth == 0)
    {
        DropHelm(theDamageFlags);
        if (mZombieType == ZombieType::ZOMBIE_DOOMSHROOM_HEAD)
        {
            // 毁灭菇头被击毁：无爆炸，直接变回普通小丑僵尸
            RevertToJackInTheBox();
        }
        return aDamageRemaining;
    }
```

- [ ] **Step 3: 编译验证**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 4: 手动验证**

Run: 游戏 → 召出毁灭菇头小丑 → 用植物把毁灭菇头（370 血）打掉
Expected: 头直接消失、无爆炸；盒子回来；之后正常开盒爆炸一次（"非理想情况"只炸一次）。

- [ ] **Step 5: 提交**

```bash
git add src/Lawn/Zombie.cpp
git commit -m "feat(zombie): doom-shroom head destroyed by damage reverts to jack-in-the-box"
```

---

### Task 6: 边界清理（死亡/黄油/磁力菇）

**Files:**
- Modify: `src/Lawn/Zombie.cpp` — `Zombie::DropHead()`（约 3520 行，`IsZombotany` 分支）
- Modify: `src/Lawn/Zombie.cpp` — `Zombie::ApplyButter()`（`IsZombotany` 分支）
- Modify: `src/Lawn/Zombie.cpp` — `Zombie::RemoveButter()`（约 8668 行，`IsZombotany` 分支）
- Modify: `src/Lawn/Plant.cpp` — 磁力菇 JITB 分支（约 2105 行）

**Interfaces:**
- Consumes: `ZombieType::ZOMBIE_DOOMSHROOM_HEAD`（Task 1）

- [ ] **Step 1: DropHead 头动画随尸销毁**

把 `src/Lawn/Zombie.cpp` 的 `DropHead()` 中：

```cpp
    if (Zombie::IsZombotany(mZombieType))
    {
        mApp->ReanimationGet(mSpecialHeadReanimID)->ReanimationDie();
        mSpecialHeadReanimID = ReanimationID::REANIMATIONID_NULL;
        return;
    }
```

改为：

```cpp
    if (Zombie::IsZombotany(mZombieType) || mZombieType == ZombieType::ZOMBIE_DOOMSHROOM_HEAD)
    {
        mApp->ReanimationGet(mSpecialHeadReanimID)->ReanimationDie();
        mSpecialHeadReanimID = ReanimationID::REANIMATIONID_NULL;
        return;
    }
```

- [ ] **Step 2: ApplyButter 冻结头动画**

把 `src/Lawn/Zombie.cpp` 的 `ApplyButter()` 中：

```cpp
    else if (Zombie::IsZombotany(mZombieType))
    {
        Reanimation* aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim)
        {
            aHeadReanim->mAnimRate = 0.0f;
        }
    }
```

改为：

```cpp
    else if (Zombie::IsZombotany(mZombieType) || mZombieType == ZombieType::ZOMBIE_DOOMSHROOM_HEAD)
    {
        Reanimation* aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim)
        {
            aHeadReanim->mAnimRate = 0.0f;
        }
    }
```

- [ ] **Step 3: RemoveButter 恢复头动画**

把 `src/Lawn/Zombie.cpp` 的 `RemoveButter()` 中：

```cpp
    if (Zombie::IsZombotany(mZombieType))
    {
        Reanimation* aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim)
        {
            if (mZombieType == ZombieType::ZOMBIE_PEA_HEAD && aHeadReanim->IsAnimPlaying("anim_shooting"))
            {
                aHeadReanim->mAnimRate = 35.0f;
            }
            else if (mZombieType == ZombieType::ZOMBIE_GATLING_HEAD && aHeadReanim->IsAnimPlaying("anim_shooting"))
            {
                aHeadReanim->mAnimRate = 38.0f;
            }
            else
            {
                aHeadReanim->mAnimRate = 15.0f;
            }
        }
    }
```

改为（条件加 DOOMSHROOM_HEAD，走 else 分支恢复 15.0f）：

```cpp
    if (Zombie::IsZombotany(mZombieType) || mZombieType == ZombieType::ZOMBIE_DOOMSHROOM_HEAD)
    {
        Reanimation* aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim)
        {
            if (mZombieType == ZombieType::ZOMBIE_PEA_HEAD && aHeadReanim->IsAnimPlaying("anim_shooting"))
            {
                aHeadReanim->mAnimRate = 35.0f;
            }
            else if (mZombieType == ZombieType::ZOMBIE_GATLING_HEAD && aHeadReanim->IsAnimPlaying("anim_shooting"))
            {
                aHeadReanim->mAnimRate = 38.0f;
            }
            else
            {
                aHeadReanim->mAnimRate = 15.0f;
            }
        }
    }
```

- [ ] **Step 4: 磁力菇对毁灭菇头无效**

把 `src/Lawn/Plant.cpp` 约 2105 行：

```cpp
    else if (theZombie->mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_RUNNING)
```

改为：

```cpp
    else if (theZombie->mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_RUNNING && theZombie->mZombieType != ZombieType::ZOMBIE_DOOMSHROOM_HEAD)
```

- [ ] **Step 5: 编译验证**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 6: 手动验证**

Run: 游戏 → 毁灭菇头小丑被植物击杀 → 无残留头动画/无崩溃；黄油命中 → 头动画同步冻结；磁力菇 → 吸不动毁灭菇头（变回小丑后可正常吸盒）。

- [ ] **Step 7: 提交**

```bash
git add src/Lawn/Zombie.cpp src/Lawn/Plant.cpp
git commit -m "feat(zombie): doom-shroom head death/butter cleanup and magnet immunity"
```

---

### Task 7: 调试召唤与全量验证

**Files:**
- Modify: `src/Lawn/Board.cpp` — 调试召唤键（`theChar == 'j'` 的 JITB 分支附近，约 9079 行）

**Interfaces:**
- Consumes: `ZombieType::ZOMBIE_DOOMSHROOM_HEAD`（Task 1）

- [ ] **Step 1: 调试召唤键 'J'**

在 `src/Lawn/Board.cpp` 的 JITB 调试召唤块（`else if(theChar == 'j')`）之后插入：

```cpp
    else if (theChar == 'j')
    {
        AddZombie(ZombieType::ZOMBIE_JACK_IN_THE_BOX, Zombie::ZOMBIE_WAVE_DEBUG);
        return;
    }
    if (theChar == 'J')
    {
        AddZombie(ZombieType::ZOMBIE_DOOMSHROOM_HEAD, Zombie::ZOMBIE_WAVE_DEBUG);
        return;
    }
```

注意：此调试召唤走 `ZOMBIE_WAVE_DEBUG`，Task 2 的转化被排除，保证稳定召出毁灭菇头本体。

- [ ] **Step 2: 编译验证**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 3: 手动全量测试矩阵**

Run: 游戏，逐项验证：

1. 冒险模式：小丑僵尸约 10% 概率带毁灭菇头。
2. 理想情况（不动它）：头引信（anim_explode + 嘶嘶声）→ 盒子式爆炸（范围内植物全灭、敌方僵尸不受影响、本体存活、震屏）→ 盒子回来（音乐盒音效）→ 开盒 → 再爆一次 → 死亡。共炸两次。
3. 非理想情况（打掉头 370 血）：头消失、无爆炸 → 盒子回来 → 开盒爆炸一次。
4. 我是僵尸模式：不出现毁灭菇头小丑。
5. 存档读档：阶段一存一次（读档后头还在、计时继续）、阶段二存一次（读档后是普通小丑）。
6. 魅惑：头爆炸与开盒爆炸都只杀敌方僵尸、不炸植物、本体存活。
7. 磁力菇：吸不动毁灭菇头；变回小丑后正常吸盒。
8. 黄油/冰冻：毁灭菇头动画同步冻结/恢复。
9. `--zombie-hp-multiplier=2`：毁灭菇头血量 740，行为正常。
10. 头部爆炸不会误伤自己（本体在爆炸后存活并继续）。

- [ ] **Step 4: 提交**

```bash
git add src/Lawn/Board.cpp
git commit -m "feat(debug): summon doom-shroom head zombie with 'J'"
```

---

## 自检说明

- **Spec 覆盖**：spec 的 10 个章节全部有对应 Task（枚举/概率/阶段一/状态机/防具击毁/清理/磁力菇/调试/存档验证）。存档零改动（Task 5 验证项覆盖）。
- **类型一致性**：`RevertToJackInTheBox()` 在 Task 4 声明与定义、Task 5 调用，签名一致；`ZOMBIE_DOOMSHROOM_HEAD`/`HELMTYPE_DOOMSHROOM` 在 Task 1 定义后各 Task 使用。
- **无占位符**：所有编辑都有完整代码。
- **无自动化测试**：本仓库无测试套件（AGENTS.md），每 Task 以编译 + 手动验证为测试循环。
