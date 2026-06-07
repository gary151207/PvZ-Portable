# GatlingPea 三弹齐射设计

**日期:** 2026-06-07  
**状态:** 已确认

## 需求

GatlingPea 每次攻击在同一行发射 3 颗豌豆，垂直堆叠排列，同时齐射。每颗豌豆伤害与普通豌豆相同（20）。

## 改动范围

仅修改 `src/Lawn/Plant.cpp` 中的 `Fire()` 函数。

## 实现方案

在 `Fire()` 的 GatlingPea 发射原点分支中，循环 3 次创建 projectile，Y 坐标分别偏移 -8, 0, +8。

### 当前代码（参考）

```cpp
else if (mSeedType == SeedType::SEED_GATLINGPEA)
{
    int aOffsetX, aOffsetY;
    GetPeaHeadOffset(aOffsetX, aOffsetY);
    aOriginX = mX + aOffsetX + 34;
    aOriginY = mY + aOffsetY - 33;
}
```

### 目标代码

```cpp
else if (mSeedType == SeedType::SEED_GATLINGPEA)
{
    int aOffsetX, aOffsetY;
    GetPeaHeadOffset(aOffsetX, aOffsetY);
    int aBaseX = mX + aOffsetX + 34;
    int aBaseY = mY + aOffsetY - 33;
    
    for (int i = 0; i < 3; i++)
    {
        int aY = aBaseY + (i - 1) * 8;  // -8, 0, +8
        Projectile* aProjectile = mBoard->AddProjectile(aBaseX, aY, mRenderOrder - 1, theRow, ProjectileType::PROJECTILE_PEA);
        aProjectile->mDamageRangeFlags = GetDamageRangeFlags(thePlantWeapon);
    }
}
```

**注意:** 后续的 `AddProjectile` 调用（原第 4676 行）和 projectile 属性设置（如 cabbage/kernel 的弧线轨迹）需要跳过 GatlingPea，因为 projectile 已在循环中创建。

## 关键约束

- 音效（`PlayFoley`）和粒子效果保持在循环之外，只播放一次
- `UpdateShooter()` 无需修改——GatlingPea 走通用 `else` 分支，`mLaunchCounter <= 0` 时调用 `FindTargetAndFire` → `Fire()`
- 不修改 `ConstEnums.h`（SEED_GATLINGPEA 已存在）
- 不修改 `PlantDefinition` 表（配置已存在）

## 伤害

- 每颗 PROJECTILE_PEA 伤害 = 20（由 `ProjectileDefinition` 表决定）
- 3 颗全中合计 60 伤害
