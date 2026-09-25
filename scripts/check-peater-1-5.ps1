$ErrorActionPreference = 'Stop'

# 1.5 发射手（SEED_PEATER_1_5）源级检查：
#   红卡 / 旅行专属 / 150 阳光 / 可直接种下 / 每次攻击 50% 一发、50% 两发 / 贴图 = 去掉眉毛的双发射手。
# 说明：本仓库无自动化测试框架（见 AGENTS.md），与其它 check-*.ps1 一样做源级断言；
#      实际手感（掷骰分布、眉毛是否消失、卡面观感）仍需手动进游戏验证。

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

# --- 枚举：新值前插在 NUM_SEED_TYPES 之前，且排在既有自定义植物之后（既有存档枚举值不变） ---
Assert-Source (P 'src/ConstEnums.h') 'SEED_FUMESHROOM_GROUP,[\s\S]{0,200}SEED_PEATER_1_5,[\s\S]{0,4096}NUM_SEED_TYPES' 'SEED_PEATER_1_5 must be declared after SEED_FUMESHROOM_GROUP and before NUM_SEED_TYPES.'

# --- 植物定义：复用双发射手 reanim、150 阳光、普通冷却 750、射手子类、普通 75 帧节奏 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'SeedType::SEED_PEATER_1_5,\s*nullptr,\s*ReanimationType::REANIM_REPEATER,\s*5,\s*150,\s*750,\s*PlantSubClass::SUBCLASS_SHOOTER,\s*75,\s*"PEATER_1_5"' 'gPlantDefs row must be REANIM_REPEATER / 150 sun / 750 refresh / SHOOTER / 75 launch rate / "PEATER_1_5".'

# --- 红卡 + 可直接种下（不进 IsUpgrade 升级卡列表） ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'bool Plant::IsRedCard[\s\S]{0,500}SEED_PEATER_1_5' 'Plant::IsRedCard must include SEED_PEATER_1_5.'
$plantCpp = Get-Content -Raw -LiteralPath (P 'src/Lawn/Plant.cpp')
$upgradeBody = [regex]::Match($plantCpp, 'bool Plant::IsUpgrade[\s\S]*?\n\}')
if (-not $upgradeBody.Success) { throw 'Could not find Plant::IsUpgrade body.' }
if ($upgradeBody.Value -match 'SEED_PEATER_1_5') { throw 'SEED_PEATER_1_5 must not be an upgrade card - it is planted directly.' }

# --- 旅行专属：Travel 表登记 + HasSeedType 仅旅行关可拥有 ---
Assert-Source (P 'src/Lawn/Travel.cpp') 'SEED_PEATER_1_5,\s*false' 'SEED_PEATER_1_5 must be registered in gTravelPlantDefs.'
Assert-Source (P 'src/LawnApp.cpp') 'case SeedType::SEED_PEATER_1_5:[\s\S]{0,80}return IsTravelLevel\(mGameMode\)' 'LawnApp::HasSeedType must gate SEED_PEATER_1_5 on IsTravelLevel.'

# --- 攻击特性：每轮开始掷骰，50% 一发 / 50% 两发 ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_PEATER_1_5\)\s*\{[\s\S]{0,160}mPeater15DoubleShot = \(Sexy::Rand\(2\) == 0\)' 'UpdateShooter must roll a 50% double-shot flag each attack cycle.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'mPeater15DoubleShot\)\s*\{[\s\S]{0,120}FindTargetAndFire\(mRow, PlantWeapon::WEAPON_PRIMARY\)' 'The optional second pea must be gated on the rolled flag.'

# --- 子弹本身（漏一处就会打出未初始化类型的弹丸 => 没有贴图 / 从错误位置飞出） ---
$fireBody = [regex]::Match($plantCpp, 'void Plant::Fire\([\s\S]*?\n\}')
if (-not $fireBody.Success) { throw 'Could not find Plant::Fire body.' }
if ($fireBody.Value -notmatch 'case SeedType::SEED_PEATER_1_5:[\s\S]{0,80}PROJECTILE_PEA') { throw 'Plant::Fire switch must map SEED_PEATER_1_5 to PROJECTILE_PEA (otherwise the pea has no sprite).' }
if ($fireBody.Value -notmatch 'mSeedType == SeedType::SEED_PEATER_1_5 \|\|[\s\S]{0,300}GetPeaHeadOffset') { throw 'Plant::Fire must use the pea-head muzzle offset for SEED_PEATER_1_5.' }

# --- 贴图：去掉眉毛（双发射手 reanim 的 PeaShooter_eyebrow 轨道） ---
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_PEATER_1_5[\s\S]{0,600}AssignRenderGroupToTrack\("PeaShooter_eyebrow", RENDER_GROUP_HIDDEN\)' 'Planted 1.5 Peater must hide the PeaShooter_eyebrow track.'
Assert-Source (P 'src/Lawn/System/ReanimationLawn.cpp') 'SEED_PEATER_1_5[\s\S]{0,300}AssignRenderGroupToTrack\("PeaShooter_eyebrow", RENDER_GROUP_HIDDEN\)' 'Cached 1.5 Peater art (seed packet / chooser / cursor) must hide the PeaShooter_eyebrow track.'

# --- 旅行模式内可获得：沙盒旅行页 + 巨大坚果体验关传送带 ---
Assert-Source (P 'src/Lawn/Board.cpp') 'gIceSandboxTravelSeeds\[\][\s\S]{0,400}SEED_PEATER_1_5' 'SEED_PEATER_1_5 must be available on the ice sandbox travel page.'
Assert-Source (P 'src/Lawn/Challenge.cpp') 'GAMEMODE_CHALLENGE_TRAVEL_2[\s\S]{0,900}SEED_PEATER_1_5' 'SEED_PEATER_1_5 must be on the TRAVEL_2 conveyor roster.'

# --- 种下 15 秒后点击免费升级为双发射手 ---
Assert-Source (P 'src/GameConstants.h') 'PEATER_1_5_UPGRADE_SECONDS\s*=\s*15' 'PEATER_1_5_UPGRADE_SECONDS must be 15 seconds.'
Assert-Source (P 'src/GameConstants.h') 'PEATER_1_5_UPGRADE_DELAY\s*=\s*PEATER_1_5_UPGRADE_FPS\s*\*\s*PEATER_1_5_UPGRADE_SECONDS' 'PEATER_1_5_UPGRADE_DELAY must convert the seconds to logic frames.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'SEED_PEATER_1_5[\s\S]{0,900}mPeater15UpgradeCountdown = PEATER_1_5_UPGRADE_DELAY' 'PlantInitialize must start the 15s upgrade countdown for SEED_PEATER_1_5.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'mPeater15UpgradeCountdown--' 'Plant::Update must tick the upgrade countdown down while it is running.'
Assert-Source (P 'src/Lawn/Plant.cpp') 'PEATER_1_5_UPGRADE_READY' 'The plant must announce that the free upgrade became available.'

$mouseDownBody = [regex]::Match($plantCpp, 'void Plant::MouseDown\([\s\S]*?\n\}')
if (-not $mouseDownBody.Success) { throw 'Could not find Plant::MouseDown body.' }
$peaterBranch = [regex]::Match($mouseDownBody.Value, 'if \(mSeedType == SeedType::SEED_PEATER_1_5\)[\s\S]*?\r?\n    \}')
if (-not $peaterBranch.Success) { throw 'Could not find the SEED_PEATER_1_5 branch of Plant::MouseDown.' }
if ($peaterBranch.Value -notmatch 'PEATER_1_5_UPGRADE_WAIT') { throw 'Plant::MouseDown must hint how many seconds are left when clicked too early.' }
if ($peaterBranch.Value -notmatch 'AddPlant\(aGridX, aRow, SeedType::SEED_REPEATER, SeedType::SEED_NONE\)') { throw 'Plant::MouseDown must replace the 1.5 Peater with a Repeater in the same cell.' }
if ($peaterBranch.Value -notmatch 'Die\(\);[\s\S]{0,80}AddPlant') { throw 'Plant::MouseDown must kill the old plant before planting the Repeater (engine upgrade order).' }
if ($peaterBranch.Value -match '(?i)mSeedBank|GetCost|mSun') { throw 'The 1.5 Peater upgrade must not touch the seed bank / sun at all (it is free).' }

# --- 可升级时闪白提示（沿用"该植物可升级"的视觉语言） ---
$colorBody = [regex]::Match($plantCpp, 'void Plant::UpdateReanimColor\([\s\S]*?\n\}')
if (-not $colorBody.Success) { throw 'Could not find Plant::UpdateReanimColor body.' }
if ($colorBody.Value -notmatch 'SEED_PEATER_1_5 && mPeater15UpgradeCountdown <= 0') { throw 'A ready 1.5 Peater must flash to advertise the click-to-upgrade.' }

# --- 翻译文案：卡名 + 悬浮说明 + 升级提示（中英各一份） ---
foreach ($file in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $file) '<String id="PEATER_1_5">' "$file must define the PEATER_1_5 name string."
    Assert-Source (P $file) '<String id="PEATER_1_5_TOOLTIP">' "$file must define the PEATER_1_5_TOOLTIP string."
    Assert-Source (P $file) '<String id="PEATER_1_5_UPGRADE_READY">' "$file must define the PEATER_1_5_UPGRADE_READY string."
    Assert-Source (P $file) '<String id="PEATER_1_5_UPGRADE_WAIT">[\s\S]{0,200}\{TIME\}' "$file must define PEATER_1_5_UPGRADE_WAIT with a {TIME} placeholder."
}

Write-Output '1.5 Peater (SEED_PEATER_1_5) source checks passed.'
