$ErrorActionPreference = 'Stop'

# 三线射手跨行弹丸检查（"子弹莫名其妙消失"回归防护）。
#
# 背景：Plant::Fire 里三线射手会对全场每一行开火（行差可达 ±5）。弹丸的竖直位移和地面影子
# 必须按棋盘的实际行高算（草坪 100、泳池 / 雾夜 / 屋顶 85，屋顶还带斜坡），否则
# Projectile::CheckForCollision 开头的 `mShadowY - mPosY > 90` 会整帧跳过碰撞（弹丸穿过僵尸
# 飞到屏幕外），而 Projectile::CheckForHighGround 的 `< 28` 会让弹丸当场消散。原版只处理
# ±1 行的写死常量（mVelY = 3.0 / mShadowY -= 80.0）在行差 ≥ 2 时会让影子间距掉出 (28, 90)：
# 草坪上 57 + 20 * 行差（行差 +2 → 97 > 90，行差 -2 → 17 < 28），子弹于是"莫名其妙消失"。
#
# 说明：本仓库无自动化测试框架（见 AGENTS.md），与其它 check-*.ps1 一样做源级断言；
#      数值自检只覆盖"运动学公式"，实际手感仍需手动进游戏验证。

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

$root = Split-Path -Parent $PSScriptRoot
$plantCpp = Join-Path $root 'src/Lawn/Plant.cpp'
$projectileCpp = Join-Path $root 'src/Lawn/Projectile.cpp'

# --- 1. Plant::Fire 的三线射手分支：位移与影子都按实际车道高度算 ---
$plant = Get-Content -Raw -LiteralPath $plantCpp
$fire = [regex]::Match($plant, 'void Plant::Fire\([\s\S]*?\n\}')
if (-not $fire.Success) {
    throw 'Could not find the Plant::Fire body.'
}
# Fire 里有两处 SEED_THREEPEATER 分支（先算出膛位置、后设运动方式），这里只取设置运动方式的那一处。
# 两个分支现在都同时服务 SEED_THREE_GATLING_PEA（三线机枪射手的跨行弹道照抄三线射手），
# 所以条件里允许出现 `|| mSeedType == SeedType::SEED_THREE_GATLING_PEA`。
$branch = $null
foreach ($match in [regex]::Matches($fire.Value, 'else if \(mSeedType == SeedType::SEED_THREEPEATER(?: \|\| mSeedType == SeedType::SEED_THREE_GATLING_PEA)?\)\s*\{[\s\S]*?\n    \}')) {
    if ($match.Value -match 'MOTION_THREEPEATER') {
        $branch = $match
        break
    }
}
if ($null -eq $branch) {
    throw 'Could not find the Threepeater motion branch of Plant::Fire.'
}
$body = $branch.Value

if ($body -notmatch 'GetPosYBasedOnRow\(aOriginX, theRow\) - mBoard->GetPosYBasedOnRow\(aOriginX, mRow\)') {
    throw 'The Threepeater branch must derive its vertical travel from the real lane height (GetPosYBasedOnRow for both rows).'
}
if ($body -notmatch 'mVelY = aLaneDeltaY \* 0\.03f') {
    throw 'The Threepeater launch velocity must be the lane delta scaled by the 0.97 decay factor (0.03), so the pea lands exactly on the target lane.'
}
if ($body -notmatch 'mShadowY -= aLaneDeltaY') {
    throw 'The Threepeater projectile shadow must be re-based onto the firing row, otherwise the shadow spacing leaves the (28, 90) window and the pea vanishes / stops colliding.'
}
if ($body -match '3\.0f \* aRowDiff|80\.0f \* aRowDiff') {
    throw 'The hardcoded single-row constants (3.0f / 80.0f times the row difference) must not come back: they only hold for one row on a 100 px board.'
}

# --- 2. 运动学另一半在 Projectile.cpp：衰减系数与影子同步 ---
Assert-Source $projectileCpp 'MOTION_THREEPEATER[\s\S]{0,200}mVelY \*= 0\.97f[\s\S]{0,140}mShadowY \+= mVelY' 'MOTION_THREEPEATER must decay mVelY by 0.97 and carry mShadowY along with the pea.'

# 两个阈值的归属（改任何一个都要重新核对影子间距）
Assert-Source $projectileCpp 'mShadowY - mPosY > 90\.0f' 'CheckForCollision must keep the > 90 shadow-spacing gate (skips collision while the pea is still transiting).'
Assert-Source $projectileCpp 'aShadowDelta < 28\.0f' 'CheckForHighGround must keep the < 28 shadow-spacing gate (pea lands).'

# --- 3. 数值自检：按新公式模拟，确认弹丸正好落在目标车道 ---
# 每帧 mPosY += mVelY; mVelY *= 0.97 → 总位移 = v0 / (1 - 0.97)。影子与弹丸同速位移，
# 所以影子间距全程等于"本行豌豆"的值（草坪 57、屋顶 45），始终落在 (28, 90) 内。
function Assert-LaneLanding([int]$rowSpacing, [int]$rowDiff) {
    $delta = $rowSpacing * $rowDiff
    $velY = $delta * 0.03
    $posY = 0.0
    for ($i = 0; $i -lt 400; $i++) {
        $posY += $velY
        $velY *= 0.97
    }
    if ([Math]::Abs($posY - $delta) -gt 0.01) {
        throw "Lane landing mismatch (row spacing $rowSpacing, row difference $rowDiff): travelled $posY px, expected $delta px."
    }
}

# 旧公式的对照：影子间距 = (行距 - 80) * 行差 + 本行间距（草坪 57 / 屋顶 45）。
function Get-OldShadowDelta([int]$rowSpacing, [int]$rowBase, [int]$rowDiff) {
    return ($rowSpacing - 80) * $rowDiff + $rowBase
}
$oldBreaks = @()
foreach ($diff in -5..5) {
    if ($diff -eq 0) { continue }
    foreach ($probe in @(@(100, 57), @(85, 45))) {
        $spacing = $probe[0]
        $delta = Get-OldShadowDelta $spacing $probe[1] $diff
        if ($delta -lt 28.0 -or $delta -gt 90.0) {
            $oldBreaks += "spacing=$spacing diff=$diff delta=$delta"
        }
    }
}
if ($oldBreaks.Count -eq 0) {
    throw 'The old single-row formula no longer reproduces the bug - re-check whether this fix is still needed.'
}

foreach ($spacing in @(100, 85)) {
    foreach ($diff in -5..5) {
        if ($diff -eq 0) { continue }
        Assert-LaneLanding $spacing $diff
    }
}

Write-Output 'Threepeater lane checks passed.'
