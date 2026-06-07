# Zombie HP Tooltip on Hover — Design

**Date:** 2026-06-07  
**Branch:** `feat/mod`

## Summary

When the player hovers the mouse over a zombie, show its total health (sum of body, helmet, shield, flying HP) in the existing `ToolTipWidget`. Works in all game scenes — both the level intro (seed selection) cutscene and active gameplay.

## Decisions

| Question | Decision |
|----------|----------|
| When to show? | All scenes (level intro + gameplay) |
| What HP to show? | Total HP only (sum of all parts): `mBodyHealth + mHelmHealth + mShieldHealth + mFlyingHealth` |
| How to display? | Reuse existing `ToolTipWidget`, set `mLabel` to HP string |
| Toggle? | Always on, no CLI flag needed |

## Affected Code

**Single file:** `src/Lawn/Board.cpp` → `Board::UpdateToolTip()`

### Change 1: Level intro scene (`SCENE_LEVEL_INTRO`)

The existing zombie hover tooltip shows the zombie name and `[CLICK_TO_VIEW]`. Add total HP to the label alongside or replacing the click hint.

### Change 2: Gameplay scene (`SCENE_PLAYING`)

Currently no zombie hover detection here. After the `mChallenge->UpdateToolTip()` call and before the `MouseHitTest` switch, insert:

```cpp
Zombie* aZombie = ZombieHitTest(aMouseX, aMouseY);
if (aZombie) {
    // Calculate total HP
    int aTotalHP = aZombie->mBodyHealth + aZombie->mHelmHealth 
                 + aZombie->mShieldHealth + aZombie->mFlyingHealth;
    int aMaxHP = aZombie->mBodyMaxHealth + aZombie->mHelmMaxHealth 
               + aZombie->mShieldMaxHealth + aZombie->mFlyingMaxHealth;
    
    mToolTip->SetTitle(GetZombieDefinition(aZombie->mZombieType).mZombieName);
    mToolTip->SetLabel(StrFormat("HP: %d/%d", aTotalHP, aMaxHP));
    mToolTip->SetWarningText("");
    
    Rect aRect = aZombie->GetZombieRect();
    mToolTip->mX = aRect.mWidth / 2 + aRect.mX + 5;
    mToolTip->mY = aRect.mHeight + aRect.mY - 10;
    mToolTip->mCenter = true;
    mToolTip->mMinLeft = 0;
    mToolTip->mMaxBottom = BOARD_HEIGHT;
    mToolTip->mVisible = true;
    return;
}
```

### HP Calculation

```cpp
int totalHP = mBodyHealth + mHelmHealth + mShieldHealth + mFlyingHealth;
int maxHP   = mBodyMaxHealth + mHelmMaxHealth + mShieldMaxHealth + mFlyingMaxHealth;
```

All four health components are always present on every zombie (default to 0 for unused parts).

## Display Format

- **Level intro:** `[Zombie Name]` as title, `HP: 300/500` as label
- **Gameplay:** Zombie name as title, `HP: 300/500` as label

## Estimated Changes

~30 lines added to `Board::UpdateToolTip()`, no new files, no header changes.
