# Gloom-shroom Knockback Design

## Summary

Make the Gloom-shroom (忧郁菇, `SEED_GLOOMSHROOM`) knock back zombies directly in front of it on every attack. Knockback only applies to zombies in the column immediately in front of the plant (`mPlantCol + 1`), across all 3 rows the Gloom-shroom hits.

## Motivation

In vanilla PvZ, Gloom-shroom does area damage in a 3×3 zone but has no crowd-control (knockback) effect. This mod adds a "push" feel to the plant: every fume volley shoves nearby zombies one step back, synergizing with its already-reasonable single-hit damage (20) by creating breathing room. The restriction to "front column only" preserves the plant's identity as an area-damage dealer rather than a perma-stunlock tool, and mirrors the fact that the fume cloud visually emanates forward from the Shroom.

## Current Gloom-shroom attack path (code facts)

| Item | Value / Location |
|---|---|
| Seed type | `SeedType::SEED_GLOOMSHROOM = 42` (`ConstEnums.h:1079`) |
| Attack entry | `Plant::Fire()` → `DoRowAreaDamage(20, 2U)` (`Plant.cpp:4614`) |
| Damage | **20** per hit |
| Hit rows | `mRow - 1, mRow, mRow + 1` (±1 row around plant; `Plant.cpp` `DoRowAreaDamage` gloom branch) |
| Attack rect | `Rect(mX - 80, mY - 80, 240, 240)` (= `GetPlantAttackRect(WEAPON_PRIMARY)`, `Plant.cpp:5346`) — roughly a 3×3 grid around the plant |
| Hit detection | `GetRectOverlap(aAttackRect, aZombieRect) > 0` inside `DoRowAreaDamage` |
| Damage loop | iterates `mBoard->IterateZombies`, filters by `EffectedByDamage(aDamageRangeFlags)` and `mOnHighGround == IsOnHighGround()` |
| Per-volley hits | 4 (gloom fires at `mShootingCounter` == 126 / 98 / 70 / 42, total cycle 200) |

## Behavior

- **When**: on every Gloom-shroom attack volley (`DoRowAreaDamage` is called), immediately after damage is dealt.
- **Who**: every zombie that took damage AND is in the plant's own column (`mPlantCol`) or the front column (`mPlantCol + 1`), across the 3 rows gloom already covers, AND passes the 50% roll.
- **Effect**:
  - Zombie's pixel X is increased by `GLOOM_KNOCKBACK` (shoved right, away from plants).
  - `mX` re-synced to `static_cast<int>(mPosX)`.
  - If the zombie was eating (`mIsEating`), eating is interrupted (zombie resumes walking).
  - Zombie is immune to knockback if it's a heavy vehicle (`ZOMBIE_ZAMBONI`, `ZOMBIE_CATAPULT`), currently airborne (any of: mid‑pole‑vault, dolphin‑rider mid‑jump, pogo mid‑bounce, balloon flying, bungee diving/rising, imp mid‑throw, snorkel entering pool), or an enraged newspaper zombie (`PHASE_NEWSPAPER_MADDENING` / `PHASE_NEWSPAPER_MAD`).
- **Not interrupted**: unchanged — full 20 damage, full 3×3 coverage, Foley `FOLEY_FUME` (or splat), existing particle cloud.
- **No change** to anything outside knockback: sight range, fire rate, damage, damage type.

### Tuning constant

`GLOOM_KNOCKBACK = 35.0f` (pixels per hit). Per 4-hit volley, a zombie in the front column could be pushed up to 140px (≈1.75 cells) — noticeable but not a launcher.

## Reasoning for column-only knockback

Gloom-shroom hits a 3×3 zone but the fume cloud's visual and the plant's "forward" identity point to the front column. Three reasons to restrict to column `mPlantCol + 1`:

1. **Plant identity**: Gloom is an area-damage dealer, not a stunlock tool. Restricting knockback to the front column keeps its area damage meaningful while giving it one narrow "push" lane.
2. **Anti-perma-lock**: Knockback on all 9 cells let a single gloom hold a lane indefinitely. Column-only caps that to one lane.
3. **Symmetric damage, asymmetric control**: still hits 3 rows for 20 dmg each, only one column pushes back — a natural-feeling asymmetry.

## Implementation

### 1. New constant — `src/GameConstants.h`

After the existing lawn constants (around line 38), add:

```cpp
constexpr const float GLOOM_KNOCKBACK = 35.0f;   // per-hit knockback distance for Gloom-shroom (pixels, +X)
```

### 2. Call site — `Plant::DoRowAreaDamage` (`src/Lawn/Plant.cpp`, the damage-dealing block)

Inside `DoRowAreaDamage`, immediately after `aZombie->TakeDamage(...)`:

```cpp
// 忧郁菇：本格和正前方一格击退 (Gloom-shroom: knock back zombies in own column and front column)
// 每次攻击 50% 概率击退
if (mSeedType == SeedType::SEED_GLOOMSHROOM)
{
    int aZombieCol = mBoard->PixelToGridX(aZombie->mX, aZombie->mY);
    if ((aZombieCol == mPlantCol || aZombieCol == mPlantCol + 1) && Sexy::Rand(2) == 0)
        aZombie->KnockBack(GLOOM_KNOCKBACK);
}
```

This is inside the `if (GetRectOverlap(aAttackRect, aZombieRect) > 0)` block, after the `TakeDamage` flow. The gloom‑specific branch (`if (mSeedType == SeedType::SEED_GLOOMSHROOM)` row-diff guard already ran earlier in the same loop iteration, so a zombie reaching this point is guaranteed row-eligible.

### 3. New `Zombie::KnockBack` method

**Declaration** — `src/Lawn/Zombie.h`, in the public method block (near `StopEating`), add:

```cpp
void                        KnockBack(float theAmount);
```

**Definition** — `src/Lawn/Zombie.cpp`:

```cpp
void Zombie::KnockBack(float theAmount)
{
    // Heavy vehicles are immune
    if (mZombieType == ZombieType::ZOMBIE_ZAMBONI || mZombieType == ZombieType::ZOMBIE_CATAPULT)
        return;

    // Airborne phases are immune — pushing a zombie in mid-air would look glitchy.
    // Enraged newspaper zombie (愤怒的读报僵尸) is also immune.
    if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT ||
        mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT ||
        mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP ||
        mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING ||
        mZombiePhase == ZombiePhase::PHASE_BUNGEE_DIVING ||
        mZombiePhase == ZombiePhase::PHASE_BUNGEE_DIVING_SCREAMING ||
        mZombiePhase == ZombiePhase::PHASE_BUNGEE_RISING ||
        mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN ||
        mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL ||
        mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MADDENING ||
        mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MAD ||
        IsBouncingPogo())
        return;

    // Interrupt eating, the zombie should walk again
    if (mIsEating)
        StopEating();

    mPosX += theAmount;
    // Clamp to right-of-board so a zombie can't be pushed de-spawnable-far.
    // Lawn's right edge is ~col 9 (≈800). Spawn cap ~= BOARD_WIDTH + 150.
    const float aRightCap = static_cast<float>(WIDE_BOARD_WIDTH + 150);
    if (mPosX > aRightCap)
        mPosX = aRightCap;

    mX = static_cast<int>(mPosX);
}
```

### Files Modified

- `src/GameConstants.h` — new `GLOOM_KNOCKBACK` constant.
- `src/Lawn/Plant.cpp` — `DoRowAreaDamage` damage block, 5-line call site.
- `src/Lawn/Zombie.h` — `KnockBack` declaration.
- `src/Lawn/Zombie.cpp` — `KnockBack` definition.

### Unchanged

- No new enums (no `ConstEnums.h` enum additions).
- No save-data format change (knockback is a late-instantiation position delta on a transient zombie; no new persistent state).
- No new animations or particles.
- No damage, fire rate, or attack-rect changes.

## Edge Cases

- **Gloom-shroom on col 8 (rightmost)**: front column is col 9 = beyond lawn. `PixelToGridX` clamps to `MAX_GRID_SIZE_X - 1 = 8`. A zombie straddling the front column lands in col 8 (same as the plant) and matches the own-column check, so it gets knocked back once — acceptable single push at the edge.
- **Multiple Gloom-shrooms**: each applies knockback independently. A zombie in the shared front column of two glooms could be pushed twice per volley (≤280 px) — intentional, rewards stacking.
- **Chilled / buttered zombie**: knockback applies regardless of status effects; chilling only affects movement speed, not position shifts.
- **Mind-controlled (Twins / hypno) zombie**: knockback still applies; `mMindControlled` has no bearing on `KnockBack` immunity list.
- **Zombie in front column but already far right**: `aRightCap = WIDE_BOARD_WIDTH + 150 = 950` clamp prevents pushing zombies beyond the spawn area into de-spawnable territory.
- **Boss (Z BOSS)**: `ZOMBIE_BOSS` is not in the vehicle list and has no airborne phase, so it takes knockback — matches "boss doesn't have implicit CC immunity" project convention. Boss moves via separate path so knockback is cosmetic at most.
- **Pole Vaulter / Dolphin mid-vault**: immune. When it lands (phase returns to walking), subsequent volleys will knock it back normally.
- **Ladder ZOMBIE_LADDER**: zombie with a ladder is still on the ground (not in the airborne list), so it takes knockback — acceptable, ladder implies grounded at that moment and knockback doesn't interact with ladder logic.
- **Enraged newspaper zombie (愤怒的读报僵尸)**: immune during `PHASE_NEWSPAPER_MADDENING` (paper shredding) and `PHASE_NEWSPAPER_MAD` (enraged charge). While calmly reading (`PHASE_NEWSPAPER_READING`) it is NOT immune and gets knocked back normally — the immunity only covers the enraged state.

## Testing

Manual gameplay across adventure / survival / minigame modes:

- [ ] Adventure IV-4 / later levels with a Gloom-shroom — confirm push occurs on own-column and front-column zombies.
- [ ] Place Gloom at col 1, watch col-1 (own) and col-2 (front) zombies — each eligible zombie is knocked back ~50% of the time; col-3+ zombies take damage but never move.
- [ ] Zamboni drives through the front column — takes damage (20) but does NOT move (vehicle-immune).
- [ ] Pole Vaulter / Dolphin / Pogo / Balloon zombie in the front column — immune during the airborne moment, push after landing.
- [ ] Snorkel-zombie entering pool in the front column — immune during `PHASE_SNORKEL_INTO_POOL`, push once swimming.
- [ ] Chilled / buttered zombie — still pushed.
- [ ] Newspaper zombie — gets knocked back while reading (`PHASE_NEWSPAPER_READING`); becomes immune once enraged (paper shredding / enraged charge) and stays immune for the duration of the enrage.
- [ ] Stacked Glooms — front-column zombie pushed harder.
- [ ] Golden-pumpkin reanim on Gloom (Gloom wearing a pumpkin) — unaffected (zombie-type check is on target, not plant outfit).
- [ ] Save / load during a Gloom attack — position deltas don't serialize, no crash, no save format change.
- [ ] WASM build (Emscripten) — smoke check, since knockback is a GPU-independent position update.

## Notes for the implementer

- Don't touch `ConstEnums.h` enums (license / serialization rule).
- Match file formatting of surrounding code in `Zombie.cpp` (4-space indent, `mX` naming).
- `KnockBack` must operate purely on `mPosX` + `mX` so it composes cleanly with the reanimation-driven animation; it doesn't touch `mPhaseCounter` (phase timer) so existing phase logic stays intact.
- No inline comment markers (`/*inline*/`) needed — `KnockBack` is a plain method.
