$ErrorActionPreference = 'Stop'

# 激光豌豆（SEED_LASER_PEA）源级检查：
#   旅行专属红卡 / 400 阳光 / 普通短冷却 750 / 每 0.2 秒一发 / 绿色贯穿激光每只僵尸 20 伤害
#   / 优先索敌空中僵尸 / 外观 = 机枪射手但只画一根枪管、一张嘴。
# 说明：本仓库无自动化测试框架（见 AGENTS.md），与其它 check-*.ps1 一样做源级断言；
#      真正的手感（光束落点、颜色、0.2 秒节奏、气球僵尸是否被优先选中）仍需手动进游戏验证。
#
# 注意：函数的第一个参数**不能**叫 $Path —— 它会遮蔽 PowerShell 的自动变量 $env:Path，
#      导致 Get-Content 读到空内容（这个坑在 Windows PowerShell 5.1 上会静默生效）。

$repoRoot = Split-Path -Parent $PSScriptRoot
function RepoPath([string]$relative) { Join-Path $repoRoot $relative }

# 必须显式按 UTF-8 读：这些文件里有大量中文注释，Windows PowerShell 的默认编码
# （ANSI）会把中文拆成多个字节字符，把"两处代码之间最多 N 个字符"的间隔断言撑爆。
function Read-Repo([string]$relative) {
    return Get-Content -Raw -Encoding UTF8 -LiteralPath (RepoPath $relative)
}

function Assert-Source([string]$relative, [string]$pattern, [string]$message) {
    $text = Read-Repo $relative
    if ($text -notmatch $pattern) {
        throw $message
    }
}

# --- 枚举：新值前插在 NUM_* 之前，既有存档枚举值不变 ---
Assert-Source 'src/ConstEnums.h' 'SEED_THREE_GATLING_PEA,[\s\S]{0,3000}SEED_LASER_PEA,[\s\S]{0,1500}NUM_SEED_TYPES' 'SEED_LASER_PEA must be declared after SEED_THREE_GATLING_PEA and before NUM_SEED_TYPES.'
Assert-Source 'src/ConstEnums.h' 'REANIM_THREE_GATLINGPEA,[\s\S]{0,4000}REANIM_LASER_PEA,[\s\S]{0,1500}NUM_REANIMS' 'REANIM_LASER_PEA must be declared after REANIM_THREE_GATLINGPEA and before NUM_REANIMS.'
Assert-Source 'src/ConstEnums.h' 'PROJECTILE_PURPLE_FIRE_PEA = 16,[\s\S]{0,400}PROJECTILE_LASER_PEA = 17,[\s\S]{0,400}NUM_PROJECTILES' 'PROJECTILE_LASER_PEA must be appended after PROJECTILE_PURPLE_FIRE_PEA and before NUM_PROJECTILES (later mod projectiles appended after it are fine).'

# --- 常量：0.2 秒（20 帧）/ 20 伤害 / 光束寿命 / 绿色配色 ---
Assert-Source 'src/GameConstants.h' 'LASER_PEA_LAUNCH_RATE\s*=\s*20' 'LASER_PEA_LAUNCH_RATE must be 20 logic frames (0.2 s at 100 fps).'
Assert-Source 'src/GameConstants.h' 'LASER_PEA_DAMAGE\s*=\s*20' 'LASER_PEA_DAMAGE must be 20.'
Assert-Source 'src/GameConstants.h' 'LASER_PEA_BEAM_TICKS\s*=\s*6' 'LASER_PEA_BEAM_TICKS must define how long the beam stays visible.'
Assert-Source 'src/GameConstants.h' 'LASER_PEA_BEAM_CORE_G\s*=\s*255' 'The beam core must be green-dominant (G = 255).'
# 内芯是接近白的亮绿：R/B 可以高（要够亮），但必须 G 最高，否则就不是"绿色激光"了
$constCpp = Read-Repo 'src/GameConstants.h'
$coreR = [int]([regex]::Match($constCpp, 'LASER_PEA_BEAM_CORE_R\s*=\s*(\d+)').Groups[1].Value)
$coreG = [int]([regex]::Match($constCpp, 'LASER_PEA_BEAM_CORE_G\s*=\s*(\d+)').Groups[1].Value)
$coreB = [int]([regex]::Match($constCpp, 'LASER_PEA_BEAM_CORE_B\s*=\s*(\d+)').Groups[1].Value)
if ($coreG -le $coreR -or $coreG -le $coreB) { throw "The beam core must stay green (G above R and B), got R=$coreR G=$coreG B=$coreB." }
Assert-Source 'src/GameConstants.h' 'LASER_PEA_MUZZLE_MARGIN' 'LASER_PEA_MUZZLE_MARGIN must exist (the beam must not hit zombies behind the muzzle).'
Assert-Source 'src/GameConstants.h' 'LASER_PEA_BEAM_RANGE\s*=\s*1200\.0f' 'LASER_PEA_BEAM_RANGE must be a large world-pixel range so the angled beam crosses the whole board.'

# --- reanim 槽位：与机枪射手同一个文件、独立槽位、NO_ATLAS ---
Assert-Source 'src/Sexy.TodLib/Reanimator.cpp' 'REANIM_LASER_PEA,\s*"reanim/GatlingPea\.reanim",\s*1 << ReanimFlags::REANIM_NO_ATLAS' 'REANIM_LASER_PEA must reuse reanim/GatlingPea.reanim with REANIM_NO_ATLAS.'

# --- 植物定义：400 阳光 / 普通冷却 750 / 射手子类 / 0.2 秒节奏 ---
Assert-Source 'src/Lawn/Plant.cpp' 'SeedType::SEED_LASER_PEA,\s*nullptr,\s*ReanimationType::REANIM_GATLINGPEA,\s*5,\s*400,\s*750,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*LASER_PEA_LAUNCH_RATE,\s*"LASER_PEA"' 'gPlantDefs row must default to REANIM_GATLINGPEA / 400 sun / 750 refresh / SHOOTER / LASER_PEA_LAUNCH_RATE / "LASER_PEA".'

# --- 红卡 + 可直接种下（不进 IsUpgrade 升级卡列表） ---
Assert-Source 'src/Lawn/Plant.cpp' 'bool Plant::IsRedCard[\s\S]{0,1400}SEED_LASER_PEA' 'Plant::IsRedCard must include SEED_LASER_PEA.'
$plantCpp = Read-Repo 'src/Lawn/Plant.cpp'
$upgradeBody = [regex]::Match($plantCpp, 'bool Plant::IsUpgrade[\s\S]*?\n\}')
if (-not $upgradeBody.Success) { throw 'Could not find Plant::IsUpgrade body.' }
if ($upgradeBody.Value -match 'SEED_LASER_PEA') { throw 'SEED_LASER_PEA must not be an upgrade card - it is planted directly.' }

# --- 只画一根枪管、一张嘴：隐藏机枪射手多出来的三条枪管与枪口叠加层 ---
Assert-Source 'src/Lawn/Plant.cpp' 'LASER_PEA_HIDDEN_TRACKS\[\][\s\S]{0,500}GatlingPea_barrel2[\s\S]{0,200}GatlingPea_barrel3[\s\S]{0,200}GatlingPea_barrel4[\s\S]{0,200}GatlingPea_mouth_overlay' 'Laser pea must hide GatlingPea_barrel2/3/4 and GatlingPea_mouth_overlay (one barrel, one mouth).'
Assert-Source 'src/Lawn/Plant.cpp' 'void LaserPeaHideExtraTracks\(Reanimation\* theReanim\)' 'LaserPeaHideExtraTracks must be defined.'
Assert-Source 'src/Lawn/Plant.cpp' 'SEED_LASER_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*LaserPeaHideExtraTracks\(aBodyReanim\);\s*\r?\n\s*LaserPeaHideExtraTracks\(aHeadReanim\);' 'PlantInitialize must hide the extra tracks on both the body and head reanimations.'
Assert-Source 'src/Lawn/System/ReanimationLawn.cpp' 'SEED_LASER_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*LaserPeaHasCustomArt\(\);' 'The cached plant frame (seed packet / almanac / cursor) must apply the laser barrel art first.'
Assert-Source 'src/Lawn/System/ReanimationLawn.cpp' 'SEED_LASER_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*LaserPeaHideExtraTracks\(&aReanim\);' 'The cached plant frame (seed packet / almanac / cursor) must hide the extra tracks too.'

# 换图表只列枪管一张（其余部位沿用机枪射手）；尺寸取自离线缩放的 LaserPea_barrel_small.png
Assert-Source 'src/Lawn/Plant.cpp' '"reanim/GATLINGPEA_BARREL",\s*"reanim/LASERPEA_BARREL_SMALL"' 'The laser pea art swap must replace exactly the barrel part.'
$swapBlock = [regex]::Match($plantCpp, 'bool LaserPeaHasCustomArt\(\)[\s\S]*?\n\}')
if (-not $swapBlock.Success) { throw 'Could not find LaserPeaHasCustomArt body.' }
$swapCount = ([regex]::Matches($swapBlock.Value, '\{\s*"reanim/')).Count
if ($swapCount -ne 1) { throw "LaserPeaHasCustomArt must swap exactly one image (the barrel), found $swapCount." }
if ($swapBlock.Value -match 'GATLINGPEA_HEAD|GATLINGPEA_MOUTH|GATLINGPEA_HELMET|GATLINGPEA_BLINK') { throw 'Head / mouth / helmet / blink must keep the plain Gatling Pea art.' }
Assert-Source 'src/Lawn/Plant.cpp' 'ReanimationType LaserPeaReanimType\(\)[\s\S]{0,300}REANIM_GATLINGPEA' 'LaserPeaReanimType() must fall back to REANIM_GATLINGPEA when the custom art is unavailable.'

# --- 0.2 秒节奏：UpdateShooter 里激光豌豆不掺 Rand(15) ---
Assert-Source 'src/Lawn/Plant.cpp' 'mLaunchCounter = \(mSeedType == SeedType::SEED_LASER_PEA\)\s*\r?\n\s*\?\s*mLaunchRate\s*\r?\n\s*:\s*mLaunchRate - Sexy::Rand\(15\);' 'UpdateShooter must give SEED_LASER_PEA an exact mLaunchRate interval (no Rand(15) jitter).'

# --- 出弹：由 mLaunchCounter 触发（0.2 秒节奏），且必须把**目标僵尸**传进 Fire ---
$shootingBody = [regex]::Match($plantCpp, 'void Plant::UpdateShooting\(\)[\s\S]*?\n\}')
if (-not $shootingBody.Success) { throw 'Could not find Plant::UpdateShooting body.' }
if ($shootingBody.Value -notmatch 'SEED_LASER_PEA\)\s*\r?\n\s*\{[\s\S]{0,700}mLaunchCounter == 1[\s\S]{0,300}FireLaserPea\(\);') { throw 'UpdateShooting must fire the laser for SEED_LASER_PEA when mLaunchCounter == 1 (the 0.2 s cadence).' }
# ⚠ 回归点：这里一旦写回 Fire(nullptr, ...)，光束就会因为拿不到目标而永远水平向右（"能锁定别的行、但光不斜"）
if ($shootingBody.Value -match 'SEED_LASER_PEA\)[\s\S]{0,700}Fire\(nullptr, mRow') { throw 'UpdateShooting must NOT call Fire(nullptr, ...) for SEED_LASER_PEA: without the target the beam direction falls back to horizontal and never angles.' }
if ($shootingBody.Value -match 'SEED_LASER_PEA \|\| mSeedType == SeedType::SEED_SNOW_GATLING_PEA|mSeedType == SeedType::SEED_SNOW_GATLING_PEA \|\| mSeedType == SeedType::SEED_LASER_PEA') { throw 'SEED_LASER_PEA must NOT be routed into the Gatling Pea 4-shot cycle.' }
# FireLaserPea 必须自己重新索敌并把目标交给 Fire
Assert-Source 'src/Lawn/Plant.cpp' 'void Plant::FireLaserPea\(\)\s*\r?\n\{\s*\r?\n\s*Zombie\* aZombie = FindTargetZombie\(mRow, PlantWeapon::WEAPON_PRIMARY\);\s*\r?\n\s*Fire\(aZombie, mRow, PlantWeapon::WEAPON_PRIMARY\);' 'FireLaserPea must re-acquire the target and pass it to Fire().'
# 0.2 秒的节奏比机枪射手的开火动画（39 帧）短得多：激光豌豆必须**跳过开火动画**，否则头会一直抽搐
Assert-Source 'src/Lawn/Plant.cpp' 'SEED_LASER_PEA\)\s*\r?\n\s*\{[\s\S]{0,900}不播开火动画' 'FindTargetAndFire must skip the shooting animation for SEED_LASER_PEA (its cadence is far shorter than anim_shooting).'

# --- 索敌：不限本行 + 射程内空中僵尸优先 ---
# 行判定必须把 SEED_LASER_PEA 和香蒲一样排除掉（否则它又只能打本行）
Assert-Source 'src/Lawn/Plant.cpp' 'mSeedType != SeedType::SEED_CATTAIL && mSeedType != SeedType::SEED_LASER_PEA' 'FindTargetZombie must exempt SEED_LASER_PEA from the same-row check (it can lock any row).'
Assert-Source 'src/Lawn/Plant.cpp' 'SEED_LASER_PEA\)\s*\r?\n\s*\{[\s\S]{0,800}IsFlying\(\)\)\s*\r?\n\s*\{[\s\S]{0,150}aWeight \+= 100000;' 'FindTargetZombie must give SEED_LASER_PEA air-target priority.'

# --- 弹种 + 出膛点（与机枪射手同一套头/枪管几何）+ 光束方向 ---
if ($plantCpp -notmatch 'case SeedType::SEED_LASER_PEA:[\s\S]{0,400}PROJECTILE_LASER_PEA') { throw 'Plant::Fire switch must map SEED_LASER_PEA to PROJECTILE_LASER_PEA.' }
if ($plantCpp -notmatch 'else if \(mSeedType == SeedType::SEED_LASER_PEA\)\s*\r?\n\s*\{[\s\S]{0,900}GetPeaHeadOffset\(aOffsetX, aOffsetY\);[\s\S]{0,300}aOriginX = mX \+ aOffsetX \+ 34 \+ static_cast<int>\(LASER_PEA_BARREL_OFFSET_X\);') { throw 'Plant::Fire must use the Gatling Pea muzzle offset (+34) plus LASER_PEA_BARREL_OFFSET_X for SEED_LASER_PEA.' }
# 方向向量必须由"枪口 → 目标中心"算出并存进 mVelX/mVelY（弹丸不移动，mVel 只当方向用）
if ($plantCpp -notmatch 'float aDirX = 1\.0f;[\s\S]{0,1500}GetZombieRect\(\)[\s\S]{0,1200}aDirX = aDeltaX / aLength;[\s\S]{0,400}aProjectile->mVelX = aDirX;\s*\r?\n\s*aProjectile->mVelY = aDirY;') { throw 'Plant::Fire must compute the unit direction toward the target and store it in mVelX/mVelY for SEED_LASER_PEA.' }

# --- 弹丸：伤害表 + 不移动 + 只结算一次 + 线段/矩形相交 + 顶层绘制 ---
$projCpp = Read-Repo 'src/Lawn/Projectile.cpp'
if ($projCpp -notmatch 'PROJECTILE_LASER_PEA,\s*0,\s*LASER_PEA_DAMAGE') { throw 'gProjectileDefinition must give PROJECTILE_LASER_PEA LASER_PEA_DAMAGE.' }
if ($projCpp -notmatch 'PROJECTILE_LASER_PEA\)\s*\r?\n\s*\{[\s\S]{0,500}mLaserPeaBeamCountdown = LASER_PEA_BEAM_TICKS;') { throw 'ProjectileInitialize must start the beam countdown for PROJECTILE_LASER_PEA.' }
if ($projCpp -notmatch 'void Projectile::UpdateLaserPeaBeam\(\)[\s\S]{0,1200}if \(mLaserPeaBeamCountdown == LASER_PEA_BEAM_TICKS\)') { throw 'UpdateLaserPeaBeam must use the beam countdown (not mProjectileAge) as its "spawning frame" marker: Projectile::Update increments mProjectileAge before this runs, so an age-based test would never deal any damage.' }
if ($projCpp -notmatch 'void Projectile::UpdateLaserPeaBeam\(\)') { throw 'UpdateLaserPeaBeam must be defined.' }
if ($projCpp -match 'void Projectile::UpdateLaserPeaBeam\(\)[\s\S]{0,1200}if \(mProjectileAge > 0\)') { throw 'UpdateLaserPeaBeam must NOT gate the damage on mProjectileAge (it is always >= 1 by the time it runs).' }
# 不限行：那两处"只有本行/僵王"的过滤必须从光束结算里消失
$beamBody = [regex]::Match($projCpp, 'void Projectile::UpdateLaserPeaBeam\(\)[\s\S]*?\n\}')
if (-not $beamBody.Success) { throw 'Could not find Projectile::UpdateLaserPeaBeam body.' }
if ($beamBody.Value -match 'mRow != mRow|aZombie->mRow != mRow') { throw 'UpdateLaserPeaBeam must not restrict the beam to its own row - the laser locks any row.' }
if ($beamBody.Value -notmatch 'LaserBeamHitsRect\(') { throw 'UpdateLaserPeaBeam must test the beam with a real segment/rect intersection (LaserBeamHitsRect).' }
Assert-Source 'src/Lawn/Projectile.cpp' 'bool LaserBeamHitsRect\(float theFromX[\s\S]{0,2200}return aMinT <= aMaxT;' 'LaserBeamHitsRect must implement the slab test on both axes and return the interval overlap.'
if ($projCpp -notmatch 'PROJECTILE_LASER_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*UpdateLaserPeaBeam\(\);\s*\r?\n\s*return;') { throw 'Projectile::Update must route PROJECTILE_LASER_PEA to UpdateLaserPeaBeam and skip motion/collision.' }
Assert-Source 'src/Lawn/Projectile.cpp' 'Projectile::DrawAllLaserBeams' 'Projectile::DrawAllLaserBeams must exist.'
Assert-Source 'src/Lawn/Board.cpp' 'DrawAllElectricChains\(this, g\);[\s\S]{0,400}DrawAllLaserBeams\(this, g\);' 'Board::Draw must draw the laser beams on the top layer, right after the electric chains.'
if ($projCpp -notmatch 'case ProjectileType::PROJECTILE_LASER_PEA:[\s\S]{0,250}aImage = nullptr;') { throw 'The laser beam must not draw a projectile sprite (drawn on the top layer instead).' }
if ($projCpp -notmatch 'case ProjectileType::PROJECTILE_LASER_PEA:[\s\S]{0,250}return;') { throw 'The laser beam must not draw a ground shadow.' }

# --- 光束粗细（太细看不出来）与枪管前移量（视觉与光束起点必须用同一个数） ---
Assert-Source 'src/GameConstants.h' 'LASER_PEA_BEAM_GLOW_WIDTH\s*=\s*15' 'The beam glow must be thick enough to be visible (LASER_PEA_BEAM_GLOW_WIDTH = 15).'
Assert-Source 'src/GameConstants.h' 'LASER_PEA_BEAM_CORE_WIDTH\s*=\s*7' 'The beam core must be thick enough to be visible (LASER_PEA_BEAM_CORE_WIDTH = 7).'
Assert-Source 'src/GameConstants.h' 'LASER_PEA_BARREL_OFFSET_X\s*=\s*10\.0f' 'LASER_PEA_BARREL_OFFSET_X must be 10 px.'
Assert-Source 'src/Lawn/Projectile.cpp' 'DrawElectricStroke\(g, aFromX, aFromY, aToX, aToY, LASER_PEA_BEAM_GLOW_WIDTH\)' 'The glow layer must use LASER_PEA_BEAM_GLOW_WIDTH.'
Assert-Source 'src/Lawn/Projectile.cpp' 'DrawElectricStroke\(g, aFromX, aFromY, aToX, aToY, LASER_PEA_BEAM_CORE_WIDTH\)' 'The core layer must use LASER_PEA_BEAM_CORE_WIDTH.'
Assert-Source 'src/Lawn/Plant.cpp' 'void LaserPeaShiftBarrel\(Reanimation\* theReanim\)[\s\S]{0,500}mShakeX = LASER_PEA_BARREL_OFFSET_X' 'LaserPeaShiftBarrel must move the barrel through the track instance shake (only that track moves).'
Assert-Source 'src/Lawn/Plant.cpp' 'SEED_LASER_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*LaserPeaHideExtraTracks\(aBodyReanim\);\s*\r?\n\s*LaserPeaHideExtraTracks\(aHeadReanim\);\s*\r?\n\s*LaserPeaShiftBarrel\(aHeadReanim\);' 'PlantInitialize must also shift the barrel on the head instance.'
Assert-Source 'src/Lawn/Plant.cpp' 'aOriginX = mX \+ aOffsetX \+ 34 \+ static_cast<int>\(LASER_PEA_BARREL_OFFSET_X\);' 'Plant::Fire must add LASER_PEA_BARREL_OFFSET_X to the muzzle so the beam starts at the (shifted) barrel.'
Assert-Source 'src/Lawn/System/ReanimationLawn.cpp' 'SEED_LASER_PEA\)\s*\r?\n\s*\{\s*\r?\n\s*LaserPeaHideExtraTracks\(&aReanim\);\s*\r?\n\s*LaserPeaShiftBarrel\(&aReanim\);' 'The cached plant frame (seed packet / almanac / cursor) must shift the barrel too.'

# --- 旅行专属：Travel 表登记 + HasSeedType 仅旅行关可拥有 + 沙盒旅行页 + 图鉴第 2 页 ---
Assert-Source 'src/Lawn/Travel.cpp' 'SEED_LASER_PEA,\s*false' 'SEED_LASER_PEA must be registered in gTravelPlantDefs.'
Assert-Source 'src/LawnApp.cpp' 'case SeedType::SEED_LASER_PEA:[\s\S]{0,80}return IsTravelLevel\(mGameMode\)' 'LawnApp::HasSeedType must gate SEED_LASER_PEA on IsTravelLevel.'
Assert-Source 'src/Lawn/Board.cpp' 'gIceSandboxTravelSeeds\[\][\s\S]{0,1200}SEED_LASER_PEA' 'SEED_LASER_PEA must be available on the ice sandbox travel page.'
Assert-Source 'src/Lawn/Widget/AlmanacDialog.cpp' 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]{0,1400}SEED_LASER_PEA' 'SEED_LASER_PEA must show up on almanac plant page 2.'

# 槽位数不写死：必须与 gAlmanacExtraSeeds 的实际条目数一致
$almanacCpp = Read-Repo 'src/Lawn/Widget/AlmanacDialog.cpp'
$extraSeeds = [regex]::Match($almanacCpp, 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]*?\n\t\};')
if (-not $extraSeeds.Success) { throw 'Could not find the almanac extra-plant list.' }
$extraSeedCount = ([regex]::Matches($extraSeeds.Value, 'SeedType::SEED_')).Count
Assert-Source 'src/Lawn/Widget/AlmanacDialog.h' "#define NUM_ALMANAC_EXTRA_SEEDS $extraSeedCount" "NUM_ALMANAC_EXTRA_SEEDS must match the number of entries in gAlmanacExtraSeeds ($extraSeedCount)."

# --- 文案：两份字符串表都要有名称 / 提示 / 图鉴描述 ---
foreach ($stringsFile in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source $stringsFile '<String id="LASER_PEA">' "$stringsFile must define [LASER_PEA]."
    Assert-Source $stringsFile '<String id="LASER_PEA_TOOLTIP">' "$stringsFile must define [LASER_PEA_TOOLTIP]."
    Assert-Source $stringsFile '<String id="LASER_PEA_DESCRIPTION">' "$stringsFile must define [LASER_PEA_DESCRIPTION]."
}

# --- 资源：离线缩放出来的枪管部件图必须存在，且是 43x27 的带透明通道 PNG ---
$barrelFile = RepoPath 'res/main/reanim/LaserPea_barrel_small.png'
if (-not (Test-Path -LiteralPath $barrelFile)) { throw "Missing $barrelFile - run: python tools/make-laser-pea-barrel.py" }
$barrelBytes = [System.IO.File]::ReadAllBytes($barrelFile)
if ($barrelBytes.Length -lt 33) { throw 'LaserPea_barrel_small.png is truncated.' }
# IHDR 固定从第 16 字节开始：宽 4 字节、高 4 字节、位深 1、颜色类型 1（6 = RGBA = 带透明通道）
$barrelW = [int]$barrelBytes[16] * 16777216 + [int]$barrelBytes[17] * 65536 + [int]$barrelBytes[18] * 256 + [int]$barrelBytes[19]
$barrelH = [int]$barrelBytes[20] * 16777216 + [int]$barrelBytes[21] * 65536 + [int]$barrelBytes[22] * 256 + [int]$barrelBytes[23]
$barrelColorType = [int]$barrelBytes[25]
if ($barrelW -ne 43 -or $barrelH -ne 27) { throw "LaserPea_barrel_small.png must be 43x27 (same as GatlingPea_barrel.png), got ${barrelW}x${barrelH}." }
if ($barrelColorType -ne 6) { throw "LaserPea_barrel_small.png must be RGBA (color type 6) so the art-swap transparency check passes, got color type $barrelColorType." }
if (-not (Test-Path -LiteralPath (RepoPath 'tools/make-laser-pea-barrel.py'))) { throw 'tools/make-laser-pea-barrel.py must exist (regenerates the part image from the 500x500 source).' }

Write-Output 'Laser Pea (SEED_LASER_PEA) source checks passed.'
