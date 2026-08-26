$ErrorActionPreference = 'Stop'

function Assert-Source([string] $Path, [string] $Pattern, [string] $Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/Lawn/Widget/SeedChooserScreen.cpp' 'void SeedChooserScreen::KeyDown\(KeyCode theKey\)[\s\S]{0,500}IsLoneWolfLevel\(\)[\s\S]{0,300}MoveLoneWolf' 'Seed chooser must forward Lone Wolf movement keys.'

Write-Output 'Lone Wolf seed chooser input check passed.'
