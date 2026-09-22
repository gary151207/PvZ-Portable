$ErrorActionPreference = 'Stop'

# Pool Endless AI source checks.  This repository has no C++ test harness, so
# this guards the opt-in contract and persistence boundary while gameplay is
# covered by the manual soak test described in AGENTS.md.

function Assert-Source([string]$Path, [string]$Pattern, [string]$Message) {
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
function P([string]$rel) { Join-Path $root $rel }

$appH = P 'src/LawnApp.h'
$appCpp = P 'src/LawnApp.cpp'
$boardH = P 'src/Lawn/Board.h'
$boardCpp = P 'src/Lawn/Board.cpp'
$chooserH = P 'src/Lawn/Widget/SeedChooserScreen.h'
$chooserCpp = P 'src/Lawn/Widget/SeedChooserScreen.cpp'
$saveCpp = P 'src/Lawn/System/SaveGame.cpp'

Assert-Source $appH 'bool\s+mEndlessAi;' 'LawnApp must retain the --endless-ai opt-in flag.'
Assert-Source $appCpp 'theParamName == "--endless-ai"' 'LawnApp must parse --endless-ai.'

Assert-Source $boardH 'bool\s+mEndlessAiManuallyDisabled;' 'Board must persist the manual Endless AI override.'
foreach ($method in @('EndlessAiCanRun', 'EndlessAiUpdate', 'EndlessAiPlant', 'EndlessAiFireCob', 'EndlessAiDisableForManualInput', 'EndlessAiChooseSeeds')) {
    Assert-Source $boardH "$method\\(" "Board.h must declare $method."
    Assert-Source $boardCpp "$method\\(" "Board.cpp must implement $method."
}
Assert-Source $boardCpp 'GAMEMODE_SURVIVAL_ENDLESS_STAGE_3' 'Endless AI must be constrained to Pool Endless.'
Assert-Source $boardCpp 'mApp->mGameScene == GameScenes::SCENE_PLAYING' 'Endless AI must only act during gameplay.'
Assert-Source $boardCpp 'CanPlantAt\(' 'Endless AI planting must use Board::CanPlantAt.'
Assert-Source $boardCpp 'WasPlanted\(\)' 'Endless AI planting must start the real seed packet cooldown.'
Assert-Source $boardCpp 'CobCannonFire\(' 'Endless AI must fire real Cob Cannon plants.'
Assert-Source $boardCpp 'Coin::MouseDown\(' 'Endless AI must collect drops through Coin::MouseDown.'
Assert-Source $boardCpp 'EndlessAiDisableForManualInput\(\)' 'Board input must permanently disable Endless AI after manual gameplay input.'

Assert-Source $chooserH 'ChooseEndlessAiSeeds\(' 'Seed chooser must expose the AI deck setup entry point.'
Assert-Source $chooserCpp 'ChooseEndlessAiSeeds\(' 'Seed chooser must implement AI deck setup.'
Assert-Source $chooserCpp 'EndlessAiChooseSeeds\(' 'Seed chooser must delegate deck eligibility to Board.'
Assert-Source $chooserCpp 'SEED_IMITATER' 'The sustain deck must configure the imitater card.'

Assert-Source $saveCpp 'BOARD_FIELD_GLOVE_COOLDOWN,\s*\r?\n\s*BOARD_FIELD_ENDLESS_AI_MANUALLY_DISABLED' 'SaveGame must append the AI override field without renumbering existing fields.'
Assert-Source $saveCpp 'case BOARD_FIELD_ENDLESS_AI_MANUALLY_DISABLED:[\s\S]{0,180}SyncBool\(theBoard->mEndlessAiManuallyDisabled\)' 'SaveGame must restore the manual AI override.'
Assert-Source $saveCpp 'AppendFieldWithSync\(aBlob, BOARD_FIELD_ENDLESS_AI_MANUALLY_DISABLED,[\s\S]{0,180}mEndlessAiManuallyDisabled' 'SaveGame must write the manual AI override.'

Write-Output 'Endless AI source checks passed.'
