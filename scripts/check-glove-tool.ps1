$ErrorActionPreference = 'Stop'

# Level glove (a plant-moving tool) source checks:
#   unlocked by buying the gardening glove in the Zen Garden store, usable in every level,
#   button next to the shovel, 10s cooldown (1000 centiseconds) outside travel modes and 0 in all
#   travel modes, moves plants only, destination must be plantable.
# Notes: this repo has no automated test suite (see AGENTS.md), so - like the other check-*.ps1
#        scripts - this is a source-level assertion pass. Game feel (button placement, cooldown
#        countdown, art following the cursor) still needs a manual play test.
# Keep this file ASCII-only: Windows PowerShell 5.1 reads BOM-less .ps1 files as ANSI, and
# non-ASCII comment bytes can swallow the following line.

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

function Assert-NotSource([string]$RelPath, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath (Resolve-RepoFile $RelPath)
    if ($content -match $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

$boardH = P 'src/Lawn/Board.h'
$boardCpp = P 'src/Lawn/Board.cpp'
$cursorCpp = P 'src/Lawn/CursorObject.cpp'
$saveCpp = P 'src/Lawn/System/SaveGame.cpp'

# --- State and API: cooldown field + glove entry points ---
Assert-Source $boardH 'int32_t\s+mGloveCooldown;' 'Board.h must declare the per-level glove cooldown field mGloveCooldown.'
Assert-Source $boardH 'bool\s+CanUseLevelGlove\(\);' 'Board.h must declare Board::CanUseLevelGlove().'
Assert-Source $boardH 'bool\s+IsGloveToolbarReady\(\);' 'Board.h must declare Board::IsGloveToolbarReady().'
Assert-Source $boardH 'Rect\s+GetGloveButtonRect\(\);' 'Board.h must declare Board::GetGloveButtonRect().'
Assert-Source $boardH 'void\s+DrawGloveButton\(Graphics\* g\);' 'Board.h must declare Board::DrawGloveButton().'
Assert-Source $boardH 'bool\s+GloveCanMovePlantTo\(Plant\* thePlant, int theGridX, int theGridY\);' 'Board.h must declare Board::GloveCanMovePlantTo().'
Assert-Source $boardH 'void\s+MovePlantWithGlove\(Plant\* thePlant, int theGridX, int theGridY\);' 'Board.h must declare Board::MovePlantWithGlove().'
Assert-Source $boardCpp 'mGloveCooldown = 0;' 'Board::Board must initialise mGloveCooldown to 0.'

# --- Unlock: only via the Zen Garden store purchase; not in Zen Garden / Tree / I-Zombie / Slot Machine ---
Assert-Source $boardCpp 'bool Board::CanUseLevelGlove\(\)[\s\S]{0,900}mPurchases\[StoreItem::STORE_ITEM_GARDENING_GLOVE\] > 0' 'CanUseLevelGlove must require the Zen Garden store purchase STORE_ITEM_GARDENING_GLOVE.'
Assert-Source $boardCpp 'bool Board::CanUseLevelGlove\(\)[\s\S]{0,400}GAMEMODE_CHALLENGE_ZEN_GARDEN[\s\S]{0,200}GAMEMODE_TREE_OF_WISDOM' 'CanUseLevelGlove must not offer a second glove in the Zen Garden / Tree of Wisdom.'
Assert-Source $boardCpp 'bool Board::CanUseLevelGlove\(\)[\s\S]{0,700}IsIZombieLevel\(\)' 'CanUseLevelGlove must skip I-Zombie levels (their mouse-down goes to Challenge).'
Assert-Source $boardCpp 'bool Board::CanUseLevelGlove\(\)[\s\S]{0,900}IsSlotMachineLevel\(\)' 'CanUseLevelGlove must skip Slot Machine levels (no plants to move).'

# --- Toolbar gate: follow the shovel, but stay available in travel modes ---
$toolbarReady = [regex]::Match((Get-Content -Raw -LiteralPath $boardCpp), 'bool Board::IsGloveToolbarReady\(\)[\s\S]*?\r?\n\}')
if (-not $toolbarReady.Success) { throw 'Could not find Board::IsGloveToolbarReady.' }
if ($toolbarReady.Value -notmatch 'SCENE_PLAYING') {
    throw 'IsGloveToolbarReady must require SCENE_PLAYING.'
}
if ($toolbarReady.Value -notmatch 'IsTravelLevel\(mApp->mGameMode\)') {
    throw 'IsGloveToolbarReady must keep the glove available in every travel mode, even when the shovel is hidden.'
}
Assert-Source $boardCpp 'void Board::DrawGloveButton[\s\S]{0,160}IsGloveToolbarReady\(\)' 'DrawGloveButton must use IsGloveToolbarReady().'

# --- Cooldown: 1000 centiseconds (10s) normally, 0 in every travel mode ---
Assert-Source $boardCpp 'int Board::GetGloveCooldownDuration\(\)[\s\S]{0,220}IsTravelLevel\(mApp->mGameMode\) \? 0 : 1000' 'GetGloveCooldownDuration must be 0 for every travel mode and 1000 (=10s) otherwise.'
Assert-Source $boardCpp 'mGloveCooldown = GetGloveCooldownDuration\(\);' 'MovePlantWithGlove must (re)start the cooldown after a successful move.'
Assert-Source $boardCpp 'mGloveCooldown--;' 'Board::UpdateGame must tick the glove cooldown down.'

# --- Button placement and drawing: next to the shovel, drawn right after it ---
Assert-Source $boardCpp 'Rect Board::GetGloveButtonRect\(\)[\s\S]{0,400}GetShovelButtonRect\(\)' 'GetGloveButtonRect must be derived from GetShovelButtonRect (next to the shovel).'
Assert-Source $boardCpp 'aRect\.mX \+= aRect\.mWidth;' 'The glove button must sit one shovel-bank width away from the shovel.'
Assert-Source $boardCpp 'if \(!IsGloveToolbarReady\(\) \|\| !CanUseLevelGlove\(\)\)' 'DrawGloveButton must draw only when the toolbar is ready and the glove is bought.'
Assert-Source $boardCpp 'DrawShovel\(g\);\s*\r?\n\s*DrawGloveButton\(g\);' 'Board::DrawUIBottom must draw the glove button right after the shovel.'

# --- Cooldown feedback: greyed icon + remaining seconds ---
Assert-Source $boardCpp 'bool aCooling = mGloveCooldown > 0;' 'DrawGloveButton must grey the icon while cooling down.'
Assert-Source $boardCpp '\(mGloveCooldown \+ 99\) / 100' 'DrawGloveButton must show the remaining cooldown in whole seconds.'

# --- Hotkey G: pick up / put down; drop a held card first (conveyor-belt travel levels) ---
$keyDownBody = [regex]::Match((Get-Content -Raw -LiteralPath $boardCpp), 'void Board::KeyDown\(KeyCode theKey\)[\s\S]*?\r?\n\}')
if (-not $keyDownBody.Success) { throw 'Could not find Board::KeyDown body.' }
$gloveKey = [regex]::Match($keyDownBody.Value, "else if \(theKey == KeyCode\('G'\) \|\| theKey == KeyCode\('g'\)\)[\s\S]*?\r?\n\t\}")
if (-not $gloveKey.Success) { throw 'Board::KeyDown must handle the G hotkey for the level glove.' }
foreach ($needle in @('CanUseLevelGlove()', 'IsGloveToolbarReady()', 'IsGloveSwappableCursor(', 'RefreshSeedPacketFromCursor();', 'PickUpTool(GameObjectType::OBJECT_TYPE_GLOVE)', 'CURSOR_TYPE_GLOVE', 'CURSOR_TYPE_PLANT_FROM_GLOVE', 'ClearCursor();')) {
    if ($gloveKey.Value -notmatch [regex]::Escape($needle)) {
        throw "The G hotkey branch must contain '$needle'."
    }
}
if ($gloveKey.Value -notmatch 'mApp->mGameScene == GameScenes::SCENE_PLAYING') {
    throw 'The G hotkey must not fire outside SCENE_PLAYING (level intro / cutscenes).'
}
if ($gloveKey.Value -notmatch 'mCursorObject->mCursorType == CursorType::CURSOR_TYPE_NORMAL') {
    throw 'The G hotkey must fall back to a clean cursor before picking the glove up.'
}
$shiftIdx = $keyDownBody.Value.IndexOf('KEYCODE_SHIFT')
$gloveIdx = $keyDownBody.Value.IndexOf("KeyCode('G')")
if ($shiftIdx -lt 0 -or $gloveIdx -lt 0 -or $gloveIdx -lt $shiftIdx) {
    throw 'The G hotkey must sit in the same else-if chain as the Shift (shovel) hotkey, after it.'
}
if ((Get-Content -Raw -LiteralPath $boardCpp) -match "theKey == KeyCode\('G'\)[\s\S]{0,200}SHOVEL") {
    throw 'G must not be wired to the shovel.'
}

# --- Held-card swap: in conveyor-belt (travel experience) levels the player always holds a card ---
$swappable = [regex]::Match((Get-Content -Raw -LiteralPath $boardCpp), 'bool IsGloveSwappableCursor\(CursorType theCursorType\)[\s\S]*?\r?\n\t\}')
if (-not $swappable.Success) { throw 'Could not find IsGloveSwappableCursor.' }
foreach ($cursor in @('CURSOR_TYPE_NORMAL', 'CURSOR_TYPE_SHOVEL', 'CURSOR_TYPE_PLANT_FROM_BANK', 'CURSOR_TYPE_PLANT_FROM_USABLE_COIN', 'CURSOR_TYPE_PLANT_FROM_DUPLICATOR')) {
    if ($swappable.Value -notmatch [regex]::Escape($cursor)) {
        throw "IsGloveSwappableCursor must accept $cursor (a held card / the shovel must not deadlock the G key)."
    }
}

# --- Pick up / move flow ---
Assert-Source $boardCpp 'IsGloveToolbarReady\(\) && CanUseLevelGlove\(\) && aGloveButtonRect\.Contains\(x, y\)' 'MouseHitTest must report OBJECT_TYPE_GLOVE on the glove button.'
Assert-Source $boardCpp 'case GameObjectType::OBJECT_TYPE_GLOVE:[\s\S]{0,320}CanUseLevelGlove\(\) && mGloveCooldown > 0[\s\S]{0,220}SOUND_BUZZER' 'PickUpTool must buzz instead of picking the glove up while it is cooling down.'
Assert-Source $boardCpp 'else if \(theCursorType == CursorType::CURSOR_TYPE_GLOVE\)[\s\S]{0,700}PickUpPlantWithGlove\(aPlant\);' 'MouseDownWithTool must hand the clicked plant to the glove.'
Assert-Source $boardCpp 'void Board::PickUpPlantWithGlove[\s\S]{0,400}mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE' 'PickUpPlantWithGlove must switch the cursor to CURSOR_TYPE_PLANT_FROM_GLOVE.'
Assert-Source $boardCpp 'mGlovePlantID = \(PlantID\)mPlants\.DataArrayGetID\(thePlant\);' 'PickUpPlantWithGlove must record the held plant id.'

# --- The in-level move runs in its own branch (never through the planting check) ---
$mouseDownBody = [regex]::Match((Get-Content -Raw -LiteralPath $boardCpp), 'void Board::MouseDownWithPlant\([\s\S]*?\n\}')
if (-not $mouseDownBody.Success) { throw 'Could not find Board::MouseDownWithPlant body.' }
$gloveBranch = [regex]::Match($mouseDownBody.Value, 'if \(mCursorObject->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE[\s\S]*?\r?\n\t\}')
if (-not $gloveBranch.Success) { throw 'Could not find the level-glove branch of Board::MouseDownWithPlant.' }
foreach ($needle in @('GetGlovePlant()', 'PixelToGridX(x, y)', 'GloveCanMovePlantTo(', 'MovePlantWithGlove(', 'SOUND_BUZZER', 'ClearCursor();')) {
    if ($gloveBranch.Value -notmatch [regex]::Escape($needle)) {
        throw "The level-glove branch of MouseDownWithPlant must contain '$needle'."
    }
}
if ($gloveBranch.Value -match 'CanPlantAt') { throw 'The level-glove branch must not go through CanPlantAt (moving is not planting).' }

# --- Plants only + planting restrictions respected; pumpkin / lily pad / flower pot as carriers ---
$gloveCanMove = [regex]::Match((Get-Content -Raw -LiteralPath $boardCpp), 'bool Board::GloveCanMovePlantTo\([\s\S]*?\r?\n\}')
if (-not $gloveCanMove.Success) { throw 'Could not find Board::GloveCanMovePlantTo body.' }
if ($gloveCanMove.Value -notmatch 'else if \(aDestLawn\.mNormalPlant != nullptr \|\| aDestLawn\.mFlyingPlant != nullptr\)') {
    throw 'GloveCanMovePlantTo must refuse to land on an occupied plant slot (never replaces / shovels other plants).'
}
if ($gloveCanMove.Value -notmatch 'if \(aIsPumpkin\)\s*\r?\n\t\{[\s\S]{0,900}if \(aDestLawn\.mPumpkinPlant != nullptr\)') {
    throw 'A pumpkin must be movable ONTO a cell that already holds a plant (that is what a pumpkin is for); only a second pumpkin is refused.'
}
if ($gloveCanMove.Value -notmatch 'aDestLawn\.mPumpkinPlant != nullptr && aIsTwoCellPlant') {
    throw 'GloveCanMovePlantTo must forbid pushing a 2-cell plant (Cob Cannon / Giant Wall-nut) into a pumpkin shell.'
}
if ($gloveCanMove.Value -match 'aDestLawn\.mUnderPlant != nullptr \|\|') {
    throw 'GloveCanMovePlantTo must allow landing on a lily pad / flower pot (carrier cell).'
}
if ($gloveCanMove.Value -notmatch 'PlantingReason aReason = CanPlantAt\(theGridX, theGridY, aSeedType\);') {
    throw 'GloveCanMovePlantTo must delegate the terrain / level restrictions to CanPlantAt().'
}
if ($gloveCanMove.Value -notmatch 'aReason != PlantingReason::PLANTING_OK &&[\s\S]{0,260}PLANTING_NEEDS_UPGRADE[\s\S]{0,160}PLANTING_NEEDS_TWO_WALLNUTS') {
    throw 'GloveCanMovePlantTo must treat NEEDS_UPGRADE / NEEDS_TWO_WALLNUTS as carrier prerequisites, not terrain restrictions.'
}
foreach ($terrainReason in @('PLANTING_NOT_HERE', 'PLANTING_ONLY_IN_POOL', 'PLANTING_NEEDS_POT', 'PLANTING_NOT_ON_CRATER', 'PLANTING_NOT_ON_GRAVE', 'PLANTING_ONLY_ON_GROUND', 'PLANTING_NOT_ON_WATER', 'PLANTING_NEEDS_GROUND', 'PLANTING_NOT_ON_ART', 'PLANTING_NOT_PASSED_LINE')) {
    if ($gloveCanMove.Value -match [regex]::Escape($terrainReason)) {
        throw "GloveCanMovePlantTo must not special-case $terrainReason - terrain restrictions come from CanPlantAt()."
    }
}
Assert-Source $boardCpp 'PlantsOnLawn aSourceLawn;[\s\S]{0,220}aSourceLawn\.mNormalPlant != nullptr \|\| aSourceLawn\.mPumpkinPlant != nullptr' 'GloveCanMovePlantTo must refuse to move an under-plant that still carries something on top.'
Assert-Source $boardCpp 'bool aIsTwoCellPlant = aSeedType == SeedType::SEED_COBCANNON \|\| aSeedType == SeedType::SEED_GIANT_WALLNUT;' 'GloveCanMovePlantTo must treat Cob Cannon / Giant Wall-nut as 2-cell plants.'
Assert-Source $boardCpp 'thePlant->mRenderOrder = thePlant->CalcRenderOrder\(\);' 'MovePlantWithGlove must recompute the render order after the move.'
Assert-Source $boardCpp 'aParticle->SystemMove\(' 'MovePlantWithGlove must carry the plant particle system along.'

# --- Cursor / ghost preview: never treat a lawn plant as a potted plant ---
Assert-Source $cursorCpp 'case CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE:[\s\S]{0,420}mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN' 'CursorObject::Draw must draw a plain seed face (not a potted plant) for the level glove.'
Assert-Source $cursorCpp 'bool aLevelGlove =[\s\S]{0,220}CURSOR_TYPE_PLANT_FROM_GLOVE' 'CursorPreview::Update must special-case the level glove.'
Assert-Source $cursorCpp 'aShow = mBoard->GloveCanMovePlantTo\(mBoard->GetGlovePlant\(\), mGridX, mGridY\);' 'CursorPreview must use the glove move rule for its ghost preview.'
Assert-Source $cursorCpp 'CURSOR_TYPE_PLANT_FROM_GLOVE &&\s*\r?\n\s*mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN' 'CursorPreview::Draw must only look up potted plants in the Zen Garden.'

# --- Save data: the cooldown lives in the .v4 save ---
Assert-Source $saveCpp 'BOARD_FIELD_CHOCOLATE_COLLECTED,\s*\r?\n\s*BOARD_FIELD_GLOVE_COOLDOWN' 'SaveGame must append BOARD_FIELD_GLOVE_COOLDOWN to the field enum.'
Assert-Source $saveCpp 'case BOARD_FIELD_GLOVE_COOLDOWN:[\s\S]{0,200}c\.SyncInt32\(theBoard->mGloveCooldown\);' 'SaveGame must read BOARD_FIELD_GLOVE_COOLDOWN into mGloveCooldown.'
Assert-Source $saveCpp 'AppendFieldWithSync\(aBlob, BOARD_FIELD_GLOVE_COOLDOWN,[\s\S]{0,160}mGloveCooldown' 'SaveGame must write BOARD_FIELD_GLOVE_COOLDOWN from mGloveCooldown.'

# --- Strings: hover tooltip + rejection hint (both string files) ---
foreach ($file in @('res/properties/pvzp-strings.xml', 'res/properties/pvzp-strings.zh-CN.xml')) {
    Assert-Source (P $file) '<String id="GLOVE_LEVEL_TOOLTIP">' "$file must define the GLOVE_LEVEL_TOOLTIP string."
    Assert-Source (P $file) '<String id="GLOVE_LEVEL_TOOLTIP_TRAVEL">' "$file must define the GLOVE_LEVEL_TOOLTIP_TRAVEL string."
    Assert-Source (P $file) '<String id="ADVICE_GLOVE_CANT_MOVE">' "$file must define the ADVICE_GLOVE_CANT_MOVE string."
}
Assert-Source $boardCpp 'IsTravelLevel\(mApp->mGameMode\) \? "\[GLOVE_LEVEL_TOOLTIP_TRAVEL\]" : "\[GLOVE_LEVEL_TOOLTIP\]"' 'UpdateToolTip must pick the travel / non-travel glove tooltip.'
Assert-Source $boardCpp 'DisplayAdvice\("\[ADVICE_GLOVE_CANT_MOVE\]"' 'A rejected move must show the ADVICE_GLOVE_CANT_MOVE hint.'

# --- The Zen Garden glove must stay untouched ---
Assert-Source $boardCpp 'void Board::MouseDownWithTool\([\s\S]{0,500}mApp->mZenGarden->MouseDownWithTool\(x, y, theCursorType\)' 'The Zen Garden glove path must stay untouched (MouseDownWithTool still forwards to ZenGarden).'
Assert-NotSource $boardCpp 'ZenGarden::MovePlant[\s\S]{0,80}mGloveCooldown' 'The Zen Garden plant mover must not be entangled with the level glove cooldown.'

Write-Output 'Level glove source checks passed.'
