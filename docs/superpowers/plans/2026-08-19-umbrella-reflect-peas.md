# Umbrella Leaf Reflects Zombie Peas — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Zombie peas (`PROJECTILE_ZOMBIE_PEA`) that would hit a plant within 1 cell of an Umbrella Leaf are reflected in place into friendly peas (`PROJECTILE_PEA`, `MOTION_STRAIGHT`) flying right and damaging zombies, with the umbrella opening animation (`anim_block`) + `FOLEY_UMBRELLA` sound + `PARTICLE_UMBRELLA_REFLECT` particle as feedback.

**Architecture:** Single edit in `Projectile::CheckForCollision()`'s `PROJECTILE_ZOMBIE_PEA` branch (the only path zombie peas actually use — `MOTION_BACKWARDS`/`MOTION_STAR`/`MOTION_WAVE` all route through `UpdateNormalMotion()` → `CheckForCollision()`). Before damaging the target plant, query `Board::FindUmbrellaPlant()`. If an umbrella exists: trigger `Plant::DoSpecial()` (starts `anim_block`) when not already animating, convert the pea in place (`mProjectileType = PROJECTILE_PEA`, `mMotionType = MOTION_STRAIGHT`, reset `mHitTorchwoodGridX`/`mDamageRangeFlags`), emit the reflect particle, and `return`. Basketballs (`UpdateLobMotion()`) and the knockback mechanic are untouched.

**Tech Stack:** C++20, CMake + Ninja, SDL2. Game assets (`main.pak`, `properties/`) are user-supplied and required to run the game.

## Global Constraints

- Build: `cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release` then `cmake --build build` (a configured `build/` may already exist; just run the build step).
- Naming: SexyAppFramework convention — `mMember`, `theParameter`, `aLocal`.
- No automated test suite in this repo: verification is manual gameplay testing.
- Do not reorder `ConstEnums.h` values; do not touch `TodDrawTriangle*.cpp` build files (not relevant here — nothing in this plan touches them).
- Follow existing enum-branching patterns; do not subclass plants/zombies.
- License headers are required on new files (this plan creates no new files).

---

### Task 1: Reflect zombie peas via the umbrella in CheckForCollision

**Files:**
- Modify: `src/Lawn/Projectile.cpp` — the `PROJECTILE_ZOMBIE_PEA` block inside `CheckForCollision()` (currently ~line 383–398, `if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA) { Plant* aPlant = FindCollisionTargetPlant(); ... }`)

**Interfaces:**
- Consumes:
  - `Board::FindUmbrellaPlant(int theGridX, int theGridY)` → `Plant*` (exists, `src/Lawn/Board.cpp:9948`; 1-cell range via `GridInRange`).
  - `Plant::DoSpecial()` (public, exists; the `SEED_UMBRELLA` case sets `STATE_UMBRELLA_TRIGGERED` + plays `anim_block`).
  - `PlantState::STATE_UMBRELLA_TRIGGERED` / `STATE_UMBRELLA_REFLECTING` (exist in `Plant.h`).
  - `PlantState` and `Plant::mState` are already used in this file (see `UpdateLobMotion()` umbrella branch).
- Produces: reflected peas are ordinary friendly peas — later tasks/nothing else depends on this change.

- [ ] **Step 1: Locate and read the current ZOMBIE_PEA branch**

Run: `sed -n '319,410p' src/Lawn/Projectile.cpp`

Expected: you see `void Projectile::CheckForCollision()` starting at line ~319, and inside it:

```cpp
	if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA)
	{
		Plant* aPlant = FindCollisionTargetPlant();
		if (aPlant)
		{
			const ProjectileDefinition& aProjectileDef = GetProjectileDef();
			aPlant->mPlantHealth -= aProjectileDef.mDamage;
			aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 25);

			mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
			mApp->AddTodParticle(mPosX - 3.0f, mPosY + 17.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_PEA_SPLAT);
			Die();
		}
		return;
	}
```

Also verify `AttachmentDie` is already called elsewhere in this file (it is, in `ConvertToPea`), and that `FoleyType::FOLEY_UMBRELLA` and `ParticleEffect::PARTICLE_UMBRELLA_REFLECT` exist (they are used in `UpdateLobMotion()`).

- [ ] **Step 2: Apply the edit**

Replace the block above with (umbrella check first; the original plant-damage code stays verbatim as the else path):

```cpp
	if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA)
	{
		Plant* aPlant = FindCollisionTargetPlant();
		if (aPlant)
		{
			Plant* aUmbrellaPlant = mBoard->FindUmbrellaPlant(aPlant->mPlantCol, aPlant->mRow);
			if (aUmbrellaPlant)
			{
				// Umbrella Leaf reflects the pea: open the umbrella (anim_block) if it
				// isn't already animating, then convert the pea into a friendly pea
				// flying right, which damages zombies (and can pass through a Torchwood).
				if (aUmbrellaPlant->mState != PlantState::STATE_UMBRELLA_TRIGGERED &&
					aUmbrellaPlant->mState != PlantState::STATE_UMBRELLA_REFLECTING)
				{
					mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA);
					aUmbrellaPlant->DoSpecial();
				}

				mProjectileType = ProjectileType::PROJECTILE_PEA;
				mMotionType = ProjectileMotion::MOTION_STRAIGHT;
				mHitTorchwoodGridX = -1;
				mDamageRangeFlags = 0;
				AttachmentDie(mAttachmentID);

				int aRenderPosition = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 1);
				mApp->AddTodParticle(mPosX + 20.0f, mPosY + 20.0f, aRenderPosition, ParticleEffect::PARTICLE_UMBRELLA_REFLECT);
				return;
			}

			const ProjectileDefinition& aProjectileDef = GetProjectileDef();
			aPlant->mPlantHealth -= aProjectileDef.mDamage;
			aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 25);

			mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
			mApp->AddTodParticle(mPosX - 3.0f, mPosY + 17.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_PEA_SPLAT);
			Die();
		}
		return;
	}
```

Notes (do not skip):
- `AttachmentDie(mAttachmentID)` is safe when `mAttachmentID == ATTACHMENTID_NULL` (zombie peas never attach anything); it mirrors `ConvertToPea()`.
- Do NOT touch the `PROJECTILE_ZOMBIE_PEA` case inside `UpdateLobMotion()` (unreachable for peas, basketball logic stays as-is).
- Tabs for indentation, matching the file's existing style.

- [ ] **Step 3: Build**

Run: `cmake --build build`

Expected: compiles cleanly (no new warnings/errors). If `build/` isn't configured yet: `cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`.

- [ ] **Step 4: Manual gameplay test**

Run the game (assets `main.pak` + `properties/` required — see `run-pvz.bat` / `run-pvz.sh`). Use a debug/cheat level or Adventure with an Umbrella Leaf placed with a plant behind it, then bring in a Peashooter zombie. Verify:

1. Zombie peas aimed at a plant within 1 cell of the umbrella bounce back as friendly peas, deal damage to zombies, and the umbrella plays `anim_block` + particle + `FOLEY_UMBRELLA` each time. (Acceptable: the animation only re-triggers when the previous `anim_block` finished — that's the intended state-machine behavior.)
2. A pea aimed at a plant NOT near an umbrella still damages the plant normally (splat, no reflect).
3. A Torchwood placed behind the umbrella turns reflected peas into fireballs.
4. Basketballs (Catapult zombie) are still blocked/destroyed as before, and the knockback shove still works.
5. No crash with gatling scatter peas (`MOTION_STAR`) and elite pea shooter (`MOTION_WAVE`); reflected peas from those fly straight right.
6. Save/load sanity check (no DataSync changes were made, but confirm the level loads).

- [ ] **Step 5: Commit**

```bash
git add src/Lawn/Projectile.cpp
git commit -m "feat: umbrella leaf reflects zombie peas into friendly peas"
```
