$ErrorActionPreference = 'Stop'

# 火豌豆射手（SEED_FIRE_PEASHOOTER）源级检查：
#   旅行专属红卡 / 175 阳光 / 每 1.125 秒一发 / 紫火豌豆 65 伤害 / 命中后僵尸变红 4 秒、受到伤害 +40%
#   / 贴图 = 带火的豌豆射手（叶子轨道被 FirePeaShooter_fire1/fire2 交替覆盖）。
# 说明：本仓库无自动化测试框架（见 AGENTS.md），与其它 check-*.ps1 一样做源级断言；
#      真正的手感（火焰是否落在叶子处、变红是否好看、65/91/49 的伤害节奏）仍需手动进游戏验证，
#      验收步骤见 docs/superpowers/specs/2026-09-20-fire-pea-shooter-design.md。

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
Assert-Source (P 'src/ConstEnums.h') 'SEED_SNOW_GATLING_PEA,[\s\S]{0,300}SEED_FIRE_PEASHOOTER,[\s\S]{0,2000}NUM_SEED_TYPES' 'SEED_FIRE_PEASHOOTER must be declared after SEED_SNOW_GATLING_PEA and before NUM_SEED_TYPES (later mod seeds appended after it are fine).'
Assert-Source (P 'src/ConstEnums.h') 'REANIM_SNOW_GATLINGPEA,[\s\S]{0,900}REANIM_FIRE_PEASHOOTER,[\s\S]{0,2000}NUM_REANIMS' 'REANIM_FIRE_PEASHOOTER must be declared after REANIM_SNOW_GATLINGPEA and before NUM_REANIMS (later mod reanims appended after it are fine).'
Assert-Source (P 'src/ConstEnums.h') 'PROJECTILE_ELECTRIC_STAR = 15,[\s\S]{0,300}PROJECTILE_PURPLE_FIRE_PEA = 16,[\s\S]{0,600}NUM_PROJECTILES = \d+' 'PROJECTILE_PURPLE_FIRE_PEA must be appended after PROJECTILE_ELECTRIC_STAR (later mod projectiles may push NUM_PROJECTILES further).'

# --- 常量：4 秒 / +40% / 1.125 秒节奏 / 火焰切换 / 紫火换色参数 ---
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEASHOOTER_LAUNCH_RATE\s*=\s*113' 'FIRE_PEASHOOTER_LAUNCH_RATE must be 113 logic frames (1.125 s at 100 fps).'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEASHOOTER_FIRE_FLIP_TICKS\s*=\s*8' 'FIRE_PEASHOOTER_FIRE_FLIP_TICKS must be defined (fire1/fire2 alternation).'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEA_VULN_TICKS\s*=\s*400' 'FIRE_PEA_VULN_TICKS must be 400 logic frames (4 seconds).'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEA_VULN_PERCENT\s*=\s*40' 'FIRE_PEA_VULN_PERCENT must be 40.'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEA_VULN_R\s*=\s*255' 'The vulnerability tint must be the red twin of the chill tint (255,75,75).'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEA_VULN_G\s*=\s*75' 'The vulnerability tint green channel must mirror the chill tint (75).'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEA_VULN_B\s*=\s*75' 'The vulnerability tint blue channel must mirror the chill tint (75).'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEA_SPLASH_WIDTH\s*=\s*100' 'FIRE_PEA_SPLASH_WIDTH must define the (area damage) splash rect.'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEASHOOTER_FIRE_OFFSET_X\s*=\s*10\.0f' 'FIRE_PEASHOOTER_FIRE_OFFSET_X must shift the head-leaf fire 10 px to the right.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'aTrackInstance->mShakeX = FIRE_PEASHOOTER_FIRE_OFFSET_X' 'The fire offset must be applied through the track instance shake (only that track moves).'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEA_PURPLE_HUE\s*=\s*5\.03' 'FIRE_PEA_PURPLE_HUE must be the 5.03 hue sector from 火豌豆射手的颜色.txt.'
Assert-Source (P 'src/GameConstants.h') 'FIRE_PEA_PURPLE_SAT\s*=\s*1\.47' 'FIRE_PEA_PURPLE_SAT must be the 1.47 saturation factor from 火豌豆射手的颜色.txt.'

# --- 紫火滤镜：新值插在 NUM_FILTER_EFFECTS 之前 + 真的接进创建路径 ---
Assert-Source (P 'src/Sexy.TodLib/FilterEffect.h') 'FILTER_EFFECT_WHITE,[\s\S]{0,600}FILTER_EFFECT_FIREPEA_PURPLE,[\s\S]{0,200}NUM_FILTER_EFFECTS' 'FILTER_EFFECT_FIREPEA_PURPLE must be declared before NUM_FILTER_EFFECTS.'
Assert-Source (P 'src/Sexy.TodLib/FilterEffect.cpp') 'case FilterEffect::FILTER_EFFECT_FIREPEA_PURPLE:[\s\S]{0,160}FilterEffectDoHueSat\(' 'FilterEffectCreateImage must route the purple fire filter through FilterEffectDoHueSat.'
Assert-Source (P 'src/Sexy.TodLib/FilterEffect.cpp') 'void FilterEffectDoHueSat\(MemoryImage\* theImage, float theHueSector, float theSatMul\)' 'FilterEffectDoHueSat must be defined with (hueSector, satMul) parameters.'

# --- reanim 槽位：与豌豆射手同一个文件、独立槽位、NO_ATLAS ---
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_FIRE_PEASHOOTER,\s*"reanim/PeaShooter\.reanim",\s*1 << ReanimFlags::REANIM_NO_ATLAS' 'REANIM_FIRE_PEASHOOTER must reuse reanim/PeaShooter.reanim with REANIM_NO_ATLAS.'

# --- 植物定义：专用 reanim / 175 阳光 / 普通冷却 750 / 射手子类 / 1.125 秒节奏 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'SeedType::SEED_FIRE_PEASHOOTER,\s*nullptr,\s*ReanimationType::REANIM_FIRE_PEASHOOTER,\s*0,\s*175,\s*750,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*FIRE_PEASHOOTER_LAUNCH_RATE,\s*"FIRE_PEASHOOTER"' 'gPlantDefs row must be REANIM_FIRE_PEASHOOTER / 175 sun / 750 refresh / SHOOTER / FIRE_PEASHOOTER_LAUNCH_RATE / "FIRE_PEASHOOTER".'

# --- 红卡 + 可直接种下（不进 IsUpgrade 升级卡列表） ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'bool Plant::IsRedCard[\s\S]{0,900}SEED_FIRE_PEASHOOTER' 'Plant::IsRedCard must include SEED_FIRE_PEASHOOTER.'
$plantCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Plant.cpp')
$upgradeBody = [regex]::Match($plantCpp, 'bool Plant::IsUpgrade[\s\S]*?\n\}')
if (-not $upgradeBody.Success) { throw 'Could not find Plant::IsUpgrade body.' }
if ($upgradeBody.Value -match 'SEED_FIRE_PEASHOOTER') { throw 'SEED_FIRE_PEASHOOTER must not be an upgrade card - it is planted directly.' }

# --- 旅行专属：Travel 表登记 + HasSeedType 仅旅行关可拥有 + 沙盒旅行页 + 图鉴第 2 页 ---
Assert-Source (P 'src/Lawn/Travel.cpp') 'SEED_FIRE_PEASHOOTER,\s*false' 'SEED_FIRE_PEASHOOTER must be registered in gTravelPlantDefs.'
Assert-Source (P 'src/LawnApp.cpp') 'case SeedType::SEED_FIRE_PEASHOOTER:[\s\S]{0,80}return IsTravelLevel\(mGameMode\)' 'LawnApp::HasSeedType must gate SEED_FIRE_PEASHOOTER on IsTravelLevel.'
Assert-Source (P 'src/Lawn/Board.cpp') 'gIceSandboxTravelSeeds\[\][\s\S]{0,500}SEED_FIRE_PEASHOOTER' 'SEED_FIRE_PEASHOOTER must be available on the ice sandbox travel page.'
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.cpp') 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]{0,600}SEED_FIRE_PEASHOOTER' 'SEED_FIRE_PEASHOOTER must show up on almanac plant page 2.'
# 槽位数不再写死：每加一只植物都要 +1，写死会让"新增植物"的提交顺手改坏这条断言。
# 这里改成"必须与 gAlmanacExtraSeeds 的实际条目数一致"。
$almanacCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Widget/AlmanacDialog.cpp')
$extraSeeds = [regex]::Match($almanacCpp, 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]*?\n\t\};')
if (-not $extraSeeds.Success) { throw 'Could not find the almanac extra-plant list.' }
$extraSeedCount = ([regex]::Matches($extraSeeds.Value, 'SeedType::SEED_')).Count
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.h') "#define NUM_ALMANAC_EXTRA_SEEDS $extraSeedCount" "NUM_ALMANAC_EXTRA_SEEDS must match the number of entries in gAlmanacExtraSeeds ($extraSeedCount)."

# --- 贴图：四张同尺寸部件（不含 LIPS，reanim 未引用）+ 两张火焰覆盖 backleaf ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'bool FirePeaShooterHasCustomArt\(\)' 'FirePeaShooterHasCustomArt must exist.'
Assert-Source (P 'src/Lawn/Plant.cpp') '"reanim/PEASHOOTER_HEAD",\s*"reanim/FIREPEASHOOTER_HEAD"' 'The head part must be swapped to FirePeaShooter_head.'
Assert-Source (P 'src/Lawn/Plant.cpp') '"reanim/PEASHOOTER_MOUTH",\s*"reanim/FIREPEASHOOTER_MOUTH"' 'The mouth part must be swapped to FirePeaShooter_mouth.'
Assert-Source (P 'src/Lawn/Plant.cpp') '"reanim/PEASHOOTER_BLINK1",\s*"reanim/FIREPEASHOOTER_BLINK1"' 'blink1 must be swapped (per-frame pointer swap, not a track override).'
Assert-Source (P 'src/Lawn/Plant.cpp') '"reanim/PEASHOOTER_BLINK2",\s*"reanim/FIREPEASHOOTER_BLINK2"' 'blink2 must be swapped (per-frame pointer swap, not a track override).'
$swapBlock = [regex]::Match($plantCpp, 'bool FirePeaShooterHasCustomArt\(\)[\s\S]*?\n\}')
if (-not $swapBlock.Success) { throw 'Could not find FirePeaShooterHasCustomArt body.' }
if ($swapBlock.Value -cmatch 'FIREPEASHOOTER_LIPS') { throw 'PeaShooter.reanim does not reference PeaShooter_Lips, so FirePeaShooter_lips must not be part of the swap list.' }
Assert-Source (P 'src/Lawn/Plant.cpp') 'FIREPEASHOOTER_FIRE1' 'The fire1 texture must be loaded.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'FIREPEASHOOTER_FIRE2' 'The fire2 texture must be loaded.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'FIRE_PEASHOOTER_FIRE_TRACK = "idle_headleaf_tip_top"' 'The fire anchor must be the topmost head-leaf tuft track (the bit behind the head, up-left of it).'
Assert-Source (P 'src/Lawn/Plant.cpp') 'void FirePeaShooterApplyFireOverride\(Reanimation\* theReanim, int theFireIndex\)[\s\S]{0,600}SetImageOverride\(FIRE_PEASHOOTER_FIRE_TRACK' 'The fire must override the head-leaf anchor track.'
$fireOverride = [regex]::Match($plantCpp, 'void FirePeaShooterApplyFireOverride\([\s\S]*?\n\}')
if (-not $fireOverride.Success) { throw 'Could not find FirePeaShooterApplyFireOverride body.' }
foreach ($wrong in @('"backleaf"', '"frontleaf"')) {
    if ($fireOverride.Value -match [regex]::Escape($wrong)) { throw "The fire must NOT go on ${wrong} - those are the base leaves at the bottom of the plant, not the tuft behind the head." }
}

# --- 没有眉毛、没有多余叶尖：原版装饰轨道一律隐藏 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'void FirePeaShooterHideTracks\(Reanimation\* theReanim\)' 'FirePeaShooterHideTracks must exist.'
$hideBlock = [regex]::Match($plantCpp, 'void FirePeaShooterHideTracks\([\s\S]*?\n\}')
if (-not $hideBlock.Success) { throw 'Could not find FirePeaShooterHideTracks body.' }
foreach ($track in @('PeaShooter_eyebrow', 'idle_headleaf_farthest', 'idle_headleaf_3rdfarthest',
                     'idle_headleaf_2ndfarthest', 'idle_headleaf_tip_bottom', 'idle_headleaf_nearest')) {
    if ($hideBlock.Value -notmatch [regex]::Escape($track)) { throw "FirePeaShooterHideTracks must hide the $track track." }
}
if ($hideBlock.Value -notmatch 'AssignRenderGroupToTrack\([\s\S]{0,120}RENDER_GROUP_HIDDEN\)') { throw 'Hidden tracks must actually be assigned to RENDER_GROUP_HIDDEN.' }
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FIRE_PEASHOOTER\)\s*\{[\s\S]{0,600}FirePeaShooterHideTracks\(aBodyReanim\);[\s\S]{0,200}FirePeaShooterHideTracks\(aHeadReanim\);[\s\S]{0,200}FirePeaShooterApplyFireOverride\(aHeadReanim, 0\)' 'PlantInitialize must hide the tracks on both instances and put the fire on the HEAD instance (the head-leaf tuft lives in the head layer only).'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FIRE_PEASHOOTER\)\s*\{[\s\S]{0,320}Reanimation\* aHeadReanim = mApp->ReanimationTryToGet\(mHeadReanimID\)[\s\S]{0,300}FirePeaShooterApplyFireOverride' 'Plant::Update must alternate the fire on the HEAD instance.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_FIRE_PEASHOOTER[\s\S]{0,500}FirePeaShooterHideTracks\(&aReanim\)' 'The cached plant frame (seed packet / almanac / cursor) must hide the eyebrow too.'

# --- 火焰交替：场上的植物每帧切、卡面静态；豌豆组的头/眨眼轨道也要挂上 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FIRE_PEASHOOTER\)\s*\{[\s\S]{0,320}FIRE_PEASHOOTER_FIRE_FLIP_TICKS[\s\S]{0,200}FirePeaShooterApplyFireOverride' 'Plant::Update must alternate the two fire images on a timer.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'case SeedType::SEED_SNOW_GATLING_PEA:\s*\r?\n\s*case SeedType::SEED_FIRE_PEASHOOTER:' 'PlantInitialize must handle SEED_FIRE_PEASHOOTER in the peashooter group (body + head reanim).'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_SNOW_GATLING_PEA \|\| mSeedType == SeedType::SEED_FIRE_PEASHOOTER' 'AttachBlinkAnim must include SEED_FIRE_PEASHOOTER (otherwise the plant never blinks).'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_FIRE_PEASHOOTER[\s\S]{0,700}FirePeaShooterApplyFireOverride' 'The cached plant frame (seed packet / almanac / cursor) must show the fire too.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_SNOW_GATLING_PEA \|\|[\s\S]{0,200}theSeedType == SeedType::SEED_FIRE_PEASHOOTER' 'MakeCachedPlantFrame must draw the head layer for SEED_FIRE_PEASHOOTER.'

# --- 子弹本身（漏一处就会打出未初始化类型的弹丸 => 没有贴图 / 从错误位置飞出） ---
$fireBody = [regex]::Match($plantCpp, 'void Plant::Fire\([\s\S]*?\n\}')
if (-not $fireBody.Success) { throw 'Could not find Plant::Fire body.' }
if ($fireBody.Value -notmatch 'case SeedType::SEED_FIRE_PEASHOOTER:[\s\S]{0,800}PROJECTILE_PURPLE_FIRE_PEA') { throw 'Plant::Fire switch must map SEED_FIRE_PEASHOOTER to PROJECTILE_PURPLE_FIRE_PEA.' }
if ($fireBody.Value -notmatch 'SEED_PEATER_1_5 \|\| mSeedType == SeedType::SEED_FIRE_PEASHOOTER\)[\s\S]{0,160}GetPeaHeadOffset') { throw 'Plant::Fire must use the pea-head muzzle offset for SEED_FIRE_PEASHOOTER.' }

$projCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Projectile.cpp')
# --- 弹丸：65 伤害 + 紫火 reanim + 群伤 + 易伤挂在伤害之后 + 命中不炸火 ---
if ($projCpp -notmatch 'PROJECTILE_PURPLE_FIRE_PEA,\s*0,\s*65') { throw 'gProjectileDefinition must give PROJECTILE_PURPLE_FIRE_PEA 65 damage.' }
if ($projCpp -notmatch 'PROJECTILE_PURPLE_FIRE_PEA:[\s\S]{0,700}REANIM_FIRE_PEA') { throw 'The purple fire pea must attach the REANIM_FIRE_PEA fireball visual.' }
if ($projCpp -notmatch 'aFireReanim->mFilterEffect = FilterEffect::FILTER_EFFECT_FIREPEA_PURPLE') { throw 'The fireball must be recoloured through FILTER_EFFECT_FIREPEA_PURPLE.' }
if ($projCpp -notmatch 'theZombie->TakeDamage\(aDamage, aDamageFlags\);[\s\S]{0,700}PROJECTILE_PURPLE_FIRE_PEA[\s\S]{0,160}theZombie->ApplyFireVulnerability\(\)') { throw 'DoImpact must apply the vulnerability AFTER the damage, so the triggering pea does not benefit from its own debuff.' }
$splashBody = [regex]::Match($projCpp, 'bool Projectile::IsSplashDamage\([\s\S]*?\n\}')
if (-not $splashBody.Success) { throw 'Could not find Projectile::IsSplashDamage body.' }
if ($splashBody.Value -notmatch 'PROJECTILE_PURPLE_FIRE_PEA') { throw 'The purple fire pea must be a splash (area damage) projectile.' }
$splashHitBody = [regex]::Match($projCpp, 'bool Projectile::IsZombieHitBySplash\([\s\S]*?\n\}')
if (-not $splashHitBody.Success) { throw 'Could not find Projectile::IsZombieHitBySplash body.' }
if ($splashHitBody.Value -notmatch 'PROJECTILE_PURPLE_FIRE_PEA[\s\S]{0,200}FIRE_PEA_SPLASH_WIDTH') { throw 'The purple fire pea splash must use its own (wider) splash rect.' }
$splashDmgBody = [regex]::Match($projCpp, 'void Projectile::DoSplashDamage\([\s\S]*?\n\}')
if (-not $splashDmgBody.Success) { throw 'Could not find Projectile::DoSplashDamage body.' }
if ($splashDmgBody.Value -notmatch 'PROJECTILE_PURPLE_FIRE_PEA[\s\S]{0,400}ApplyFireVulnerability\(\)') { throw 'Splash victims of the purple fire pea must get the vulnerability too.' }
if ($projCpp -match 'PARTICLE_FIREBALL_DEATH') { throw 'The impact must NOT spawn PARTICLE_FIREBALL_DEATH (that is the Zombie Boss fireball burst - a screen-filling flame).' }
if ($projCpp -notmatch 'FOLEY_IGNITE') { throw 'The purple fire pea impact should still play the ignite foley.' }
foreach ($reg in @('CantHitHighGround', 'GetProjectileRect')) {
    $body = [regex]::Match($projCpp, "::$reg\([\s\S]*?\n\}")
    if (-not $body.Success) { throw "Could not find Projectile::$reg body." }
    if ($body.Value -notmatch 'PROJECTILE_PURPLE_FIRE_PEA') { throw "Projectile::$reg must register PROJECTILE_PURPLE_FIRE_PEA like a normal pea." }
}

# --- 僵尸：变红（叠加通道）+ 4 秒 + 加伤 40% + 存档字段 ---
Assert-Source (P 'src/Lawn/Zombie.h') 'int32_t\s+mFireVulnCounter;' 'Zombie must declare mFireVulnCounter.'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'void Zombie::ApplyFireVulnerability\(\)[\s\S]{0,320}mFireVulnCounter = FIRE_PEA_VULN_TICKS' 'ApplyFireVulnerability must refresh the timer to the full 4 seconds.'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'mFireVulnCounter\s*=\s*0;\s*\r?\n\s*mMindControlled' 'ZombieInitialize must reset mFireVulnCounter (reanim/zombie objects are pooled).'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'void Zombie::TakeDamage[\s\S]{0,1200}mFireVulnCounter > 0[\s\S]{0,200}\(100 \+ FIRE_PEA_VULN_PERCENT\)\s*\)\s*/\s*100' 'Zombie::TakeDamage must multiply all incoming damage by (100 + FIRE_PEA_VULN_PERCENT)/100 while vulnerable.'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'if \(mFireVulnCounter > 0\)\s*\r?\n\s*\{\s*\r?\n\s*mFireVulnCounter--;' 'Zombie::Update must tick the vulnerability timer down.'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'DAMAGE_HITS_SHIELD_AND_BODY\)\)[\s\S]{0,700}mFireVulnCounter > 0' 'The vulnerability bonus must also survive the DAMAGE_HITS_SHIELD_AND_BODY reset (fireball / melon body damage).'
$drawReanim = [regex]::Match((Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Zombie.cpp')), 'void Zombie::DrawReanim\([\s\S]*?\n\}')
if (-not $drawReanim.Success) { throw 'Could not find Zombie::DrawReanim body.' }
# 变红必须和"冰冻/减速"同一套画法：覆写色正片叠底 + 同色加色（加色让红更实），而不是叠加通道
if ($drawReanim.Value -notmatch 'mFireVulnCounter > 0[\s\S]{0,600}aColorOverride = Color\(FIRE_PEA_VULN_R, FIRE_PEA_VULN_G, FIRE_PEA_VULN_B') { throw 'DrawReanim must tint a vulnerable zombie through mColorOverride, exactly like the chill effect does.' }
if ($drawReanim.Value -notmatch 'mFireVulnCounter > 0[\s\S]{0,700}aExtraAdditiveColor = aColorOverride;[\s\S]{0,80}aEnableExtraAdditiveDraw = true;') { throw 'The vulnerability tint must reuse the chill recipe: same colour as an additive layer too.' }
Assert-Source (P 'src/Lawn/Zombie.cpp') 'mFireVulnCounter > 0\)\s*\{[\s\S]{0,900}mChilledCounter > 0 \|\| mIceTrapCounter > 0' 'The red tint must sit right before the chill branch (red wins visually, speed still chilled).'
Assert-Source (P 'src/Lawn/System/SaveGame.cpp') 'mTorchwoodSummoned\);[\s\S]{0,400}SyncInt32\(theZombie\.mFireVulnCounter\)' 'The vulnerability timer must be appended at the END of the zombie save tail (old saves read 0).'

# --- 翻译文案：卡名 + 悬浮说明 + 图鉴描述（正式文件与中文备份各一份） ---
foreach ($file in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $file) '<String id="FIRE_PEASHOOTER">' "$file must define the FIRE_PEASHOOTER name string."
    Assert-Source (P $file) '<String id="FIRE_PEASHOOTER_TOOLTIP">' "$file must define the FIRE_PEASHOOTER_TOOLTIP string."
    Assert-Source (P $file) '<String id="FIRE_PEASHOOTER_DESCRIPTION">' "$file must define the FIRE_PEASHOOTER_DESCRIPTION almanac entry."
}

# --- 资源：玩家提供的贴图必须真的在 res/main 里（否则整体回退成普通豌豆射手） ---
foreach ($png in @('FirePeaShooter_head.png', 'FirePeaShooter_mouth.png', 'FirePeaShooter_blink1.png', 'FirePeaShooter_blink2.png', 'FirePeaShooter_fire1.png', 'FirePeaShooter_fire2.png')) {
    if (-not (Test-Path -LiteralPath (P "res/main/reanim/$png"))) { throw "res/main/reanim/$png is missing - the plant would silently fall back to plain PeaShooter art." }
}

Write-Output 'Fire Pea Shooter (SEED_FIRE_PEASHOOTER) source checks passed.'



