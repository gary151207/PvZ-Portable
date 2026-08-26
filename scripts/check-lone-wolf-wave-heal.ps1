$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/Lawn/Board.cpp' 'void Board::NextWaveComing\(\)[\s\S]{0,900}IsLoneWolfLevel\(\)[\s\S]{0,500}mPlantHealth = aPlant->mPlantMaxHealth' 'Lone Wolf must heal at the start of each wave.'

Write-Output 'Lone Wolf wave-heal check passed.'
