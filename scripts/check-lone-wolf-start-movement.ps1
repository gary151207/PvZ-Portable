$ErrorActionPreference = 'Stop'

$content = Get-Content -Raw -LiteralPath 'src/Lawn/Board.cpp'
if ($content -notmatch 'if \(mApp->IsLoneWolfLevel\(\) &&\s*\(') {
    throw 'Lone Wolf movement must be available before SCENE_PLAYING.'
}

Write-Output 'Lone Wolf start-movement check passed.'
