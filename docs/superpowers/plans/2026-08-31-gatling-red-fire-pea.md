# 机枪射手红色火豌豆（灼烧穿刺）实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 机枪射手每颗子弹 1% 概率被替换为新的红色火豌豆子弹（`PROJECTILE_FIREPEA_RED`）：接触僵尸期间每帧造成 1 点伤害，无限穿透。

**Architecture:** 新增 `ProjectileType` 枚举值 + `gProjectileDefinition` 定义；`Plant::Fire()` 内用局部 lambda 逐颗掷 1% 骰子决定子弹类型；`Projectile` 增加该类型的初始化（火焰拖尾）、渲染（红色乘色豌豆）、碰撞分支（每帧对重叠僵尸 `TakeDamage(1)` 且不 `Die()`）。

**Tech Stack:** C++20, CMake + Ninja, SDL2。无自动化测试框架——验证方式为编译 + 手动游戏测试（遵循 AGENTS.md）。

## Global Constraints

- **不重排 `ConstEnums.h` 既有值**：新值只能加在 `NUM_*` 哨兵之前。本次在 `PROJECTILE_ZOMBIE_PEA = 13` 后插入 `PROJECTILE_FIREPEA_RED = 14`，`NUM_PROJECTILES` 14 → 15。
- **无自动化测试套件**（AGENTS.md：`No automated test suite`）。不要新建测试框架；每任务以 `cmake --build build` 编译通过为验收，最终任务做手动游戏测试。
- **命名风格**：成员 `mMember`、参数 `theParameter`、局部 `aLocal`。命名空间限定 `ProjectileType::`、`ZombieType::`、`ParticleEffect::` 等与既有代码一致。
- **缩进跟随所在文件**：`src/Lawn/Projectile.cpp` 用 Tab；`src/Lawn/Plant.cpp` 用 4 空格。`src/ConstEnums.h` 用 4 空格。
- **不新建类、不子类化植物/僵尸、不动存档格式、不新增数据成员**（`Projectile` 现有字段够用）。
- **灼烧伤害固定每帧 1 点**，与 `mDamageOverride`（孤狼 200 / 散射 200）无关。
- 只影响机枪射手植物；魅惑机枪头僵尸（`Zombie.cpp` 路径）明确不做。

---

### Task 1: 新枚举值与子弹定义

**Files:**
- Modify: `src/ConstEnums.h:821-822`
- Modify: `src/Lawn/Projectile.cpp:37-52`

**Interfaces:**
- Consumes: 无（首个任务）。
- Produces: 枚举 `ProjectileType::PROJECTILE_FIREPEA_RED`；`gProjectileDefinition` 新条目（`mDamage = 1`）。后续所有任务依赖此枚举。

- [ ] **Step 1: 在 `ConstEnums.h` 新增枚举值**

`src/ConstEnums.h` 第 821-822 行，把：

```cpp
    PROJECTILE_ZOMBIE_PEA = 13,  // 僵尸豌豆
    NUM_PROJECTILES = 14
```

改为：

```cpp
    PROJECTILE_ZOMBIE_PEA = 13,  // 僵尸豌豆
    PROJECTILE_FIREPEA_RED = 14,  // 红色火豌豆（机枪射手 1% 概率，灼烧穿刺）
    NUM_PROJECTILES = 15
```

- [ ] **Step 2: 在 `gProjectileDefinition[]` 追加条目**

`src/Lawn/Projectile.cpp` 第 51 行 `{ ProjectileType::PROJECTILE_ZOMBIE_PEA, 0, 20 }` 后（数组末尾）追加：

```cpp
	{ ProjectileType::PROJECTILE_FIREPEA_RED,   0,  1   }
```

（注意：该数组没有尾随逗号，最后一个条目不加逗号。`mDamage = 1` 与灼烧伤害一致；实际伤害走 Task 4 的专用逻辑，不走定义值。）

- [ ] **Step 3: 编译验证**

Run: `cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
Expected: 编译成功，无报错。`NUM_PROJECTILES` 变化不会触发任何编译错误（数组按枚举序排列，条目数恰好匹配）。

- [ ] **Step 4: 提交**

```bash
git add src/ConstEnums.h src/Lawn/Projectile.cpp
git commit -m "feat(projectile): add PROJECTILE_FIREPEA_RED enum and definition"
```

---

### Task 2: 初始化（火焰拖尾）+ 高地一致性 + aTime

**Files:**
- Modify: `src/Lawn/Projectile.cpp`：`ProjectileInitialize` switch（约 120-150 行）、`CantHitHighGround()`（444-450 行）、`CheckForHighGround()`（457-466 行）、`Update()` aTime 列表（1112-1124 行）

**Interfaces:**
- Consumes: Task 1 的 `ProjectileType::PROJECTILE_FIREPEA_RED`。
- Produces: 该类型创建时自动挂火焰拖尾；在高地（屋顶）行为与豌豆/火球一致。

- [ ] **Step 1: `ProjectileInitialize` 挂火焰拖尾**

`src/Lawn/Projectile.cpp` 中 `ProjectileInitialize` 的 switch，在 `PROJECTILE_SNOWPEA` case（第 126-129 行，末尾 `break;`）之后插入：

```cpp
	case ProjectileType::PROJECTILE_FIREPEA_RED:
	{
		TodParticleSystem* aParticle = mApp->AddTodParticle(mPosX + 8.0f, mPosY + 13.0f, 400000, ParticleEffect::PARTICLE_FIREBALL_TRAIL);
		AttachParticle(mAttachmentID, aParticle, 8.0f, 13.0f);
		break;
	}
```

- [ ] **Step 2: `CantHitHighGround()` 并入新类型**

第 444-449 行的返回列表，在 `PROJECTILE_FIREBALL` 之后加一行：

```cpp
	return (
		mProjectileType == ProjectileType::PROJECTILE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_SNOWPEA ||
		mProjectileType == ProjectileType::PROJECTILE_STAR ||
		mProjectileType == ProjectileType::PROJECTILE_PUFF ||
		mProjectileType == ProjectileType::PROJECTILE_FIREBALL ||
		mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED
		) && !mOnHighGround;
```

- [ ] **Step 3: `CheckForHighGround()` 并入新类型**

第 458-464 行的 if 条件列表，在 `PROJECTILE_COBBIG` 之后加：

```cpp
	if (mProjectileType == ProjectileType::PROJECTILE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_SNOWPEA ||
		mProjectileType == ProjectileType::PROJECTILE_FIREBALL ||
		mProjectileType == ProjectileType::PROJECTILE_SPIKE ||
		mProjectileType == ProjectileType::PROJECTILE_COBBIG ||
		mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
	{
		if (aShadowDelta < 28.0f)
		{
			DoImpact(nullptr);
			return;
		}
	}
```

（高地过渡时 `DoImpact(nullptr)` → 该类型非溅射、无僵尸 → 直接 `Die()`，与豌豆/火球一致。）

- [ ] **Step 4: `Update()` 的 aTime 列表并入新类型**

第 1113-1123 行的 if 条件，在 `PROJECTILE_SPIKE` 之后加 `||`：

```cpp
	int aTime = 20;
	if (mProjectileType == ProjectileType::PROJECTILE_PEA || 
		mProjectileType == ProjectileType::PROJECTILE_SNOWPEA || 
		mProjectileType == ProjectileType::PROJECTILE_CABBAGE || 
		mProjectileType == ProjectileType::PROJECTILE_MELON || 
		mProjectileType == ProjectileType::PROJECTILE_WINTERMELON || 
		mProjectileType == ProjectileType::PROJECTILE_KERNEL || 
		mProjectileType == ProjectileType::PROJECTILE_BUTTER || 
		mProjectileType == ProjectileType::PROJECTILE_COBBIG || 
		mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA || 
		mProjectileType == ProjectileType::PROJECTILE_SPIKE || 
		mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
	{
		aTime = 0;
	}
```

（`aTime` 仅控制 `mRenderOrder` 何时进入 `RENDER_LAYER_PROJECTILE` 层；与豌豆一致立即更新。）

- [ ] **Step 5: 编译验证**

Run: `cmake --build build`
Expected: 编译成功。

- [ ] **Step 6: 提交**

```bash
git add src/Lawn/Projectile.cpp
git commit -m "feat(projectile): fire pea red - fire trail, high-ground and aTime parity"
```

---

### Task 3: 渲染——红色豌豆 + 阴影 + 碰撞矩形

**Files:**
- Modify: `src/Lawn/Projectile.cpp`：`Draw()` switch（1153-1216 行）、`Draw()` 绘制块（1219-1246 行）、`DrawShadow()` switch（1272-1277 行）、`GetProjectileRect()`（1338-1343 行）

**Interfaces:**
- Consumes: Task 1 的枚举。
- Produces: 红色豌豆外观（`IMAGE_PROJECTILEPEA` + `TodBltMatrix` 乘色 `Color(255, 96, 96)`）。

- [ ] **Step 1: `Draw()` switch 指定豌豆贴图**

在 `PROJECTILE_PEA`/`PROJECTILE_ZOMBIE_PEA` case（1157-1160 行）之后插入：

```cpp
	case ProjectileType::PROJECTILE_FIREPEA_RED:
		aImage = IMAGE_PROJECTILEPEA;
		break;
```

- [ ] **Step 2: `Draw()` 绘制块加红色乘色分支**

把 1219-1225 行的快速路径条件改为先判断红火豌豆：

```cpp
		Rect aSrcRect(aCelWidth * mFrame, aCelHeight * aProjectileDef.mImageRow, aCelWidth, aCelHeight);
		if (mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
		{
			// 红色火豌豆：红色乘色绘制，保留豌豆明暗轮廓
			float aOffsetX = mPosX + aCelWidth * 0.5f;
			float aOffsetY = mPosZ + mPosY + aCelHeight * 0.5f;
			SexyTransform2D aTransform;
			TodScaleRotateTransformMatrix(aTransform, aOffsetX + mBoard->mX, aOffsetY + mBoard->mY, mRotation, aScale, aScale);
			TodBltMatrix(g, aImage, aTransform, g->mClipRect, Color(255, 96, 96), g->mDrawMode, aSrcRect);
		}
		else if (FloatApproxEqual(mRotation, 0.0f) && FloatApproxEqual(aScale, 1.0f))
		{
			Rect aDestRect(0, 0, aCelWidth, aCelHeight);
			g->DrawImageMirror(aImage, aDestRect, aSrcRect, aMirror);
		}
		else
		{
			// 原旋转路径，保持不变
			float aOffsetX = mPosX + aCelWidth * 0.5f;
			float aOffsetY = mPosZ + mPosY + aCelHeight * 0.5f;
			SexyTransform2D aTransform;
			TodScaleRotateTransformMatrix(aTransform, aOffsetX + mBoard->mX, aOffsetY + mBoard->mY, mRotation, aScale, aScale);
			TodBltMatrix(g, aImage, aTransform, g->mClipRect, Color::White, g->mDrawMode, aSrcRect);
		}
```

（注意：`Draw()` 的 switch 里 `aImage` 可能为 `nullptr`（如火球），红火豌豆分支已在 `if (aImage)` 块内，安全。）

- [ ] **Step 3: `DrawShadow()` 并入豌豆 case**

把 1273-1275 行的 case 合并为：

```cpp
	case ProjectileType::PROJECTILE_PEA:
	case ProjectileType::PROJECTILE_ZOMBIE_PEA:
	case ProjectileType::PROJECTILE_FIREPEA_RED:
		aOffsetX += 3.0f;
		break;
```

- [ ] **Step 4: `GetProjectileRect()` 并入豌豆分支**

把 1338-1342 行的条件改为：

```cpp
	if (mProjectileType == ProjectileType::PROJECTILE_PEA || 
		mProjectileType == ProjectileType::PROJECTILE_SNOWPEA ||
		mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
	{
		return Rect(mX - 15, mY, mWidth + 15, mHeight);
	}
```

- [ ] **Step 5: 编译验证**

Run: `cmake --build build`
Expected: 编译成功。

- [ ] **Step 6: 提交**

```bash
git add src/Lawn/Projectile.cpp
git commit -m "feat(projectile): render red fire pea as red-tinted pea with shadow and rect"
```

---

### Task 4: 灼烧 + 无限穿透（`CheckForCollision()` 专用分支）

**Files:**
- Modify: `src/Lawn/Projectile.cpp:426`（`CheckForCollision()` 中 `Zombie* aZombie = FindCollisionTarget();` 之前）

**Interfaces:**
- Consumes: Task 1 的枚举；`Zombie::TakeDamage(int, unsigned int)`（`src/Lawn/Zombie.h:209`）；`Projectile::GetDamageFlags(Zombie*)`、`GetProjectileRect()`、`CantHitHighGround()`；`Board::IterateZombies(Zombie*&)`（`DoSplashDamage` 同款用法）。
- Produces: 红火豌豆的完整战斗行为——接触灼烧 + 无限穿透。

- [ ] **Step 1: 插入灼烧穿刺分支**

在 `CheckForCollision()` 末尾（第 426 行 `Zombie* aZombie = FindCollisionTarget();` 之前）插入：

```cpp
	if (mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
	{
		// 灼烧穿刺：每帧对矩形重叠的每个僵尸造成 1 点伤害；无限穿透，永不因击中僵尸而消亡（出屏判定在上方，正常 Die）
		Rect aProjectileRect = GetProjectileRect();
		Zombie* aZombie = nullptr;
		while (mBoard->IterateZombies(aZombie))
		{
			if (aZombie->mZombieType != ZombieType::ZOMBIE_BOSS && aZombie->mRow != mRow)
				continue;
			if (!aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
				continue;
			if (aZombie->mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL && mPosZ >= 45.0f)
				continue;
			if (aZombie->mOnHighGround && CantHitHighGround())
				continue;

			Rect aZombieRect = aZombie->GetZombieRect();
			if (GetRectOverlap(aProjectileRect, aZombieRect) > 0)
			{
				aZombie->TakeDamage(1, GetDamageFlags(aZombie));
			}
		}
		return;  // 无限穿透
	}
```

要点（对照既有 `FindCollisionTarget` 的过滤条件，保持一致）：
- 同排或 Boss；`EffectedByDamage(mDamageRangeFlags)`；潜泳状态（`mPosZ >= 45`）跳过；高台规则 `CantHitHighGround`。
- `GetDamageFlags` 对该类型返回 0（非溅射/非冰冻/直射不绕盾）→ 铁桶/路障先吃伤害，与豌豆一致。
- 多个僵尸重叠时全部同时每帧受 1 点伤害。

- [ ] **Step 2: 编译验证**

Run: `cmake --build build`
Expected: 编译成功。

- [ ] **Step 3: 提交**

```bash
git add src/Lawn/Projectile.cpp
git commit -m "feat(projectile): fire pea red burn contact damage per frame with infinite pierce"
```

---

### Task 5: `Plant::Fire()` 逐颗 1% 判定 + 音效

**Files:**
- Modify: `src/Lawn/Plant.cpp:4898-4903`（`aProjectileType` 定稿后的掷骰 + 音效块）、`5046-5060`（散射循环逐颗掷骰）、`5065`（普通模式主子弹生成）

**Interfaces:**
- Consumes: Task 1 的枚举；`Plant::mSeedType`、`mGatlingScatterCountdown`；`Sexy::Rand(100)`（Plant.cpp 内直接用 `Rand`）。
- Produces: 机枪射手（含大招散射）每颗子弹独立 1% 概率变为红火豌豆。

- [ ] **Step 1: 掷骰 lambda + 音效改造**

在 `Fire()` 中 `if (mSeedType == SeedType::SEED_KERNELPULT && thePlantWeapon == PlantWeapon::WEAPON_SECONDARY) { aProjectileType = PROJECTILE_BUTTER; }` 之后（约 4898 行），把原音效块：

```cpp
    if (mSeedType != SeedType::SEED_GATLINGPEA || mGatlingScatterCountdown == 0)
    {
        mApp->PlayFoley(FoleyType::FOLEY_THROW);
    }
```

替换为：

```cpp
    // 红火豌豆掷骰：机枪射手每颗子弹独立 1% 判定（散射模式的每颗在循环内另行掷骰）
    auto RollGatlingBulletType = [this](ProjectileType theBaseType) -> ProjectileType
    {
        if (mSeedType == SeedType::SEED_GATLINGPEA && Rand(100) < 1)
            return ProjectileType::PROJECTILE_FIREPEA_RED;
        return theBaseType;
    };

    ProjectileType aMainBulletType = aProjectileType;
    if (mSeedType != SeedType::SEED_GATLINGPEA || mGatlingScatterCountdown == 0)
    {
        aMainBulletType = RollGatlingBulletType(aProjectileType);
        mApp->PlayFoley(aMainBulletType == ProjectileType::PROJECTILE_FIREPEA_RED ? FoleyType::FOLEY_FIREPEA : FoleyType::FOLEY_THROW);
    }
```

（非机枪射手：`aMainBulletType == aProjectileType` 且 `FOLEY_THROW`，行为不变。散射模式不播抛射声，也不在此处浪费随机数。）

- [ ] **Step 2: 散射循环逐颗掷骰**

把散射循环（5046-5060 行）中的：

```cpp
            Projectile* aScatterPea = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder - 1, theRow, aProjectileType);
```

替换为：

```cpp
            ProjectileType aScatterType = RollGatlingBulletType(aProjectileType);
            if (aScatterType == ProjectileType::PROJECTILE_FIREPEA_RED)
            {
                mApp->PlayFoley(FoleyType::FOLEY_FIREPEA);
            }
            Projectile* aScatterPea = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder - 1, theRow, aScatterType);
```

- [ ] **Step 3: 普通模式主子弹用掷骰结果**

把普通模式生成（5064-5065 行）中的：

```cpp
        aProjectile = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder - 1, theRow, aProjectileType);
```

替换为：

```cpp
        aProjectile = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder - 1, theRow, aMainBulletType);
```

（此分支是 `else`，即非散射；机枪射手普通 4 连发与非机枪植物都走这里。`mDamageOverride` 逻辑不动——红火豌豆的灼烧伤害固定 1，不走 `mDamageOverride`。）

- [ ] **Step 4: 编译验证**

Run: `cmake --build build`
Expected: 编译成功。

- [ ] **Step 5: 提交**

```bash
git add src/Lawn/Plant.cpp
git commit -m "feat(gatling): 1% chance per bullet to fire red fire pea (incl. scatter ultimate)"
```

---

### Task 6: 手动游戏测试（验证清单）

**Files:** 无（仅临时改判据测试，测完改回）。

- [ ] **Step 1: 构建并启动游戏**

Run: `cmake --build build && ./build/PvZ-Portable`（或项目 `run-pvz.sh` / `run-pvz.bat`）。
Expected: 游戏正常启动，进入冒险模式关卡。

- [ ] **Step 2: 临时放大概率便于观察**

把 `Plant.cpp` 中 `Rand(100) < 1` 临时改为 `Rand(100) < 50`（50% 概率），重新编译。**测试完成后必须改回 `1` 并重新编译。**
（若只想看 1% 自然概率，可跳过本步，多打几轮观察。）

- [ ] **Step 3: 验证清单（逐项勾选）**

1. **外观**：红火豌豆是红色豌豆贴图 + 火焰拖尾粒子，与普通豌豆明显区分。
2. **灼烧**：单只僵尸被红火豌豆穿过时，接触期间血量每帧 -1（帧数 ≈ 矩形重叠帧数 ≈ 僵尸宽度/3.33px）；未接触不扣血。
3. **无限穿透**：子弹穿过整排所有僵尸后从屏幕右侧飞出；出屏后消失、无残留粒子/附件。
4. **盾牌**：铁桶/路障僵尸先扣盾牌（桶破前本体不掉血），桶破后扣本体。
5. **高台（屋顶关卡）**：低处发射打不到高台僵尸；子弹在高地过渡处消亡。
6. **大招散射**：进入散射模式（掷中开大后），每颗散射子弹独立判定；掷中时播放 `FOLEY_FIREPEA`。
7. **音效**：普通模式掷中红火豌豆时播 `FOLEY_FIREPEA`（非 `FOLEY_THROW`）。

- [ ] **Step 4: 回归**

1. 普通豌豆外观/伤害（20）/音效不变。
2. 豌豆射手、寒冰射手、双发等非机枪植物行为不变。
3. 火炬树桩转换（豌豆→火球、冰豌豆→豌豆）不受影响；红火豌豆穿过火炬不被再次转换。
4. 读取旧存档正常（子弹不入档）。

- [ ] **Step 5: 恢复 1% 并最终构建**

若 Step 2 改过概率，改回 `Rand(100) < 1`，`cmake --build build` 确认编译通过。

- [ ] **Step 6: 提交（如测试中未改代码则跳过）**

```bash
git status  # 确认无遗留调试改动
```
