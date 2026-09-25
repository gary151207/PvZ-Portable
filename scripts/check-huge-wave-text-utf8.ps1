$ErrorActionPreference = 'Stop'

# "一大波僵尸即将来袭"（MESSAGE_STYLE_HUGE_WAVE）字幕的 UTF-8 检查：
#   该字幕是整个游戏里唯一走"逐字淡入"文字动画（REANIM_TEXT_FADE_ON）的提示，
#   布局与绘制原先按**字节**遍历，中文（多字节 UTF-8）每个字节都查不到字形，
#   于是旅行模式/其它模式的大波提示整体不显示（还会解引用字体矩形表的 end()）。
# 说明：本仓库无自动化测试框架（见 AGENTS.md），与其它 check-*.ps1 一样做源级断言；
#      实际观感（红字是否出现、逐字淡入是否正常）仍需手动进游戏验证。

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

$messageHeader = P 'src/Lawn/MessageWidget.h'
$messageCpp = P 'src/Lawn/MessageWidget.cpp'
$todCommon = P 'src/Sexy.TodLib/TodCommon.cpp'

# --- 头文件：每个文字动画记录字符的字节偏移 + 字符数（按码点计） ---
Assert-Source $messageHeader 'mTextReanimByteOffset\[MAX_MESSAGE_LENGTH\]' 'MessageWidget must record each glyph reanim''s byte offset in mLabel.'
Assert-Source $messageHeader 'mTextReanimCount' 'MessageWidget must track how many glyph reanims the current label owns.'
Assert-Source $messageHeader 'RebuildReanimLayout\(\)' 'MessageWidget must expose RebuildReanimLayout for the load path.'

# --- 构造函数：ID 数组必须逐个初始化（int 数组用 memset 只填了 1/4，余下是野 ID） ---
$ctorBody = [regex]::Match((Get-Content -Raw -LiteralPath $messageCpp), 'MessageWidget::MessageWidget[\s\S]*?\n\}')
if (-not $ctorBody.Success) { throw 'Could not find the MessageWidget constructor.' }
if ($ctorBody.Value -match 'memset\(mTextReanimID') { throw 'Do not memset the int array mTextReanimID (only the first MAX_MESSAGE_LENGTH bytes get written).' }
if ($ctorBody.Value -notmatch 'mTextReanimID\[i\] = ReanimationID::REANIMATIONID_NULL') { throw 'The constructor must initialize every mTextReanimID slot.' }
if ($ctorBody.Value -notmatch 'mTextReanimByteOffset\[i\] = 0') { throw 'The constructor must initialize every mTextReanimByteOffset slot.' }

# --- 布局：按 UTF-8 码点创建每字动画，并记录字节偏移 ---
$layoutBody = [regex]::Match((Get-Content -Raw -LiteralPath $messageCpp), 'void MessageWidget::LayoutReanimText\(\)[\s\S]*?\n\}')
if (-not $layoutBody.Success) { throw 'Could not find MessageWidget::LayoutReanimText.' }
if ($layoutBody.Value -notmatch 'UTF8DecodeNext') { throw 'LayoutReanimText must iterate the label by UTF-8 code point.' }
if ($layoutBody.Value -notmatch 'mTextReanimByteOffset\[aCharIdx\] = static_cast<int32_t>\(aCharStart\)') { throw 'LayoutReanimText must record each character''s byte offset.' }
if ($layoutBody.Value -notmatch 'mTextReanimCount = aCharIdx') { throw 'LayoutReanimText must publish the glyph count.' }
if ($layoutBody.Value -match 'CharWidth\(mLabel\[') { throw 'Per-byte CharWidth(mLabel[aPos]) mis-advances the cursor for multi-byte characters.' }

# --- 更新/绘制：按字符索引遍历，并用整字符的字节切片绘制 ---
$updateBody = [regex]::Match((Get-Content -Raw -LiteralPath $messageCpp), 'void MessageWidget::Update\(\)[\s\S]*?\n\}')
if (-not $updateBody.Success) { throw 'Could not find MessageWidget::Update.' }
if ($updateBody.Value -notmatch 'aCharIdx < mTextReanimCount') { throw 'Update must iterate glyph reanims by character index.' }

$drawBody = [regex]::Match((Get-Content -Raw -LiteralPath $messageCpp), 'void MessageWidget::DrawReanimatedText[\s\S]*?\n\}')
if (-not $drawBody.Success) { throw 'Could not find MessageWidget::DrawReanimatedText.' }
if ($drawBody.Value -notmatch 'aCharIdx < mTextReanimCount') { throw 'DrawReanimatedText must iterate glyph reanims by character index.' }
if ($drawBody.Value -notmatch 'mTextReanimByteOffset\[aCharIdx \+ 1\]') { throw 'DrawReanimatedText must slice mLabel by the next character''s byte offset.' }
if ($drawBody.Value -match 'aLetter\.append\(1, mLabel\[') { throw 'A single UTF-8 byte is not a character; never feed one byte to the glyph renderer.' }

# --- 字形绘制：按码点查表，缺字直接跳过（不能解引用 end()） ---
$matrixBody = [regex]::Match((Get-Content -Raw -LiteralPath $todCommon), 'void TodDrawStringMatrix[\s\S]*?\r?\n\}')
if (-not $matrixBody.Success) { throw 'Could not find TodDrawStringMatrix.' }
if ($matrixBody.Value -notmatch 'UTF8DecodeNext\(aFinalString') { throw 'TodDrawStringMatrix must decode the string by UTF-8 code point.' }
if ($matrixBody.Value -notmatch 'mScaledCharImageRects\.find\(aChar\)') { throw 'TodDrawStringMatrix must look the glyph rect up once and check the result.' }
if ($matrixBody.Value -notmatch 'aRectItr == aKernItr->mScaledCharImageRects\.end\(\)') { throw 'TodDrawStringMatrix must skip glyphs the font does not contain.' }
if ($matrixBody.Value -notmatch 'aRenderCommand->mSrc\[0\] = aRectItr->second\.mX;') { throw 'TodDrawStringMatrix must draw the looked-up glyph rect, not re-index the map.' }

# --- 读档：字节偏移不入存档，必须在读档后重建 ---
Assert-Source (P 'src/Lawn/System/SaveGame.cpp') 'mAdvice->RebuildReanimLayout\(\)' 'FixBoardAfterLoad must rebuild the advice glyph byte offsets after loading.'

Write-Host 'OK: huge wave text is laid out and drawn per UTF-8 code point.'
