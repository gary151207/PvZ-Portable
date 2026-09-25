$ErrorActionPreference = 'Stop'

# 火焰机枪射手（SEED_FIRE_GATLING_PEA）源级检查：
#   合成态（火豌豆射手卡 × 已种的机枪射手）/ 普攻 4 发紫火豌豆 / 大招散射 50% 紫火 + 50% 火豌豆
#   / 贴图 = 火焰版机枪射手（head/mouth/mouth_overlay/blink1/blink2/helmet，**枪管沿用原版**）。
# 说明：本仓库无自动化测试框架（见 AGENTS.md），与其它 check-*.ps1 一样做源级断言；
#      真正的手感（贴图是否对得上、散射里紫/橙是否大致各半）仍需手动进游戏验证，
#      验收步骤见 docs/superpowers/specs/2026-09-22-fire-gatling-pea-design.md。

# 调用点有的传相对路径、有的传 P 出来的绝对路径，这里统一处理。
# 注意：本函数在 $root 定义**之前**就被插入，所以根目录在这里按 $PSScriptRoot 现算，不依赖外部变量。
function Resolve-RepoFile([string]$theRelPath) {
    if ([System.IO.Path]::IsPathRooted($theRelPath)) { return $theRelPath }
    return (Join-Path (Split-Path -Parent $PSScriptRoot) $theRelPath)
}
function Assert-Source([string]$RelPath, [string]$Pattern, [string]$Message) {
    # 必须显式按 UTF-8 读：这些文件里有大量中文注释，Windows PowerShell 的默认编码
    # （ANSI）会把中文拆成多个字节字符，把"两处代码之间最多 N 个字符"的间隔断言撑爆。
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath (Resolve-RepoFile $RelPath)
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

# --- 枚举：新值前插在 NUM_* 之前，既有存档枚举值不变 ---
# 窗口放宽到 2000：之后追加的模组种子/槽位会夹在这一条与 NUM_* 之间（只追加、不重排）。
Assert-Source (P 'src/ConstEnums.h') 'SEED_FIRE_PEASHOOTER,[\s\S]{0,300}SEED_FIRE_GATLING_PEA,[\s\S]{0,2000}NUM_SEED_TYPES' 'SEED_FIRE_GATLING_PEA must be declared after SEED_FIRE_PEASHOOTER and before NUM_SEED_TYPES (later mod seeds appended after it are fine).'
Assert-Source (P 'src/ConstEnums.h') 'REANIM_FIRE_PEASHOOTER,[\s\S]{0,900}REANIM_FIRE_GATLINGPEA,[\s\S]{0,4000}NUM_REANIMS' 'REANIM_FIRE_GATLINGPEA must be declared after REANIM_FIRE_PEASHOOTER and before NUM_REANIMS (later mod reanims appended after it are fine).'
# gLawnReanimationArray 是**按 ReanimationType 下标**取用的（ReanimatorEnsureDefinitionLoaded 里
# 用的是 gReanimationParamArray[theReanimType]），所以条目顺序必须与枚举顺序逐条对齐 ——
# 顺序错了不会报错，只会让某个类型悄悄加载别人的 reanim 文件。这里锁住模组追加的那 5 条。
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_ELECTRIC_GATLINGPEA,[\s\S]{0,4000}REANIM_ELECTRIC_STARFRUIT,[\s\S]{0,4000}REANIM_SNOW_GATLINGPEA,[\s\S]{0,4000}REANIM_FIRE_PEASHOOTER,[\s\S]{0,4000}REANIM_FIRE_GATLINGPEA,\s*"reanim/GatlingPea\.reanim"' 'gLawnReanimationArray is indexed by ReanimationType: the mod reanim slots must stay in the same order as in ConstEnums.h.'

# --- 常量：大招里紫火豌豆的占比 ---
Assert-Source (P 'src/GameConstants.h') 'FIRE_GATLING_SCATTER_PURPLE_PERCENT\s*=\s*50' 'FIRE_GATLING_SCATTER_PURPLE_PERCENT must be 50 (50% purple fire pea in the scatter ultimate).'

# --- reanim 槽位：与机枪射手同一个文件、独立槽位、NO_ATLAS ---Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_FIRE_GATLINGPEA,\s*"reanim/GatlingPea\.reanim",\s*1 << ReanimFlags::REANIM_NO_ATLAS' 'REANIM_FIRE_GATLINGPEA must reuse reanim/GatlingPea.reanim with REANIM_NO_ATLAS.'

# --- 植物定义：属性与机枪射手一致（175 阳光 / 普通冷却 750 / 射手子类 / 100 发射节奏） ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'SeedType::SEED_FIRE_GATLING_PEA,\s*nullptr,\s*ReanimationType::REANIM_GATLINGPEA,\s*5,\s*175,\s*750,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*100,\s*"FIRE_GATLING_PEA"' 'gPlantDefs row must default to REANIM_GATLINGPEA / 175 sun / 750 refresh / SHOOTER / 100 launch rate / "FIRE_GATLING_PEA".'

$plantCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Plant.cpp')

# --- 合成态：不是升级卡（没有自己的种子卡），但能被火豌豆射手升级出来 ---
$upgradeBody = [regex]::Match($plantCpp, 'bool Plant::IsUpgrade[\s\S]*?\n\}')
if (-not $upgradeBody.Success) { throw 'Could not find Plant::IsUpgrade body.' }
if ($upgradeBody.Value -match 'SEED_FIRE_GATLING_PEA') { throw 'SEED_FIRE_GATLING_PEA must not be an upgrade card - it is a hidden synthesis state with no seed packet of its own.' }
$upgradableBody = [regex]::Match($plantCpp, 'bool Plant::IsUpgradableTo[\s\S]*?\n\}')
if (-not $upgradableBody.Success) { throw 'Could not find Plant::IsUpgradableTo body.' }
if ($upgradableBody.Value -notmatch 'theUpgradedType == SeedType::SEED_FIRE_PEASHOOTER && mSeedType == SeedType::SEED_GATLINGPEA') { throw 'Plant::IsUpgradableTo must allow a Fire Pea Shooter card to convert a Gatling Pea.' }

# --- 合成改写：消耗火豌豆射手卡，实际种下火焰机枪射手 ---
Assert-Source (P 'src/Lawn/Board.cpp') 'aPlantingSeedType == SeedType::SEED_FIRE_PEASHOOTER && aNormalPlant->mSeedType == SeedType::SEED_GATLINGPEA\)[\s\S]{0,400}aPlantSeedType = SeedType::SEED_FIRE_GATLING_PEA' 'Board::MouseDownWithPlant must rewrite the planted seed to SEED_FIRE_GATLING_PEA when a Fire Pea Shooter card lands on a Gatling Pea.'

# --- 普攻：与火豌豆射手共用同一个弹种 case（紫火豌豆） ---
$fireBody = [regex]::Match($plantCpp, 'void Plant::Fire\([\s\S]*?\n\}')
if (-not $fireBody.Success) { throw 'Could not find Plant::Fire body.' }
if ($fireBody.Value -notmatch 'case SeedType::SEED_FIRE_PEASHOOTER:\s*\r?\n\s*case SeedType::SEED_FIRE_GATLING_PEA:[\s\S]{0,600}PROJECTILE_PURPLE_FIRE_PEA') { throw 'Plant::Fire switch must map SEED_FIRE_GATLING_PEA to PROJECTILE_PURPLE_FIRE_PEA (same case as SEED_FIRE_PEASHOOTER).' }

# --- 大招：散射分支收本类型 + 每颗独立掷骰 50/50 + 其余机枪口径不变 ---
if ($fireBody.Value -notmatch 'SEED_SNOW_GATLING_PEA \|\| mSeedType == SeedType::SEED_FIRE_GATLING_PEA\)\s*&&\s*\r?\n?\s*mGatlingScatterCountdown > 0') { throw 'Plant::Fire must route SEED_FIRE_GATLING_PEA into the scatter (ultimate) branch.' }
if ($fireBody.Value -notmatch 'auto RollGatlingScatterType = \[this, &RollGatlingBulletType\]') { throw 'The scatter bullet roll must be a named lambda (RollGatlingScatterType) so the fire gatling can branch on its own seed type.' }
if ($fireBody.Value -notmatch 'SEED_FIRE_GATLING_PEA\)\s*\r?\n\s*\{[\s\S]{0,300}Rand\(100\) < FIRE_GATLING_SCATTER_PURPLE_PERCENT[\s\S]{0,200}PROJECTILE_PURPLE_FIRE_PEA[\s\S]{0,200}PROJECTILE_FIREBALL') { throw 'The scatter roll must give the fire gatling 50% purple fire pea / 50% plain fire pea (PROJECTILE_FIREBALL).' }
if ($fireBody.Value -notmatch 'ProjectileType aScatterType = RollGatlingScatterType\(aProjectileType\);') { throw 'The scatter loop must take its bullet type from RollGatlingScatterType.' }
if ($fireBody.Value -notmatch 'aScatterPea->mDamageOverride = 200;') { throw 'The scatter damage override (200) must stay untouched - the ultimate mechanics are the same as the Gatling Pea.' }

# --- 大招里的"普通火豌豆"必须真的看得见 ---
# 直接 AddProjectile 出来的 PROJECTILE_FIREBALL 走的是 ProjectileInitialize：原版那一支只有一句
# TOD_ASSERT(false)（普通火球只由 ConvertToFireball 在飞过火炬树桩时补挂动画）。火焰机枪射手的大招会
# **直接**生成这种弹丸，所以 Initialize 必须自己挂上不带紫火滤镜的火球动画，否则 Draw 里 aImage 为
# nullptr —— 这半发弹丸会变成隐形弹（看起来就像"大招 100% 紫火"）。
$projCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Projectile.cpp')
$fireballCase = [regex]::Match($projCpp, 'case ProjectileType::PROJECTILE_FIREBALL:[\s\S]{0,1600}?break;')
if (-not $fireballCase.Success) { throw 'Could not find the PROJECTILE_FIREBALL case of ProjectileInitialize.' }
if ($fireballCase.Value -match 'TOD_ASSERT\(false\)') { throw 'PROJECTILE_FIREBALL must not be assertion-only in ProjectileInitialize: the scatter ultimate creates plain fire peas directly, and without an attached fire reanim they are invisible.' }
if ($fireballCase.Value -notmatch 'AddReanimation\(0\.0f, 0\.0f, 0, ReanimationType::REANIM_FIRE_PEA\)') { throw 'A directly created PROJECTILE_FIREBALL must attach the REANIM_FIRE_PEA fireball visual.' }
if ($fireballCase.Value -notmatch 'AttachReanim\(mAttachmentID, aFireReanim') { throw 'The plain fire pea visual must actually be attached to the projectile.' }
if ($fireballCase.Value -match 'FILTER_EFFECT_FIREPEA_PURPLE') { throw 'The plain fire pea must NOT carry the purple filter - that filter is what distinguishes it from the purple one.' }
if ($fireBody.Value -notmatch 'SEED_SNOW_GATLING_PEA \|\| mSeedType == SeedType::SEED_FIRE_GATLING_PEA\)\s*\r?\n\s*\{[\s\S]{0,200}GetPeaHeadOffset\(aOffsetX, aOffsetY\);[\s\S]{0,120}aOriginX = mX \+ aOffsetX \+ 34;') { throw 'Plant::Fire must use the gatling muzzle offset (+34) for SEED_FIRE_GATLING_PEA.' }
if ($fireBody.Value -notmatch 'SEED_SNOW_GATLING_PEA \|\| mSeedType == SeedType::SEED_FIRE_GATLING_PEA\)\s*&&\s*mGatlingScatterCountdown == 0\)[\s\S]{0,400}mGatlingScatterChance\+\+') { throw 'The scatter-chance growth must cover SEED_FIRE_GATLING_PEA (otherwise the ultimate never ramps up).' }
if ($fireBody.Value -notmatch 'IsLoneWolfLevel\(\)[\s\S]{0,400}SEED_FIRE_GATLING_PEA') { throw 'The lone-wolf damage override list must include SEED_FIRE_GATLING_PEA (same as the other gatlings).' }

# --- 连发节奏：枪口/开火帧/眨眼都并入机枪射手家族 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_SNOW_GATLING_PEA \|\| mSeedType == SeedType::SEED_FIRE_GATLING_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*aHeadReanim->mAnimRate = 38\.0f;\s*\r?\n\s*mShootingCounter = 100;' 'FindTargetAndFire must give SEED_FIRE_GATLING_PEA the gatling head animation rate and the 100-frame 4-shot cycle.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_SNOW_GATLING_PEA \|\| mSeedType == SeedType::SEED_FIRE_GATLING_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*if \(mGatlingScatterCountdown > 0\)' 'UpdateShooting must route SEED_FIRE_GATLING_PEA into the gatling 4-shot / scatter branch.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'case SeedType::SEED_FIRE_PEASHOOTER:\s*\r?\n\s*case SeedType::SEED_FIRE_GATLING_PEA:' 'PlantInitialize must handle SEED_FIRE_GATLING_PEA in the peashooter group (body + head reanim).'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FIRE_PEASHOOTER \|\| mSeedType == SeedType::SEED_FIRE_GATLING_PEA[\s\S]{0,2000}?aBlinkReanimType' 'AttachBlinkAnim must include SEED_FIRE_GATLING_PEA (otherwise the plant never blinks).'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FIRE_GATLING_PEA\)\s*\r?\n\s*aBlinkReanimType = FireGatlingReanimType\(\)' 'AttachBlinkAnim must use FireGatlingReanimType for the blink attachment (fire-coloured eyelids).'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FIRE_GATLING_PEA\)\s*\r?\n\s*aReanimType = FireGatlingReanimType\(\);[\s\S]{0,400}?mPlantCol' 'PlantInitialize must resolve the fire gatling reanim type before building the animation.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FIRE_GATLING_PEA\)\s*\r?\n\s*aReanimType = FireGatlingReanimType\(\);[\s\S]{0,400}?if \(aReanimType' 'PreloadPlantResources must preload the fire gatling reanim type.'

# --- 贴图：六张同尺寸部件（**没有枪管**，枪管沿用原版） ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'bool FireGatlingHasCustomArt\(\)' 'FireGatlingHasCustomArt must exist.'
$swapBlock = [regex]::Match($plantCpp, 'bool FireGatlingHasCustomArt\(\)[\s\S]*?\n\}')
if (-not $swapBlock.Success) { throw 'Could not find FireGatlingHasCustomArt body.' }
foreach ($part in @('HEAD', 'MOUTH', 'MOUTH_OVERLAY', 'BLINK1', 'BLINK2', 'HELMET')) {
    $pattern = '"reanim/GATLINGPEA_' + $part + '",\s*"reanim/FIREGATLING_' + $part + '"'
    if ($swapBlock.Value -notmatch $pattern) { throw "FireGatlingHasCustomArt must swap GATLINGPEA_$part to FIREGATLING_$part (per-frame pointer swap, not a track override)." }
}
if ($swapBlock.Value -cmatch 'FIREGATLING_BARREL') {
    throw 'FireGatlingHasCustomArt must NOT swap the barrel: there is no FireGatling_barrel.png, and ApplyReanimArtSwaps fails as a whole when one image is missing.'
}
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_FIRE_GATLING_PEA\)\s*\r?\n\s*\{[\s\S]{0,120}FireGatlingHasCustomArt\(\)' 'The cached plant frame (seed packet / almanac / cursor) must swap in the fire gatling art before building the temporary reanimation.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_FIRE_GATLING_PEA\)\s*\r?\n\s*aReanimType = FireGatlingReanimType\(\)' 'MakeCachedPlantFrame must use FireGatlingReanimType.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_SNOW_GATLING_PEA \|\|\s*\r?\n\s*theSeedType == SeedType::SEED_FIRE_GATLING_PEA' 'MakeCachedPlantFrame must draw the head layer for SEED_FIRE_GATLING_PEA.'

# --- HP 悬浮提示 / 沙盒 / 图鉴 ---
Assert-Source (P 'src/Lawn/Board.cpp') 'SEED_SNOW_GATLING_PEA \|\| aPlant->mSeedType == SeedType::SEED_FIRE_GATLING_PEA' 'The HP tooltip must report scatter state for SEED_FIRE_GATLING_PEA too.'
Assert-Source (P 'src/Lawn/Board.cpp') 'gIceSandboxTravelSeeds\[\][\s\S]{0,600}SEED_FIRE_GATLING_PEA' 'SEED_FIRE_GATLING_PEA must be available on the ice sandbox travel page.'
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.cpp') 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]{0,700}SEED_FIRE_GATLING_PEA' 'SEED_FIRE_GATLING_PEA must show up on almanac plant page 2.'
# 槽位数不写死：每加一只植物都要 +1，写死会让"新增植物"的提交顺手改坏这条断言。
# 这里改成"必须与 gAlmanacExtraSeeds 的实际条目数一致"。
$almanacCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Widget/AlmanacDialog.cpp')
$extraSeeds = [regex]::Match($almanacCpp, 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]*?\n\t\};')
if (-not $extraSeeds.Success) { throw 'Could not find the almanac extra-plant list.' }
$extraSeedCount = ([regex]::Matches($extraSeeds.Value, 'SeedType::SEED_')).Count
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.h') "#define NUM_ALMANAC_EXTRA_SEEDS $extraSeedCount" "NUM_ALMANAC_EXTRA_SEEDS must match the number of entries in gAlmanacExtraSeeds ($extraSeedCount)."

# --- 合成返阳光：寒冰机枪射手 / 火焰机枪射手 两种合成都要把 175 阳光原样还给玩家 ---
Assert-Source (P 'src/GameConstants.h') 'GATLING_SYNTHESIS_REFUND\s*=\s*175' 'GATLING_SYNTHESIS_REFUND must be 175 (the price of both the Snow Pea and the Fire Pea Shooter card).'
$boardCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Board.cpp')
if ($boardCpp -notmatch 'SEED_SNOWPEA && aNormalPlant->mSeedType == SeedType::SEED_GATLINGPEA\)[\s\S]{0,500}aIsGatlingSynthesis = true;') { throw 'The snow gatling synthesis (Snow Pea @ Gatling Pea) must set the refund flag.' }
if ($boardCpp -notmatch 'SEED_FIRE_PEASHOOTER && aNormalPlant->mSeedType == SeedType::SEED_GATLINGPEA\)[\s\S]{0,500}aIsGatlingSynthesis = true;') { throw 'The fire gatling synthesis (Fire Pea Shooter @ Gatling Pea) must set the refund flag.' }
if ($boardCpp -notmatch 'aIsGatlingSynthesis && aPaysWithSun[\s\S]{0,500}AddSunMoney\(aGatlingSynthesisRefund\)') { throw 'The gatling synthesis must refund its card price (aGatlingSynthesisRefund) after the card was charged.' }
if ($boardCpp -notmatch 'const bool aPaysWithSun = [\s\S]{0,200}CURSOR_TYPE_PLANT_FROM_BANK[\s\S]{0,120}!HasConveyorBeltSeedBank\(\)') { throw 'The refund must be gated on "the card actually cost sun" (conveyor belts / free planting must not hand out free sun).' }

# --- 翻译文案：卡名 + 悬浮说明 + 图鉴描述 + 合成返阳光（正式文件与中文备份各一份） ---
foreach ($file in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $file) '<String id="FIRE_GATLING_PEA">' "$file must define the FIRE_GATLING_PEA name string."
    Assert-Source (P $file) '<String id="FIRE_GATLING_PEA_TOOLTIP">' "$file must define the FIRE_GATLING_PEA_TOOLTIP string."
    Assert-Source (P $file) '<String id="FIRE_GATLING_PEA_DESCRIPTION">' "$file must define the FIRE_GATLING_PEA_DESCRIPTION almanac entry."
    Assert-Source (P $file) '<String id="GATLING_SYNTHESIS_REFUND">' "$file must define the GATLING_SYNTHESIS_REFUND advice string (used by both gatling syntheses)."
}

# --- 资源：玩家提供的贴图必须真的在 res/main 里（否则整体回退成普通机枪射手） ---
foreach ($png in @('FireGatling_head.png', 'FireGatling_mouth.png', 'FireGatling_mouth_overlay.png',
                   'FireGatling_blink1.png', 'FireGatling_blink2.png', 'FireGatling_helmet.png')) {
    if (-not (Test-Path -LiteralPath (P "res/main/reanim/$png"))) { throw "res/main/reanim/$png is missing - the plant would silently fall back to plain GatlingPea art." }
}

Write-Output 'Fire Gatling Pea (SEED_FIRE_GATLING_PEA) source checks passed.'
