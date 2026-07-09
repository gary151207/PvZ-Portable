# Leaf Protector (叶子保护伞) Proximity Knockback Design

## Summary

Make the **Umbrella Leaf** (叶子保护伞 / Leaf Protector, `SEED_UMBRELLA`) knock back nearby zombies **at its own expense**. When a zombie is in the same cell as the plant or the cell immediately to its right (same row), the leaf "shoves" every qualifying zombie one step to the right and then **takes 50 HP of damage itself**. This is an entirely **new, additive behavior**; the pre-existing "reflect projectile" mechanic (反射, `anim_block`) stays untouched.

## Motivation

In vanilla PvZ the Leaf Protector's only job is reflecting Bungee-zombie grabs and catapult projectiles — a pure utility plant that does nothing once the front line closes in. Giving it a focused "proximity shove" turns it into a defensive plant with an active crowd-control role, synergizing thematically (a big leaf batting zombies away) and mechanically (creating breathing room for offensive plants behind it). The shove is **costly**: the leaf loses 50 HP each time it shoves. An unupgraded leaf starts at 300 HP, so it can absorb 6 shoves before wilting — a deliberate budget that keeps it a tempo tool, not a self-sufficient lane holder. The tight 2-cell range reinforces this: the leaf buys space, not a permanent wall.

## Current Umbrella Leaf behavior (code facts)

| Item | Value / Location |
|---|---|
| Seed type | `SeedType::SEED_UMBRELLA` (`ConstEnums.h`) |
| Reanim | `REANIM_UMBRELLALEAF` — `reanim/Umbrellaleaf.reanim` (`Reanimator.cpp:120`) |
| Current reflect trigger | `case SeedType::SEED_UMBRELLA` in the plant-hit-by-projectile handler (`Plant.cpp:~4526`) |
| Reflect animation | `PlayBodyReanim("anim_block", REANIM_PLAY_ONCE_AND_HOLD, 0, 22.0f)` |
| State machine for reflect | `STATE_UMBRELLA_TRIGGERED` (countdown 5) → `STATE_UMBRELLA_REFLECTING` → idle (`Plant.cpp:1672`) |
| State enum values | `STATE_UMBRELLA_TRIGGERED`, `STATE_UMBRELLA_REFLECTING` (`Plant.h:92-93`) |
| `SpriteSeqIndex` / render | scale `0.5f`, shadow offset `(-7, 52)` (`Plant.cpp:4025`) |
| Save/reflect logic | `Board::FindUmbrellaPlant` checks `GridInRange(col, row, …, 1, 1)` (`Board.cpp:9658`) — a 3×3 reflect zone, but reflect is only triggered on projectile contact, NOT proximity. |

The umbrella already has an animation asset for "shove" action (`"anim_block"`). Knockback will reuse it as a visual cue for the new proximity hit.

## Behavior

The mechanic runs as a **new, parallel attack loop** driven by a new `PlantState`. It does NOT touch the existing reflect state machine.

- **When** (trigger): every `Update()` frame, when the leaf is idle (`STATE_NOTREADY`), a new helper `FindUmbrellaTarget()` scans zombies. If any non-dying zombie of the **same row** has a cell X of `mPlantCol` or `mPlantCol + 1`, the leaf starts a knockback attack.
- **Animation**: play `"anim_block"` once (the existing reanim block animation doubles as the "bat" motion) at a brisk rate.
- **Hit frame**: at a fixed point in the attack animation (e.g. a frame near the start so the shove reads as causal), call `DoUmbrellaKnockback()` which scans the target zone again and, for each valid zombie:
  - applies `aZombie->KnockBack(UMBRELLA_KNOCKBACK)` to every qualifying zombie — a **~40 px rightward position delta** (same underlying method Gloom-shroom uses). Eating is interrupted, heavy/airborne zombies are skipped per `KnockBack`'s built-in immunity list. Immune zombies are simply not shoved; they are NOT damaged either.
  - **The leaf itself loses 50 HP** after the shove resolves: `mPlantHealth -= UMBRELLA_KNOCKBACK_DAMAGE`. If `mPlantHealth` drops to 0 or below, the leaf plays `FOLEY_SQUISH` and calls `Die()`. This is the leaf's "exhaustion" cost for shoving — its defining tradeoff.
- **Cooldown**: after the attack animation + hit frame, the leaf enters a short cooldown (`STATE_NOTREADY` with `mStateCountdown`) before it can trigger another shove. This prevents per-frame multi-hitting.
- **Not interrupted**: all pre-existing umbrella behavior (reflect on projectile contact) runs unchanged. A reflect can still interrupt or run between proximity attacks; both share the `"anim_block"` animation but use different state values, so they do not collide. Priority edge case: the existing reflect handler (`Plant.cpp:~4526`) checks `mState != STATE_UMBRELLA_TRIGGERED && mState != STATE_UMBRELLA_REFLECTING` — it does NOT guard on the new state, so a reflect that fires during a proximity attack overrides the attack state. Acceptable and rare; noted in Edge Cases.
- **Range**: strictly same row, columns `mPlantCol` and `mPlantCol + 1`. Uses `Board::PixelToGridX` for cell resolution (consistent with Gloom and Scaredy-shroom). Adjacent-row zombies are intentionally NOT hit — the leaf is a leaf, not a shockwave.
- **No change** to sun cost, recharge, reflect behavior, art assets, or reflect 3×3 scan.

### Tuning constants

| Constant | Value | Rationale |
|---|---|---|
| `UMBRELLA_KNOCKBACK` | `40.0f` (pixels, +X) | Slightly stronger than Gloom's 35 px — a "bat" should shove a bit harder than a puff of spores, but not launch. |
| `UMBRELLA_KNOCKBACK_DAMAGE` | `50` (HP) | Self-exhaustion cost. With a base leaf at 300 HP, the plant can absorb 6 shoves; an unupgraded leaf must be budgeted carefully or it wilts mid-fight. |
| `UMBRELLA_KNOCKBACK_COOLDOWN` | `90` frames (~0.9 s @ 100 fps) | Prevents per-frame multi-hit while remaining responsive to a crowd shuffling in. |
| `UMBRELLA_KNOCKBACK_ANIM_FRAMES` | matches `"anim_block"` length; hit fires at frame ~8 of the one-shot | Reads as causal ("leaf pushes, zombie lurches") |

## Reasoning for cell-based range (the "本格 + 右边 1 格" rule)

1. **Predictable for the player**: a 2-cell rectangle the player can see and reason about. Pixel-based triggers (like `DoRowAreaDamage`'s rect overlap) feel fuzzy when a zombie is "slightly too far" — cells are unambiguous.
2. **Synergy with "leaf bat" identity**: the leaf whips forward-right, so hitting the cell it occupies and the one immediately forward reads visually.
3. **Budget pressure**: each shove costs the leaf 50 HP on top of consuming the zombie's approach. Combined with the short 2-cell range and the cooldown, the leaf is a tempo tool — it buys breathing room but must be budgeted or it wilts.

## Implementation

### 1. New constant — `src/GameConstants.h`

Near the existing `GLOOM_KNOCKBACK` (around line 38), add:

```cpp
constexpr const float UMBRELLA_KNOCKBACK        = 40.0f;  // per-hit knockback distance for Umbrella Leaf (pixels, +X)
constexpr const int   UMBRELLA_KNOCKBACK_DAMAGE = 50;     // HP the leaf loses per shove (self-exhaustion cost)
constexpr const int   UMBRELLA_KNOCKBACK_COOLDOWN = 90;   // frames between proximity attacks
```

No damage-flag constant is needed — zombies are no longer damaged; the leaf takes plain `mPlantHealth -= 50` damage to itself.

### 2. New state — `src/Lawn/Plant.h`

Add a new state to the `PlantState` enum. **Critical**: `PlantState` is serialized via `SyncEnum32` (`SaveGame.cpp:879`). To preserve save-compatibility, append the new value at the END of the enum (after `STATE_LILYPAD_INVULNERABLE`), not in the middle — existing state integer values must not shift.

```cpp
    STATE_MARIGOLD_ENDING,
    STATE_FLOWERPOT_INVULNERABLE,
    STATE_LILYPAD_INVULNERABLE,
    STATE_UMBRELLA_KNOCKING   // NEW: proximity shove in progress
};
```

**Do NOT add a `NUM_PLANTSTATES` sentinel** — the existing enum has none, and inserting one would shift all integer values and break saves.

### 3. Target-finding helper — `src/Lawn/Plant.cpp`

A small scan mirroring how `FindTargetZombie` / `ScaredyShroom` (line ~1338) iterate zombies, but gating on cell membership rather than lateral distance. Add a private declaration in `Plant.h`:

```cpp
bool                        FindUmbrellaTarget() const;
```

Definition:

```cpp
bool Plant::FindUmbrellaTarget() const
{
    Zombie* aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie))
    {
        if (aZombie->IsDeadOrDying() || aZombie->mRow != mRow)
            continue;

        // Boss ignores position-based filters — it lives on every row. Skip it,
        // the leaf should not be a boss trap.
        if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS)
            continue;

        const int aCol = mBoard->PixelToGridX(aZombie->mX, aZombie->mY);
        if (aCol == mPlantCol || aCol == mPlantCol + 1)
            return true;
    }
    return false;
}
```

### 4. The knockback application — `src/Lawn/Plant.cpp`

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
        aZombie->KnockBack(UMBRELLA_KNOCKBACK);
    }

    // The leaf exhausts itself shoving: lose 50 HP. At 0 HP, it dies.
    mPlantHealth -= UMBRELLA_KNOCKBACK_DAMAGE;
    if (mPlantHealth <= 0)
    {
        mPlantHealth = 0;
        mApp->PlayFoley(FoleyType::FOLEY_SQUISH);
        Die();
    }
    else
    {
        mApp->PlayFoley(FoleyType::FOLEY_SPLAT);   // satisfying "thwack"
    }
}
```

### 5. New attack state driver — modify `Plant::UpdateUmbrella()` (`src/Lawn/Plant.cpp:1672`)

Current:

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
}
```

Replace the idle branch (`STATE_NOTREADY`) with gated cooldown handling + new state driving:

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
        // Drive the one-shot "anim_block": fire the shove around the apex of the animation.
        Reanimation* aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (!mDoUmbrellaKnockbackFired && aBodyReanim->mLoopCount > 0)
        {
            // anim_block is short; treat any loop completion as the "hit" if it hasn't fired yet.
            // Fire the hit on the first frame where the bat visually connects — using the
            // animation completion is a safe proxy for a one-shot clip.
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
        // Proximity shove trigger (NEW). Only when not on cooldown.
        if (mStateCountdown == 0 && FindUmbrellaTarget())
        {
            PlayBodyReanim("anim_block", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 22.0f);
            mState = PlantState::STATE_UMBRELLA_KNOCKING;
            mStateCountdown = UMBRELLA_KNOCKBACK_COOLDOWN;   // attack duration+cooldown share this countdown
            mDoUmbrellaKnockbackFired = false;
        }
    }
}
```

> **On the `mCountdown` semantics**: in this design, `STATE_UMBRELLA_KNOCKING` ENTERS with `mStateCountdown = UMBRELLA_KNOCKBACK_COOLDOWN`, which counts down to 0; when it reaches 0 the leaf returns to `STATE_NOTREADY` and re-arms with another `UMBRELLA_KNOCKBACK_COOLDOWN` frames of cooldown. The hit itself fires mid-animation (when `anim_block` loops). If the implementer prefers a SHORTER attack state with a separate post-attack cooldown, split the counter into `mAttackCountdown` / `mKnockbackCooldown` — but the shared-counter form above is the minimal-diff choice matching Spikeweed's `STATE_SPIKEWEED_ATTACKING` pattern.

> **On the hit frame**: `"anim_block"` is a 1-shot clip (`REANIM_PLAY_ONCE_AND_HOLD`). `aBodyReanim->mLoopCount` becomes > 0 when the clip finishes. For a *causal* hit, it's better to fire `DoUmbrellaKnockback()` a few frames in rather than at the clip end. The minimal-diff way: fire the hit immediately on state entry (reflects "leaf bats now"), and let the hit result visually land on the next frame. Either placement is acceptable; the doc commits to **fire on `anim_block` first-loop** (cleanest with existing infra). The implementer may shift 1-3 frames earlier if playtesting reads the hit as lagging the animation.

### 6. New `Plant::UpdateUmbrella` route is already wired — verify dispatcher

Ensure `UpdateUmbrella` is called for `SEED_UMBRELLA` in the main dispatcher. It already is (`Plant.cpp:2657`):

```cpp
else if (mSeedType == SeedType::SEED_UMBRELLA)                                              UpdateUmbrella();
```

No change needed.

### 7. New member — `src/Lawn/Plant.h`

In the `Plant` class private members, add:

```cpp
bool                        mDoUmbrellaKnockbackFired = false;   // prevents multi-fire of the one-shot shove
```

And forward-declare the helpers:

```cpp
bool                        FindUmbrellaTarget() const;
void                        DoUmbrellaKnockback();
```

### Files Modified

- `src/GameConstants.h` — new `UMBRELLA_KNOCKBACK`, `UMBRELLA_KNOCKBACK_DAMAGE`, `UMBRELLA_KNOCKBACK_COOLDOWN` constants.
- `src/Lawn/Plant.h` — new `STATE_UMBRELLA_KNOCKING` enum value (appended), new `mDoUmbrellaKnockbackFired` member and `FindUmbrellaTarget` / `DoUmbrellaKnockback` declarations.
- `src/Lawn/Plant.cpp` — `UpdateUmbrella` expanded with new state driver; `FindUmbrellaTarget` + `DoUmbrellaKnockback` definitions.

### Unchanged

- **Existing reflect mechanic** (`STATE_UMBRELLA_TRIGGERED` / `STATE_UMBRELLA_REFLECTING`, `anim_block` reflect at `Plant.cpp:4526`, `FindUmbrellaPlant`) — untouched.
- No new `ConstEnums.h` enum additions (license/serialization rule).
- No new animations, particles, or sound files. Knockback reuses `"anim_block"` animation and `FOLEY_SPLAT`.
- No save-format change for OTHER data. The new `PlantState` enum value is appended at the END, so existing save deserialization is not affected by value shifts — a vanilla save loaded here will simply never use `STATE_UMBRELLA_KNOCKING`. (Round-trip test still required — see Testing.)

## Edge Cases

- **Umbrella Leaf at rightmost col (8 on a 9-col lawn)**: `mPlantCol + 1 == 9` is out of bounds. `PixelToGridX` returns `MAX_GRID_SIZE_X - 1 = 8` for any off-board-right X (clamped). A zombie shuffled to col-8-or-beyond will match `aCol == mPlantCol` only — the right column match is unreachable. This is acceptable; the left-column check still shoves a zombie exactly on the plant. A zombie far right of the board (e.g. post-offscreen shuffle) still gets knocked back-and-damaged if it falls in cell 8. Documented as acceptable edge behavior.
- **Multiple Umbrella Leaves in the same row**: each triggers independently. A zombie in the shared range of two leaves could be shoved twice in one frame (≤80 px, ≤100 damage) — intentional, rewards stacking. Cooldowns are per-plant, so staggered timing is natural; simultaneous-trigger is rare.
- **Zombie on high ground vs. leaf on low ground (and vice versa)**: `DoRowAreaDamage` normally checks `aZombie->mOnHighGround == IsOnHighGround()`. The umbrella proximity check **currently omits this filter**. Decision: the cell match is the gate; high/low mismatch does not block. If the project wants high-ground parity matching, add the check in `FindUmbrellaTarget`. The doc commits to **no gate** (theoretical; the leaf is only placeable on the same high-ground state in vanilla, but mods may break that). Add the `mOnHighGround` check if terrain parity matters.
- **Reflect firing mid-knockback-attack** (rare): the existing projectile-hit handler at `~4526` overrides `mState` to `STATE_UMBRELLA_TRIGGERED` without checking `STATE_UMBRELLA_KNOCKING`. Result: the current attack cycle is interrupted, reflect plays, then the leaf re-arms after reflect completes. Acceptable — defensive utility takes priority. If the implementer wants strict separation, add `&& mState != STATE_UMBRELLA_KNOCKING` to the handler's guard.
- **Umbrella Leaf self-exhaustion death mid-fight**: with base HP 300 and 50 HP per shove, the leaf survives 6 shoves. On the 7th shove `mPlantHealth` hits 0, `FOLEY_SQUISH` plays, `Die()` is called, `mDead = true`, and the plant is cleaned up on the next board tick. Design consequence: the leaf is a **consumable tempo tool**, not permanent defense. Players should budget shoves or tank with high-HP plants.
- **Multiple Umbrella Leaves in the same row**: each triggers independently and each pays its own 50 HP cost. Stacking is a tempo choice, not free — two leaves both shoving means both lose 50 HP per cycle.
- **Zombie on high ground vs. leaf on low ground (and vice versa)**: `DoRowAreaDamage` normally checks `aZombie->mOnHighGround == IsOnHighGround()`. The umbrella proximity check **currently omits this filter**. Decision: the cell match is the gate; high/low mismatch does not block. If the project wants high-ground parity matching, add the check in `FindUmbrellaTarget`. The doc commits to **no gate** (theoretical; the leaf is only placeable on the same high-ground state in vanilla, but mods may break that). Add the `mOnHighGround` check if terrain parity matters.
- **Reflect firing mid-knockback-attack** (rare): the existing projectile-hit handler at `~4526` overrides `mState` to `STATE_UMBRELLA_TRIGGERED` without checking `STATE_UMBRELLA_KNOCKING`. Result: the current attack cycle is interrupted, reflect plays, then the leaf re-arms after reflect completes. Acceptable — defensive utility takes priority. If the implementer wants strict separation, add `&& mState != STATE_UMBRELLA_KNOCKING` to the handler's guard.
- **Boss (`ZOMBIE_BOSS`)**: the scan in `FindUmbrellaTarget` explicitly skips the Boss (treats it as every-row). This prevents the leaf from being a cheap Boss-lock that also self-destructs on the Boss's behalf. If a future version wants the leaf to boss-hit, remove the skip.
- **Save/load round-trip**: new enum value appended at the end does not shift existing values. A save written by this build with `mState == STATE_UMBRELLA_KNOCKING` cannot be loaded by vanilla (acceptable — this is the mod's new state). A vanilla save will never contain the new value. The new member `mDoUmbrellaKnockbackFired` defaults to `false` and is not persisted (boolean flags that reset on load are safe). WARN: the implementer MUST confirm `Plant`::'s `Sync` block in `SaveGame.cpp` does **not** Sync this bool (it shouldn't — it isn't in the current sync block). Verify by reading the plant sync block (~line 879).
- **Chilled / buttered zombie in range**: cell match still triggers; chill/butter affect movement speed and eating speed, not position shifts. Shove applies normally and the leaf still pays 50 HP.
- **Mind-controlled (Hypno / Mind-controlled) zombie**: cell match still triggers; `mMindControlled` has no bearing on knockback. A hypno-zombie can be shoved back.
- **Pole Vaulter / Dolphin / Pogo mid-vault-jump in the trigger cell**: KnockBack's built-in airborne phase list skips the position delta (can't shove mid-air), BUT the leaf **still pays the 50 HP exhaustion cost** every cycle the flying zombie sits in range — a minor leak, acceptable; the leaf player should be aware flying units are "HP tax" unless they land and get shoved back out of range. Implementer may choose to skip() when no zombie was actually shoved (`FindUmbrellaTarget` already returns false for them since they're airborne — note they are NOT excluded from `FindUmbrellaTarget` by phase, only by `KnockBack`). Verify the phase list vs. `FindUmbrellaTarget`'s filters: airborne phases are not in `FindUmbrellaTarget`, so flying zombies do NOT trigger the attack in the first place. Good — they are free. If the project prefers they be excluded more visibly, add an airborne-phase check to `FindUmbrellaTarget`.
- **Snorkel in the trigger cell entering the pool (`PHASE_SNORKEL_INTO_POOL`)**: same as above for flying units; `FindUmbrellaTarget` does not filter by phase, but KnockBack does. Verify whether a snorkel entering the pool sits in range long enough to matter — it is a short transient phase.

## Testing

Manual gameplay across adventure / survival / minigame modes. No automated test suite (project convention).

### Setup sanity

- [ ] `GameConstants.h` compiles; `UMBRELLA_KNOCKBACK`, `UMBRELLA_KNOCKBACK_DAMAGE`, `UMBRELLA_KNOCKBACK_COOLDOWN` all reachable.
- [ ] New `PlantState` value appended at enum end; existing states' integer values unchanged (verify by reading `Plant.h` diff).

### Functional core

- [ ] Adventure level with an Umbrella Leaf in a middle row: spawn/let a basic zombie approach. When the zombie enters `mPlantCol` or `mPlantCol + 1`, the leaf plays `"anim_block"`, the zombie's X jumps right by ~40 px, AND the leaf loses 50 HP. Confirm HP delta via save-file export (`python scripts/pvzp-v4-converter.py export <save.v4> dump.yaml`) — the leaf's `mPlantHealth` drops by 50 per hit. Zombie's HP is unchanged. Repeat; confirm the cooldown (~0.9s) before next shove.
- [ ] Confirm both cells trigger: zombie entering col `mPlantCol` only (standing in front of but not eating the leaf, e.g. a lane where the leaf is behind a wall-nut) — leaf shoves and loses 50 HP.
- [ ] Confirm right cell triggers: zombie entering `mPlantCol + 1` only — leaf shoves and loses 50 HP.
- [ ] Confirm col `mPlantCol - 1` (left cell, behind/past the leaf's front) does NOT trigger — zombie in place, leaf takes no damage.
- [ ] Confirm adjacent row (mRow ± 1) does NOT trigger — zombie in place, leaf takes no damage.
- [ ] Confirm `"anim_block"` animation plays exactly once per trigger (not looping, not double-firing on cooldown re-entry).

### Self-exhaustion & knockback

- [ ] Leaf HP budget: with base HP 300, the leaf survives exactly 6 shoves. On the 6th shove it drops to 50 HP; a 7th shove brings it to 0 HP, plays `FOLEY_SQUISH`, and the leaf dies. Verify `mPlantHealth` decrements by exactly 50 per shove, independent of how many zombies were shoved.
- [ ] Confirm the leaf HP cost is **per-knockback-cycle**, not per-zombie: if 3 zombies are in range simultaneously, the leaf shoves all of them but pays only 50 HP total for that cycle.
- [ ] Zamboni in range: `KnockBack` is no-op (heavy vehicle), but the leaf still pays 50 HP for the cycle — acceptable; Zamboni is a valid shoving target by cell match.
- [ ] Bungee zombie / Catapult: FindUmbrellaTarget (cell-based) matches them; KnockBack skips the delta; leaf pays 50 HP. Documented as acceptable.
- [ ] Newspaper zombie while CALM (`PHASE_NEWSPAPER_READING`): gets shoved, leaf pays 50 HP. Newspaper ENRAGED (`PHASE_NEWSPAPER_MADDENING` / `PHASE_NEWSPAPER_MAD`): `KnockBack` skips position delta (airborne-list), FindUmbrellaTarget still matches, leaf pays 50 HP. Acceptable.
- [ ] **Zombie HP is unchanged** across all the above: the leaf does no damage to any zombie. Cross-check zombie's `mHP` in a save export before/after a shove — it must be identical.

### Parity & interactions

- [ ] Reflect mechanic STILL WORKS: a Bungee zombie drops, Umbrella reflects it (existing `STATE_UMBRELLA_TRIGGERED` path plays). Both mechanics coexist.
- [ ] BlowAwayFliers / tornado: unaffected — umbrella's new logic does NOT integrate with `BlowAwayFliers`.
- [ ] Hypno zombie in range: gets shoved; its status does not matter.
- [ ] Chilled zombie: shoved at normal-ish pace; chill does not stop the shove.
- [ ] Umbrella Leaf at COL 8 (rightmost): cell-8-only match works; col-9 match is unreachable (documented).
- [ ] TWO Umbrella Leaves in the same row, overlapping range: zombie shoved twice (verify position); BOTH leaves lose 50 HP on that cycle.
- [ ] Leaf on high ground / zombie on low ground (if placeable): shove applies (no high-ground gate).

### Save/load

- [ ] Save game **with** an Umbrella Leaf mid-knockback (`mState == STATE_UMBRELLA_KNOCKING`), load it: no crash, leaf returns to a coherent state. Verify `mDoUmbrellaKnockbackFired` is NOT Sync'd (defaults false on load — leaf may re-arm early; acceptable for this transient ID-aware flag, or Sync it if strictness).
- [ ] Save game **without** the new state (e.g. during reflect), load it: no regression.

### Cross-platform / build

- [ ] Release build compiles: `cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`.
- [ ] WASM (Emscripten) smoke check — knockback is a CPU-side position/HP delta, GPU-independent.

## Notes for the implementer

- **Do NOT add a `NUM_PLANTSTATES` sentinel** to `PlantState`, and do NOT insert `STATE_UMBRELLA_KNOCKING` in the middle of the enum — both would shift serialized state values and break save compatibility for every other state (`NUM_PLANT_LAYERS` exists in a DIFFERENT enum and is irrelevant here).
- `mDoUmbrellaKnockbackFired` is a transient boolean reset on load — verify it is NOT added to the `Plant::Sync` block in `SaveGame.cpp`. If it is added, set `Sync` flags appropriately or omit.
- Match 4-space indent / `mMember` naming of surrounding code in `Zombie.cpp` if mirroring `KnockBack`'s immunity list anywhere in `Plant`.
- No `/*inline*/` comment markers are required for the new helpers — they are plain methods.
- `"anim_block"` is a short one-shot clip. If playtesting shows the hit firing on `mLoopCount > 0` reads as lagging the animation, move the `DoUmbrellaKnockback()` call to 2-3 frames after state entry (use a separate small counter) — a few lines, no structural change.
