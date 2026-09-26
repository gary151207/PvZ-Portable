$ErrorActionPreference = 'Stop'

# 魅惑大喷菇（SEED_HYPNOSHROOM_FUME）源级检查：
#   旅行专属形态（魅惑菇 × 大喷菇群 来回切换）/ 没有两侧小喷菇 / 大喷菇攻击管道 /
#   每次攻击 15% 概率魅惑半血僵尸（路障射手僵尸与僵王博士免疫）/
#   魅惑阵营对抗（巨人 500 砸击、冰车每刻 2 点，双向生效）。
# 本仓库没有自动化测试框架，因此用静态契约覆盖容易回归的接线；观感与实战行为仍需进游戏验收。

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

# --- 枚举：只能追加，保证旧 .v4 存档里的整数值稳定 ---
Assert-Source (P 'src/ConstEnums.h') 'SEED_SPORESHROOM,[\s\S]{0,300}SEED_HYPNOSHROOM_FUME,[\s\S]{0,200}NUM_SEED_TYPES' 'SEED_HYPNOSHROOM_FUME must be appended before NUM_SEED_TYPES.'
Assert-Source (P 'src/ConstEnums.h') 'REANIM_SPORESHROOM_PROJECTILE,[\s\S]{0,400}REANIM_HYPNOSHROOM_FUME,[\s\S]{0,200}NUM_REANIMS' 'REANIM_HYPNOSHROOM_FUME must be appended before NUM_REANIMS.'

# --- 动画槽位：与普通大喷菇同一个 .reanim 文件，用 NO_ATLAS 以便随时换图 ---
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_HYPNOSHROOM_FUME,\s*"reanim/FumeShroom\.reanim",\s*1 << ReanimFlags::REANIM_NO_ATLAS' 'Hypno Fume-shroom must reuse FumeShroom.reanim without an atlas.'

# --- 专用贴图：只换脑袋一张，尺寸必须与原图一致 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'bool HypnoFumeshroomHasCustomArt\(\)' 'HypnoFumeshroomHasCustomArt must exist.'
$plantCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Plant.cpp')
$swapBlock = [regex]::Match($plantCpp, 'bool HypnoFumeshroomHasCustomArt\(\)[\s\S]*?\n\}')
if (-not $swapBlock.Success) { throw 'Could not find HypnoFumeshroomHasCustomArt body.' }
$swapCount = ([regex]::Matches($swapBlock.Value, '\{\s*"reanim/')).Count
if ($swapCount -ne 1) { throw "HypnoFumeshroomHasCustomArt must swap exactly one image (the head), found $swapCount." }
if ($swapBlock.Value -notmatch 'reanim/FUMESHROOM_HEAD') { throw 'HypnoFumeshroomHasCustomArt must replace reanim/FUMESHROOM_HEAD.' }
if ($swapBlock.Value -notmatch 'reanim/HYPNOFUMESHROOM_HEAD') { throw 'HypnoFumeshroomHasCustomArt must load reanim/HypnoFumeshroom_head.png.' }
Assert-Source (P 'src/Lawn/Plant.cpp') 'ReanimationType HypnoFumeshroomReanimType\(\)[\s\S]{0,300}REANIM_FUMESHROOM' 'HypnoFumeshroomReanimType() must fall back to REANIM_FUMESHROOM when the custom art is unavailable.'
if (-not (Test-Path -LiteralPath (P 'res/main/reanim/HypnoFumeshroom_head.png'))) { throw 'Missing bundled art: res/main/reanim/HypnoFumeshroom_head.png' }

# --- 卡面 / 图鉴 / 光标预览都要走专用贴图与专用槽位 ---
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_HYPNOSHROOM_FUME\)\s*\r?\n\s*\{\s*\r?\n\s*HypnoFumeshroomHasCustomArt\(\);' 'The cached plant frame (seed packet / almanac / cursor) must apply the hypno head art first.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_HYPNOSHROOM_FUME\)\s*\r?\n\s*aReanimType = HypnoFumeshroomReanimType\(\)' 'The cached plant frame must use the same reanim-type resolution.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_HYPNOSHROOM_FUME\)\s*\r?\n\s*aReanimType = HypnoFumeshroomReanimType\(\)' 'PlantInitialize must resolve the hypno reanim type before AddReanimation().'

# --- 基础数值：沿用大喷菇（60 伤害穿透烟雾、90 帧节奏、300 生命），攻击矩形同大喷菇 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_HYPNOSHROOM_FUME,\s*nullptr,\s*ReanimationType::REANIM_FUMESHROOM,\s*9,\s*0,\s*3000,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*90,\s*"HYPNOSHROOM_FUME"' 'Plant definition must mirror the Fume-shroom timing (90) with the FUMESHROOM_GROUP-style name key.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FUMESHROOM_GROUP:[\s\S]{0,160}SEED_HYPNOSHROOM_FUME:\s*aRect = Rect\(mX \+ 60' 'Hypno Fume-shroom must share the Fume-shroom attack rect.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FUMESHROOM_GROUP[\s\S]{0,200}SEED_HYPNOSHROOM_FUME[\s\S]{0,300}DoRowAreaDamage\(60, 2U\)' 'Hypno Fume-shroom must fire the 60 damage fume cloud.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_FUMESHROOM_GROUP:[\s\S]{0,160}SEED_HYPNOSHROOM_FUME: mShootingCounter = 50' 'Hypno Fume-shroom must use the Fume-shroom 50 frame shooting counter.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'bool Plant::IsNocturnal[\s\S]{0,1200}SEED_HYPNOSHROOM_FUME' 'Hypno Fume-shroom must be nocturnal (it is still a mushroom).'
Assert-Source (P 'src/Lawn/Plant.cpp') 'bool Plant::IsFungus[\s\S]{0,1400}SEED_HYPNOSHROOM_FUME' 'Hypno Fume-shroom must be classified as a fungus.'

# --- 没有两侧小喷菇：新形态不能建任何子 reanim ---
$initCase = [regex]::Match($plantCpp, 'case SeedType::SEED_HYPNOSHROOM_FUME:[\s\S]*?break;\s*\r?\n\s*\}')
if (-not $initCase.Success) { throw 'Could not find the SEED_HYPNOSHROOM_FUME PlantInitialize case.' }
if ($initCase.Value -match 'AddReanimation|REANIM_PUFFSHROOM') { throw 'Hypno Fume-shroom must not spawn the two side Puff-shrooms.' }
Assert-Source (P 'src/Lawn/Plant.cpp') 'case SeedType::SEED_HYPNOSHROOM_FUME:[\s\S]{0,400}mPlantHealth = 300' 'Hypno Fume-shroom must keep 300 hit points.'

# --- 切换路径：魅惑菇 @ 大喷菇群 → 魅惑大喷菇；小喷菇 @ 魅惑大喷菇 → 大喷菇群 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'theUpgradedType == SeedType::SEED_HYPNOSHROOM && mSeedType == SeedType::SEED_FUMESHROOM_GROUP' 'Planting a Hypno-shroom on a Fumeshroom Group must be allowed.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'theUpgradedType == SeedType::SEED_PUFFSHROOM && mSeedType == SeedType::SEED_HYPNOSHROOM_FUME' 'Planting a Puff-shroom on a Hypno Fume-shroom must be allowed.'
Assert-Source (P 'src/Lawn/Board.cpp') 'SEED_HYPNOSHROOM && aNormalPlant->mSeedType == SeedType::SEED_FUMESHROOM_GROUP[\s\S]{0,300}aPlantSeedType = SeedType::SEED_HYPNOSHROOM_FUME' 'Board::MouseDownWithPlant must rewrite the planted seed to SEED_HYPNOSHROOM_FUME.'
Assert-Source (P 'src/Lawn/Board.cpp') 'SEED_PUFFSHROOM && aNormalPlant->mSeedType == SeedType::SEED_HYPNOSHROOM_FUME[\s\S]{0,300}aPlantSeedType = SeedType::SEED_FUMESHROOM_GROUP' 'Board::MouseDownWithPlant must rewrite the planted seed back to SEED_FUMESHROOM_GROUP.'
# 它没有任何独立卡牌：不能登记成旅行专属种子（否则会出现在选卡器页 1 上）
$travelCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Travel.cpp')
if ($travelCpp -match 'SEED_HYPNOSHROOM_FUME') { throw 'Hypno Fume-shroom must not be registered as a standalone travel seed (it is a switch form, not a card).' }

# --- 魅惑判定：每次攻击掷一次 15% 的骰；只魅惑半血；排除 BOSS ---
Assert-Source (P 'src/GameConstants.h') 'HYPNOSHROOM_FUME_CHARM_PERCENT\s*=\s*15' 'The charm chance must be 15%.'
Assert-Source (P 'src/GameConstants.h') 'HYPNOSHROOM_FUME_CHARM_HP_PERCENT\s*=\s*50' 'Only zombies below 50% health may be charmed.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'void Plant::TryHypnotizeFumeTargets\(\)[\s\S]{0,600}Rand\(100\) >= HYPNOSHROOM_FUME_CHARM_PERCENT[\s\S]{0,2500}mBodyHealth \* 100[\s\S]{0,60}mBodyMaxHealth \* HYPNOSHROOM_FUME_CHARM_HP_PERCENT' 'The charm roll must be a single per-volley roll filtered by body health.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'void Plant::TryHypnotizeFumeTargets\(\)[\s\S]{0,2500}ZOMBIE_BOSS[\s\S]{0,2500}ApplyCharmedByPlant' 'Boss Zombie and the BOSS Conehead Pea must be excluded from charming.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_HYPNOSHROOM_FUME\)\s*\r?\n\s*\{[\s\S]{0,500}TryHypnotizeFumeTargets\(\);' 'Fire() must run the charm roll on every Hypno Fume-shroom volley.'

# --- 魅惑流程与阵营对抗：巨人/冰车双向伤害 ---
Assert-Source (P 'src/Lawn/Zombie.h') 'void\s+ApplyCharmedByPlant\(\);' 'Zombie::ApplyCharmedByPlant must be declared.'
Assert-Source (P 'src/Lawn/Zombie.h') 'void\s+DamageOpposingZombiesInAttackRect\(int theDamage\);' 'Zombie::DamageOpposingZombiesInAttackRect must be declared.'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'void Zombie::ApplyCharmedByPlant\(\)[\s\S]{0,600}StartMindControlled\(\)[\s\S]{0,600}PARTICLE_MIND_CONTROL' 'The charm flow must be shared between the Hypno-shroom and the Hypno Fume-shroom.'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'void Zombie::DamageOpposingZombiesInAttackRect\(int theDamage\)[\s\S]{0,900}DAMAGES_ONLY_MINDCONTROLLED' 'Opposing-zombie damage must flip the DAMAGES_ONLY_MINDCONTROLLED bit with the attacker side.'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'ShouldTriggerTimedEvent\(0\.64f\)[\s\S]{0,900}DamageOpposingZombiesInAttackRect\(GARGANTUAR_ZOMBIE_SMASH_DAMAGE\)' 'The Gargantuar smash must damage the opposing zombies in its attack rect.'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'ShouldTriggerTimedEvent\(0\.64f\)[\s\S]{0,1400}if \(!mMindControlled\)' 'A mind-controlled Gargantuar must not smash plants.'
$zombieCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Zombie.cpp')
$gargantuarSmash = [regex]::Match($zombieCpp, 'void Zombie::UpdateZombieGargantuar\(\)[\s\S]*?\n\}\r?\n\r?\nvoid Zombie::UpdateZombieImp')
if (-not $gargantuarSmash.Success) { throw 'Could not find Zombie::UpdateZombieGargantuar.' }
if ($gargantuarSmash.Value -match 'ZOMBIE_REDEYE_GARGANTUAR \? 1000 : 500') { throw 'Both Gargantuars must deal the same 500 zombie smash damage.' }

# --- 魅惑僵尸死亡时**不**再释放樱桃炸弹爆炸（该特性已按需求删除，防止回归） ---
$dieNoLoot = [regex]::Match($zombieCpp, 'void Zombie::DieNoLoot\(\)[\s\S]*?\r?\n\}')
if (-not $dieNoLoot.Success) { throw 'Could not find Zombie::DieNoLoot.' }
if ($dieNoLoot.Value -match 'mMindControlled' -or $dieNoLoot.Value -match 'FOLEY_CHERRYBOMB' -or $dieNoLoot.Value -match 'sCherryInProgress') {
    throw 'Charmed zombies must no longer explode on death (the cherry-bomb death effect was removed).'
}
# --- 被魅惑的巨人扔出的小鬼同样是被魅惑状态（不受 DO_FIX_BUGS 影响） ---
Assert-Source (P 'src/Lawn/Zombie.cpp') 'if \(mMindControlled\)\s*\r?\n\s*\{\s*\r?\n\s*aZombieImp->mPosX = mPosX \+ mWidth;\s*\r?\n\s*aZombieImp->ApplyCharmedByPlant\(\);\s*\r?\n\s*aZombieImp->mVelX = -3\.0f;' 'A mind-controlled Gargantuar must throw an already-charmed Imp (from its right side, flying right).'
$throwImp = [regex]::Match($zombieCpp, 'aZombieImp->mZombiePhase = ZombiePhase::PHASE_IMP_GETTING_THROWN;[\s\S]{0,1800}?PlayZombieReanim\("anim_thrown"')
if (-not $throwImp.Success) { throw 'Could not find the Gargantuar Imp throw setup.' }
if ($throwImp.Value -match '#ifdef DO_FIX_BUGS([\s\S]*?)#endif') {
    if ($Matches[1] -match 'mMindControlled') { throw 'The charmed-Imp throw must not be hidden behind DO_FIX_BUGS.' }
}

Assert-Source (P 'src/Lawn/Zombie.cpp') 'void Zombie::CheckForZombieStep\(\)[\s\S]{0,900}ZOMBIE_ZAMBONI && !mFlatTires[\s\S]{0,300}DamageOpposingZombiesInAttackRect\(ZAMBONI_ZOMBIE_GRIND_DAMAGE\)' 'The Zamboni must grind the opposing zombies every game tick.'
Assert-Source (P 'src/GameConstants.h') 'GARGANTUAR_ZOMBIE_SMASH_DAMAGE\s*=\s*500' 'The Gargantuar zombie smash damage must be 500.'
Assert-Source (P 'src/GameConstants.h') 'ZAMBONI_ZOMBIE_GRIND_DAMAGE\s*=\s*2' 'The Zamboni zombie grind damage must be 2 per tick.'
Assert-Source (P 'src/Lawn/Zombie.cpp') 'bool Zombie::CanTargetPlant\([\s\S]{0,400}if \(mMindControlled\)[\s\S]{0,200}return false;' 'A mind-controlled zombie must never target plants.'

# --- 冰面沙盒（旅行页）与图鉴 ---
Assert-Source (P 'src/Lawn/Board.cpp') 'gIceSandboxTravelSeeds\[\][\s\S]{0,1400}SEED_HYPNOSHROOM_FUME' 'Hypno Fume-shroom should be plantable from the travel sandbox page.'
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.cpp') 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]{0,1600}SEED_HYPNOSHROOM_FUME' 'Hypno Fume-shroom must appear on almanac page 2.'
$almanacCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Widget/AlmanacDialog.cpp')
$extraSeeds = [regex]::Match($almanacCpp, 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]*?\n\t\};')
$extraSeedCount = ([regex]::Matches($extraSeeds.Value, 'SeedType::SEED_')).Count
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.h') "#define NUM_ALMANAC_EXTRA_SEEDS $extraSeedCount" "NUM_ALMANAC_EXTRA_SEEDS must match the number of entries in gAlmanacExtraSeeds ($extraSeedCount)."
Assert-Source (P 'src/Lawn/SeedPacket.cpp') 'case SeedType::SEED_HYPNOSHROOM_FUME:' 'Hypno Fume-shroom needs a seed packet scale entry.'

# --- 文案（断言只用 ASCII，避免脚本文件编码影响匹配） ---
foreach ($path in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $path) '<String id="HYPNOSHROOM_FUME">' 'Missing Hypno Fume-shroom name.'
    Assert-Source (P $path) '<String id="HYPNOSHROOM_FUME_TOOLTIP">' 'Missing Hypno Fume-shroom tooltip.'
    Assert-Source (P $path) '<String id="HYPNOSHROOM_FUME_DESCRIPTION">' 'Missing Hypno Fume-shroom almanac description.'
}

Write-Output 'Hypno Fume-shroom (SEED_HYPNOSHROOM_FUME) source checks passed.'
