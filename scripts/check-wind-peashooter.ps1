$ErrorActionPreference = 'Stop'

# 风神豌豆射手（SEED_WIND_PEASHOOTER）源级检查：
#   旅行专属红卡 / 200 阳光 / 7.5 秒冷却 / 贴图 = 加上三叶草叶子的豌豆射手 /
#   每发风神豌豆 40 点伤害 + 命中击退僵尸 / **不**发射豌豆射手那发首发大豌豆。
# 说明：本仓库无自动化测试框架（见 AGENTS.md），与其它 check-*.ps1 一样只做源级断言；
#      真正的观感（三叶草落位、击退手感、卡面/图鉴/影子）仍需手动进游戏验证，
#      验收步骤见 docs/wind-peashooter-port.md。

# 调用点有的传相对路径、有的传 P 出来的绝对路径，这里统一处理。
function Resolve-RepoFile([string]$theRelPath) {
    if ([System.IO.Path]::IsPathRooted($theRelPath)) { return $theRelPath }
    return (Join-Path (Split-Path -Parent $PSScriptRoot) $theRelPath)
}
function Assert-Source([string]$RelPath, [string]$Pattern, [string]$Message) {
    # 必须显式按 UTF-8 读：这些文件里有大量中文注释，Windows PowerShell 的默认编码（ANSI）
    # 会把中文拆成多个字节字符，把"两处代码之间最多 N 个字符"的间隔断言撑爆。
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath (Resolve-RepoFile $RelPath)
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

$plantPath = P 'src/Lawn/Plant.cpp'
$plantCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath $plantPath
$projPath = P 'src/Lawn/Projectile.cpp'
$projCpp = Get-Content -Raw -Encoding UTF8 -LiteralPath $projPath
$boardPath = P 'src/Lawn/Board.cpp'

# --- 枚举：只能追加在 NUM_* 哨兵之前，旧存档里的整数值不动 ---
Assert-Source (P 'src/ConstEnums.h') 'SEED_COCONUT_CANNON,[\s\S]{0,300}SEED_WIND_PEASHOOTER,[\s\S]{0,200}NUM_SEED_TYPES' 'SEED_WIND_PEASHOOTER must be appended after SEED_COCONUT_CANNON and before NUM_SEED_TYPES.'
Assert-Source (P 'src/ConstEnums.h') 'PROJECTILE_COCONUT = 20,[\s\S]{0,300}PROJECTILE_WIND_PEA = 21,[\s\S]{0,200}NUM_PROJECTILES = 22' 'PROJECTILE_WIND_PEA must be appended as value 21 (NUM_PROJECTILES becomes 22).'
Assert-Source (P 'src/ConstEnums.h') 'REANIM_COCONUT_PROJECTILE,[\s\S]{0,900}REANIM_WIND_PEASHOOTER,[\s\S]{0,600}NUM_REANIMS' 'REANIM_WIND_PEASHOOTER must be appended before NUM_REANIMS.'

# --- 动画资源：独立 reanim 文件 + 无 Atlas 定义槽 ---
Assert-Source (P 'src/Sexy.TodLib/Reanimator.cpp') 'REANIM_WIND_PEASHOOTER,\s*"reanim/WindPeashooter\.reanim",\s*1 << ReanimFlags::REANIM_NO_ATLAS' 'The wind peashooter must use its own reanim file without an atlas.'
$reanimPath = P 'res/main/reanim/WindPeashooter.reanim'
if (-not (Test-Path -LiteralPath $reanimPath)) { throw 'Missing res/main/reanim/WindPeashooter.reanim (run scripts/gen-wind-peashooter-reanim.py).' }
Assert-Source $reanimPath '<name>anim_idle</name>' 'Wind peashooter reanim must expose anim_idle.'
Assert-Source $reanimPath '<name>anim_head_idle</name>' 'Wind peashooter reanim must expose anim_head_idle.'
Assert-Source $reanimPath '<name>anim_shooting</name>' 'Wind peashooter reanim must expose anim_shooting.'
Assert-Source $reanimPath '<name>anim_sprout</name>' 'Wind peashooter reanim must keep the peashooter sprout it hangs the clover on.'

# 三叶草：三条轨道、引用原版三叶草叶片贴图，并且**排在 anim_sprout / anim_face 之前**
#（绘制顺序 = 轨道顺序，"叶子长在脑袋后面"全靠它）。
foreach ($clover in @('WindPeashooter_clover1', 'WindPeashooter_clover2', 'WindPeashooter_clover3')) {
    Assert-Source $reanimPath "<name>$clover</name>" "Wind peashooter reanim must add the clover track $clover."
}
$reanimText = Get-Content -Raw -Encoding UTF8 -LiteralPath $reanimPath
if (([regex]::Matches($reanimText, 'IMAGE_REANIM_BLOVER_PETAL')).Count -ne 3) {
    throw 'The three clover tracks must reference IMAGE_REANIM_BLOVER_PETAL exactly once each.'
}
foreach ($clover in @('WindPeashooter_clover1', 'WindPeashooter_clover2', 'WindPeashooter_clover3')) {
    $cloverAt = $reanimText.IndexOf("<name>$clover</name>")
    $sproutAt = $reanimText.IndexOf('<name>anim_sprout</name>')
    $faceAt = $reanimText.IndexOf('<name>anim_face</name>')
    if ($cloverAt -lt 0 -or $sproutAt -lt 0 -or $faceAt -lt 0) { throw "Could not locate $clover / anim_sprout / anim_face in the reanim." }
    if ($cloverAt -gt $sproutAt -or $cloverAt -gt $faceAt) {
        throw "$clover must be drawn before anim_sprout/anim_face (it grows behind the head)."
    }
}
# 三叶草只在头部层可见：动画内容区间 29..103（body 实例的 4..28 与眨眼实例的 1..3 都没有它）
if ($reanimText -notmatch '(?s)<name>WindPeashooter_clover1</name>.{0,400}?<f>0</f>') {
    throw 'The clover track must start with an explicit first content frame.'
}

# --- 植物数值与分类 ---
Assert-Source $plantPath 'SeedType::SEED_WIND_PEASHOOTER,\s*nullptr,\s*ReanimationType::REANIM_WIND_PEASHOOTER,\s*0,\s*200,\s*750,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*75,\s*"WIND_PEASHOOTER"' 'gPlantDefs row must be 200 sun / 750 refresh / SHOOTER / 75 launch rate (same cadence as the peashooter).'
Assert-Source $plantPath 'Plant::IsRedCard[\s\S]{0,900}SEED_WIND_PEASHOOTER' 'The wind peashooter must be a red card.'
Assert-Source (P 'src/GameConstants.h') 'WIND_PEA_KNOCKBACK\s*=\s*[\d.]+f' 'The per-hit knockback distance must live in GameConstants.h.'

# --- 出弹：走豌豆射手那套"头部枪口"几何，但**不**参与首发大豌豆 ---
$fireBody = [regex]::Match($plantCpp, 'void Plant::Fire\([\s\S]*?\n\}')
if (-not $fireBody.Success) { throw 'Could not find Plant::Fire body.' }
if ($fireBody.Value -notmatch 'case SeedType::SEED_WIND_PEASHOOTER:[\s\S]{0,400}PROJECTILE_WIND_PEA') { throw 'Plant::Fire must map SEED_WIND_PEASHOOTER to PROJECTILE_WIND_PEA.' }
if ($fireBody.Value -notmatch 'SEED_WIND_PEASHOOTER[\s\S]{0,260}GetPeaHeadOffset') { throw 'Plant::Fire must use the pea-head muzzle offset for SEED_WIND_PEASHOOTER.' }
# 首发大豌豆（300 伤害 + 2 倍体型）是豌豆射手自己的机制：断言它仍然只挂在 SEED_PEASHOOTER 上，
# 而且风神豌豆射手没有出现在那条条件里。
$bigPea = [regex]::Match($fireBody.Value, 'if \(mSeedType == SeedType::SEED_PEASHOOTER[^\r\n]*mHasFiredFirstPea\)[\s\S]{0,300}?\n\s*\}')
if (-not $bigPea.Success) { throw 'Could not find the peashooter first-pea (big pea) branch.' }
if ($bigPea.Value -match 'WIND_PEASHOOTER') { throw 'The wind peashooter must NOT fire the peashooter big first pea.' }

# 也不参与"精英豌豆射手"掷骰（20% 淡蓝 + 波浪弹道）：那只属于豌豆射手/双发射手。
# 断言 mIsElite 的赋值语句里没有 SEED_WIND_PEASHOOTER —— 加回去会直接报错。
$eliteAssign = [regex]::Match($plantCpp, 'mIsElite = \([^\r\n]*')
if (-not $eliteAssign.Success) { throw 'Could not find the mIsElite assignment.' }
if ($eliteAssign.Value -match 'WIND_PEASHOOTER') { throw 'The wind peashooter must NOT roll the elite pea variant (only peashooter/repeater do).' }

# --- 弹丸：40 伤害 + 命中击退 + 命中判定/贴图/影子/碰撞矩形都已登记 ---
Assert-Source $projPath 'PROJECTILE_WIND_PEA,\s*0,\s*40' 'The wind pea must deal 40 damage.'
# 新弹丸类型必须在 Projectile.cpp 的每一处逐类型分支里登记（贴图 / 影子 / 触地 / 矩形 / 伤害…）。
# 当前是 9 处；漏掉任何一处都会表现为"弹丸隐形 / 不消失 / 打不到僵尸"，所以用计数兜住。
$windPeaRefs = ([regex]::Matches($projCpp, 'PROJECTILE_WIND_PEA')).Count
if ($windPeaRefs -lt 9) { throw "PROJECTILE_WIND_PEA must be registered in every per-type branch of Projectile.cpp (found $windPeaRefs references, expected at least 9)." }
if ($projCpp -notmatch 'theZombie->TakeDamage\(aDamage, aDamageFlags\);[\s\S]{0,900}?PROJECTILE_WIND_PEA[\s\S]{0,300}?theZombie->KnockBack\(WIND_PEA_KNOCKBACK\)') { throw 'DoImpact must shove the zombie AFTER the damage is dealt.' }
if ($projCpp -notmatch 'PROJECTILE_WIND_PEA && theZombie && !theZombie->IsDeadOrDying\(\)') { throw 'The knockback must skip zombies this very pea just killed.' }
Assert-Source $projPath 'case ProjectileType::PROJECTILE_WIND_PEA:[\s\S]{0,300}aImage = IMAGE_PROJECTILEPEA' 'The wind pea must draw the vanilla pea sprite.'
Assert-Source $projPath 'PROJECTILE_POISON_PEA \|\|[\s\S]{0,400}PROJECTILE_WIND_PEA' 'CantHitHighGround / CheckForHighGround must know the wind pea (it is a low pea, not a lobbed shot).'
Assert-Source $projPath 'PROJECTILE_POISON_PEA \|\|\s*[\r\n\s]*mProjectileType == ProjectileType::PROJECTILE_WIND_PEA' 'GetProjectileRect must treat the wind pea like a pea (hitbox in front of the sprite).'
Assert-Source $projPath 'case ProjectileType::PROJECTILE_WIND_PEA:[\s\S]{0,300}PARTICLE_PEA_SPLAT' 'The wind pea must reuse the vanilla pea impact splat.'
# 风神豌豆是"特制豌豆"，和紫火/毒液那两颗一样**不**会被火炬树桩点燃。
Assert-Source $projPath 'bool Projectile::PeaAboutToHitTorchwood\(\)[\s\S]{0,600}PROJECTILE_PEA && mProjectileType != ProjectileType::PROJECTILE_SNOWPEA[\s\S]{0,200}return false' 'PeaAboutToHitTorchwood must stay limited to PEA/SNOWPEA so the wind pea is not converted.'

# --- 模式与入口：旅行专属红卡 + 沙盒旅行页 + 图鉴第 2 页 ---
Assert-Source (P 'src/LawnApp.cpp') 'case SeedType::SEED_WIND_PEASHOOTER:\s*[\r\n\s]*return IsTravelLevel\(mGameMode\)' 'HasSeedType must expose the wind peashooter in travel levels only.'
Assert-Source (P 'src/Lawn/Travel.cpp') 'SEED_WIND_PEASHOOTER' 'The wind peashooter must be registered as a travel-only plant (chooser page 1).'
Assert-Source $boardPath 'gIceSandboxTravelSeeds\[\][\s\S]{0,1200}SEED_WIND_PEASHOOTER' 'The ice sandbox travel page must include the wind peashooter.'
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.h') '#define NUM_ALMANAC_EXTRA_SEEDS 16' 'The almanac extra-plant page must have grown to 16 slots.'
Assert-Source (P 'src/Lawn/Widget/AlmanacDialog.cpp') 'gAlmanacExtraSeeds\[NUM_ALMANAC_EXTRA_SEEDS\][\s\S]{0,1800}SEED_WIND_PEASHOOTER' 'The wind peashooter must appear on almanac page 2.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_LASER_PEA \|\|[\s\S]{0,200}theSeedType == SeedType::SEED_WIND_PEASHOOTER' 'MakeCachedPlantFrame must draw the head layer (card / almanac / cursor preview) for the wind peashooter.'
# PlantInitialize 的"豌豆射手系"分支（body + head 两个实例）与 AttachBlinkAnim 的同一份名单
# 都必须带上它，否则种下去只有一个身体、也不会眨眼。
Assert-Source $plantPath 'case SeedType::SEED_FIRE_GATLING_PEA:[\s\S]{0,300}case SeedType::SEED_LASER_PEA:[\s\S]{0,200}case SeedType::SEED_WIND_PEASHOOTER:' 'PlantInitialize must create the head reanim for the wind peashooter.'
Assert-Source $plantPath 'SEED_LASER_PEA \|\| mSeedType == SeedType::SEED_WIND_PEASHOOTER\)' 'AttachBlinkAnim must include the wind peashooter in the peashooter blink group.'

# --- 文案：两份 strings 都要有名称 / 提示 / 图鉴说明 ---
foreach ($path in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $path) '<String id="WIND_PEASHOOTER">风神豌豆射手</String>' 'Missing wind peashooter name.'
    Assert-Source (P $path) '<String id="WIND_PEASHOOTER_TOOLTIP">' 'Missing wind peashooter tooltip.'
    Assert-Source (P $path) '<String id="WIND_PEASHOOTER_DESCRIPTION">' 'Missing wind peashooter almanac description.'
}

Write-Output 'Wind Peashooter (SEED_WIND_PEASHOOTER) source checks passed.'
