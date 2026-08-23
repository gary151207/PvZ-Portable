$ErrorActionPreference = 'Stop'

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/LawnApp.cpp' 'mBoard->mLevel == 50 \|\| mBoard->mLevel == FINAL_LEVEL' 'Levels 5-10 and 6-10 must use the final Boss flow.'
Assert-Source 'src/Lawn/Zombie.cpp' 'mLevel == FINAL_LEVEL[\s\S]{0,80}mBodyHealth = 1200000' 'Level 6-10 Boss health must be 1,200,000.'

Write-Output 'Level 6-10 Boss checks passed.'
