$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/Lawn/Plant.cpp' 'IsLoneWolfLevel\(\)[\s\S]{0,180}mDamageOverride = 200' 'Lone Wolf shots must deal 200 damage.'

Write-Output 'Lone Wolf attack check passed.'
