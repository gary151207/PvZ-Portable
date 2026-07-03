# Tall-nut Counterattack Skill — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Tall-nut deals 100 damage to each zombie eating it every 500 ticks and heals 75 HP (capped).

**Architecture:** New member `mTallnutCounterCooldown` + new method `UpdateTallnut()` on `Plant`, hooked into `UpdateAbilities()` seed-type branch.

**Tech Stack:** C++20, no new dependencies.

## Global Constraints

- Do not subclass plants. Use enum-branching in `UpdateAbilities()`.
- Do not reorder `ConstEnums.h`. No new enum values.
- No new files. Only modify `Plant.h` and `Plant.cpp`.

---

### Task 1: Add member variable and method declaration to Plant.h

**Files:**
- Modify: `src/Lawn/Plant.h`

**Interfaces:**
- Produces: `int32_t mTallnutCounterCooldown` (member), `void UpdateTallnut()` (method)

- [ ] **Step 1: Add member variable**

After `mRecentlyEatenCountdown` (~line 199), insert:
```cpp
    int32_t                 mTallnutCounterCooldown;
```

- [ ] **Step 2: Add method declaration**

After `UpdateLilypad();` (~line 319), insert:
```cpp
    void                    UpdateTallnut();
```

- [ ] **Step 3: Verify**

Run: build check. Expected: compiles (with undefined `UpdateTallnut` if not yet implemented in .cpp — this is fine at this stage).

- [ ] **Step 4: Commit**

```bash
git add src/Lawn/Plant.h
git commit -m "feat: add tall-nut counterattack member and declaration"
```

---

### Task 2: Implement UpdateTallnut and hook into UpdateAbilities

**Files:**
- Modify: `src/Lawn/Plant.cpp`

**Interfaces:**
- Consumes: `mTallnutCounterCooldown`, `mBoard->IterateZombies()`, `mBoard->mPlants.DataArrayGetID()`, `Zombie::mTargetPlantID`, `Zombie::TakeDamage()`
- Produces: `Plant::UpdateTallnut()`, hook in `UpdateAbilities()`

- [ ] **Step 1: Initialize mTallnutCounterCooldown in constructor**

After `mRecentlyEatenCountdown = 0;` (~line 148), insert:
```cpp
    mTallnutCounterCooldown = 0;
```

- [ ] **Step 2: Add UpdateTallnut() implementation**

Insert before `UpdateAbilities()` or near other `Update*` methods. At a natural location — near `AnimateNuts()` (~line 3079):
```cpp
void Plant::UpdateTallnut()
{
    if (mTallnutCounterCooldown > 0)
    {
        mTallnutCounterCooldown--;
        return;
    }

    bool aHasEater = false;
    Zombie* aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie))
    {
        if (aZombie->mTargetPlantID == (PlantID)mBoard->mPlants.DataArrayGetID(this))
        {
            aZombie->TakeDamage(100, 0U);
            aHasEater = true;
        }
    }

    if (aHasEater)
    {
        mPlantHealth += 75;
        if (mPlantHealth > mPlantMaxHealth)
            mPlantHealth = mPlantMaxHealth;
    }

    mTallnutCounterCooldown = 500;
}
```

- [ ] **Step 3: Hook into UpdateAbilities()**

In `Plant::UpdateAbilities()`, after the `SEED_TANGLEKELP` branch (~line 2547), insert:
```cpp
    else if (mSeedType == SeedType::SEED_TALLNUT)                                           UpdateTallnut();
```

- [ ] **Step 4: Build**

```bash
cmake --build build
```

Expected: compile + link success.

- [ ] **Step 5: Commit**

```bash
git add src/Lawn/Plant.cpp
git commit -m "feat: implement tall-nut counterattack every 5s"
```
