# 植物血量悬停提示实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为植物添加鼠标悬停显示血量提示功能，样式与僵尸血量提示一致。

**Architecture:** 新增 `Board::PlantHitTest()` 方法（镜像 `ZombieHitTest`），在 `Board::UpdateToolTip()` 的僵尸分支之后插入植物分支，复用 `ToolTipWidget` 渲染。

**Tech Stack:** C++20, SDL2, 现有 Lawn 层框架

## Global Constraints

- 遵循项目代码风格：`mMember` 命名、`/*inline*/` 注释、`#ifndef __FOO_H__` 头文件守卫
- 每文件改动需保留 LGPL-3.0-or-later 许可证头
- 不重排 `ConstEnums.h` 的值
- 模仿者植物需传双参数 `GetNameString(mSeedType, mImitaterType)` 显示正确名称
- 无自动化测试套件，需手动 gameplay 测试

---

## Task 1: 声明 `PlantHitTest` 方法

**Files:**
- Modify: `src/Lawn/Board.h`

- [ ] **Step 1: 在 Board.h 中添加方法声明**

在 `Board.h` 的 public 方法区域中，找到 `ZombieHitTest` 声明（约 `Zombie* ZombieHitTest(int theMouseX, int theMouseY)`）之后，添加：

```cpp
Plant*                     PlantHitTest(int theMouseX, int theMouseY);
```

- [ ] **Step 2: 提交**

```bash
git add src/Lawn/Board.h
git commit -m "feat(plant): declare PlantHitTest method"
```

---

## Task 2: 实现 `PlantHitTest` 方法

**Files:**
- Modify: `src/Lawn/Board.cpp`（在 `ZombieHitTest` 实现之后）

- [ ] **Step 1: 实现 PlantHitTest**

在 `Board::ZombieHitTest` 函数实现之后（`Board.cpp` 约第 3195 行 `}` 之后），添加：

```cpp
Plant* Board::PlantHitTest(int theMouseX, int theMouseY)
{
    Plant* aPlant = nullptr;
    Plant* aRecord = nullptr;
    while (IteratePlants(aPlant))
    {
        if (aPlant->mDead)
            continue;

        if (aPlant->GetPlantRect().Contains(theMouseX, theMouseY))
        {
            if (aRecord == nullptr || aPlant->mY > aRecord->mY)
            {
                aRecord = aPlant;
            }
        }
    }

    return aRecord;
}
```

逻辑与 `ZombieHitTest` 完全镜像：遍历植物 → 跳过死亡 → 矩形包含检测 → 取最靠前（Y 最大）。

- [ ] **Step 2: 提交**

```bash
git add src/Lawn/Board.cpp
git commit -m "feat(plant): implement PlantHitTest with rect containment and Y priority"
```

---

## Task 3: 在 `UpdateToolTip` 插入植物提示分支

**Files:**
- Modify: `src/Lawn/Board.cpp`（`Board::UpdateToolTip` 方法内）

- [ ] **Step 1: 定位插入点**

在 `UpdateToolTip()` 中找到以下代码段（僵尸提示块之后、`mChallenge->UpdateToolTip` 之前）：

```cpp
	mToolTip->mCenter = true;
	mToolTip->mVisible = true;
	return;
}

if (mChallenge->UpdateToolTip(aMouseX, aMouseY))
{
```

- [ ] **Step 2: 在两者之间插入植物提示分支**

在上述 `return;` 与 `if (mChallenge->...)` 之间插入：

```cpp
	Plant* aPlant = PlantHitTest(aMouseX, aMouseY);
	if (aPlant)
	{
		mToolTip->SetTitle(Plant::GetNameString(aPlant->mSeedType, aPlant->mImitaterType));
		mToolTip->SetLabel(StrFormat("HP: %d/%d", aPlant->mPlantHealth, aPlant->mPlantMaxHealth));
		mToolTip->SetWarningText("");

		Rect aRect = aPlant->GetPlantRect();
		mToolTip->mX = aRect.mWidth / 2 + aRect.mX + 5;
		mToolTip->mY = aRect.mHeight + aRect.mY - 10;
		mToolTip->mCenter = true;
		mToolTip->mVisible = true;
		return;
	}
```

- [ ] **Step 3: 提交**

```bash
git add src/Lawn/Board.cpp
git commit -m "feat(plant): show HP tooltip on plant hover in UpdateToolTip"
```

---

## Task 4: 编译验证 + 手动测试

**Files:** 无新文件，仅验证

- [ ] **Step 1: 编译**

```bash
cmake --build build
```

Expected: 编译通过，无错误。

- [ ] **Step 2: 手动 gameplay 测试**

运行游戏（需用户提供 `main.pak` 与 `properties/` 资源目录，`-resdir=PATH`）：

1. 进入冒险模式第一关，种植豌豆射手
2. 鼠标悬停豌豆射手 → 应显示 `[PEASHOOTER]` + `HP: 300/300`
3. 让僵尸攻击豌豆射手，再悬停 → HP 当前值应下降
4. 种植坚果墙（Wall-nut），悬停 → 应显示 `HP: 4000/4000`
5. 种植模仿者（Imitater），悬停 → 应显示模仿者名 + 被模仿植物名
6. 鼠标悬停南瓜壳（Pumpkinshell），悬停 → 显示南瓜壳血量 4000/4000
7. 鼠标移开植物 → 提示框消失
8. 僵尸与植物重叠时，悬停僵尸位置显示僵尸 HP（僵尸优先）
9. 僵尸死亡后悬停该位置 → 应无僵尸 HP 提示（如后方有植物则显示植物 HP）
