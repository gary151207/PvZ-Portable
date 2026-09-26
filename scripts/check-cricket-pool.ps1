$ErrorActionPreference = 'Stop'

# 斗蛐蛐 1（GAMEMODE_CHALLENGE_CRICKET）源级检查：
#   植物池必须**全量自动收集**（模组新增植物零登记自动参赛）、僵尸池是写死名单、
#   关卡是夜间草坪（蘑菇类才不用睡觉）。
# 起因：新增植物后常被问"新植物怎么没加进斗蛐蛐 1"——池子从 2026-09-21（火豌豆射手）起
# 就是遍历 SeedType 自动收集的，所以这里把"别再改回逐个登记"钉住。

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

$boardCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Board.cpp')
$enumsH = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/ConstEnums.h')

# --- 植物池：遍历整个 SeedType 区间，而不是逐个登记 ---
Assert-Source (P 'src/Lawn/Board.cpp') 'void Board::SetupCricketFight\(\)[\s\S]{0,2500}for \(int aSeed = SeedType::SEED_PEASHOOTER; aSeed < SeedType::NUM_SEED_TYPES; aSeed\+\+\)' 'The cricket plant pool must auto-collect every SeedType up to NUM_SEED_TYPES (new plants join for free).'

# 排除名单必须恰好是这 7 种：多一种会误伤可用植物，少一种会把非战斗种子混进池子。
$bannedBlock = [regex]::Match($boardCpp, 'gCricketBannedSeeds\[\] = \{([\s\S]*?)\};')
if (-not $bannedBlock.Success) { throw 'Could not find gCricketBannedSeeds.' }
$banned = @([regex]::Matches($bannedBlock.Groups[1].Value, 'SeedType::(\w+)') | ForEach-Object { $_.Groups[1].Value })
$expectedBanned = @('SEED_PUMPKINSHELL', 'SEED_FLOWERPOT', 'SEED_COBCANNON', 'SEED_IMITATER', 'SEED_EXPLODE_O_NUT', 'SEED_LEFTPEATER', 'SEED_SPROUT')
$bannedKey = (($banned | Sort-Object) -join ',')
$expectedKey = (($expectedBanned | Sort-Object) -join ',')
if ($bannedKey -ne $expectedKey) {
    throw "gCricketBannedSeeds must be exactly: $($expectedBanned -join ', ') -- got: $($banned -join ', ')"
}

# --- 所有模组新植物都必须落在池子区间里（枚举值在 SEED_PEASHOOTER..NUM_SEED_TYPES 之内） ---
$modPlants = @(
    'SEED_GIANT_WALLNUT', 'SEED_FUMESHROOM_GROUP', 'SEED_PEATER_1_5',
    'SEED_ELECTRIC_GATLING_PEA', 'SEED_ELECTRIC_STARFRUIT', 'SEED_SNOW_GATLING_PEA',
    'SEED_FIRE_PEASHOOTER', 'SEED_FIRE_GATLING_PEA', 'SEED_THREE_GATLING_PEA',
    'SEED_LASER_PEA', 'SEED_SPORESHROOM', 'SEED_HYPNOSHROOM_FUME'
)
foreach ($plant in $modPlants) {
    Assert-Source (P 'src/ConstEnums.h') "$plant," "$plant must exist in SeedType."
    if ($banned -contains $plant) { throw "$plant must stay in the cricket plant pool (it is a real combat plant)." }
}

# 新植物必须排在 SEED_PEASHOOTER 与 NUM_SEED_TYPES 之间，否则自动收集会漏掉它
$poolStart = $enumsH.IndexOf('SEED_PEASHOOTER')
$poolEnd = $enumsH.IndexOf('NUM_SEED_TYPES')
foreach ($plant in $modPlants) {
    $at = $enumsH.IndexOf("$plant,")
    if ($at -lt $poolStart -or $at -gt $poolEnd) { throw "$plant must sit between SEED_PEASHOOTER and NUM_SEED_TYPES so the cricket pool picks it up." }
}

# --- 夜间草坪：蘑菇类（大喷菇群 / 魅惑大喷菇 / 孢子菇）在斗蛐蛐里不能睡着 ---
Assert-Source (P 'src/Lawn/Board.cpp') 'case GameMode::GAMEMODE_CHALLENGE_CRICKET:[\s\S]{0,200}BACKGROUND_2_NIGHT' 'Cricket Fight must stay a night lawn so every mushroom plant can fight.'

# --- 僵尸池：写死名单 + 旅行专属僵尸（每场至多 1 只） ---
Assert-Source (P 'src/Lawn/Board.cpp') 'static const ZombieType gCricketZombiePool\[\] = \{[\s\S]{0,900}ZOMBIE_REDEYE_GARGANTUAR' 'The cricket zombie pool must stay a curated list (currently through the Red-eye Gargantuar).'
Assert-Source (P 'src/Lawn/Board.cpp') 'gTravelZombieDefs\[aPick - aBasePoolSize\][\s\S]{0,900}aTravelZombieTaken' 'Travel-only zombies join the cricket pool at most once per match.'

Write-Output 'Cricket Fight 1 (GAMEMODE_CHALLENGE_CRICKET) source checks passed.'
