$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/Lawn/Board.cpp' 'theLevel == 58 \|\| theLevel == 59[\s\S]{0,120}ZOMBIE_LADDER' 'Levels 6-8 and 6-9 must allow ladder zombies.'

Write-Output 'Levels 6-8 and 6-9 ladder zombie check passed.'
