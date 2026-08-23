$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/LawnApp.cpp' 'mBoard->mLevel == 50[\s\S]{0,500}mHasUnlockedMinigames = 1[\s\S]{0,160}mHasUnlockedSurvivalMode = 1' '5-10 must unlock minigames and survival mode.'
Assert-Source 'src/Lawn/Zombie.cpp' 'mBoard->mLevel == 50 \|\| mBoard->mLevel == FINAL_LEVEL[\s\S]{0,180}COIN_AWARD_SILVER_SUNFLOWER' '5-10 must award the silver sunflower trophy.'

Write-Output '5-10 unlock and silver sunflower trophy checks passed.'
