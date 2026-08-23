$ErrorActionPreference = 'Stop'

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
$constants = Get-Content -Raw -LiteralPath (Join-Path $root 'src/GameConstants.h')
$challenge = Get-Content -Raw -LiteralPath (Join-Path $root 'src/Lawn/Challenge.cpp')
$board = Get-Content -Raw -LiteralPath (Join-Path $root 'src/Lawn/Board.cpp')
$app = Get-Content -Raw -LiteralPath (Join-Path $root 'src/LawnApp.cpp')

if ($constants -notmatch 'ADVENTURE_AREAS = 6') { throw 'Adventure areas must be six.' }
if ($constants -notmatch 'NUM_LEVELS = ADVENTURE_AREAS \* LEVELS_PER_AREA') { throw 'NUM_LEVELS must derive from adventure areas.' }

$waveMatch = [regex]::Match($challenge, 'int gZombieWaves\[NUM_LEVELS\] = \{(?<values>.*?)\};', [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $waveMatch.Success) { throw 'Could not find gZombieWaves initializer.' }
$waveCount = ([regex]::Matches($waveMatch.Groups['values'].Value, '\b\d+\b')).Count
if ($waveCount -ne 60) { throw "Expected 60 wave entries, found $waveCount." }

Assert-Source 'src/Lawn/Board.cpp' 'mLevel < 5 \* LEVELS_PER_AREA' 'Chapter five must retain the ordinary roof boundary.'
Assert-Source 'src/Lawn/Board.cpp' 'mLevel <= FINAL_LEVEL[\s\S]{0,200}BACKGROUND_6_BOSS' 'Chapter six must route through the Boss map branch.'
Assert-Source 'src/LawnApp.cpp' 'mPlayerInfo->mLevel >= 51[\s\S]*mPlayerInfo->mLevel <= FINAL_LEVEL' 'Chapter six must be night.'
Assert-Source 'src/LawnApp.cpp' 'mBoard->mLevel == FINAL_LEVEL' 'Campaign final must use FINAL_LEVEL.'
Assert-Source 'src/Lawn/Board.cpp' 'mLevel == 50 \|\| mLevel == FINAL_LEVEL.*aIsFinalWave' 'Level 60 must spawn the final-wave Boss.'
Assert-Source 'src/Lawn/Coin.cpp' 'mBoard->mLevel == 50 \|\| mBoard->mLevel == FINAL_LEVEL' 'Level 60 must use final Boss coin rewards.'
Assert-Source 'src/Lawn/Zombie.cpp' 'mBoard->mLevel == 59' 'Level 59 must preserve the chapter award boundary.'

Write-Output 'Moonlit roof source checks passed.'
