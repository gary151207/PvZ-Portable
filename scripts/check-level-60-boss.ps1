$ErrorActionPreference = 'Stop'

# 调用点有的传相对路径、有的传 P 出来的绝对路径，这里统一处理。
# 注意：本函数在 $root 定义**之前**就被插入，所以根目录在这里按 $PSScriptRoot 现算，不依赖外部变量。
function Resolve-RepoFile([string]$theRelPath) {
    if ([System.IO.Path]::IsPathRooted($theRelPath)) { return $theRelPath }
    return (Join-Path (Split-Path -Parent $PSScriptRoot) $theRelPath)
}
function Assert-Source([string]$RelPath, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath (Resolve-RepoFile $RelPath)
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Source 'src/LawnApp.cpp' 'mBoard->mLevel == 50 \|\| mBoard->mLevel == FINAL_LEVEL' 'Levels 5-10 and 6-10 must use the final Boss flow.'
Assert-Source 'src/Lawn/Zombie.cpp' 'mLevel == FINAL_LEVEL[\s\S]{0,80}mBodyHealth = 1200000' 'Level 6-10 Boss health must be 1,200,000.'

Write-Output 'Level 6-10 Boss checks passed.'
