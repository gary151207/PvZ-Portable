$ErrorActionPreference = 'Stop'

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/LawnApp.cpp' 'mBoard->mLevel == 55' 'Lone Wolf must target adventure level 6-5.'
Assert-Source 'src/Lawn/Board.cpp' 'mLevel == 55[\s\S]{0,160}BACKGROUND_2_NIGHT' 'Level 6-5 must use the black-night map.'
Assert-Source 'src/LawnApp.cpp' 'IsLoneWolfLevel\(\)' 'Lone Wolf must disable seed selection.'
Assert-Source 'src/Lawn/Board.cpp' 'NewPlant\(4, 2, SeedType::SEED_GATLINGPEA' 'Lone Wolf must create a Gatling shooter.'
Assert-Source 'src/Lawn/Board.cpp' "KeyCode\('W'\)" 'W must move the Lone Wolf shooter.'
Assert-Source 'src/Lawn/Board.cpp' "KeyCode\('A'\)" 'A must move the Lone Wolf shooter.'
Assert-Source 'src/Lawn/Board.cpp' "KeyCode\('S'\)" 'S must move the Lone Wolf shooter.'
Assert-Source 'src/Lawn/Board.cpp' "KeyCode\('D'\)" 'D must move the Lone Wolf shooter.'
Assert-Source 'src/Lawn/Board.cpp' 'IsLoneWolfLevel\(\)[\s\S]{0,120}mNumWaves = 40' 'Lone Wolf must have forty waves.'
Assert-Source 'src/Lawn/Board.cpp' 'IsLoneWolfLevel\(\)[\s\S]{0,220}SEED_GATLINGPEA[\s\S]{0,120}mDead[\s\S]{0,120}ZombiesWon\(' 'Lone Wolf must lose when the Gatling shooter dies.'

Write-Output 'Lone Wolf source checks passed.'
