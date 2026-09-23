$ErrorActionPreference = 'Stop'

# Source-level checks for the Electric Starfruit (SEED_ELECTRIC_STARFRUIT):
#   red card + upgrade card (upgraded from Starfruit, 300 sun, 30s cooldown) / travel-only /
#   5 homing electric stars that stick to the zombie they hit for 5 s, dealing 30 damage
#   per game tick (10 ms, the same tick the electric pea uses) /
#   everything else identical to Starfruit / art = Starfruit with body+eyes swapped for the
#   electric sprites.
#   Plus the new "ultimate switch": Starfruit @ Electric Gatling Pea -> Electric Starfruit
#   (and Gatling Pea @ Electric Starfruit -> Electric Gatling Pea), each refunding 225 sun.
# This repo has no automated test suite (see AGENTS.md), so like the other check-*.ps1 scripts
# this asserts against the source. Actual feel (homing, 5 s stick, damage rate) still needs
# manual play-testing.
# Design doc: docs/superpowers/specs/2026-09-14-electric-starfruit-design.md

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

$plantPath = P 'src/Lawn/Plant.cpp'
$plantCpp = Get-Content -Raw -LiteralPath $plantPath
$projPath = P 'src/Lawn/Projectile.cpp'
$projCpp = Get-Content -Raw -LiteralPath $projPath
$boardPath = P 'src/Lawn/Board.cpp'

# --- enums: inserted before the NUM_* sentinels, no existing value moves ---
Assert-Source (P 'src/ConstEnums.h') 'SEED_ELECTRIC_GATLING_PEA,[\s\S]{0,300}SEED_ELECTRIC_STARFRUIT,[\s\S]{0,2000}NUM_SEED_TYPES' 'SEED_ELECTRIC_STARFRUIT must be declared after SEED_ELECTRIC_GATLING_PEA and before NUM_SEED_TYPES (later mod seeds appended after it are fine).'
Assert-Source (P 'src/ConstEnums.h') 'REANIM_ELECTRIC_GATLINGPEA,[\s\S]{0,400}REANIM_ELECTRIC_STARFRUIT,[\s\S]{0,2000}NUM_REANIMS' 'REANIM_ELECTRIC_STARFRUIT must be declared after REANIM_ELECTRIC_GATLINGPEA and before NUM_REANIMS (later mod reanims appended after it are fine).'
Assert-Source (P 'src/ConstEnums.h') 'PROJECTILE_FIREPEA_RED = 14,[\s\S]{0,300}PROJECTILE_ELECTRIC_STAR = 15,[\s\S]{0,400}NUM_PROJECTILES = 17' 'PROJECTILE_ELECTRIC_STAR must stay right after PROJECTILE_FIREPEA_RED; later mod projectiles append after it.'

# --- plant definition: 300 sun, 30.01s cooldown, shooter, 100 launch rate, Starfruit health.
# The reanim type stays REANIM_STARFRUIT here on purpose -- the electric type is only chosen at
# runtime by ElectricStarfruitReanimType(), so a missed call site degrades safely.
Assert-Source $plantPath 'SeedType::SEED_ELECTRIC_STARFRUIT,\s*nullptr,\s*ReanimationType::REANIM_STARFRUIT,\s*30,\s*300,\s*3000,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*100,\s*"ELECTRIC_STARFRUIT"' 'gPlantDefs row must default to REANIM_STARFRUIT / 300 sun / 3000 refresh / SHOOTER / 100 launch rate / "ELECTRIC_STARFRUIT".'

# --- dedicated reanim type: same file as Starfruit, own definition slot, no atlas ---
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_ELECTRIC_STARFRUIT,\s*"reanim/Starfruit\.reanim"' 'The electric type must load the same reanim file as Starfruit (its images are patched afterwards).'
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_ELECTRIC_STARFRUIT,[\s\S]{0,200}REANIM_NO_ATLAS' 'The electric reanim type must use REANIM_NO_ATLAS.'

# --- custom art: body + both eye cels, swapped per FRAME by original image pointer ---
$artBody = [regex]::Match($plantCpp, 'bool ElectricStarfruitHasCustomArt\(\)[\s\S]*?\n\}')
if (-not $artBody.Success) { throw 'Could not find ElectricStarfruitHasCustomArt().' }
foreach ($art in @('ELECTRIC_STARFRUIT_BODY', 'ELECTRIC_STARFRUIT_EYES1', 'ELECTRIC_STARFRUIT_EYES2')) {
    if ($artBody.Value -notmatch $art) { throw "ElectricStarfruitHasCustomArt() must swap in $art." }
}
if ($artBody.Value -notmatch 'ApplyReanimArtSwaps') { throw 'The electric starfruit art swap must reuse the shared ApplyReanimArtSwaps() helper.' }
# Fail safe: an image without real transparency would be drawn as a white box; a size mismatch
# would shift the reanim's per-frame sprite columns.
if ($plantCpp -notmatch 'mHasTrans' -or $plantCpp -notmatch 'CommitBits\(\)') { throw 'The shared art swap must reject opaque art (mHasTrans/mHasAlpha after CommitBits).' }
if ($plantCpp -notmatch 'aImage->mWidth != aOldImage->mWidth') { throw 'The shared art swap must reject art whose size differs from the original.' }
# The tint must only be the fallback, never applied on top of the custom art.
Assert-Source $plantPath 'SEED_ELECTRIC_STARFRUIT && !ElectricStarfruitUsesCustomArt\(\)' 'The electric-blue tint must be gated on the custom art being unavailable.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_ELECTRIC_STARFRUIT && !ElectricStarfruitUsesCustomArt\(\)' 'The cached card art must use the same gate.'
Assert-Source (P 'src/GameConstants.h') 'ELECTRIC_STARFRUIT_USE_CUSTOM_ART\s*=\s*true' 'The custom-art switch should be on (the packed art has alpha).'
Assert-Source $plantPath 'if \(!ELECTRIC_STARFRUIT_USE_CUSTOM_ART\)\s*return false' 'ElectricStarfruitUsesCustomArt() must short-circuit when the switch is off.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'ELECTRIC_STARFRUIT_USE_CUSTOM_ART[\s\S]{0,200}ElectricStarfruitHasCustomArt\(\)[\s\S]{0,2000}ReanimationInitializeType' 'When enabled, the card-art path must load the custom art BEFORE ReanimationInitializeType (atlas is created there).'
Assert-Source $plantPath 'theSeedType == SeedType::SEED_ELECTRIC_STARFRUIT\)\s*aReanimType = ElectricStarfruitReanimType\(\)' 'PlantInitialize must resolve the electric reanim type (which loads the custom art) before AddReanimation().'
Assert-Source $plantPath 'ReanimationType ElectricStarfruitReanimType\(\)[\s\S]{0,300}REANIM_STARFRUIT' 'ElectricStarfruitReanimType() must fall back to REANIM_STARFRUIT when the custom art is unavailable.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'aReanimType = ElectricStarfruitReanimType\(\)' 'The cached card art must use the same reanim-type resolution.'
# Starfruit draws its eyes from its own blink reanim, so that instance needs the electric type too.
Assert-Source $plantPath 'SEED_ELECTRIC_STARFRUIT\)\s*aBlinkReanimType = ElectricStarfruitReanimType\(\)' 'AttachBlinkAnim must use the electric reanim type so the blink cels are electric too.'
# Starfruit-specific presentation must cover the electric variant as well.
Assert-Source $plantPath 'SEED_STARFRUIT \|\| theSeedType == SeedType::SEED_ELECTRIC_STARFRUIT\)\s*\{[\s\S]{0,120}10\.0f' 'PlantDrawHeightOffset must give the electric starfruit the same +10 offset as Starfruit.'
Assert-Source $plantPath 'mSeedType == SeedType::SEED_ELECTRIC_STARFRUIT \|\|\s*mSeedType == SeedType::SEED_TANGLEKELP' 'DrawShadow must skip the shadow for the electric starfruit too.'

# --- red card AND upgrade card ---
Assert-Source $plantPath 'bool Plant::IsRedCard[\s\S]{0,900}SEED_ELECTRIC_STARFRUIT' 'Plant::IsRedCard must include SEED_ELECTRIC_STARFRUIT.'
$upgradeBody = [regex]::Match($plantCpp, 'bool Plant::IsUpgrade[\s\S]*?\n\}')
if (-not $upgradeBody.Success) { throw 'Could not find Plant::IsUpgrade body.' }
if ($upgradeBody.Value -notmatch 'SEED_ELECTRIC_STARFRUIT') { throw 'SEED_ELECTRIC_STARFRUIT must be an upgrade card.' }

# --- firing: same 5-way star spread as Starfruit, but the electric projectile ---
$starFire = [regex]::Match($plantCpp, 'void Plant::ElectricStarFruitFire\(\)[\s\S]*?\n\}')
if (-not $starFire.Success) { throw 'Could not find Plant::ElectricStarFruitFire().' }
if ($starFire.Value -notmatch 'ProjectileType::PROJECTILE_ELECTRIC_STAR') { throw 'ElectricStarFruitFire must spawn PROJECTILE_ELECTRIC_STAR.' }
if ($starFire.Value -notmatch 'MOTION_STAR') { throw 'ElectricStarFruitFire must reuse the Starfruit MOTION_STAR trajectory.' }
if ($starFire.Value -notmatch 'i < 5') { throw 'ElectricStarFruitFire must fire exactly 5 stars (same 5-way spread as Starfruit).' }
Assert-Source $plantPath 'if \(mSeedType == SeedType::SEED_ELECTRIC_STARFRUIT\)\s*\{\s*ElectricStarFruitFire\(\)' 'Plant::Fire must route SEED_ELECTRIC_STARFRUIT to ElectricStarFruitFire().'
Assert-Source $plantPath 'mSeedType == SeedType::SEED_STARFRUIT \|\| mSeedType == SeedType::SEED_ELECTRIC_STARFRUIT\)\s*\{\s*LaunchStarFruit\(\)' 'Plant::UpdateShooter must give the electric starfruit the Starfruit attack cadence.'

# --- upgrade relation + requirements ---
Assert-Source $plantPath 'theUpgradedType == SeedType::SEED_ELECTRIC_STARFRUIT && mSeedType == SeedType::SEED_STARFRUIT' 'Plant::IsUpgradableTo must allow Starfruit -> Electric Starfruit.'
Assert-Source $boardPath 'case SeedType::SEED_ELECTRIC_STARFRUIT:\s*return CountPlantByType\(SeedType::SEED_STARFRUIT\)' 'Board::PlantingRequirementsMet must require a planted Starfruit.'

# --- ultimate switch: Starfruit <-> the electric ultimates, refunding sun in Board::MouseDownWithPlant ---
Assert-Source $plantPath 'theUpgradedType == SeedType::SEED_STARFRUIT && mSeedType == SeedType::SEED_ELECTRIC_GATLING_PEA' 'Plant::IsUpgradableTo must let a Starfruit card convert an Electric Gatling Pea.'
Assert-Source $plantPath 'theUpgradedType == SeedType::SEED_GATLINGPEA && mSeedType == SeedType::SEED_ELECTRIC_STARFRUIT' 'Plant::IsUpgradableTo must let a Gatling Pea card convert an Electric Starfruit.'
Assert-Source (P 'src/GameConstants.h') 'ELECTRIC_STARFRUIT_SWITCH_REFUND\s*=\s*225' 'GameConstants.h must define the 225 sun switch refund.'
$mouseBody = [regex]::Match((Get-Content -Raw -LiteralPath $boardPath), 'void Board::MouseDownWithPlant[\s\S]*?\n\}')
if (-not $mouseBody.Success) { throw 'Could not find Board::MouseDownWithPlant body.' }
if ($mouseBody.Value -notmatch 'aPlantSeedType = SeedType::SEED_ELECTRIC_STARFRUIT') { throw 'MouseDownWithPlant must rewrite the planted seed to SEED_ELECTRIC_STARFRUIT when a Starfruit is dropped on an Electric Gatling Pea.' }
if ($mouseBody.Value -notmatch 'aPlantSeedType = SeedType::SEED_ELECTRIC_GATLING_PEA') { throw 'MouseDownWithPlant must rewrite the planted seed to SEED_ELECTRIC_GATLING_PEA on the reverse switch.' }
if ($mouseBody.Value -notmatch 'AddSunMoney\(ELECTRIC_STARFRUIT_SWITCH_REFUND\)') { throw 'MouseDownWithPlant must refund the switch cost with AddSunMoney(ELECTRIC_STARFRUIT_SWITCH_REFUND).' }
if ($mouseBody.Value -notmatch 'AddPlant\(aGridX, aGridY, aPlantSeedType, aPlantImitaterType\)') { throw 'MouseDownWithPlant must plant the rewritten seed type (aPlantSeedType), not the raw cursor type.' }
# The Gatling Pea card must be pickable when the only switch target is an Electric Starfruit.
Assert-Source $boardPath 'case SeedType::SEED_GATLINGPEA:\s*return CountPlantByType\(SeedType::SEED_REPEATER\) \|\| CountPlantByType\(SeedType::SEED_ELECTRIC_STARFRUIT\)' 'The Gatling Pea card must also be satisfied by a planted Electric Starfruit (it is the reverse switch).'

# --- travel-only: registration, HasSeedType gate, chooser pairing, obtain path ---
Assert-Source (P 'src/Lawn/Travel.cpp') 'SEED_ELECTRIC_STARFRUIT,\s*true' 'SEED_ELECTRIC_STARFRUIT must be registered as an upgrade card in gTravelPlantDefs.'
Assert-Source (P 'src/LawnApp.cpp') 'case SeedType::SEED_ELECTRIC_STARFRUIT:[\s\S]{0,80}return IsTravelLevel\(mGameMode\)' 'LawnApp::HasSeedType must gate SEED_ELECTRIC_STARFRUIT on IsTravelLevel.'
Assert-Source (P 'src/Lawn/Widget/SeedChooserScreen.cpp') 'CheckSeedUpgrade\(SEED_ELECTRIC_STARFRUIT, SEED_STARFRUIT\)' 'The seed chooser must require Starfruit alongside SEED_ELECTRIC_STARFRUIT.'
Assert-Source $boardPath 'gIceSandboxTravelSeeds\[\][\s\S]{0,700}SEED_ELECTRIC_STARFRUIT' 'SEED_ELECTRIC_STARFRUIT must be on the ice sandbox travel page.'

# --- cooldown: keep the 30.01s tier, not the generic 3500 upgrade override ---
Assert-Source (P 'src/Lawn/SeedPacket.cpp') 'aUseSeedType == SeedType::SEED_ELECTRIC_GATLING_PEA \|\| aUseSeedType == SeedType::SEED_ELECTRIC_STARFRUIT' 'SeedPacket::SetPacketType must exempt SEED_ELECTRIC_STARFRUIT from the generic upgrade cooldown overrides.'
Assert-Source (P 'src/Lawn/SeedPacket.cpp') 'case SeedType::SEED_STARFRUIT:\s*case SeedType::SEED_ELECTRIC_STARFRUIT:\s*aScale = 0\.5f' 'The seed packet art must reuse the Starfruit card scale/offset.'

# --- the electric star projectile: homing flight, 5 s stick, 30 damage per tick ---
Assert-Source $projPath 'PROJECTILE_ELECTRIC_STAR,\s*0,\s*ELECTRIC_STAR_HIT_DAMAGE' 'gProjectileDefinition must register PROJECTILE_ELECTRIC_STAR with the electric-star damage.'
Assert-Source (P 'src/GameConstants.h') 'ELECTRIC_STAR_HIT_DAMAGE\s*=\s*30' 'GameConstants.h must define the 30 damage per tick.'
Assert-Source (P 'src/GameConstants.h') 'ELECTRIC_STAR_LINGER_TICKS\s*=\s*250' 'GameConstants.h must define the 250-tick (2.5 s) stick duration (rebalanced after the initial 5 s).'
# Homing: the MOTION_STAR target acquisition must cover the electric star as well.
Assert-Source $projPath 'PROJECTILE_STAR \|\| mProjectileType == ProjectileType::PROJECTILE_ELECTRIC_STAR\) && mProjectileAge >= 24' 'The "fly one tile then home" logic must cover PROJECTILE_ELECTRIC_STAR.'
# It must not vanish on impact: the collision branch hands over to the lingering state.
Assert-Source $projPath 'PROJECTILE_ELECTRIC_STAR\)\s*\{[\s\S]{0,900}StartElectricStarLinger\(aZombie\)' 'CheckForCollision must start the sticking state for PROJECTILE_ELECTRIC_STAR instead of DoImpact/Die.'
$lingerBody = [regex]::Match($projCpp, 'void Projectile::UpdateElectricStarLinger\(\)[\s\S]*?\n\}')
if (-not $lingerBody.Success) { throw 'Could not find Projectile::UpdateElectricStarLinger().' }
if ($lingerBody.Value -notmatch 'TakeDamage\(ELECTRIC_STAR_HIT_DAMAGE') { throw 'The sticking star must damage its zombie once per game tick while it lasts.' }
if ($lingerBody.Value -notmatch 'mLingerCountdown') { throw 'The sticking star must count its remaining ticks down.' }
if ($lingerBody.Value -notmatch 'Die\(\)') { throw 'The sticking star must disappear when its run is over.' }
if ($lingerBody.Value -notmatch 'mPosX = aZombie->mX') { throw 'The sticking star must follow the zombie it is stuck to.' }
# Retarget: once the stuck zombie dies (EffectedByDamage() already rejects dying zombies), the star
# must hunt the next target instead of idling on the corpse.
if ($lingerBody.Value -notmatch 'RetargetElectricStar\(\)') { throw 'When the stuck zombie dies the star must automatically retarget.' }
if ($lingerBody.Value -notmatch 'EffectedByDamage') { throw 'The sticking star must treat "no longer damageable" (i.e. dying) as "target gone".' }
$retargetBody = [regex]::Match($projCpp, 'bool Projectile::RetargetElectricStar\(\)[\s\S]*?\n\}')
if (-not $retargetBody.Success) { throw 'Could not find Projectile::RetargetElectricStar().' }
if ($retargetBody.Value -notmatch 'EffectedByDamage') { throw 'Retargeting must only pick a zombie the star can actually damage (this is what skips dead/dying zombies).' }
if ($retargetBody.Value -notmatch 'mElectricStarStuck = false') { throw 'Retargeting must leave the sticking state so the homing flight takes over.' }
if ($retargetBody.Value -notmatch 'mTargetZombieID = mBoard->ZombieGetID\(aBestZombie\)') { throw 'Retargeting must lock onto the newly chosen zombie.' }
if ($retargetBody.Value -notmatch 'return false') { throw 'Retargeting must report failure when no target is left (the star then idles out its remaining life).' }
$startBody = [regex]::Match($projCpp, 'void Projectile::StartElectricStarLinger\(Zombie\* theZombie\)[\s\S]*?\n\}')
if (-not $startBody.Success) { throw 'Could not find Projectile::StartElectricStarLinger().' }
if ($startBody.Value -notmatch 'mLingerCountdown = ELECTRIC_STAR_LINGER_TICKS') { throw 'StartElectricStarLinger must arm the 5 s duration.' }
if ($startBody.Value -notmatch 'if \(mLingerCountdown <= 0\)[\s\S]{0,120}mLingerCountdown = ELECTRIC_STAR_LINGER_TICKS') { throw 'The lifetime must only be armed on the FIRST impact, not refreshed on every re-hit after retargeting.' }
if ($startBody.Value -notmatch 'mElectricStarStuck = true') { throw 'StartElectricStarLinger must enter the sticking state.' }
if ($startBody.Value -notmatch 'mTargetZombieID = mBoard->ZombieGetID\(theZombie\)') { throw 'StartElectricStarLinger must remember which zombie it stuck to.' }
# While stuck it must skip flight/collision entirely; while flying to the next target the lifetime
# must keep draining (so the 5 s stays a total lifetime).
Assert-Source $projPath 'IsElectricStarStuck\(\)\)\s*\{\s*UpdateElectricStarLinger\(\);\s*return;' 'Projectile::Update must run the sticking tick instead of UpdateMotion() while stuck.'
Assert-Source $projPath 'mLingerCountdown > 0\)\s*\{\s*mLingerCountdown--;' 'The star lifetime must also drain while it flies to the next target.'
# Same electric-blue silhouette treatment as the electric pea, so the star matches the plant.
Assert-Source $projPath 'PROJECTILE_FIREPEA_RED \|\| mProjectileType == ProjectileType::PROJECTILE_ELECTRIC_STAR' 'The electric star must be drawn with the shared white-silhouette + ELECTRIC_BLUE paint path.'
Assert-Source $projPath 'case ProjectileType::PROJECTILE_ELECTRIC_STAR:\s*// 钉在僵尸身上' 'The electric star must not draw a ground shadow.'

# --- save compatibility: the new projectile fields are appended to the TLV tail ---
Assert-Source (P 'src/Lawn/System/SaveGame.cpp') 'mLastPortalX\);[\s\S]{0,400}SyncInt32\(theProjectile\.mLingerCountdown\)[\s\S]{0,200}SyncBool\(theProjectile\.mElectricStarStuck\)' 'mLingerCountdown/mElectricStarStuck must be appended to the end of SyncProjectileTailPortable so old saves still load.'

# --- player-facing text ---
Assert-Source (P 'src/Lawn/SeedPacket.cpp') '\[ADVICE_PLANT_NEEDS_STARFRUIT\]' 'SeedPacket must show a "needs Starfruit" advice.'
Assert-Source $boardPath '\[ADVICE_ONLY_ON_STARFRUIT\]' 'Board must show a "only on Starfruit" advice.'
Assert-Source $boardPath '\[REQUIRES_STARFRUIT\]' 'The seed packet tooltip must show a "requires Starfruit" warning.'

foreach ($file in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $file) '<String id="ELECTRIC_STARFRUIT">' "$file must define the ELECTRIC_STARFRUIT name string."
    Assert-Source (P $file) '<String id="ELECTRIC_STARFRUIT_TOOLTIP">' "$file must define the ELECTRIC_STARFRUIT_TOOLTIP string."
    Assert-Source (P $file) '<String id="REQUIRES_STARFRUIT">' "$file must define the REQUIRES_STARFRUIT string."
    Assert-Source (P $file) '<String id="ADVICE_PLANT_NEEDS_STARFRUIT">' "$file must define the ADVICE_PLANT_NEEDS_STARFRUIT string."
    Assert-Source (P $file) '<String id="ADVICE_ONLY_ON_STARFRUIT">' "$file must define the ADVICE_ONLY_ON_STARFRUIT string."
    Assert-Source (P $file) '<String id="ULTIMATE_SWITCH_REFUND">' "$file must define the ULTIMATE_SWITCH_REFUND string."
}

Write-Output 'Electric Starfruit (SEED_ELECTRIC_STARFRUIT) source checks passed.'
