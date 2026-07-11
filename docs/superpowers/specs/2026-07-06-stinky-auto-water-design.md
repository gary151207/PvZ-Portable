# Stinky Auto-Watering Design

## Summary

Make Zen Garden's snail (Stinky) automatically water plants that need it, with a watering animation.

## Motivation

In the vanilla game, Stinky only collects coins. The player must manually use the watering can on each thirsty plant. This feature lets Stinky also water plants automatically while he's awake and wandering the garden.

## Behavior

- Stinky automatically detects plants that need water (`PLANTNEED_WATER`)
- Coin collection still takes priority over watering
- When Stinky arrives at a thirsty plant, a watering can animation plays, then `PlantWatered()` is called
- Stinky then picks a new goal and continues crawling
- Only active when Stinky is awake (`ShouldStinkyBeAwake()`)
- Only in the main garden (`GARDEN_MAIN`), since Stinky only exists there

## Implementation

### 1. `StinkyPickGoal` — add thirsty plants as secondary goal

**Current logic:**
1. Scan coins → pick best coin as goal
2. No coins → pick random plant grid as goal (weighted by plant presence)

**New logic:**
1. Scan coins → pick best coin as goal (unchanged)
2. No coins → scan for plants needing water → pick as goal
3. No thirsty plants → pick random plant grid as goal (unchanged)

For step 2, iterate plants on the board. For each plant with a `PottedPlantIndex`, check `GetPlantsNeed() == PLANTNEED_WATER`. Build a weighted grid array favoring closer plants, then pick one.

### 2. `StinkyUpdate` — trigger watering on arrival

In the arrival check (`fabs(aDeltaX) < 5.0f && fabs(aDeltaY) < 5.0f`), before calling `StinkyPickGoal`:

Check if there's a thirsty plant at the goal position. If so:
- Create a `GRIDITEM_ZEN_TOOL` with `GRIDITEM_STATE_ZEN_TOOL_WATERING_CAN`
- Attach the existing watering can reanimation (`REANIM_ZENGARDEN_WATERINGCAN`, `anim_water`)
- Set its position to the plant's position
- `ZenToolUpdate` will call `DoFeedingTool` → `PlantWatered()` when the animation finishes

Then call `StinkyPickGoal` to get the next target.

### 3. `StinkyPickGoal` new function signature or approach

No signature change needed. The function can access `mBoard` to iterate plants and check their needs via `mApp->mZenGarden->GetPlantsNeed()`.

### Files Modified

- `src/Lawn/ZenGarden.cpp` — `StinkyPickGoal` and `StinkyUpdate`

### Unchanged

- No new animations needed (reuses existing watering can animation)
- No new save data
- No new UI
- `PlantWatered()` logic unchanged (watering timer, coin drop, tutorial advancement)

## Edge Cases

- **Stinky asleep**: `StinkyUpdate` returns early before movement logic, so no watering occurs
- **Golden watering can**: Golden can logic is in `DoFeedingTool` / `MouseDownWithFeedingTool`; auto-watering uses the standard watering can tool, not the golden one
- **Plant dies or is removed**: The plant check at arrival time verifies the plant still exists and still needs water before applying
- **Multiple thirsty plants**: Picked one at a time via weighted grid; Stinky will visit them sequentially
