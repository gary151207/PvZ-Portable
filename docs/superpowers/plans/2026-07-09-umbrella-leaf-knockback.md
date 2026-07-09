# Umbrella Leaf Proximity Knockback — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Umbrella Leaf (`SEED_UMBRELLA`) knocks back + deals 50 damage to every zombie in its own cell or the cell immediately to its right (same row), on a short cooldown, while keeping its pre-existing reflect mechanic intact.

**Architecture:** New `PlantState::STATE_UMBRELLA_KNOCKING` (appended at the end of the enum for save-compat) + two private helper methods `FindUmbrellaTarget()` / `DoUmbrellaKnockback()` on `Plant`. `UpdateUmbrella()` is extended with the new state driver and idle-trigger branch. No changes to `ConstEnums.h`.

**Tech Stack:** C++20, SDL2. No new dependencies, no new files.

## Global Constraints (from project rules)

- Do not subclass plants; no new subclasses needed here.
- Do not reorder `ConstEnums.h` values (no new enums there anyway).
- `PlantState` is serialized via `SyncEnum32` (`SaveGame.cpp:879`) — new state appended at END of enum only; existing integer values must not shift.
- Every `.h` / `.cpp` change follows existing style (`mMember`, 4-space indent, `/*inline*/` markers where used).
- No automated test suite → each task ends with a manual gameplay verification step from `docs/superpowers/specs/2026-07-09-umbrella-leaf-knockback-design.md` § Testing.

---

### Task 1: Add tuning constants to `GameConstants.h`

**Files:**
- Modify: `src/GameConstants.h` (near line 38, after `GLOOM_KNOCKBACK`)

**Interfaces:**
- Produces: `UMBRELLA_KNOCKBACK`, `UMBRELLA_KNOCKBACK_DAMAGE`, `UMBRELLA_KNOCKBACK_COOLDOWN`

- [ ] **Step 1: Add constants**

After the existing `GLOOM_KNOCKBACK` line (~line 38), insert:

```cpp
constexpr const float UMBRELLA_KNOCKBACK        = 40.0f;  // per-hit knockback distance for Umbrella Leaf (pixels, +X)
constexpr const int   UMBRELLA_KNOCKBACK_DAMAGE = 50;     // raw HP removed per hit
constexpr const int   UMBRELLA_KNOCKBACK_COOLDOWN = 90;   // frames of attack+cooldown per cycle (~0.9s @ 100fps)
```

Damage flag: use `0U` directly at call sites (matches pea's `GetDamageFlags` default — damage goes through shield→body normally). Do NOT add a named `UMBRELLA_DAMAGE_FLAGS` constant; inline `0U` with a short comment to avoid an ambiguous constant.

- [ ] **Step 2: Build check**

```bash
cmake --build build --target pvzportable 2>&1 | tail -20
```
Expected: compiles clean (constants unused yet → may trigger `-Wunused` only if the compiler warns on unused constexpr; ignore or confirm zero errors).

- [ ] **Step 3: Commit**

```bash
git add src/GameConstants.h
git commit -m "feat(umbrella): add knockback tuning constants"
```

---

### Task 2: Add new PlantState + Plant members/method declarations

**Files:**
- Modify: `src/Lawn/Plant.h`

**Interfaces:**
- Produces (enum): `STATE_UMBRELLA_KNOCKING`
- Produces (members): `bool mDoUmbrellaKnockbackFired`
- Produces (methods): `FindUmbrellaTarget() const`, `DoUmbrellaKnockback()`

- [ ] **Step 1: Append new state to PlantState enum**

In `src/Lawn/Plant.h` (~line 92-93), change:

```cpp
    STATE_KERNELPULT_BUTTER,
    STATE_UMBRELLA_TRIGGERED,
    STATE_UMBRELLA_REFLECTING,
    STATE_IMITATER_MORPHING,
```

No edit needed above — instead, at the END of the enum (after `STATE_LILYPAD_INVULNERABLE`, ~line 100), append:

```cpp
    STATE_LILYPAD_INVULNERABLE,
    STATE_UMBRELLA_KNOCKING   // proximity shove in progress
};
```

**CRITICAL:** verify the new entry is AFTER `STATE_LILYPAD_INVULNERABLE`, NOT inserted in the middle. Existing state integer values must not shift (save-compat).

- [ ] **Step 2: Add transient bool member**

In the private members block (~line 215, near the other `bool` members), after `bool mHighlighted;` add:

```cpp
    bool                    mDoUmbrellaKnockbackFired = false;   // prevents multi-fire of the one-shot shove
```

- [ ] **Step 3: Add method declarations**

In the private methods block, after `void UpdateUmbrella();` (~line 346), add:

```cpp
    bool                    FindUmbrellaTarget() const;
    void                    DoUmbrellaKnockback();
```

- [ ] **Step 4: Build check**

```bash
cmake --build build --target pvzportable 2>&1 | tail -20
```
Expected: compiles clean. `mDoUmbrellaKnockbackFired` must NOT be in the `Sync` block (`SyncPlantTailPortable`, `SaveGame.cpp:~879-915`) — it isn't, since we only added it as a header member. Confirm it is NOT synced.

- [ ] **Step 5: Commit**

```bash
git add src/Lawn/Plant.h
git commit -m "feat(umbrella): declare STATE_UMBRELLA_KNOCKING, helpers, and transient flag"
```

---

### Task 3: Implement `FindUmbrellaTarget` and `DoUmbrellaKnockback` definitions

**Files:**
- Modify: `src/Lawn/Plant.cpp`

**Interfaces:**
- Consumes: `mBoard`, `mRow`, `mPlantCol`, `UMBRELLA_KNOCKBACK`, `UMBRELLA_KNOCKBACK_DAMAGE`, `Zombie::KnockBack`, `Zombie::TakeDamage`
- Produces: `FindUmbrellaTarget()`, `DoUmbrellaKnockback()`

- [ ] **Step 1: Add stub definitions (compile-only first)**

In `src/Lawn/Plant.cpp`, immediately before `void Plant::UpdateUmbrella()` (~line 1672), add minimal stubs:

```cpp
bool Plant::FindUmbrellaTarget() const
{
    return false;   // temporary — replaced in step 3
}

void Plant::DoUmbrellaKnockback()
{
    // temporary — replaced in step 4
}
```

- [ ] **Step 2: Build check (stubs compile)**

```bash
cmake --build build --target pvzportable 2>&1 | tail -20
```
Expected: compiles clean.

- [ ] **Step 3: Implement `FindUmbrellaTarget`**

Replace the stub body with:

```cpp
bool Plant::FindUmbrellaTarget() const
{
    Zombie* aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie))
    {
        if (aZombie->IsDeadOrDying() || aZombie->mRow != mRow)
            continue;

        // Skip Boss — umbrella should not be a Boss soft-lock.
        if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS)
            continue;

        const int aCol = mBoard->PixelToGridX(aZombie->mX, aZombie->mY);
        if (aCol == mPlantCol || aCol == mPlantCol + 1)
            return true;
    }
    return false;
}
```

- [ ] **Step 4: Implement `DoUmbrellaKnockback`**

Replace the stub body with:

```cpp
void Plant::DoUmbrellaKnockback()
{
    Zombie* aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie))
    {
        if (aZombie->IsDeadOrDying() || aZombie->mRow != mRow)
            continue;

        const int aCol = mBoard->PixelToGridX(aZombie->mX, aZombie->mY);
        if (aCol != mPlantCol && aCol != mPlantCol + 1)
            continue;

        // KnockBack has its own immunity (heavy vehicles + airborne + enraged newspaper).
        // Immune zombies take the damage but are NOT shoved (hurt-but-don't-move rule).
        aZombie->KnockBack(UMBRELLA_KNOCKBACK);
        aZombie->TakeDamage(UMBRELLA_KNOCKBACK_DAMAGE, 0U);   // 0U = standard direct damage (shield→body)
    }

    mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
}
```

`0U` matches the default in `Projectile::GetDamageFlags` for a direct (non-splash) pea hit: damage goes through the shield first, then leftover hits the body. Verified against `src/Lawn/Projectile.cpp:416-439`.

- [ ] **Step 5: Build check**

```bash
cmake --build build --target pvzportable 2>&1 | tail -20
```
Expected: compiles clean.

- [ ] **Step 6: Commit**

```bash
git add src/Lawn/Plant.cpp
git commit -m "feat(umbrella): implement FindUmbrellaTarget and DoUmbrellaKnockback"
```

---

### Task 4: Extend `UpdateUmbrella` with the new state driver

**Files:**
- Modify: `src/Lawn/Plant.cpp` (`Plant::UpdateUmbrella` at ~line 1672, shifted by Task 3 insertions)

**Interfaces:**
- Consumes: `FindUmbrellaTarget`, `DoUmbrellaKnockback`, `mDoUmbrellaKnockbackFired`, `UMBRELLA_KNOCKBACK_COOLDOWN`, `PlayBodyReanim`, `PlayIdleAnim`
- Used by: dispatcher already calls `UpdateUmbrella()` for `SEED_UMBRELLA` (`Plant.cpp:~2657`)

- [ ] **Step 1: Replace `UpdateUmbrella`**

Current `UpdateUmbrella` (the block reading `if (mState == STATE_UMBRELLA_TRIGGERED) { ... } else if (mState == STATE_UMBRELLA_REFLECTING) { ... }`). Replace the entire function body with:

```cpp
void Plant::UpdateUmbrella()
{
    if (mState == PlantState::STATE_UMBRELLA_TRIGGERED)
    {
        if (mStateCountdown == 0)
        {
            mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PROJECTILE, mRow + 1, 0);
            mState = PlantState::STATE_UMBRELLA_REFLECTING;
        }
    }
    else if (mState == PlantState::STATE_UMBRELLA_REFLECTING)
    {
        Reanimation* aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0)
        {
            PlayIdleAnim(0.0f);
            mState = PlantState::STATE_NOTREADY;
            mRenderOrder = CalcRenderOrder();
        }
    }
    else if (mState == PlantState::STATE_UMBRELLA_KNOCKING)
    {
        // Fire the shove when the one-shot "anim_block" clip first completes, then idle out the rest of the cooldown.
        Reanimation* aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (!mDoUmbrellaKnockbackFired && aBodyReanim->mLoopCount > 0)
        {
            DoUmbrellaKnockback();
            mDoUmbrellaKnockbackFired = true;
        }

        if (mStateCountdown == 0)
        {
            PlayIdleAnim(0.0f);
            mState = PlantState::STATE_NOTREADY;
            mStateCountdown = UMBRELLA_KNOCKBACK_COOLDOWN;
            mDoUmbrellaKnockbackFired = false;
        }
    }
    else if (mState == PlantState::STATE_NOTREADY)
    {
        // Proximity shove trigger — only when not on cooldown and no target state is active.
        if (mStateCountdown == 0 && FindUmbrellaTarget())
        {
            PlayBodyReanim("anim_block", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 22.0f);
            mState = PlantState::STATE_UMBRELLA_KNOCKING;
            mStateCountdown = UMBRELLA_KNOCKBACK_COOLDOWN;
            mDoUmbrellaKnockbackFired = false;
        }
    }
}
```

**Notes on the design:**
- `STATE_UMBRELLA_KNOCKING` enters with `mStateCountdown = UMBRELLA_KNOCKBACK_COOLDOWN`. Knoback fires ~1 shot after `anim_block` completes (aBodyReanim->mLoopCount > 0). When countdown reaches 0, the leaf returns to `STATE_NOTREADY` and rearms for another `UMBRELLA_KNOCKBACK_COOLDOWN` frames. The shared counter keeps attack-latency + cooldown within one cycle.
- Existing reflect (`STATE_UMBRELLA_TRIGGERED`/`REFLECTING`) is untouched. The reflect event handler at `Plant.cpp` projectile-hit (`case SeedType::SEED_UMBRELLA`) does not guard on `STATE_UMBRELLA_KNOCKING`, so a reflect firing mid-attack will override the attack state — acceptable; rare. Documented in spec Edge Cases.

- [ ] **Step 2: Build check**

```bash
cmake --build build --target pvzportable 2>&1 | tail -20
```
Expected: compiles clean.

- [ ] **Step 3: Manual per-function verification**

Per spec § Testing → Functional core: place an Umbrella Leaf in a middle row of an Adventure level. Confirm:
  - Zombie entering `mPlantCol` → `anim_block` plays once, zombie X jumps ~40 px right, HP drops by 50.
  - Zombie entering `mPlantCol + 1` → same effect.
  - Cooldown (~0.9s) before next shove.
  - Confirm col `mPlantCol - 1` (left cell) and mRow ± 1 (adjacent rows) do NOT trigger.

- [ ] **Step 4: Commit**

```bash
git add src/Lawn/Plant.cpp
git commit -m "feat(umbrella): drive proximity knockback via STATE_UMBRELLA_KNOCKING"
```

---

### Task 5: Manual QA pass across the spec's testing matrix

**Files:** none changed — verification only. Builds from Tasks 1-4 are used.

**Interfaces:** consumes the play-tested builds from Tasks 1-4.

- [ ] **Step 1: Damage & immunity checks**

Per spec § Testing → Damage & immunity:
  - Basic zombie (200 HP): 50 HP per hit, confirm via `python scripts/pvzp-v4-converter.py export <save.v4> dump.yaml` that `mHP` drops by exactly 50 per shove, 4 shoves to kill.
  - Cone/helmet: 50 raw split helmet→body.
  - Door-shield: 50 absorbed by shield first, leftover to body.
  - Zamboni in range: takes 50 HP but position unchanged (hurt-but-don't-move).
  - Football WITH helmet: 50 → halved to 25 by `TakeBodyDamage` Football reduction.
  - Newspaper CALM: takes hit + shoved. Newspaper ENRAGED (`PHASE_NEWSPAPER_MADDENING`/`PHASE_NEWSPAPER_MAD`): no shove (KnockBack airborne-list), but still takes 50 HP per design rule.
  - Bungee / Catapult: not hit (different phase / airborne list).

- [ ] **Step 2: Reflect parity check**

Per spec § Testing → Parity & interactions:
  - Bungee-zombie drop onto umbrella → reflect still works (`anim_block`, `STATE_UMBRELLA_TRIGGERED` path unchanged). Both mechanics coexist.

- [ ] **Step 3: Edge-case checks**

  - Umbrella at col 8 (rightmost): zombie matches col 8 (own column) but col 9 unreachable — documents acceptable.
  - Two Umbrellas same row, overlapping range: ≤80 px shove, ≤100 HP one-shot.
  - Hypno zombie in range: gets shoved (mind-control doesn't matter).
  - Chill/butter: shove applies; statuses affect movement/eating speed only.

- [ ] **Step 4: Save/load round-trip**

  - Save WITH umbrella mid-knockback (`mState == STATE_UMBRELLA_KNOCKING`), load it: no crash, leaf returns to coherent `STATE_NOTREADY`. `mDoUmbrellaKnockbackFired` defaults to false on load (not synced) — leaf may re-arm early; acceptable.
  - Save during reflect, load: no regression.

- [ ] **Step 5: Commit (if any fixes arise from QA)**

If QA surfaced a bug, fix it on a branch and commit with message describing the fix. If QA passes clean, no commit needed for this task.

```bash
git status   # verify clean or show fixes
```

---

### Task 6: Final verification build

**Files:** none changed (verification gate).

- [ ] **Step 1: Clean Release build**

```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -10
cmake --build build --target pvzportable 2>&1 | tail -10
```
Expected: `ninja: no work to do` or successful build; zero errors; linker happy.

- [ ] **Step 2: Confirm final diff against spec § Files Modified**

Files changed should be exactly:
  - `src/GameConstants.h` (3 new constants)
  - `src/Lawn/Plant.h` (1 new enum value appended at END + 1 new bool member + 2 new private method decls)
  - `src/Lawn/Plant.cpp` (`FindUmbrellaTarget` + `DoUmbrellaKnockback` defs + `UpdateUmbrella` extension)

No other files should be touched:
```bash
git diff --stat HEAD~4..HEAD
```
If `ConstEnums.h`, `SyncGame.cpp`, or any unlisted file shows up in the diff, STOP and reconcile — that indicates an out-of-scope change that must be justified.

- [ ] **Step 3: Commit a summary reference (doc-only, optional)**

If you want a single reference commit pointing to the spec+plan pair:

```bash
git commit --allow-empty -m "feat(umbrella): proximity knockback shippable; see docs/superpowers/specs/2026-07-09-umbrella-leaf-knockback-design.md and plans/2026-07-09-umbrella-leaf-knockback.md"
```

---

## Notes for the implementer

- `mDoUmbrellaKnockbackFired` is deliberately NOT added to `SyncPlantTailPortable` — verify after Task 2 that the sync block (`SaveGame.cpp:~879-915`) does not include it.
- Match surrounding `Zombie.cpp` immune-list style (4-space indent, `mMember` naming) if extending `KnockBack` later; for now `KnockBack`'s existing immunity list is reused as-is.
- If playtesting reads the hit-frame as lagging the `anim_block` animation (fires at `mLoopCount > 0` = clip end), move the `DoUmbrellaKnockback()` call to a couple frames after state entry (use a new small counter, or call it immediately on entry for "bat now, lags on next frame" feel). Either is acceptable per spec.
- No `/*inline*/` comment markers required on the new helpers — they're plain methods.
- This plant uses `mBoard->IterateZombies` and `mBoard->PixelToGridX` (consistent with `FindTargetZombie`, `ScaredyShroom`, and Gloom) — pattern is established, no new abstraction needed.
