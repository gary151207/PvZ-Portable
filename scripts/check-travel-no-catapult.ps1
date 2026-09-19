$ErrorActionPreference = 'Stop'

# Travel modes must never spawn the Catapult zombie (ZOMBIE_CATAPULT).
# Three enforcement points share Board::IsTravelForbiddenZombie():
#   1. Challenge::InitZombieWavesTravelJourney() must not put it in the per-round roster
#   2. Board::PickZombieType() must skip it when rolling a wave
#   3. Board::SpawnZombieWave() must skip it when the wave list is played back
#      (covers .v4 saves whose mZombiesInWave was baked before this rule existed)
# Notes: this repo has no automated test suite (see AGENTS.md), so - like the other check-*.ps1
#        scripts - this is a source-level assertion pass; the actual wave content still needs a
#        manual play test (travel journey round 9 is the one that used to unlock it).
# Keep this file ASCII-only: Windows PowerShell 5.1 reads BOM-less .ps1 files as ANSI, and
# non-ASCII comment bytes can swallow the following line.

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-NotSource([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -match $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

$boardH = P 'src/Lawn/Board.h'
$boardCpp = P 'src/Lawn/Board.cpp'
$challengeCpp = P 'src/Lawn/Challenge.cpp'

# --- Shared predicate ---
Assert-Source $boardH 'bool\s+IsTravelForbiddenZombie\(ZombieType theZombieType\);' 'Board.h must declare Board::IsTravelForbiddenZombie().'
$predicate = [regex]::Match((Get-Content -Raw -LiteralPath $boardCpp), 'bool Board::IsTravelForbiddenZombie\(ZombieType theZombieType\)[\s\S]*?\r?\n\}')
if (-not $predicate.Success) { throw 'Could not find Board::IsTravelForbiddenZombie body.' }
if ($predicate.Value -notmatch 'theZombieType == ZombieType::ZOMBIE_CATAPULT') {
    throw 'IsTravelForbiddenZombie must name ZOMBIE_CATAPULT.'
}
if ($predicate.Value -notmatch 'IsTravelLevel\(mApp->mGameMode\)') {
    throw 'IsTravelForbiddenZombie must be scoped to IsTravelLevel() (all travel modes, not one level).'
}

# --- 1. Per-round roster: the catapult must not be unlocked ---
$rosterBody = [regex]::Match((Get-Content -Raw -LiteralPath $challengeCpp), 'void Challenge::InitZombieWavesTravelJourney\(\)[\s\S]*?\r?\n\}')
if (-not $rosterBody.Success) { throw 'Could not find Challenge::InitZombieWavesTravelJourney body.' }
if ($rosterBody.Value -match 'ZOMBIE_CATAPULT') {
    throw 'Challenge::InitZombieWavesTravelJourney must not reference ZOMBIE_CATAPULT (it used to unlock it at round 9+).'
}
if ($rosterBody.Value -notmatch 'aList\[ZOMBIE_BUNGEE\] = true') {
    throw 'Sanity: the travel journey roster must still exist (bungee at round 9+).'
}

# --- 2. Random roll: PickZombieType must call the predicate before weighting ---
$pickBody = [regex]::Match((Get-Content -Raw -LiteralPath $boardCpp), 'ZombieType Board::PickZombieType\([\s\S]*?\r?\n\}')
if (-not $pickBody.Success) { throw 'Could not find Board::PickZombieType body.' }
if ($pickBody.Value -notmatch 'if \(IsTravelForbiddenZombie\(\(ZombieType\)aZombieType\)\)\s*\r?\n\s*\{\s*\r?\n\s*continue;') {
    throw 'PickZombieType must skip travel-forbidden zombies (continue) before adding them to the weight array.'
}

# --- 3. Spawn playback: SpawnZombieWave must call the predicate ---
$spawnBody = [regex]::Match((Get-Content -Raw -LiteralPath $boardCpp), 'void Board::SpawnZombieWave\(\)[\s\S]*?\r?\n\}')
if (-not $spawnBody.Success) { throw 'Could not find Board::SpawnZombieWave body.' }
if ($spawnBody.Value -notmatch 'if \(IsTravelForbiddenZombie\(aZombieType\)\)\s*\r?\n\s*continue;') {
    throw 'SpawnZombieWave must skip travel-forbidden zombies so baked-in saves cannot spawn them either.'
}

# --- Travel experience levels keep their own whitelist (no catapult there either) ---
Assert-Source $boardCpp 'bool Board::CanZombieSpawnOnLevel[\s\S]{0,2400}IsTravelLevel\(gLawnApp->mGameMode\)[\s\S]{0,600}ZOMBIE_DOLPHIN_RIDER' 'CanZombieSpawnOnLevel must keep the travel experience whitelist (no catapult in it).'
$spawnOnLevel = [regex]::Match((Get-Content -Raw -LiteralPath $boardCpp), 'bool Board::CanZombieSpawnOnLevel\([\s\S]*?\r?\n\}')
if ($spawnOnLevel.Success) {
    $travelPart = [regex]::Match($spawnOnLevel.Value, 'if \(IsTravelLevel\(gLawnApp->mGameMode\)\)[\s\S]{0,700}')
    if ($travelPart.Success -and $travelPart.Value -match 'ZOMBIE_CATAPULT') {
        throw 'The travel experience whitelist in CanZombieSpawnOnLevel must not allow ZOMBIE_CATAPULT.'
    }
}

Write-Output 'Travel no-catapult source checks passed.'
