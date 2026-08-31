# 设计：机枪射手 1% 概率发射红色火豌豆（灼烧穿刺）

日期：2026-08-31
状态：已批准（2026-08-31 用户确认设计）

## 目标

给机枪射手（Gatling Pea）添加一个新机制：**每一发子弹有 1% 概率被替换为一种新的子弹类型——红色火豌豆（`PROJECTILE_FIREPEA_RED`）**。该子弹的特性（用户选择 C）：

- **接触灼烧**：子弹与僵尸矩形重叠期间，每帧对该僵尸造成 1 点伤害。
- **无限穿透**：子弹永不因击中僵尸而消亡，飞出屏幕才消失。
- **外观**（用户选择 B）：红色豌豆贴图 + 火焰拖尾粒子。

覆盖范围（用户选择 B）：普通 4 连发与大招散射模式的**每一颗**子弹各自独立掷 1% 骰子。

## 术语

- **红火豌豆**：新增子弹类型 `PROJECTILE_FIREPEA_RED = 14`，仅由机枪射手的 `Plant::Fire()` 生成。
- **每帧**：游戏主循环一帧（`UpdateNormalMotion` 每帧调用一次 `CheckForCollision`）。
- **接触**：`GetProjectileRect()` 与 `Zombie::GetZombieRect()` 矩形重叠面积 > 0。

## 机制背景（现状）

- 机枪射手射击入口：`Plant::Fire()`（`src/Lawn/Plant.cpp:4842`）：
  - 普通模式：`mShootingCounter` 68/51/35/18 各触发一次 `Fire()`，每轮 4 发，走 `else` 分支 `AddProjectile(..., PROJECTILE_PEA)`。
  - 大招散射模式（`mGatlingScatterCountdown > 0`）：每 2 帧一次 `Fire()`，`SCATTER_COUNT = 2`，每颗 ±10° 扇形、`mDamageOverride = 200`。
  - 抛射音效：`if (mSeedType != SEED_GATLINGPEA || mGatlingScatterCountdown == 0) PlayFoley(FOLEY_THROW);`（散射模式无抛射声）。
- 碰撞流程：`UpdateNormalMotion()` 每帧 `mPosX += 3.33` 后调 `CheckForCollision()` → 出屏判定（`mPosX > WIDE_BOARD_WIDTH` 或 `mPosX + mWidth < 0` → `Die()`）→ `FindCollisionTarget()` 取最左重叠僵尸 → `DoImpact()` → 默认 `Die()`。
- 已有穿透机制：`mPenetrations`（尖刺用，命中后递减并 `mLastHitZombieID` 去重），但"每帧持续伤害 + 无限穿透"无现成实现，需专用分支。
- 子弹为瞬时对象（`Board::AddProjectile` → `DataArrayAlloc`），**不写入存档**；新增枚举值在 `NUM_PROJECTILES` 哨兵之前，旧存档不受影响。
- 火炬树桩只转换 `PROJECTILE_PEA` / `PROJECTILE_SNOWPEA`（`PeaAboutToHitTorchwood`），新类型天然不被转换。
- 渲染：`Projectile::BeginDraw` 平移画布到 `(mX, mY)`；`TodBltMatrix`（`src/Sexy.TodLib/TodCommon.cpp:710`）支持乘色，`mColorOverride` 同款 `ColorsMultiply` 语义。
- 粒子：`PARTICLE_FIREBALL_TRAIL`（`particles/Fireball_Trail.xml`，Boss 火球同款拖尾）是游戏内唯一的火焰拖尾粒子。

## 设计

### 1. 新枚举值

`src/ConstEnums.h` `enum ProjectileType`（808 行起）：在 `PROJECTILE_ZOMBIE_PEA = 13` 之后、`NUM_PROJECTILES` 之前新增：

```cpp
PROJECTILE_FIREPEA_RED = 14,   // 红色火豌豆（机枪射手 1% 概率，灼烧穿刺）
NUM_PROJECTILES = 15
```

不重排任何现有值。

### 2. 子弹定义

`src/Lawn/Projectile.cpp` `gProjectileDefinition[]` 末尾追加：

```cpp
{ ProjectileType::PROJECTILE_FIREPEA_RED, 0, 1 },
```

`mImageRow = 0`（绘制不走定义行）、`mDamage = 1`（与灼烧伤害一致；实际伤害走专用逻辑）。

### 3. 生成与 1% 判定（`Plant::Fire()`）

新增一个局部判定（普通模式在 `else` 分支、散射在 `SCATTER_COUNT` 循环内各调用一次，保证每颗独立掷骰）：

```cpp
auto RollGatlingBulletType = [this](ProjectileType theBaseType) -> ProjectileType
{
    if (mSeedType == SeedType::SEED_GATLINGPEA && Rand(100) < 1)
        return ProjectileType::PROJECTILE_FIREPEA_RED;
    return theBaseType;
};
```

- 普通模式：`AddProjectile(..., RollGatlingBulletType(aProjectileType))`。
- 散射模式：循环内每颗 `AddProjectile(..., RollGatlingBulletType(aProjectileType))`。
- 音效：掷中时该发播放 `FOLEY_FIREPEA`（普通模式以 `FOLEY_FIREPEA` 替代当次 `FOLEY_THROW`；散射模式掷中时额外播放 `FOLEY_FIREPEA`）。
- 散射/孤狼模式的 `mDamageOverride = 200` 对红火豌豆不生效（灼烧固定每帧 1 点，保持设定）。

### 4. 初始化与外观

`ProjectileInitialize` 的 switch 新增 case（参照 `PROJECTILE_SNOWPEA` 的拖尾写法）：

```cpp
case ProjectileType::PROJECTILE_FIREPEA_RED:
{
    TodParticleSystem* aParticle = mApp->AddTodParticle(mPosX + 8.0f, mPosY + 13.0f, 400000, ParticleEffect::PARTICLE_FIREBALL_TRAIL);
    AttachParticle(mAttachmentID, aParticle, 8.0f, 13.0f);
    break;
}
```

`Draw()`：
- switch case：`aImage = IMAGE_PROJECTILEPEA;`。
- 绘制块内，该类型走 `TodBltMatrix` 路径并以红色乘色（`Color(255, 96, 96)`，可调），旋转/缩放沿用现有矩阵代码，位置与现有旋转路径一致：

```cpp
if (mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
{
    float aOffsetX = mPosX + aCelWidth * 0.5f;
    float aOffsetY = mPosZ + mPosY + aCelHeight * 0.5f;
    SexyTransform2D aTransform;
    TodScaleRotateTransformMatrix(aTransform, aOffsetX + mBoard->mX, aOffsetY + mBoard->mY, mRotation, aScale, aScale);
    TodBltMatrix(g, aImage, aTransform, g->mClipRect, Color(255, 96, 96), g->mDrawMode, aSrcRect);
}
else if (FloatApproxEqual(mRotation, 0.0f) && FloatApproxEqual(aScale, 1.0f)) { /* 原快速路径 */ }
else { /* 原旋转路径 */ }
```

其余沿用豌豆分支：
- `DrawShadow()`：pea 分支并入新类型（+3 偏移）。
- `GetProjectileRect()`：pea 分支并入新类型（`Rect(mX - 15, mY, mWidth + 15, mHeight)`）。

### 5. 灼烧 + 无限穿透（`CheckForCollision()`）

在出屏判定之后、通用 `FindCollisionTarget()` 流程之前插入专用分支：

```cpp
if (mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
{
    // 灼烧穿刺：每帧对矩形重叠的每个僵尸造成 1 点伤害；无限穿透，永不因击中而消亡
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
    return;  // 无限穿透：不 Die
}
```

要点：
- `GetDamageFlags` 对该类型返回 0（非溅射/非冰冻/直射不绕盾）→ 铁桶/路障等盾牌先吃伤害，与豌豆一致。
- 多个僵尸同时重叠 → 全部同时每帧受 1 点伤害。
- 消亡时机：出屏（既有判定）、高地过渡（见下）。

### 6. 高地一致性

- `CantHitHighGround()` 列表并入新类型（与 `PROJECTILE_FIREBALL` 一致，低处打不到高台僵尸）。
- `CheckForHighGround()` 的 `aShadowDelta < 28.0f` 列表并入新类型（高地过渡时 `DoImpact(nullptr)` → 正常 `Die()`，与豌豆/火球一致）。
- `Update()` 的 `aTime = 0` 列表并入新类型（已核实 `aTime` 仅控制 `mRenderOrder` 何时进入 `RENDER_LAYER_PROJECTILE` 层，豌豆类立即更新；红火豌豆与豌豆一致）。

### 7. 范围与边界（明确不做）

- **不**作用于魅惑机枪头僵尸（`ZOMBIE_GATLING_HEAD`，`Zombie.cpp` 独立发射路径）——用户仅要求机枪射手植物。
- **不**被火炬树桩转换（已天然排除）。
- **不**绕过盾牌。
- **不**新增 `Projectile` 数据成员、不修改 `ConstEnums.h` 既有值、不新建类继承、不写入存档。

## 测试

- 普通模式：连续射击观察（1% 概率较低，可临时把 `Rand(100) < 1` 改为 `Rand(100) < 50` 验证后改回，或直接改判据跑一次）：
  1. 红火豌豆外观：红色豌豆 + 火焰拖尾。
  2. 灼烧：单只僵尸被红火豌豆穿过时血量逐帧 -1（接触期间），未接触不扣血。
  3. 无限穿透：子弹穿过整排所有僵尸后从屏幕右侧飞出；出屏后消失；无残留粒子。
  4. 盾牌僵尸（铁桶/路障）：先扣盾牌，盾破后扣本体。
  5. 高台关卡（屋顶）：低处发射打不到高台僵尸；高地过渡处子弹消亡。
  6. 大招散射模式：每颗散射子弹独立判定；掷中时播放 `FOLEY_FIREPEA`。
- 回归：普通豌豆外观/伤害/音效不变；非机枪射手植物（豌豆射手、寒冰射手等）行为不变；火炬树桩转换不受影响。
- 存档：读旧存档正常（子弹不入档，无需迁移）。
