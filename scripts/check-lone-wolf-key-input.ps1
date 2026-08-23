$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/Lawn/Board.cpp' 'KeyCode\(''W''\).*KeyCode\(''w''\)|KeyCode\(''w''\).*KeyCode\(''W''\)' 'Lone Wolf must accept both W key cases.'
Assert-Source 'src/Lawn/Board.cpp' 'KeyCode\(''A''\).*KeyCode\(''a''\)|KeyCode\(''a''\).*KeyCode\(''A''\)' 'Lone Wolf must accept both A key cases.'
Assert-Source 'src/Lawn/Board.cpp' 'KeyCode\(''S''\).*KeyCode\(''s''\)|KeyCode\(''s''\).*KeyCode\(''S''\)' 'Lone Wolf must accept both S key cases.'
Assert-Source 'src/Lawn/Board.cpp' 'KeyCode\(''D''\).*KeyCode\(''d''\)|KeyCode\(''d''\).*KeyCode\(''D''\)' 'Lone Wolf must accept both D key cases.'

Write-Output 'Lone Wolf key input checks passed.'
