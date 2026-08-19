# Umbrella Leaf Reflects Zombie Peas — Design

Date: 2026-08-19
Status: Approved by user (brainstorming session)

## Goal

Let the Umbrella Leaf (`SEED_UMBRELLA`) reflect enemy zombie peas
(`PROJECTILE_ZOMBIE_PEA`) into friendly peas that fly right and damage zombies,
with a visual/animation feedback each time.

## Current behavior (as found in code)

- The Umbrella Leaf already has a custom state machine driven by
  `Plant::UpdateUmbrella()`:
  - `STATE_UMBRELLA_TRIGGERED` (5 frames) → `STATE_UMBRELLA_REFLECTING`
    (while `anim_block` plays) → `STATE_NOTREADY`.
  - A separate knockback mechanic (`STATE_UMBRELLA_KNOCKING`) shoves adjacent
    zombies and costs 50 HP per shove (project-specific, keep untouched).
- Basketballs (`PROJECTILE_BASKETBALL`, `MOTION_LOBBED`) are intercepted inside
  `Projectile::UpdateLobMotion()`: if the umbrella is `STATE_UMBRELLA_REFLECTING`
  the basketball is destroyed with `PARTICLE_UMBRELLA_REFLECT` + `FOLEY_SPLAT`;
  otherwise `Plant::DoSpecial()` is triggered (starts `anim_block`).
- **Zombie peas never reach that code path.** They travel with
  `MOTION_BACKWARDS` (also `MOTION_STAR` scatter peas and `MOTION_WAVE` elite
  peas), so they are updated by `Projectile::UpdateNormalMotion()` →
  `Projectile::CheckForCollision()`, whose `PROJECTILE_ZOMBIE_PEA` branch
  directly damages the first plant it overlaps. The umbrella currently does
  nothing against zombie peas.

## Behavior decisions (agreed with user)

1. **Always reflect** — every zombie pea that would hit a plant within 1 cell
   of an umbrella is reflected the instant it arrives. No cooldown.
2. **Free** — reflecting costs no HP (unlike the 50 HP knockback).
3. **Only peas** — basketballs keep their current block/destroy behavior.
4. **Reflected pea = normal friendly pea** — `PROJECTILE_PEA`, 20 damage to
   zombies, can be upgraded by Torchwood into a fireball (natural interaction,
   keep it). It flies right (`MOTION_STRAIGHT`) in the same row and never
   damages friendly plants.

## Implementation (Approach 1: convert in place)

Single edit site: `Projectile::CheckForCollision()`, `PROJECTILE_ZOMBIE_PEA`
branch (around line 383 of `src/Lawn/Projectile.cpp`).

Before damaging `aPlant`:

1. `Plant* aUmbrellaPlant = mBoard->FindUmbrellaPlant(aPlant->mPlantCol, aPlant->mRow);`
   (reuses `Board::FindUmbrellaPlant`, range 1 cell, same as the basketball path).
2. If an umbrella was found:
   - If it is not already `STATE_UMBRELLA_TRIGGERED` / `STATE_UMBRELLA_REFLECTING`:
     `mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA); aUmbrellaPlant->DoSpecial();`
     (starts the `anim_block` open animation). Do NOT restart the animation for
     peas arriving during the animation window.
   - Reflect the pea in place:
     - `mProjectileType = ProjectileType::PROJECTILE_PEA;`
     - `mMotionType = ProjectileMotion::MOTION_STRAIGHT;` (moves right 3.33 px/frame)
     - `mHitTorchwoodGridX = -1;` (so Torchwood conversion works)
     - `mDamageRangeFlags = 0;` (defensive; zombie peas already spawn with 0)
     - `AttachmentDie(mAttachmentID);` (zombie pea has no attachment; safe, mirrors `ConvertToPea`)
   - Spawn feedback at the reflect point (mirrors basketball style):
     `mApp->AddTodParticle(mPosX + 20.0f, mPosY + 20.0f,
     Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 1),
     ParticleEffect::PARTICLE_UMBRELLA_REFLECT);`
   - `return;` — the pea is now a friendly pea; on later frames it uses the
     zombie-collision path (`FindCollisionTarget` → `DoImpact`, 20 damage).
3. Otherwise (no umbrella) — existing plant-damage code runs unchanged.

### Why this is safe

- The converted pea's type is no longer `PROJECTILE_ZOMBIE_PEA`, so it can
  never re-enter the umbrella branch (no infinite reflection).
- Friendly peas collide only with zombies (`FindCollisionTarget`), never with
  the umbrella or other plants — except the intended Torchwood upgrade.
- `MOTION_STRAIGHT` ignores `mVelX/mVelY/mBaseY`, so the leftover velocity from
  `MOTION_STAR`/`MOTION_WAVE` spawns is irrelevant.
- `STATE_UMBRELLA_TRIGGERED`/`REFLECTING` still work for basketballs unchanged.

## Out of scope

- Changing basketball behavior.
- Changing the knockback mechanic / HP costs.
- Cleaning up the dead `PROJECTILE_ZOMBIE_PEA` case inside `UpdateLobMotion()`
  (harmless, unreachable for peas).

## Testing

No automated test suite — manual gameplay testing:
- Place an umbrella + a plant behind it; have a Peashooter zombie shoot:
  peas get bounced back and damage zombies; umbrella plays `anim_block` +
  particle + sound each time.
- Verify a pea aimed at a plant NOT near an umbrella still damages it normally.
- Verify a Torchwood behind the umbrella turns reflected peas into fireballs.
- Verify basketballs still get blocked/destroyed as before.
- Verify no crashes with gatling scatter peas (`MOTION_STAR`) and elite pea
  shooter (`MOTION_WAVE`) reflections.
- Quick save/load sanity check (no DataSync changes, but cheap to confirm).
