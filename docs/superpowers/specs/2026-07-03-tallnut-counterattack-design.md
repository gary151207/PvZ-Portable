# Tall-nut Counterattack Skill — Design

## Date
2026-07-03

## Summary
Tall-nut periodically damages zombies eating it and heals itself.

## Mechanics

| Parameter | Value | Notes |
|-----------|-------|-------|
| Cooldown | 500 ticks (~5s) | Game ticks |
| Damage | 100 per eating zombie | Goes through zombie armor (helmet/bucket/etc) |
| Self-heal | 75 HP | Capped at `mPlantMaxHealth` |
| Trigger | Any zombie eating Tall-nut | No minimum eater count |
| Damage flags | 0U | Plain damage, no special flags |
| VFX/SFX | None | No particles or sound effects |

## Implementation

### Files modified

- `src/Lawn/Plant.h` — new member `mTallnutCounterCooldown`, new method `UpdateTallnut()`
- `src/Lawn/Plant.cpp` — init in constructor, implement `UpdateTallnut()`, hook into `UpdateAbilities()`

### `Plant::UpdateTallnut()`

1. Countdown tick: decrement `mTallnutCounterCooldown` if > 0, return early
2. Iterate all zombies on board; for each with `mTargetPlantID == this`, deal 100 damage via `TakeDamage(100, 0U)`
3. If any eater found, heal `std::min(mPlantHealth + 75, mPlantMaxHealth)`
4. Set `mTallnutCounterCooldown = 500`

### Files NOT modified

- `ConstEnums.h` — no new enum values
- `Zombie.*` — no changes
- `Board.*` — no changes
