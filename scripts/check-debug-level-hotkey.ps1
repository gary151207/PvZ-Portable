$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/Lawn/Board.cpp' 'void Board::KeyDown\(KeyCode theKey\)[\s\S]{0,700}mDebugKeysEnabled[\s\S]{0,180}KeyCode\(''L''\)[\s\S]{0,180}DoCheatDialog' 'Debug level hotkey must be handled from physical key input.'

Assert-Source 'src/Lawn/Board.cpp' 'else if \(theChar == ''l'' \|\| theChar == ''L''\)[\s\S]{0,100}DoCheatDialog' 'Debug level hotkey must accept both lowercase and uppercase character input.'

Write-Output 'Debug level hotkey check passed.'
