# 设计：火炬树桩每影响 100 颗子弹 → 在自身格子生成魅惑机枪头僵尸

日期：2026-08-29
状态：已批准（2026-08-29 用户确认设计）

## 目标

给火炬树桩（Torchwood）添加一个被动技能：**每个火炬树桩每影响 100 颗子弹，就在它自己的格子上生成一只被魅惑的机枪头僵尸（ZOMBIE_GATLING_HEAD）**，该僵尸为友军，向右走（回头迎战）并攻击敌方僵尸，效果与魅惑菇（Hypno-shroom）魅惑的僵尸完全一致。

## 术语

- **影响一颗子弹**：一颗豌豆（`PROJECTILE_PEA`）被火炬变成火焰豌豆（`PROJECTILE_FIREBALL`），或一颗冰豌豆（`PROJECTILE_SNOWPEA`）被火炬变回普通豌豆。两种情况都计数（用户选择 B）。
- **每个火炬独立计数**（用户选择 A）：每颗子弹对经过的每个火炬各计 1 次。一颗子弹先后经过两个火炬时，两个火炬各计 1 次。

## 机制背景（现状）

- `Plant::UpdateTorchwood()`（`src/Lawn/Plant.cpp`）每帧遍历弹道，把与攻击判定框重叠 ≥10 像素的豌豆/冰豌豆转换为火球/豌豆，分别调用 `Projectile::ConvertToFireball(mPlantCol)` / `ConvertToPea(mPlantCol)`。
- `ConvertToFireball`/`ConvertToPea` 内部有去重保护：`if (mHitTorchwoodGridX == theGridX) return;`，保证每颗子弹对每个火炬只转换一次。这天然可作为"已计数"标志。
- 魅惑参考实现：`Zombie::StartMindControlled()`（`src/Lawn/Zombie.cpp:7170`）设置 `mMindControlled = true` 并播放 `SOUND_MINDCONTROLLED`；魅惑菇路径（`Zombie.cpp:4964`）额外添加 `PARTICLE_MIND_CONTROL` 粒子、`mVelX = 0.17f`、`mAnimTicksPerFrame = 18` + `UpdateAnimSpeed()`，僵尸向右行走（`IsWalkingBackwards()` 因 `mMindControlled` 返回 true → `mPosX += aSpeed`）并攻击僵尸。
- 存档：植物序列化为 TLV 字段（`SyncPlantsPortable`），尾部字段 `SyncPlantTailPortable`（`src/Lawn/System/SaveGame.cpp`）为位置式同步。读旧存档时尾部字段若短于新格式，`SyncInt32` 抛 `DataReaderException` → 字段归零、`mFailed=true`，但调用方忽略返回值 → **旧存档可正常读取，计数器归零，不报错**（已验证 `SyncDataArrayPortableTLV` 的读取循环）。因此把新字段追加到尾部字段末尾即可向后兼容。

## 设计

### 1. 数据成员

`src/Lawn/Plant.h` 的 `Plant` 类新增：

```cpp
int32_t  mTorchwoodPeaCount;   // 火炬树桩已影响的子弹计数（仅火炬使用）
```

- `PlantInitialize`（`src/Lawn/Plant.cpp:107`）初始化为 0。
- 存档：在 `SyncPlantTailPortable` 末尾追加 `theContext.SyncInt32(thePlant.mTorchwoodPeaCount);`。
  - 写：新存档包含该字段。
  - 读：旧存档缺字段 → 自动归零，兼容；新存档正常恢复。

### 2. 计数（`Plant::UpdateTorchwood()`）

在转换分支计数，复用去重保护，每颗子弹每个火炬恰计一次：

```cpp
if (GetRectOverlap(aAttackRect, aProjectileRect) >= 10)
{
    bool aAlreadyCounted = (aProjectile->mHitTorchwoodGridX == mPlantCol);
    if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_PEA)
        aProjectile->ConvertToFireball(mPlantCol);
    else if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_SNOWPEA)
        aProjectile->ConvertToPea(mPlantCol);
    if (!aAlreadyCounted)
    {
        mTorchwoodPeaCount++;
        if (mTorchwoodPeaCount >= 100)
            SpawnCharmedGatlingZombie();   // 内部计数归零
    }
}
```

说明：`mHitTorchwoodGridX` 在转换前判断"这发子弹是否已被本火炬处理过"；转换后才由 Convert 函数写入该值，因此 `aAlreadyCounted` 判断与 Convert 内部去重一致。

### 3. 生成（新私有方法 `Plant::SpawnCharmedGatlingZombie()`）

```cpp
void Plant::SpawnCharmedGatlingZombie()
{
    Zombie* aZombie = mBoard->AddZombieInRow(ZombieType::ZOMBIE_GATLING_HEAD, mRow, Zombie::ZOMBIE_WAVE_DEBUG);
    if (aZombie == nullptr)   // 僵尸数组已满：不归零，攒到下一个 100 再试
        return;

    mTorchwoodPeaCount = 0;
    aZombie->mPosX = mBoard->GridToPixelX(mPlantCol, mRow);   // 摆到火炬格子
    aZombie->StartMindControlled();                            // 魅惑：音效 + mMindControlled
    mApp->AddTodParticle(aZombie->mPosX + 60.0f, aZombie->mPosY + 40.0f,
        aZombie->mRenderOrder + 1, ParticleEffect::PARTICLE_MIND_CONTROL);
    aZombie->mVelX = 0.17f;
    aZombie->mAnimTicksPerFrame = 18;
    aZombie->UpdateAnimSpeed();
}
```

要点：
- `ZOMBIE_WAVE_DEBUG` 波次号与调试召唤一致，不影响正常波次/旗帜/`mZombiesLeftToSpawn` 逻辑。
- 生成失败（数组满，`AddZombieInRow` 返回 `nullptr`）时不归零，下次攒满 100 再试。
- 魅惑参数（粒子、速度、动画速率）逐项复刻魅惑菇路径。
- 方法放入 `Plant.cpp`/`Plant.h` 私有区；无需在 `ConstEnums.h` 或 `Zombie` 类做任何改动。

### 4. 边界

- 不限制游戏模式：火炬只能由玩家在战斗地图种下（"我是僵尸"模式种不了植物），天然只在玩家侧生效。
- 火炬被吃掉/铲掉/消失 → 计数器随之消失（成员随 Plant 生命周期）。
- 多火炬同行：各自独立计数（去重键是各自 `mPlantCol`）。
- 已生效的火球经过第二个火炬：`UpdateTorchwood` 只处理 `PEA`/`SNOWPEA`，不计数——语义正确（第二个火炬未"影响"它）。
- 不影响任何波次、旗帜、成就逻辑。
- 计数在 `0..99` 与 `100` 之间循环；达到 100 即生成并归零。

## 测试

手动测试（无自动化测试套件）：

1. **基本功能**：冒险模式，种 1 个火炬树桩 + 1 个机枪射手，让豌豆穿过火炬。数到 100 发后：
   - 火炬格子上出现一只机枪头僵尸；
   - 播放魅惑音效、绿色旋涡粒子；
   - 僵尸向右走并攻击敌方僵尸；
   - 计数器归零，继续攒下一个 100。
2. **冰豌豆计数**：换冰豌豆射手（或冰冻生菜+火炬组合），确认冰豌豆穿过火炬也计数（设计 B）。
3. **多火炬独立计数**：同行种 2 个火炬，确认各自独立计数、各自在自身格子生成。
4. **存档/读档**：计数器在保存后恢复；加载旧版本存档不报错（计数器归零）。
5. **边界**：僵尸数量接近上限时生成失败不崩溃、不归零，后续可继续触发。
6. **回归**：无火炬时无任何行为变化；火炬正常转换豌豆/冰豌豆功能不受影响。

## 非目标

- 不加 UI（无计数器显示、无提示文字）。
- 不引入新资源（音效/粒子均复用现有）。
- 不改动 `ConstEnums.h`、存档版本号、`Zombie` 类或生成系统。
