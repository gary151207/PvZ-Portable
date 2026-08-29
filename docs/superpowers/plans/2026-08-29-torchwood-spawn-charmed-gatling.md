# 火炬树桩每影响 100 颗子弹生成魅惑机枪头僵尸 — 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 每个火炬树桩每影响 100 颗子弹（豌豆→火球、冰豌豆→普通豌豆都计，每个火炬独立计数），在自身格子生成一只被魅惑的机枪头僵尸（`ZOMBIE_GATLING_HEAD`），效果复刻魅惑菇。

**Architecture:** 在 `Plant` 上新增计数器成员，在 `Plant::UpdateTorchwood()` 的转换分支里计数（复用 `Projectile::mHitTorchwoodGridX` 去重保证每颗子弹每火炬恰计一次）；满 100 调用新方法 `Plant::SpawnCharmedGatlingZombie()`，通过 `Board::AddZombieInRow` 生成僵尸、摆到火炬格子、调用 `Zombie::StartMindControlled()` 复刻魅惑效果。计数器随存档 TLV 尾部字段持久化（追加到末尾，旧存档兼容归零）。

**Tech Stack:** C++20, CMake/Ninja, SDL2。**无自动化测试套件** —— 每个任务的"测试"= 编译通过 + 手动游戏验证（任务 4 汇总）。

## Global Constraints

- 不改动 `ConstEnums.h`、存档版本号（`SAVE_FILE_V4_VERSION`）、`Zombie` 类或生成系统。
- 命名遵循 SexyAppFramework 惯例：成员 `mXxx`。
- 新存档字段必须追加在 `SyncPlantTailPortable` **末尾**（保证旧存档兼容：读旧存档时该字段归零、不报错）。
- 不引入新资源（音效/粒子全部复用：`SOUND_MINDCONTROLLED`、`PARTICLE_MIND_CONTROL`）。
- 波次号用 `Zombie::ZOMBIE_WAVE_DEBUG`，不影响正常波次/旗帜逻辑。
- 生成失败（僵尸数组满，`AddZombieInRow` 返回 `nullptr`）时**不归零**计数器。
- 构建命令：`cmake --build build`（已配置 Release/Ninja）；可执行文件 `build/pvz-portable.exe`。

---

### Task 1: 新增计数器成员与初始化

**Files:**
- Modify: `src/Lawn/Plant.h:186`（`mLaunchCounter` 之后）
- Modify: `src/Lawn/Plant.cpp:128`（`PlantInitialize` 中 `mShootingCounter = 0;` 之后）

**Interfaces:**
- Consumes: 无
- Produces: `Plant::mTorchwoodPeaCount`（`int32_t`，公共成员，仅火炬使用，默认 0）—— Task 2、3 依赖

- [ ] **Step 1: 在 `Plant.h` 添加成员**

在 `mLaunchCounter` 与 `mLaunchRate` 之间插入（约 186 行）：

```cpp
    int32_t                 mLaunchCounter;
    int32_t                 mTorchwoodPeaCount;   // 火炬树桩已影响的子弹计数（仅火炬使用）
    int32_t                 mLaunchRate;
```

- [ ] **Step 2: 在 `PlantInitialize` 初始化**

在 `src/Lawn/Plant.cpp` 的 `Plant::PlantInitialize` 中，`mShootingCounter = 0;` 之后插入：

```cpp
    mShootingCounter = 0;
    mTorchwoodPeaCount = 0;
```

- [ ] **Step 3: 编译验证**

Run: `cmake --build build`
Expected: 编译成功，无错误无新警告。

- [ ] **Step 4: 提交**

```bash
git add src/Lawn/Plant.h src/Lawn/Plant.cpp
git commit -m "feat(torchwood): add per-torchwood bullet counter member"
```

---

### Task 2: 计数逻辑与魅惑生成

**Files:**
- Modify: `src/Lawn/Plant.h:278`（`UpdateTorchwood` 声明附近，新增方法声明）
- Modify: `src/Lawn/Plant.cpp:1432-1455`（`UpdateTorchwood` 函数体）
- Modify: `src/Lawn/Plant.cpp`（`UpdateTorchwood` 与 `DoSquashDamage` 之间新增方法定义）

**Interfaces:**
- Consumes: `Plant::mTorchwoodPeaCount`（Task 1）；`Board::AddZombieInRow(ZombieType, int, int)`（`src/Lawn/Board.h:374`，public）；`Board::GridToPixelX(int, int)`（`src/Lawn/Board.h:321`）；`Zombie::StartMindControlled()`、`Zombie::UpdateAnimSpeed()`、`Zombie::mPosX/mPosY/mVelX/mAnimTicksPerFrame/mRenderOrder`（均 public，`src/Lawn/Zombie.cpp:7170` 魅惑菇路径为参考）；`LawnApp::AddTodParticle(float, float, int, ParticleEffect)`；`ParticleEffect::PARTICLE_MIND_CONTROL`；`Zombie::ZOMBIE_WAVE_DEBUG`
- Produces: `Plant::SpawnCharmedGatlingZombie()`（私有方法，无参数无返回值）—— Task 4 手动验证依赖

- [ ] **Step 1: 在 `Plant.h` 声明新方法**

在 `void UpdateTorchwood();`（约 278 行）之后插入：

```cpp
    void                    UpdateTorchwood();
    void                    SpawnCharmedGatlingZombie();
```

- [ ] **Step 2: 修改 `Plant::UpdateTorchwood()` 加入计数**

把 `src/Lawn/Plant.cpp` 的 `Plant::UpdateTorchwood()` 函数体（1432 行起）整体替换为：

```cpp
void Plant::UpdateTorchwood()
{
    Rect aAttackRect = GetPlantAttackRect(PlantWeapon::WEAPON_PRIMARY);

    Projectile* aProjectile = nullptr;
    while (mBoard->IterateProjectiles(aProjectile))
    {
        if (aProjectile->mRow != mRow)
            continue;

        if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_PEA || aProjectile->mProjectileType == ProjectileType::PROJECTILE_SNOWPEA)
        {
            Rect aProjectileRect = aProjectile->GetProjectileRect();
            if (GetRectOverlap(aAttackRect, aProjectileRect) >= 10)
            {
                bool aAlreadyCounted = (aProjectile->mHitTorchwoodGridX == mPlantCol);
                if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_PEA)
                {
                    aProjectile->ConvertToFireball(mPlantCol);
                }
                else if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_SNOWPEA)
                {
                    aProjectile->ConvertToPea(mPlantCol);
                }
                if (!aAlreadyCounted)  // 每颗子弹每个火炬只计一次（与 Convert 内部去重一致）
                {
                    mTorchwoodPeaCount++;
                    if (mTorchwoodPeaCount >= 100)
                    {
                        SpawnCharmedGatlingZombie();
                    }
                }
            }
        }
    }
}
```

- [ ] **Step 3: 新增 `Plant::SpawnCharmedGatlingZombie()` 定义**

在 `UpdateTorchwood()` 结束的 `}` 与 `void Plant::DoSquashDamage()` 之间插入：

```cpp
void Plant::SpawnCharmedGatlingZombie()
{
    Zombie* aZombie = mBoard->AddZombieInRow(ZombieType::ZOMBIE_GATLING_HEAD, mRow, Zombie::ZOMBIE_WAVE_DEBUG);
    if (aZombie == nullptr)  // 僵尸数组已满：不归零，攒到下一个 100 再试
        return;

    mTorchwoodPeaCount = 0;
    aZombie->mPosX = mBoard->GridToPixelX(mPlantCol, mRow);  // 摆到火炬所在格子

    // 以下复刻魅惑菇路径（src/Lawn/Zombie.cpp:4964）
    aZombie->StartMindControlled();  // 魅惑：mMindControlled = true + SOUND_MINDCONTROLLED
    mApp->AddTodParticle(aZombie->mPosX + 60.0f, aZombie->mPosY + 40.0f, aZombie->mRenderOrder + 1, ParticleEffect::PARTICLE_MIND_CONTROL);
    aZombie->mVelX = 0.17f;
    aZombie->mAnimTicksPerFrame = 18;
    aZombie->UpdateAnimSpeed();
}
```

- [ ] **Step 4: 编译验证**

Run: `cmake --build build`
Expected: 编译成功。若 `Zombie::ZOMBIE_WAVE_DEBUG` 或 `AddTodParticle` 报未声明，确认 `src/Lawn/Plant.cpp` 已 `#include "Zombie.h"`（已存在，第 25 行）且 `mApp` 类型为 `LawnApp*`（已存在，第 32 行 include）。

- [ ] **Step 5: 提交**

```bash
git add src/Lawn/Plant.h src/Lawn/Plant.cpp
git commit -m "feat(torchwood): spawn charmed gatling-head zombie every 100 affected bullets"
```

---

### Task 3: 计数器存档持久化

**Files:**
- Modify: `src/Lawn/System/SaveGame.cpp:912`（`SyncPlantTailPortable` 末尾，`SyncBool(thePlant.mHighlighted);` 之后）

**Interfaces:**
- Consumes: `Plant::mTorchwoodPeaCount`（Task 1）
- Produces: 存档尾部字段新增 4 字节（int32），新存档可恢复计数器；旧存档读取时该字段缺失 → 归零、不报错（`SyncInt32` 捕获 `DataReaderException` 置 `mFailed`，调用方忽略返回值——见 `src/Lawn/System/SaveGame.cpp:1028-1053` 与 1415-1440）

- [ ] **Step 1: 在 `SyncPlantTailPortable` 末尾追加同步**

在函数体末尾 `theContext.SyncBool(thePlant.mHighlighted);` 之后、`}` 之前插入（注意该文件用 Tab 缩进）：

```cpp
	theContext.SyncBool(thePlant.mHighlighted);
	theContext.SyncInt32(thePlant.mTorchwoodPeaCount);
}
```

- [ ] **Step 2: 编译验证**

Run: `cmake --build build`
Expected: 编译成功。

- [ ] **Step 3: 提交**

```bash
git add src/Lawn/System/SaveGame.cpp
git commit -m "feat(torchwood): persist per-torchwood bullet counter in save (backward compatible)"
```

---

### Task 4: 全量手动验证

**Files:** 无（验证任务）

**Interfaces:**
- Consumes: Task 1–3 全部产物

- [ ] **Step 1: Release 构建**

Run: `cmake --build build`
Expected: 成功，产物 `build/pvz-portable.exe`。

- [ ] **Step 2: 基本功能验证（冒险模式）**

运行 `build/pvz-portable.exe`（需要用户自备 `main.pak` 与 `properties/`，或用 `-resdir=PATH` 指向资源目录）。
进入冒险模式，种 1 个火炬树桩 + 1 个机枪射手（放火炬右侧），让豌豆穿过火炬，累计 100 发后确认：
1. 火炬格子上出现一只机枪头僵尸（`ZOMBIE_GATLING_HEAD`）；
2. 播放魅惑音效、出现绿色旋涡粒子（`PARTICLE_MIND_CONTROL`）；
3. 僵尸向左走，攻击敌方僵尸（打植物？——不应打植物）；
4. 计数器归零，继续攒下一个 100，再次生成。

- [ ] **Step 3: 冰豌豆计数验证（设计 B）**

用冰豌豆射手（火炬左侧），确认冰豌豆穿过火炬变回普通豌豆**且计入计数**。

- [ ] **Step 4: 多火炬独立计数验证（设计 A）**

同一行种 2 个火炬树桩，确认各自独立计数、各自在自身格子生成。

- [ ] **Step 5: 存档/读档验证**

游戏中途保存（含未满 100 的计数），读档后确认：计数保持；用旧版本（Task 3 之前）存档读档不报错（计数归零）。

- [ ] **Step 6: 边界与回归**

1. 用大量射手制造接近僵尸数组上限的场景，确认生成失败不崩溃、不归零，之后仍可触发；
2. 无火炬时游戏行为无变化；
3. 火炬对豌豆/冰豌豆的正常转换功能不受影响。

- [ ] **Step 7: 修复与收尾提交**

若验证发现问题，修复后重新构建验证，并提交：

```bash
git add -A
git commit -m "fix(torchwood): ..."
```
