$ErrorActionPreference = 'Stop'

# Source-level checks for the Electric Gatling Pea (SEED_ELECTRIC_GATLING_PEA):
#   red card + upgrade card (upgraded from Gatling Pea, 200 sun) / travel-only /
#   100% electric peas / scatter ultimate identical to Gatling Pea /
#   art = Gatling Pea with the head+barrel brightened to electric white while the
#   lower leaves/stalk and the helmet stay untouched.
# This repo has no automated test suite (see AGENTS.md), so like the other
# check-*.ps1 scripts this asserts against the source. Actual feel (whitening
# look, scatter cadence, pierce/burn) still needs manual play-testing.
# Design doc: docs/superpowers/specs/2026-08-31... see
#   docs/superpowers/specs/2026-09-12-electric-gatling-pea-design.md

# 调用点有的传相对路径、有的传 P 出来的绝对路径，这里统一处理。
# 注意：本函数在 $root 定义**之前**就被插入，所以根目录在这里按 $PSScriptRoot 现算，不依赖外部变量。
function Resolve-RepoFile([string]$theRelPath) {
    if ([System.IO.Path]::IsPathRooted($theRelPath)) { return $theRelPath }
    return (Join-Path (Split-Path -Parent $PSScriptRoot) $theRelPath)
}
function Assert-Source([string]$RelPath, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath (Resolve-RepoFile $RelPath)
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

$plantPath = P 'src/Lawn/Plant.cpp'
$plantCpp = Get-Content -Raw -LiteralPath $plantPath
$plantLines = Get-Content -LiteralPath $plantPath

# --- enum: inserted before NUM_SEED_TYPES, after the existing custom plants ---
# Window widened to 600 chars: the Electric Starfruit seed/reanim entries now sit between
# this value and the NUM_* sentinels (they are appended, never reordered).
Assert-Source (P 'src/ConstEnums.h') 'SEED_PEATER_1_5,[\s\S]{0,600}SEED_ELECTRIC_GATLING_PEA,[\s\S]{0,2000}NUM_SEED_TYPES' 'SEED_ELECTRIC_GATLING_PEA must be declared after SEED_PEATER_1_5 and before NUM_SEED_TYPES (later mod seeds appended after it are fine).'

# --- plant definition: 200 sun, 30.01s cooldown, shooter, 100 launch rate.
# The reanim type stays REANIM_GATLINGPEA here on purpose -- the electric type is only chosen at
# runtime by ElectricGatlingReanimType(), so a missed call site degrades safely.
Assert-Source $plantPath 'SeedType::SEED_ELECTRIC_GATLING_PEA,\s*nullptr,\s*ReanimationType::REANIM_GATLINGPEA,\s*5,\s*200,\s*3000,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*100,\s*"ELECTRIC_GATLING_PEA"' 'gPlantDefs row must default to REANIM_GATLINGPEA / 200 sun / 3000 refresh / SHOOTER / 100 launch rate / "ELECTRIC_GATLING_PEA".'

# --- dedicated reanim type: same file as the Gatling Pea, own definition slot ---
# Window widened to 900 chars for the same reason as the seed enum above.
Assert-Source (P 'src/ConstEnums.h') 'REANIM_FLAG,[\s\S]{0,600}REANIM_ELECTRIC_GATLINGPEA,[\s\S]{0,2000}NUM_REANIMS' 'REANIM_ELECTRIC_GATLINGPEA must be declared after REANIM_FLAG and before NUM_REANIMS (later mod reanims appended after it are fine).'
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_ELECTRIC_GATLINGPEA,\s*"reanim/GatlingPea\.reanim"' 'The electric type must load the same reanim file as the Gatling Pea (its images are patched afterwards).'

# --- custom art: swapped per FRAME by original image pointer (blink tracks use two images) ---
$artBody = [regex]::Match($plantCpp, 'bool ElectricGatlingHasCustomArt\(\)[\s\S]*?\n\}')
if (-not $artBody.Success) { throw 'Could not find ElectricGatlingHasCustomArt().' }
foreach ($art in @('ELECTRICGATLING_HEAD', 'ELECTRICGATLING_MOUTH', 'ELECTRICGATLING_MOUTH_OVERLAY', 'ELECTRICGATLING_BARREL', 'ELETRICGATLING_BLINK1', 'ELETRICGATLING_BLINK2')) {
    if ($artBody.Value -notmatch $art) { throw "ElectricGatlingHasCustomArt() must swap in $art (note the pak spelling 'Eletric' for the blink files)." }
}
# The actual swap mechanics live in the shared ApplyReanimArtSwaps() helper (also used by the
# Electric Starfruit): per-frame image-pointer matching, the transparency / size safety valves
# and the overall bail-out.
$swapHelper = [regex]::Match($plantCpp, 'static bool ApplyReanimArtSwaps\([\s\S]*?\n\}')
if (-not $swapHelper.Success) { throw 'Could not find ApplyReanimArtSwaps().' }
if ($swapHelper.Value -notmatch 'aTransform\.mImage == aOldImage') { throw 'The art swap must match per-frame transforms by original image pointer, not per track: anim_blink / idle_shoot_blink each reference BOTH blink images.' }
# Fail safe: an image without real transparency would be drawn as a white box.
if ($swapHelper.Value -notmatch 'mHasTrans' -or $swapHelper.Value -notmatch 'CommitBits\(\)') { throw 'The art swap must reject opaque art (mHasTrans/mHasAlpha after CommitBits) so a baked white background never renders as boxes.' }
if ($swapHelper.Value -notmatch 'return false') { throw 'The art swap must fall back when the art is missing or unusable.' }
# The tint must only be the fallback, never applied on top of the custom art.
Assert-Source $plantPath 'SEED_ELECTRIC_GATLING_PEA && !ElectricGatlingUsesCustomArt\(\)' 'The electric-blue tint must be gated on the custom art being unavailable.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_ELECTRIC_GATLING_PEA && !ElectricGatlingUsesCustomArt\(\)' 'The cached card art must use the same gate.'
# Feature switch: enabled now that the packed art has a real alpha channel.
Assert-Source (P 'src/GameConstants.h') 'ELECTRIC_GATLING_USE_CUSTOM_ART\s*=\s*true' 'The custom-art switch should be on (the shipped art has alpha).'
Assert-Source $plantPath 'if \(!ELECTRIC_GATLING_USE_CUSTOM_ART\)\s*return false' 'ElectricGatlingUsesCustomArt() must short-circuit when the switch is off.'
# Dimensions must match the art being replaced: the reanim indexes a track image by frame column.
Assert-Source $plantPath 'aImage->mWidth != aOldImage->mWidth' 'The art swap must reject art whose size differs from the original (frame indexing would break).'
# The electric type must not build its own atlas: with REANIM_NO_ATLAS the definition keeps real
# Image pointers, so swapping art never has to race atlas creation (and shares no atlas with the Gatling Pea).
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_ELECTRIC_GATLINGPEA,[\s\S]{0,200}REANIM_NO_ATLAS' 'The electric reanim type must use REANIM_NO_ATLAS.'
# The patch has to run before any reanim of the type is initialised, or the atlas is built from the old art.
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'ELECTRIC_GATLING_USE_CUSTOM_ART[\s\S]{0,200}ElectricGatlingHasCustomArt\(\)[\s\S]{0,2000}ReanimationInitializeType' 'When enabled, the card-art path must load the custom art BEFORE ReanimationInitializeType (atlas is created there).'
Assert-Source $plantPath 'theSeedType == SeedType::SEED_ELECTRIC_GATLING_PEA\)\s*aReanimType = ElectricGatlingReanimType\(\)' 'PlantInitialize must resolve the electric reanim type (which loads the custom art) before AddReanimation().'
# When the custom art is unusable, fall back to the plain Gatling Pea reanim type: that keeps the
# old tinted look and avoids creating a second Atlas over the same shared images.
Assert-Source $plantPath 'ReanimationType ElectricGatlingReanimType\(\)[\s\S]{0,300}REANIM_GATLINGPEA' 'ElectricGatlingReanimType() must fall back to REANIM_GATLINGPEA when the custom art is unavailable.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'aReanimType = ElectricGatlingReanimType\(\)' 'The cached card art must use the same reanim-type resolution.'

# --- red card AND upgrade card ---
Assert-Source $plantPath 'bool Plant::IsRedCard[\s\S]{0,700}SEED_ELECTRIC_GATLING_PEA' 'Plant::IsRedCard must include SEED_ELECTRIC_GATLING_PEA.'
$upgradeBody = [regex]::Match($plantCpp, 'bool Plant::IsUpgrade[\s\S]*?\n\}')
if (-not $upgradeBody.Success) { throw 'Could not find Plant::IsUpgrade body.' }
if ($upgradeBody.Value -notmatch 'SEED_ELECTRIC_GATLING_PEA') { throw 'SEED_ELECTRIC_GATLING_PEA must be an upgrade card.' }

# --- upgrade relation: Gatling Pea -> Electric Gatling Pea, and it needs one planted ---
Assert-Source $plantPath 'theUpgradedType == SeedType::SEED_ELECTRIC_GATLING_PEA && mSeedType == SeedType::SEED_GATLINGPEA' 'Plant::IsUpgradableTo must allow Gatling Pea -> Electric Gatling Pea.'
Assert-Source (P 'src/Lawn/Board.cpp') 'case SeedType::SEED_ELECTRIC_GATLING_PEA:\s*return CountPlantByType\(SeedType::SEED_GATLINGPEA\)' 'Board::PlantingRequirementsMet must require a planted Gatling Pea.'

# --- travel-only: registration, HasSeedType gate, chooser pairing, real obtain path ---
Assert-Source (P 'src/Lawn/Travel.cpp') 'SEED_ELECTRIC_GATLING_PEA,\s*true' 'SEED_ELECTRIC_GATLING_PEA must be registered as an upgrade card in gTravelPlantDefs.'
Assert-Source (P 'src/LawnApp.cpp') 'case SeedType::SEED_ELECTRIC_GATLING_PEA:[\s\S]{0,80}return IsTravelLevel\(mGameMode\)' 'LawnApp::HasSeedType must gate SEED_ELECTRIC_GATLING_PEA on IsTravelLevel.'
Assert-Source (P 'src/Lawn/Widget/SeedChooserScreen.cpp') 'CheckSeedUpgrade\(SEED_ELECTRIC_GATLING_PEA, SEED_GATLINGPEA\)' 'The seed chooser must require Gatling Pea alongside SEED_ELECTRIC_GATLING_PEA.'
Assert-Source (P 'src/Lawn/Board.cpp') 'gIceSandboxTravelSeeds\[\][\s\S]{0,500}SEED_ELECTRIC_GATLING_PEA' 'SEED_ELECTRIC_GATLING_PEA must be on the ice sandbox travel page.'
# Travel levels are all conveyor-belt levels (gTravelLevelDefs mConveyorBelt = true), so chooser
# page 1 never appears in real travel play: the conveyor roster is the only real way to get it.
Assert-Source (P 'src/Lawn/Challenge.cpp') 'GAMEMODE_CHALLENGE_TRAVEL_2[\s\S]{0,1200}SEED_ELECTRIC_GATLING_PEA' 'SEED_ELECTRIC_GATLING_PEA must be on the TRAVEL_2 conveyor roster.'

# --- cooldown: keep the Gatling Pea 30.01s tier, not the generic 3500 upgrade override ---
Assert-Source (P 'src/Lawn/SeedPacket.cpp') 'SEED_GATLINGPEA \|\| aUseSeedType == SeedType::SEED_GIANT_WALLNUT \|\|\s*aUseSeedType == SeedType::SEED_ELECTRIC_GATLING_PEA' 'SeedPacket::SetPacketType must exempt SEED_ELECTRIC_GATLING_PEA from the generic upgrade cooldown overrides.'

# --- bullets: 100% electric peas, Gatling Pea muzzle, 3% roll stays Gatling-only ---
$fireBody = [regex]::Match($plantCpp, 'void Plant::Fire\([\s\S]*?\n\}')
if (-not $fireBody.Success) { throw 'Could not find Plant::Fire body.' }
if ($fireBody.Value -notmatch 'case SeedType::SEED_ELECTRIC_GATLING_PEA:[\s\S]{0,400}PROJECTILE_FIREPEA_RED') { throw 'Plant::Fire must map SEED_ELECTRIC_GATLING_PEA to PROJECTILE_FIREPEA_RED (100% electric peas).' }
if ($fireBody.Value -notmatch 'mSeedType == SeedType::SEED_ELECTRIC_GATLING_PEA[\s\S]{0,300}GetPeaHeadOffset') { throw 'Plant::Fire must use the Gatling Pea muzzle offset for SEED_ELECTRIC_GATLING_PEA.' }
if ($fireBody.Value -notmatch 'mSeedType == SeedType::SEED_GATLINGPEA && Rand\(100\) < 3') { throw 'The 3% red-fire-pea roll must stay exclusive to SEED_GATLINGPEA.' }

# --- scatter ultimate: trigger, cadence and chance growth share the Gatling branches ---
Assert-Source $plantPath 'SEED_ELECTRIC_GATLING_PEA[\s\S]{0,800}mGatlingScatterCountdown = 300' 'The scatter ultimate trigger must cover SEED_ELECTRIC_GATLING_PEA.'
Assert-Source $plantPath 'SEED_ELECTRIC_GATLING_PEA[\s\S]{0,1000}mShootingCounter % 2 == 1' 'The scatter firing cadence must cover SEED_ELECTRIC_GATLING_PEA.'
Assert-Source $plantPath 'SEED_ELECTRIC_GATLING_PEA[\s\S]{0,800}mGatlingScatterCountdown > 0' 'Plant::Fire must use the scatter branch for SEED_ELECTRIC_GATLING_PEA.'
Assert-Source $plantPath 'SEED_ELECTRIC_GATLING_PEA[\s\S]{0,1000}mGatlingScatterCountdown == 0' 'The scatter-chance growth must cover SEED_ELECTRIC_GATLING_PEA.'
Assert-Source (P 'src/Lawn/Board.cpp') 'SEED_GATLINGPEA \|\| aPlant->mSeedType == SeedType::SEED_ELECTRIC_GATLING_PEA[\s\S]{0,260}mGatlingScatterCountdown' 'The HP tooltip must report scatter state for SEED_ELECTRIC_GATLING_PEA too.'

# --- tinting: head instance only, helmet exempt, lower leaves/stalk untouched ---
# Must use the extra OVERLAY draw, not the additive one. Overlay replaces the track
# image with its FILTER_EFFECT_WHITE version and blends it on, so the re-colour is
# exact; the additive pass adds proportionally to the source texel, so a green sprite
# can never be pushed to blue or white (that was the original bug).
Assert-Source (P 'src/GameConstants.h') 'ELECTRIC_BLUE_R\s*=' 'GameConstants.h must define the shared electric-blue colour.'
Assert-Source (P 'src/GameConstants.h') 'ELECTRIC_GATLING_TINT_A\s*=' 'GameConstants.h must define the electric-gatling tint alpha constant.'
Assert-Source $plantPath 'mExtraOverlayColor = Color\(ELECTRIC_BLUE_R, ELECTRIC_BLUE_G, ELECTRIC_BLUE_B, ELECTRIC_GATLING_TINT_A\)' 'The tint must be applied through the overlay channel (mExtraOverlayColor) using the shared electric blue.'
Assert-Source $plantPath 'mEnableExtraOverlayDraw = true' 'The overlay draw must be enabled for the tint.'
# Guard against regressing back to the additive channel, which cannot re-colour the sprite.
if ($plantLines | Where-Object { $_ -match 'mExtraAdditiveColor = Color\(ELECTRIC_BLUE' }) {
    throw 'The tint must NOT use mExtraAdditiveColor: additive blends proportionally to the source texel, so a green sprite can never be re-coloured.'
}
# The per-track overlay exemption flag is what keeps the helmet original.
Assert-Source (P 'src/Sexy.TodLib/Reanimator.h') 'mIgnoreExtraOverlayColor' 'ReanimatorTrackInstance must expose a per-track overlay exemption flag.'
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'mEnableExtraOverlayDraw && !aTrackInstance->mIgnoreExtraOverlayColor' 'Reanimation::DrawTrack must honour mIgnoreExtraOverlayColor.'
Assert-Source $plantPath 'mIgnoreExtraOverlayColor = true' 'The GatlingPea_helmet track must be exempted from the tint overlay.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_ELECTRIC_GATLING_PEA[\s\S]{0,900}mIgnoreExtraOverlayColor = true' 'Cached packet/almanac art must apply the same tint and helmet exemption.'

# The head reanim must be the tinted one, and the tint must be applied AFTER
# PropogateColorToAttachments() or the body colour propagation overwrites it.
$propagateLines = @(1..$plantLines.Count | Where-Object { $plantLines[$_ - 1] -match 'PropogateColorToAttachments\(\)' })
$headWhitenLines = @(1..$plantLines.Count | Where-Object { $plantLines[$_ - 1] -match 'aHeadReanim->mExtraOverlayColor\s*=' })
if ($propagateLines.Count -eq 0) { throw 'Plant::UpdateReanimColor must still propagate colour to attachments.' }
if ($headWhitenLines.Count -eq 0) { throw 'The tint must be applied to the head reanim instance (aHeadReanim->mExtraOverlayColor).' }
$firstPropagate = ($propagateLines | Measure-Object -Minimum).Minimum
$firstWhiten = ($headWhitenLines | Measure-Object -Minimum).Minimum
if ($firstWhiten -lt $firstPropagate) { throw 'The tint must be applied AFTER PropogateColorToAttachments(), otherwise the body colour propagation overwrites it.' }

# The body instance carries the lower leaves/stalk, so it must never receive the tint.
$bodyTinted = @(1..$plantLines.Count | Where-Object {
    $plantLines[$_ - 1] -match 'aBodyReanim' -and $plantLines[$_ - 1] -match 'ELECTRIC_BLUE'
})
if ($bodyTinted.Count -ne 0) { throw "The body reanim (leaves/stalk = the lower part) must NOT be tinted (line $($bodyTinted[0]))." }

# --- electric pea: blue silhouette, sharing the plant's exact electric blue ---
$projPath = P 'src/Lawn/Projectile.cpp'
Assert-Source $projPath 'FilterEffectGetImage\(aImage, FilterEffect::FILTER_EFFECT_WHITE\)' 'The electric pea must draw the white-filtered silhouette of the pea sprite.'
Assert-Source $projPath 'Color\(ELECTRIC_BLUE_R, ELECTRIC_BLUE_G, ELECTRIC_BLUE_B, 255\)' 'The electric pea must be painted with the shared ELECTRIC_BLUE_* colour so it matches the plant.'
Assert-Source $projPath '#include "../Sexy.TodLib/FilterEffect.h"' 'Projectile.cpp must include FilterEffect.h for FilterEffectGetImage.'
if ((Get-Content -Raw -LiteralPath $projPath) -match 'for \(int i = 0; i < 4; i\+\+\)[\s\S]{0,200}DRAWMODE_ADDITIVE') {
    throw 'The electric pea must not use the old 4x additive white stack: it cannot remove the sprite green (the rim stayed green).'
}
# The pea and the plant must not drift apart: neither may define its own blue.
Assert-Source (P 'src/GameConstants.h') 'ELECTRIC_BLUE_B\s*=' 'GameConstants.h must define the shared electric-blue colour.'
if (Select-String -Path (P 'src/GameConstants.h') -Pattern 'ELECTRIC_PEA_[RGB]' -Quiet) {
    throw 'The electric pea must reuse ELECTRIC_BLUE_*, not keep a separate colour (the two would drift apart again).'
}

# --- the Beghouled flash uses the overlay channel on the body and propagates to the
# --- head, so the tint must step aside there instead of fighting it.
Assert-Source $plantPath 'mBeghouledFlashCountdown <= 0[\s\S]{0,260}mExtraOverlayColor = Color\(ELECTRIC_BLUE_R' 'The tint must step aside while the Beghouled overlay flash is active.'

# --- player-facing text ---
Assert-Source (P 'src/Lawn/SeedPacket.cpp') '\[ADVICE_PLANT_NEEDS_GATLINGPEA\]' 'SeedPacket must show a "needs Gatling Pea" advice.'
Assert-Source (P 'src/Lawn/Board.cpp') '\[REQUIRES_GATLINGPEA\]' 'The seed packet tooltip must show a "requires Gatling Pea" warning.'

foreach ($file in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $file) '<String id="ELECTRIC_GATLING_PEA">' "$file must define the ELECTRIC_GATLING_PEA name string."
    Assert-Source (P $file) '<String id="ELECTRIC_GATLING_PEA_TOOLTIP">' "$file must define the ELECTRIC_GATLING_PEA_TOOLTIP string."
    Assert-Source (P $file) '<String id="REQUIRES_GATLINGPEA">' "$file must define the REQUIRES_GATLINGPEA string."
    Assert-Source (P $file) '<String id="ADVICE_PLANT_NEEDS_GATLINGPEA">' "$file must define the ADVICE_PLANT_NEEDS_GATLINGPEA string."
}

Write-Output 'Electric Gatling Pea (SEED_ELECTRIC_GATLING_PEA) source checks passed.'
