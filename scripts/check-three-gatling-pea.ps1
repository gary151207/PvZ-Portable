$ErrorActionPreference = 'Stop'

# 三线机枪射手（SEED_THREE_GATLING_PEA）源级检查：
#   合成态（三线射手卡 325 × 已种的机枪射手，落地返还 325 → 净花费 0）
#   / 普攻 = 每轮向每一行各 4 连发
#   / 大招 = 每一行每 0.03 秒一发、每颗高度 ±15px、持续 3 秒（**不是**随机角度散射）
#   / 贴图 = 独立的 reanim/ThreeGaling.reanim（三线射手三个头 + 三顶机枪头盔 + 每头一根四段枪管 + 嘴覆层）
#   / 弹幕量必须留在弹丸池与性能预算内（0.02 秒两发那一档会把池子顶满并闪退）
#
# 说明：本仓库无自动化测试框架（见 AGENTS.md），与其它 check-*.ps1 一样做源级断言；
#      真正的手感（头盔位置/缩放、大招观感）仍需手动进游戏验证，
#      验收步骤见 docs/superpowers/specs/2026-09-25-three-gatling-pea-design.md。

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
Assert-Source (P 'src/ConstEnums.h') 'SEED_FIRE_GATLING_PEA,[\s\S]{0,400}SEED_THREE_GATLING_PEA,[\s\S]{0,800}NUM_SEED_TYPES' 'SEED_THREE_GATLING_PEA must be appended after SEED_FIRE_GATLING_PEA and just before NUM_SEED_TYPES.'
Assert-Source (P 'src/ConstEnums.h') 'REANIM_FIRE_GATLINGPEA,[\s\S]{0,1200}REANIM_THREE_GATLINGPEA,[\s\S]{0,800}NUM_REANIMS' 'REANIM_THREE_GATLINGPEA must be appended after REANIM_FIRE_GATLINGPEA and just before NUM_REANIMS.'

# --- reanim 槽位：**自己的**文件 + REANIM_NO_ATLAS ---
# gLawnReanimationArray 是按 ReanimationType 下标取用的，条目顺序必须与枚举逐条对齐。
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_FIRE_GATLINGPEA,[\s\S]{0,4000}REANIM_THREE_GATLINGPEA,\s*"reanim/ThreeGaling\.reanim",\s*1 << ReanimFlags::REANIM_NO_ATLAS' 'REANIM_THREE_GATLINGPEA must be the last gLawnReanimationArray row, load reanim/ThreeGaling.reanim and use REANIM_NO_ATLAS.'

# --- 常量 ---
Assert-Source (P 'src/GameConstants.h') 'THREE_GATLING_SYNTHESIS_REFUND\s*=\s*325' 'THREE_GATLING_SYNTHESIS_REFUND must be 325 (the price of the Threepeater card that gets consumed).'
Assert-Source (P 'src/GameConstants.h') 'THREE_GATLING_ULTIMATE_TICKS\s*=\s*300' 'THREE_GATLING_ULTIMATE_TICKS must be 300 frames (3 s at 100 fps).'
Assert-Source (P 'src/GameConstants.h') 'THREE_GATLING_ULTIMATE_INTERVAL\s*=\s*3' 'THREE_GATLING_ULTIMATE_INTERVAL must be 3 frames (0.03 s per volley): the denser 0.02 s / 2-per-row version froze and crashed the game.'
Assert-Source (P 'src/GameConstants.h') 'THREE_GATLING_BULLETS_PER_ROW\s*=\s*1' 'THREE_GATLING_BULLETS_PER_ROW must be 1 (one bullet at every row per 0.03 s volley).'
Assert-Source (P 'src/GameConstants.h') 'THREE_GATLING_HEIGHT_JITTER\s*=\s*15' 'THREE_GATLING_HEIGHT_JITTER must be 15 px (the ultimate height float is +/-15 px).'
Assert-Source (P 'src/GameConstants.h') 'THREE_GATLING_PROJECTILE_POOL_GUARD\s*=\s*3072' 'THREE_GATLING_PROJECTILE_POOL_GUARD must exist below the 4096-slot projectile pool: a full pool does not assert in Release, it writes out of bounds.'

# --- 植物定义：325 阳光 / 普通冷却 750 / 射手子类 / 100 发射节奏（= 机枪射手 4 连发周期） ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'SeedType::SEED_THREE_GATLING_PEA,\s*nullptr,\s*ReanimationType::REANIM_THREE_GATLINGPEA,\s*12,\s*325,\s*750,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*100,\s*"THREE_GATLING_PEA"' 'gPlantDefs row must be REANIM_THREE_GATLINGPEA / 325 sun / 750 refresh / SHOOTER / 100 launch rate / "THREE_GATLING_PEA".'

$plantCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Plant.cpp')

# --- 合成态：没有自己的种子卡（不是紫卡），但能被三线射手卡升级出来 ---
$upgradeBody = [regex]::Match($plantCpp, 'bool Plant::IsUpgrade[\s\S]*?\n\}')
if (-not $upgradeBody.Success) { throw 'Could not find Plant::IsUpgrade body.' }
if ($upgradeBody.Value -match 'SEED_THREE_GATLING_PEA') { throw 'SEED_THREE_GATLING_PEA must not be an upgrade card - it is a hidden synthesis state with no seed packet of its own.' }
$upgradableBody = [regex]::Match($plantCpp, 'bool Plant::IsUpgradableTo[\s\S]*?\n\}')
if (-not $upgradableBody.Success) { throw 'Could not find Plant::IsUpgradableTo body.' }
if ($upgradableBody.Value -notmatch 'theUpgradedType == SeedType::SEED_THREEPEATER && mSeedType == SeedType::SEED_GATLINGPEA') { throw 'Plant::IsUpgradableTo must allow a Threepeater card to convert a Gatling Pea.' }

# --- 外观接线：三个头（PlantInitialize / 眨眼 / 卡面）都并入三线射手那条分支 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'case SeedType::SEED_THREEPEATER:\s*\r?\n\s*case SeedType::SEED_THREE_GATLING_PEA:' 'PlantInitialize must build the three heads for SEED_THREE_GATLING_PEA (same case as SEED_THREEPEATER).'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_THREEPEATER \|\| mSeedType == SeedType::SEED_THREE_GATLING_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*int aHit = Rand\(3\)' 'AttachBlinkAnim must blink SEED_THREE_GATLING_PEA through one of its three heads.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_THREEPEATER \|\| theSeedType == SeedType::SEED_THREE_GATLING_PEA' 'MakeCachedPlantFrame must draw all three head layers for SEED_THREE_GATLING_PEA (card / almanac / cursor art).'
Assert-Source (P 'src/Lawn/Plant.cpp') 'PlantFlowerPotHeightOffset[\s\S]{0,4000}case SeedType::SEED_THREE_GATLING_PEA:' 'PlantFlowerPotHeightOffset must treat SEED_THREE_GATLING_PEA like the other shooters.'

# --- 普攻：每轮向每一行各 4 连发（复用机枪射手的 18/35/51/68 计时） ---
$volleyBody = [regex]::Match($plantCpp, 'void Plant::FireThreeGatlingVolley\([\s\S]*?\n\}')
if (-not $volleyBody.Success) { throw 'Could not find Plant::FireThreeGatlingVolley body.' }
if ($volleyBody.Value -notmatch 'RowCanHaveZombies\(aRow\)') { throw 'FireThreeGatlingVolley must fire into every row that can have zombies (the Threepeater row rule).' }
if ($volleyBody.Value -notmatch 'THREE_GATLING_BULLETS_PER_ROW') { throw 'The ultimate must fire THREE_GATLING_BULLETS_PER_ROW bullets at every row per volley.' }
if ($volleyBody.Value -notmatch 'mProjectiles\.mSize \+ aVolleySlots > THREE_GATLING_PROJECTILE_POOL_GUARD') { throw 'FireThreeGatlingVolley must reserve the whole volley worth of projectile slots before firing: a full Board::mProjectiles (4096) does NOT assert in Release, it writes out of bounds and crashes.' }
if ($volleyBody.Value -notmatch 'RandRangeInt\(-THREE_GATLING_HEIGHT_JITTER, THREE_GATLING_HEIGHT_JITTER\)') { throw 'The ultimate height float must be a random +/-THREE_GATLING_HEIGHT_JITTER offset per bullet.' }
if ($volleyBody.Value -notmatch 'Fire\(nullptr, aRow, PlantWeapon::WEAPON_PRIMARY, aYOffset\)') { throw 'FireThreeGatlingVolley must pass the height offset through Plant::Fire''s theYOffset parameter.' }
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_THREE_GATLING_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*// 三线机枪射手普攻[\s\S]{0,900}mShootingCounter == 18 \|\| mShootingCounter == 35 \|\| mShootingCounter == 51 \|\| mShootingCounter == 68[\s\S]{0,200}FireThreeGatlingVolley\(false\)' 'UpdateShooting must give SEED_THREE_GATLING_PEA the Gatling Pea 4-shot cadence (18/35/51/68) firing a full-row volley each time.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'void Plant::LaunchThreeGatling\(\)[\s\S]{0,1600}mShootingCounter = 100' 'LaunchThreeGatling must hand the plant the 100-frame Gatling Pea volley cycle.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_THREE_GATLING_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*// 三线机枪射手：每行 4 连发[\s\S]{0,200}LaunchThreeGatling\(\)' 'UpdateShooter must route SEED_THREE_GATLING_PEA to LaunchThreeGatling.'

# --- 大招：每行每 0.02 秒两发、持续 3 秒（按时间开火，与 4 连发计数解耦） ---
$shootingBody = [regex]::Match($plantCpp, 'void Plant::UpdateShooting\(\)[\s\S]*?\n\}\r?\n')
if (-not $shootingBody.Success) { throw 'Could not find Plant::UpdateShooting body.' }
if ($shootingBody.Value -notmatch 'SEED_THREE_GATLING_PEA && mGatlingScatterCountdown > 0') { throw 'UpdateShooting must drive the SEED_THREE_GATLING_PEA ultimate from the scatter countdown.' }
if ($shootingBody.Value -notmatch 'aUltimateAge = THREE_GATLING_ULTIMATE_TICKS - mGatlingScatterCountdown') { throw 'The ultimate age must be measured from the 300-frame scatter countdown (3 s window).' }
if ($shootingBody.Value -notmatch 'aUltimateAge % THREE_GATLING_ULTIMATE_INTERVAL == 0') { throw 'The ultimate must fire one volley every THREE_GATLING_ULTIMATE_INTERVAL frames (0.02 s).' }
if ($shootingBody.Value -notmatch 'FireThreeGatlingVolley\(true\)') { throw 'The ultimate must fire the per-row volley WITH the height float (FireThreeGatlingVolley(true)).' }
if ($shootingBody.Value -notmatch 'PlayThreeGatlingShootAnim\(true\)') { throw 'The ultimate must switch the three heads to the looping shoot animation.' }
# 大招分支必须在 `mShootingCounter == 0` 早退之前，否则大招会被 4 连发的空闲期掐断
$ultimateIndex = $shootingBody.Value.IndexOf('SEED_THREE_GATLING_PEA && mGatlingScatterCountdown > 0')
$earlyReturnIndex = $shootingBody.Value.IndexOf('if (mShootingCounter == 0)')
if ($ultimateIndex -lt 0 -or $earlyReturnIndex -lt 0 -or $ultimateIndex -gt $earlyReturnIndex) { throw 'The ultimate branch must sit BEFORE the "mShootingCounter == 0" early return, otherwise the ultimate stops firing between volleys.' }
# 大招结束要把三个头收回 idle：尾部收招分支必须同时认这两个种子
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_THREEPEATER \|\| mSeedType == SeedType::SEED_THREE_GATLING_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*Reanimation\* aHeadReanim2 = mApp->ReanimationGet\(mHeadReanimID2\)' 'UpdateShooting must restore the three heads to idle for SEED_THREE_GATLING_PEA too.'

# --- Fire：普通豌豆 / 出膛点带高度浮动 / 跨行弹道 / 大招子弹 200 伤害 / 音效不叠 ---
$fireBody = [regex]::Match($plantCpp, 'void Plant::Fire\([\s\S]*?\n\}\r?\n\r?\nZombie\* Plant::FindTargetZombie')
if (-not $fireBody.Success) { throw 'Could not find Plant::Fire body.' }
if ($fireBody.Value -notmatch 'case SeedType::SEED_THREEPEATER:\s*\r?\n\s*case SeedType::SEED_THREE_GATLING_PEA:[\s\S]{0,200}PROJECTILE_PEA') { throw 'Plant::Fire must map SEED_THREE_GATLING_PEA to plain PROJECTILE_PEA (right next to the Threepeater case).' }
if ($fireBody.Value -match 'mSeedType == SeedType::SEED_THREE_GATLING_PEA[\s\S]{0,120}Rand\(100\) < 3') { throw 'SEED_THREE_GATLING_PEA must NOT roll the Gatling Pea 3% electric pea.' }
if ($fireBody.Value -notmatch 'SEED_THREEPEATER \|\| mSeedType == SeedType::SEED_THREE_GATLING_PEA\)\s*\r?\n\s*\{[\s\S]{0,700}aOriginY = mY \+ 10 \+ theYOffset;') { throw 'The Threepeater muzzle branch must include SEED_THREE_GATLING_PEA and add theYOffset (the ultimate height float).' }
if ($fireBody.Value -notmatch 'SEED_THREEPEATER \|\| mSeedType == SeedType::SEED_THREE_GATLING_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*if \(theRow != mRow\)[\s\S]{0,900}MOTION_THREEPEATER') { throw 'SEED_THREE_GATLING_PEA must reuse the Threepeater cross-row ballistics (MOTION_THREEPEATER + lane delta).' }
if ($fireBody.Value -notmatch 'SEED_THREE_GATLING_PEA && mGatlingScatterCountdown > 0\)\s*\r?\n\s*\{[\s\S]{0,1200}mDamageOverride = 200;') { throw 'The SEED_THREE_GATLING_PEA ultimate bullet must be a straight pea with the Gatling scatter damage override (200) - no random angle, no scatter fan.' }
# 反过来：原版随机角度散射分支（机枪/电能/寒冰/火焰）的条件里**不能**出现本种子
$scatterHeader = [regex]::Match($fireBody.Value, 'else if \(\(mSeedType == SeedType::SEED_GATLINGPEA[\s\S]{0,400}?mGatlingScatterCountdown > 0\)')
if (-not $scatterHeader.Success) { throw 'Could not find the random-angle scatter branch of Plant::Fire.' }
if ($scatterHeader.Value -match 'SEED_THREE_GATLING_PEA') { throw 'SEED_THREE_GATLING_PEA must NOT be routed into the random-angle scatter branch.' }
if ($fireBody.Value -notmatch 'mSeedType != SeedType::SEED_THREE_GATLING_PEA \|\| theRow == mRow') { throw 'The throw foley must be gated to the plant''s own row: otherwise one full-row volley plays 6 overlapping samples (24 per cycle).' }
if ($fireBody.Value -notmatch 'SEED_FIRE_GATLING_PEA \|\|\s*\r?\n?\s*mSeedType == SeedType::SEED_THREE_GATLING_PEA\)\)') { throw 'The lone-wolf damage override list must include SEED_THREE_GATLING_PEA (same as the rest of the Gatling family).' }

# --- 合成：消耗三线射手卡，落地返还 325（只有真扣过款才返） ---
Assert-Source (P 'src/Lawn/Board.cpp') 'aPlantingSeedType == SeedType::SEED_THREEPEATER && aNormalPlant->mSeedType == SeedType::SEED_GATLINGPEA\)[\s\S]{0,600}aPlantSeedType = SeedType::SEED_THREE_GATLING_PEA' 'Board::MouseDownWithPlant must rewrite the planted seed to SEED_THREE_GATLING_PEA when a Threepeater card lands on a Gatling Pea.'
Assert-Source (P 'src/Lawn/Board.cpp') 'aPlantingSeedType == SeedType::SEED_THREEPEATER && aNormalPlant->mSeedType == SeedType::SEED_GATLINGPEA\)[\s\S]{0,900}aGatlingSynthesisRefund = THREE_GATLING_SYNTHESIS_REFUND' 'The Threepeater synthesis must schedule the 325 sun refund.'
Assert-Source (P 'src/Lawn/Board.cpp') 'aIsGatlingSynthesis && aPaysWithSun[\s\S]{0,600}AddSunMoney\(aGatlingSynthesisRefund\)' 'The synthesis refund must be paid from aGatlingSynthesisRefund after the card was charged.'
Assert-Source (P 'src/Lawn/Board.cpp') 'const bool aPaysWithSun = [\s\S]{0,200}CURSOR_TYPE_PLANT_FROM_BANK[\s\S]{0,120}!HasConveyorBeltSeedBank\(\)' 'The refund must stay gated on "the card actually cost sun" (conveyor belts / free planting must not hand out free sun).'

# --- HP 悬浮提示 / 沙盒 / 图鉴 / 卡面比例 ---
Assert-Source (P 'src/Lawn/Board.cpp') 'SEED_FIRE_GATLING_PEA \|\|\s*\r?\n\s*aPlant->mSeedType == SeedType::SEED_THREE_GATLING_PEA' 'The HP tooltip must report the ultimate state for SEED_THREE_GATLING_PEA too.'
Assert-Source (P 'src/Lawn/Board.cpp') 'gIceSandboxTravelSeeds\[\][\s\S]{0,700}SEED_THREE_GATLING_PEA' 'SEED_THREE_GATLING_PEA must be available on the ice sandbox travel page.'
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.cpp') 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]{0,800}SEED_THREE_GATLING_PEA' 'SEED_THREE_GATLING_PEA must show up on almanac plant page 2.'
# 槽位数不写死：每加一只植物都要 +1，写死会让"新增植物"的提交顺手改坏这条断言。
# 这里改成"必须与 gAlmanacExtraSeeds 的实际条目数一致"。
$almanacCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Lawn/Widget/AlmanacDialog.cpp')
$extraSeeds = [regex]::Match($almanacCpp, 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]*?\n\t\};')
if (-not $extraSeeds.Success) { throw 'Could not find the almanac extra-plant list.' }
$extraSeedCount = ([regex]::Matches($extraSeeds.Value, 'SeedType::SEED_')).Count
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.h') "#define NUM_ALMANAC_EXTRA_SEEDS $extraSeedCount" "NUM_ALMANAC_EXTRA_SEEDS must match the number of entries in gAlmanacExtraSeeds ($extraSeedCount)."
Assert-Source (P 'src/Lawn/SeedPacket.cpp') 'case SeedType::SEED_THREEPEATER:\s*\r?\n\s*case SeedType::SEED_THREE_GATLING_PEA:' 'DrawSeedPacket must scale the SEED_THREE_GATLING_PEA card art like the Threepeater (three heads).'
Assert-Source (P 'src/Lawn/Board.cpp') 'SEED_THREEPEATER \|\|\s*\r?\n\s*theSeedType == SeedType::SEED_THREE_GATLING_PEA' 'NotifyPlanting must mark SEED_THREE_GATLING_PEA as a pea shooter (it fires peas).'

# --- 翻译文案：卡名 + 悬浮说明 + 图鉴描述（正式文件与中文备份各一份） ---
foreach ($file in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $file) '<String id="THREE_GATLING_PEA">' "$file must define the THREE_GATLING_PEA name string."
    Assert-Source (P $file) '<String id="THREE_GATLING_PEA_TOOLTIP">' "$file must define the THREE_GATLING_PEA_TOOLTIP string."
    Assert-Source (P $file) '<String id="THREE_GATLING_PEA_DESCRIPTION">' "$file must define the THREE_GATLING_PEA_DESCRIPTION almanac entry."
}

# --- 美术资源：两张玩家贴图 + 独立 reanim 文件 ---
foreach ($png in @('ThreeGaling_helmet.png', 'ThreeGaling_mouth_overlay.png')) {
    if (-not (Test-Path -LiteralPath (P "res/main/reanim/$png"))) { throw "res/main/reanim/$png is missing - the plant reanim would fail to load." }
}
if (-not (Test-Path -LiteralPath (P 'res/main/reanim/ThreeGaling.reanim'))) { throw 'res/main/reanim/ThreeGaling.reanim is missing - run scripts/gen-three-galing-reanim.py.' }

$reanimXml = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'res/main/reanim/ThreeGaling.reanim')
# 原版嘴**保持不动**：它在机枪射手那边对应 GatlingPea_mouth（底层嘴洞），三条嘴轨道各一次
if (([regex]::Matches($reanimXml, 'IMAGE_REANIM_THREEPEATER_MOUTH')).Count -ne 3) { throw 'ThreeGaling.reanim must keep the vanilla IMAGE_REANIM_THREEPEATER_MOUTH on the three mouth tracks (it plays the GatlingPea_mouth role).' }
# 玩家给的唇形是 mouth_overlay（对应 GatlingPea_mouth_overlay），三个头各一层
if (([regex]::Matches($reanimXml, 'IMAGE_REANIM_THREEGALING_MOUTH_OVERLAY')).Count -ne 3) { throw 'ThreeGaling.reanim must reference IMAGE_REANIM_THREEGALING_MOUTH_OVERLAY exactly 3 times (one mouth overlay per head).' }
# 三个头各一条头盔轨道
foreach ($track in @('ThreeGaling_helmet1', 'ThreeGaling_helmet2', 'ThreeGaling_helmet3')) {
    if ($reanimXml -notmatch "<name>$track</name>") { throw "ThreeGaling.reanim must define the helmet track $track." }
}
if (([regex]::Matches($reanimXml, 'IMAGE_REANIM_THREEGALING_HELMET')).Count -ne 3) { throw 'ThreeGaling.reanim must reference IMAGE_REANIM_THREEGALING_HELMET exactly 3 times (one helmet per head).' }
# 三个头各一根四段式机枪枪管（段号顺序与 GatlingPea.reanim 一致：3、4、2、1）
foreach ($head in @('1', '2', '3')) {
    foreach ($segment in @('3', '4', '2', '1')) {
        $track = "ThreeGaling_head${head}_barrel$segment"
        if ($reanimXml -notmatch "<name>$track</name>") { throw "ThreeGaling.reanim must define the barrel track $track (head $head, segment $segment)." }
    }
}
if (([regex]::Matches($reanimXml, 'IMAGE_REANIM_GATLINGPEA_BARREL')).Count -ne 12) { throw 'ThreeGaling.reanim must reference IMAGE_REANIM_GATLINGPEA_BARREL exactly 12 times (4 segments x 3 heads).' }
# 轨道顺序即绘制顺序，必须与机枪射手一致：嘴（原版轨道，在前）→ 枪管 → 嘴覆层 → 头盔
foreach ($head in @('1', '2', '3')) {
    $barrel = $reanimXml.IndexOf("<name>ThreeGaling_head${head}_barrel1</name>")
    $overlay = $reanimXml.IndexOf("<name>ThreeGaling_mouth_overlay$head</name>")
    $helmet = $reanimXml.IndexOf("<name>ThreeGaling_helmet$head</name>")
    if ($barrel -lt 0 -or $overlay -lt 0 -or $helmet -lt 0) { throw "ThreeGaling.reanim is missing one of head $head's barrel/overlay/helmet tracks." }
    if (-not ($barrel -lt $overlay -and $overlay -lt $helmet)) { throw "Head $head must draw barrel -> mouth overlay -> helmet, in that order (same layering as the Gatling Pea: mouth, barrels, mouth_overlay, helmet)." }
    if (-not ($reanimXml.IndexOf("<name>ThreePeater_mouth$head</name>") -lt $barrel)) { throw "Head $head's vanilla mouth must stay BEFORE the barrels (the gun comes out of the mouth)." }
}

# 生成器自检：头盔轨道的帧区间必须与对应脸轨道完全一致、且不与眨眼窗口重叠，
# 并确认提交的文件与当前生成结果逐字节一致（改了 ThreePeater.reanim 就得重跑）。
$python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $python) { throw 'python is required to verify the generated reanim (scripts/gen-three-galing-reanim.py --check).' }
& $python.Source (P 'scripts/gen-three-galing-reanim.py') --check | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'scripts/gen-three-galing-reanim.py --check failed: ThreeGaling.reanim is out of date or its invariants broke.' }

# --- 编译缓存不能被盲目信任（这是本植物踩过的坑，必须留住这个修复） ---
# DefinitionCompileAndLoad 以前在 Release 下写成 `!aRequireCompiledUpToDate || DefinitionIsCompiled(...)`，
# 等于**无条件**先读 <AppData>/cache64/compiled/<path>.compiled：模组第一次运行编译 XML 时会把这个缓存写下来，
# 之后无论怎么改 XML / 重打 pak / 重启游戏，Release 版都会一直加载旧定义（表现为"贴图改了但游戏里没变"）。
# 现在一律 `DefinitionIsCompiled(...)`：pak 里随包带了预编译版本的原版定义行为不变，
# 模组新增、没有预编译版本的定义则每次按 XML 现编 —— 改了就一定生效。
Assert-Source (P 'src/Sexy.TodLib/Definition.cpp') 'const bool aShouldTryCompiled = DefinitionIsCompiled\(theXMLFilePath\);' 'DefinitionCompileAndLoad must validate the compiled cache in Release too: otherwise a stale <AppData>/cache64 compiled definition shadows any reanim/Particle XML change forever.'
$definitionCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/Sexy.TodLib/Definition.cpp')
if ($definitionCpp -match 'aRequireCompiledUpToDate') { throw 'The old "trust the cache blindly in Release" switch (aRequireCompiledUpToDate) must not come back: it silently shadows reanim XML edits with a stale user cache.' }

# --- 数值自检：±15px 的高度浮动必须留在弹丸影子间距的 (28, 90) 判定窗口里 ---
# 弹丸出生时影子按目标行摆好（草坪间距 57、屋顶/泳池 45），本行直飞的弹丸影子间距全程不变，
# 跨行弹丸有 `mShadowY -= aLaneDeltaY` 校正 —— 所以间距恒为"本行间距 - 浮动"。
# 一旦掉出 (28, 90)：> 90 会让 Projectile::CheckForCollision 整帧跳过碰撞（弹丸穿过僵尸飞出屏幕），
# < 28 会让 CheckForHighGround 当场把弹丸消散 —— 两者都表现为"子弹莫名其妙消失"。
# （基底间距的取值与 scripts/check-threepeater-lanes.ps1 一致：行距 100 → 57，行距 85 → 45。）
$constants = Get-Content -Raw -Encoding UTF8 -LiteralPath (P 'src/GameConstants.h')
$jitterMatch = [regex]::Match($constants, 'THREE_GATLING_HEIGHT_JITTER\s*=\s*(\d+)')
$intervalMatch = [regex]::Match($constants, 'THREE_GATLING_ULTIMATE_INTERVAL\s*=\s*(\d+)')
$ticksMatch = [regex]::Match($constants, 'THREE_GATLING_ULTIMATE_TICKS\s*=\s*(\d+)')
$perRowMatch = [regex]::Match($constants, 'THREE_GATLING_BULLETS_PER_ROW\s*=\s*(\d+)')
$guardMatch = [regex]::Match($constants, 'THREE_GATLING_PROJECTILE_POOL_GUARD\s*=\s*(\d+)')
if (-not $jitterMatch.Success -or -not $intervalMatch.Success -or -not $ticksMatch.Success -or
    -not $perRowMatch.Success -or -not $guardMatch.Success) { throw 'Could not parse the THREE_GATLING_* constants.' }
$jitter = [int]$jitterMatch.Groups[1].Value
$interval = [int]$intervalMatch.Groups[1].Value
$ticks = [int]$ticksMatch.Groups[1].Value
$perRow = [int]$perRowMatch.Groups[1].Value
$poolGuard = [int]$guardMatch.Groups[1].Value

$roofShadowBase = 45.0   # 最紧的一档：45 - 28 = 17 是浮动的上限
foreach ($shadowBase in @(57.0, $roofShadowBase)) {
    foreach ($offset in -$jitter..$jitter) {
        $delta = $shadowBase - $offset
        if ($delta -le 28.0 -or $delta -ge 90.0) { throw "A $offset px height float pushes the projectile shadow spacing to $delta (base $shadowBase), outside the (28, 90) window: the pea would vanish or stop colliding." }
    }
}
if ($jitter -ge ($roofShadowBase - 28.0)) { throw "THREE_GATLING_HEIGHT_JITTER=$jitter is too large: on a small-row board (shadow base 45) the (28, 90) window only tolerates ±16." }
# 每 0.03 秒一波、每波每行 1 发、持续 3 秒 → 每行 100 发。
# 计时口径：开大那一帧散射计数被置为 300（UpdateShooter 跑在 UpdateShooting 之后），
# 下一帧 UpdateShooting 看到 age = 300-300 = 0 → 第一波；计数在 UpdateAbilities 里递减，
# 等它归零时 `mGatlingScatterCountdown > 0` 已经不成立 —— 所以能开火的 age 是 0,3,6,...,297。
if ($ticks % $interval -ne 0) { throw 'THREE_GATLING_ULTIMATE_TICKS must be a whole multiple of THREE_GATLING_ULTIMATE_INTERVAL.' }
$expectedVolleys = [Math]::Floor(($ticks - 1) / $interval) + 1
$expectedBullets = $expectedVolleys * $perRow
if ($expectedVolleys -ne 100 -or $expectedBullets -ne 100) { throw "The ultimate must land 100 volleys x $perRow bullets = 100 bullets per row (3 s / 0.03 s), got $expectedVolleys volleys / $expectedBullets bullets." }

# --- 数值自检：大招的弹幕量必须留在"弹丸池 + 性能"的预算内 ---
# 弹丸存活约 200 帧（速度 3.33 px/帧、射程约 700 px），所以
#     峰值同时在场 ≈ (满场 6 行 × 每行 $perRow 发 / 每 $interval 帧) × 200 帧。
# 机枪射手散射是 1 发/帧（≈200 颗/株），引擎本来就扛得住；这里要求同量级（≤2 发/帧）。
# 曾经试过 0.02 秒两发（6 发/帧 ≈ 1200 颗/株）：几株齐开就把 Board::mProjectiles（上限 4096）
# 顶满 —— 而 Release 下池满不会报错，只会让 DataArrayAlloc 越界写内存 → 卡死闪退。
$projectilePoolCap = 4096        # Board::mProjectiles（DataArrayInitialize）
$rowsPerBoard = 6                # MAX_GRID_SIZE_Y
$lifetimeFrames = 200            # 弹丸穿过整块草坪所需帧数（估算）
$bulletsPerVolley = $rowsPerBoard * $perRow
$bulletsPerFrame = [Math]::Round($bulletsPerVolley / $interval, 3)
$peakAlivePerPlant = $bulletsPerFrame * $lifetimeFrames
if ($bulletsPerFrame -gt 2.0) { throw "The ultimate fires $bulletsPerFrame bullets per frame (per plant): that is denser than the Gatling Pea scatter (1/frame) and already froze the game once. Lower THREE_GATLING_BULLETS_PER_ROW or raise THREE_GATLING_ULTIMATE_INTERVAL." }
if ($peakAlivePerPlant -gt 512) { throw "Peak concurrent ultimate projectiles is about $peakAlivePerPlant per plant - over the 512 budget (the Gatling Pea scatter peaks near 200)." }
if ($bulletsPerVolley * 1 -ge $projectilePoolCap) { throw "One ultimate volley fires $bulletsPerVolley bullets - that alone risks the $projectilePoolCap-slot projectile pool." }
if ($poolGuard -ge $projectilePoolCap) { throw "THREE_GATLING_PROJECTILE_POOL_GUARD=$poolGuard must stay below the $projectilePoolCap-slot projectile pool." }
if ($poolGuard -le $projectilePoolCap / 2) { throw "THREE_GATLING_PROJECTILE_POOL_GUARD=$poolGuard is too conservative: it would cut the ultimate off during normal play." }
# 闸门要按"这一波还要占几个槽位"判断（整波要么都发、要么都不发），不能只看当前占用
Assert-Source (P 'src/Lawn/Plant.cpp') 'aVolleySlots = aBulletsPerRow \* MAX_GRID_SIZE_Y[\s\S]{0,200}mProjectiles\.mSize \+ aVolleySlots > THREE_GATLING_PROJECTILE_POOL_GUARD' 'FireThreeGatlingVolley must reserve the whole volley worth of pool slots before firing (a full pool writes out of bounds in Release instead of asserting).'

Write-Output 'Three Gatling Pea (SEED_THREE_GATLING_PEA) source checks passed.'
