$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/Lawn/Board.cpp' 'if \(mApp->IsLoneWolfLevel\(\)\)\s*return false;' 'Level 6-5 must disable grave stones.'

Write-Output 'Level 6-5 no-graves check passed.'
