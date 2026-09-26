$ErrorActionPreference = 'Stop'

# 孢子菇（SEED_SPORESHROOM）源级检查：
#   国服 4.2.4 一级数值 / 仅斗蛐蛐 2 第二选卡页 / 单体抛射 / 直接击杀原格繁殖。
# 本仓库没有自动化测试框架，因此用静态契约覆盖容易回归的接线；动画观感和实战行为仍需进游戏验收。

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

# 枚举只能在现有自定义值之后追加，保证旧 .v4 存档中的整数值稳定。
Assert-Source (P 'src/ConstEnums.h') 'PROJECTILE_PURPLE_FIRE_PEA = 16,[\s\S]{0,300}PROJECTILE_LASER_PEA = 17,[\s\S]{0,300}PROJECTILE_SPORESHROOM = 18,[\s\S]{0,200}NUM_PROJECTILES = 19' 'Spore projectile must be appended as value 18 (after the laser pea beam added earlier).'
Assert-Source (P 'src/ConstEnums.h') 'REANIM_THREE_GATLINGPEA,[\s\S]{0,900}REANIM_SPORESHROOM,[\s\S]{0,200}REANIM_SPORESHROOM_PROJECTILE,[\s\S]{0,600}NUM_REANIMS' 'Spore reanimations must be appended before NUM_REANIMS (later mod reanimations appended after them are fine).'
Assert-Source (P 'src/ConstEnums.h') 'SEED_THREE_GATLING_PEA,[\s\S]{0,300}SEED_SPORESHROOM,[\s\S]{0,200}NUM_SEED_TYPES' 'Spore-shroom must be appended before NUM_SEED_TYPES.'
Assert-Source (P 'src/Lawn/Plant.h') 'STATE_UMBRELLA_KNOCKING,[\s\S]{0,200}STATE_SPORESHROOM_GROWING' 'The growing state must be appended to PlantState.'

# 动画及资源。
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_SPORESHROOM,\s*"reanim/SporeShroom\.reanim",\s*1 << ReanimFlags::REANIM_NO_ATLAS' 'Spore-shroom reanim must be registered without an atlas.'
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_SPORESHROOM_PROJECTILE,\s*"reanim/SporeShroomProjectile\.reanim",\s*1 << ReanimFlags::REANIM_NO_ATLAS' 'Spore projectile reanim must be registered without an atlas.'
foreach ($path in @('res/main/reanim/SporeShroom.reanim', 'res/main/reanim/SporeShroomProjectile.reanim')) {
    if (-not (Test-Path -LiteralPath (P $path))) { throw "Missing generated animation: $path" }
}
Assert-Source (P 'res/main/reanim/SporeShroom.reanim') '<name>anim_idle</name>' 'Plant reanim must expose anim_idle.'
Assert-Source (P 'res/main/reanim/SporeShroom.reanim') '<name>anim_shooting</name>' 'Plant reanim must expose anim_shooting.'
Assert-Source (P 'res/main/reanim/SporeShroom.reanim') '<name>anim_grow</name>' 'Plant reanim must expose anim_grow.'
$plantReanim = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'res/main/reanim/SporeShroom.reanim')
if ($plantReanim -match 'IMAGE_REANIM_SPORESHROOM_00[67]') { throw 'Base Spore-shroom animation must not reference the paper-bag costume.' }
Assert-Source (P 'res/main/reanim/SporeShroomProjectile.reanim') '<name>anim_fly</name>' 'Projectile reanim must expose anim_fly.'
Assert-Source (P 'res/main/reanim/SporeShroomProjectile.reanim') '<name>anim_hit</name>' 'Projectile reanim must expose anim_hit.'
Assert-Source (P 'res/main/reanim/SporeShroomProjectile.reanim') '<name>anim_hit2</name>' 'Projectile reanim must expose anim_hit2.'

# 基础数值和植物分类。
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_SPORESHROOM,\s*nullptr,\s*ReanimationType::REANIM_SPORESHROOM,\s*[^,]+,\s*150,\s*500,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*290,\s*"SPORE_SHROOM"' 'Plant definition must use 150 sun, 5 second recharge and 2.9 second launch rate.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'bool Plant::IsFungus[\s\S]{0,900}SEED_SPORESHROOM' 'Spore-shroom must be classified as a fungus.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'mPlantHealth\s*=\s*300;' 'New plants must retain the 300 hit point default.'
$plantCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Plant.cpp')
$nocturnal = [regex]::Match($plantCpp, 'bool Plant::IsNocturnal[\s\S]*?\n\}')
if (-not $nocturnal.Success) { throw 'Could not find Plant::IsNocturnal.' }
if ($nocturnal.Value -match 'SEED_SPORESHROOM') { throw 'Spore-shroom must not sleep during the day.' }
Assert-Source (P 'src/Lawn/Projectile.cpp') 'PROJECTILE_SPORESHROOM,\s*0,\s*40' 'Spore projectile must deal 40 damage.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_SPORESHROOM:[\s\S]{0,400}PROJECTILE_SPORESHROOM' 'Plant::Fire must map Spore-shroom to its dedicated projectile.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'IsPultPlant\(mSeedType\)\s*\|\|\s*mSeedType\s*==\s*SeedType::SEED_SPORESHROOM[\s\S]{0,1800}MOTION_LOBBED' 'Spore-shroom must use a lobbed single-target projectile.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_SPORESHROOM[\s\S]{0,500}anim_shooting[\s\S]{0,300}mShootingCounter\s*=\s*97' 'Spore-shroom must fire on the source animation action point.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'STATE_SPORESHROOM_GROWING[\s\S]{0,500}PlayIdleAnim[\s\S]{0,100}\}' 'Growth must block attacks until the animation finishes, then return to idle.'

# 直接击杀才繁殖；Board 负责原格合法性与成长状态初始化。
Assert-Source (P 'src/Lawn/Projectile.cpp') 'PROJECTILE_SPORESHROOM[\s\S]{0,500}IsDeadOrDying\(\)[\s\S]{0,500}TrySpawnSporeShroom' 'The projectile must detect its own killing blow and request a clone.'
Assert-Source (P 'src/Lawn/Board.cpp') 'bool Board::TrySpawnSporeShroom[\s\S]{0,800}PixelToGridX[\s\S]{0,500}CanPlantAt\([\s\S]{0,200}SEED_SPORESHROOM[\s\S]{0,400}NewPlant[\s\S]{0,300}StartSporeGrowth' 'Board must validate the exact death tile and initialize clone growth.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'void Plant::StartSporeGrowth\(\)[\s\S]{0,500}STATE_SPORESHROOM_GROWING[\s\S]{0,300}anim_grow' 'Clones must begin with the source grow animation.'

# 模式隔离：只在斗蛐蛐 2 开放，不进入 Travel 或冰面沙盒表。
Assert-Source (P 'src/LawnApp.cpp') 'IsCricketFight2Level\(\)[\s\S]{0,500}SEED_SPORESHROOM' 'LawnApp::HasSeedType must expose Spore-shroom in Cricket Fight 2.'
Assert-Source (P 'src/Lawn/Widget/SeedChooserScreen.cpp') 'IsCricketFight2Level\(\)[\s\S]{0,300}SEED_SPORESHROOM' 'Chooser page 2 must include Spore-shroom only for Cricket Fight 2.'
Assert-Source (P 'src/Lawn/SeedPacket.cpp') 'case SeedType::SEED_SPORESHROOM:[\s\S]{0,250}aScale\s*=\s*0\.5f[\s\S]{0,200}aOffset' 'Spore-shroom must have dedicated seed packet scaling and placement.'
$travelCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Travel.cpp')
if ($travelCpp -match 'SEED_SPORESHROOM') { throw 'Spore-shroom must not be registered as a travel plant.' }
$boardCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Board.cpp')
$iceSeeds = [regex]::Match($boardCpp, 'gIceSandboxTravelSeeds\[\][\s\S]*?\};')
if ($iceSeeds.Success -and $iceSeeds.Value -match 'SEED_SPORESHROOM') { throw 'Spore-shroom must not be available in the ice sandbox.' }

# 图鉴与文案。
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.cpp') 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]{0,1800}SEED_SPORESHROOM' 'Spore-shroom must appear on almanac page 2.'
foreach ($path in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $path) '<String id="SPORE_SHROOM">孢子菇</String>' 'Missing Spore-shroom name.'
    Assert-Source (P $path) '<String id="SPORE_SHROOM_TOOLTIP">' 'Missing Spore-shroom tooltip.'
    Assert-Source (P $path) '<String id="SPORE_SHROOM_DESCRIPTION">' 'Missing Spore-shroom almanac description.'
}

Write-Output 'Spore-shroom (SEED_SPORESHROOM) source checks passed.'
