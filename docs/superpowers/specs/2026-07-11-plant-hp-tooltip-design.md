# 植物血量悬停提示设计

**日期:** 2026-07-11
**状态:** 待实现
**参考:** 僵尸血量提示（`Board::UpdateToolTip` 中的僵尸分支）

## 目标

为植物添加鼠标悬停显示血量功能，样式与现有僵尸血量提示完全一致。

## 现状分析

### 僵尸血量提示（已有）

- 位置：`src/Lawn/Board.cpp` → `Board::UpdateToolTip()`
- 命中检测：`Board::ZombieHitTest()` — 遍历僵尸，跳过死亡，用 `GetZombieRect().Contains()` 检测，取 Y 最大者
- 提示内容：标题 = 僵尸名（`GetZombieDefinition().mZombieName`），标签 = `HP: 总当前血量/总最大血量`（body + helm + shield + flying）
- 渲染：复用 `ToolTipWidget`，黄色背景黑框，标题 + 标签两行

### 植物血量数据（已有）

- `Plant::mPlantHealth` — 当前血量
- `Plant::mPlantMaxHealth` — 最大血量
- 初始化：`Plant::PlantInitialize()` 中按 `SeedType` 设定（默认 300，坚果类 4000 等），结束时 `mPlantMaxHealth = mPlantHealth`
- 命中检测：`Plant::GetPlantRect()` 返回植物矩形
- 名称：`Plant::GetNameString(SeedType)` 返回本地化名称如 `[PEASHOOTER]`

## 设计

### 改动文件

| 文件 | 改动 |
|------|------|
| `src/Lawn/Board.h` | 声明 `Plant* PlantHitTest(int theMouseX, int theMouseY)` |
| `src/Lawn/Board.cpp` | 实现 `PlantHitTest`；在 `UpdateToolTip` 中加入植物提示分支 |

### 新增方法 `Board::PlantHitTest`

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

逻辑与 `ZombieHitTest` 完全镜像：遍历 → 跳过死亡 → 矩形包含 → 取最靠前（Y 最大）。

### 修改 `Board::UpdateToolTip`

在僵尸提示块之后、`mChallenge->UpdateToolTip` 之前插入：

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

### 执行顺序

```
僵尸检测 → 命中则显示僵尸 HP 提示，return
    ↓ 未命中
植物检测 → 命中则显示植物 HP 提示，return   ← 新增
    ↓ 未命中
mChallenge->UpdateToolTip(...)
    ↓
MouseHitTest → switch(对象类型) ...
```

僵尸优先于植物（与渲染层级一致：僵尸通常在植物前方）。

## 不做的事

- 不新增配置开关或命令行参数
- 不改动 `ToolTipWidget` 渲染逻辑
- 不进入选卡界面（`SeedChooserScreen` 已有独立提示）
- 不显示血量百分比或血条（保持与僵尸一致的文字格式）
- 不处理"僵尸与植物重叠时显示哪个"的复杂情况（Y 坐标优先已足够）

## 风险与注意事项

- `GetPlantRect()` 对部分植物（Tallnut、Pumpkin、Cobcannon）有特殊矩形，与僵尸的 `GetZombieRect()` 类似，行为合理
- 睡莲盆/花盆上的植物：`GetPlantRect` 返回的是该植物自身矩形，命中检测按实际植物位置，符合直觉
- 南瓜壳套种：南瓜壳是独立植物，悬停南瓜壳位置显示南瓜壳血量，悬停内部植物位置（若露出来）显示内部植物血量
