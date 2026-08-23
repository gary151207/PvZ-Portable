$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/LawnApp.cpp' 'IsFirstTimeAdventureMode\(\) && aLevel < 51[\s\S]{0,100}ShowAwardScreen\(AwardType::AWARD_FORLEVEL' 'Area 6 must not open the plant award screen.'
Assert-Source 'src/LawnApp.cpp' 'aLevel == 9 \|\| aLevel == 19 \|\| aLevel == 29 \|\| aLevel == 39 \|\| aLevel == 49\)' 'Area 6 must not use the chapter plant award screen.'

Write-Output 'Area 6 no-plant award screen checks passed.'
