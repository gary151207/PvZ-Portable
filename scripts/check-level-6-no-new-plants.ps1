$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/LawnApp.cpp' 'if \(HasFinishedAdventure\(\)\)\s*\{[\s\S]{0,80}return 49;' 'Only a finished adventure should expose all plants.'
Assert-Source 'src/Lawn/Zombie.cpp' 'mBoard->mLevel >= 51 && mBoard->mLevel < FINAL_LEVEL[\s\S]{0,180}COIN_AWARD_MONEY_BAG' 'Levels 6-1 through 6-9 must not drop new plant packets.'

Write-Output 'Area 6 no-new-plants checks passed.'
