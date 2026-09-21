/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#include "SysFontFallback.h"
#include "MemoryImage.h"
#include <cstring>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace Sexy;

namespace
{
	// 缺字兜底字形的基线微调（像素，正数 = 往下）：位图字库里的中文身比 GDI 同号字略低，
	// 往下压一点才与周围的原生字对齐（调参点）。
	constexpr int FALLBACK_BASELINE_BIAS = 2;
}

#ifdef _WIN32

namespace
{
	// 候选系统字体：优先中文雅黑/黑体，再退到日韩字体（覆盖假名、谚文）
	const wchar_t* gFallbackFaces[] = {
		L"Microsoft YaHei UI",
		L"Microsoft YaHei",
		L"SimHei",
		L"SimSun",
		L"Malgun Gothic",
		L"MS Gothic",
	};

	bool FaceCanRender(HDC theDC, HFONT theFont, char32_t theChar)
	{
		HGDIOBJ anOldFont = SelectObject(theDC, theFont);
		WCHAR aWide = static_cast<WCHAR>(theChar);
		WORD anIndex = 0;
		DWORD aResult = GetGlyphIndicesW(theDC, &aWide, 1, &anIndex, GGI_MARK_NONEXISTING_GLYPHS);
		SelectObject(theDC, anOldFont);

		return aResult != GDI_ERROR && anIndex != 0xFFFF;
	}
}

#endif // _WIN32

SysFontFallback* SysFontFallback::Get()
{
	static SysFontFallback* aInstance = nullptr;
	if (aInstance == nullptr)
		aInstance = new SysFontFallback();

	return aInstance;
}

SysFontFallback::SysFontFallback()
{
#ifdef _WIN32
	mAvailable = true;
#else
	mAvailable = false;
#endif
}

bool SysFontFallback::Available()
{
	return mAvailable;
}

SysFontFallback::Glyph* SysFontFallback::GetGlyph(char32_t theChar, int thePixelHeight)
{
	// 只兜底 BMP 内的字；代理对（emoji 等）交给原来的行为
	if (!mAvailable || theChar < 0x20 || theChar > 0xFFFF || thePixelHeight <= 0)
		return nullptr;

	uint64_t aKey = (static_cast<uint64_t>(thePixelHeight) << 32) | static_cast<uint64_t>(theChar);
	auto anItr = mGlyphs.find(aKey);
	if (anItr != mGlyphs.end())
		return anItr->second.mImage != nullptr ? &anItr->second : nullptr;

	Glyph aGlyph = { nullptr, 0, 0, 0 };
	if (RenderGlyph(theChar, thePixelHeight, aGlyph))
		mGlyphs[aKey] = aGlyph;
	else
		mGlyphs[aKey] = Glyph{ nullptr, 0, 0, 0 };   // 渲染失败的字符也记下来，避免每次绘制都重试

	return aGlyph.mImage != nullptr ? &mGlyphs[aKey] : nullptr;
}

bool SysFontFallback::RenderGlyph(char32_t theChar, int thePixelHeight, Glyph& theGlyph)
{
#ifdef _WIN32
	HDC aDC = CreateCompatibleDC(nullptr);
	if (aDC == nullptr)
		return false;

	HFONT aFont = nullptr;
	HGDIOBJ anOldFont = nullptr;
	for (const wchar_t* aFace : gFallbackFaces)
	{
		HFONT aCandidate = CreateFontW(
			-thePixelHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, aFace);
		if (aCandidate == nullptr)
			continue;

		if (FaceCanRender(aDC, aCandidate, theChar))
		{
			aFont = aCandidate;
			anOldFont = SelectObject(aDC, aFont);
			break;
		}

		DeleteObject(aCandidate);
	}

	if (aFont == nullptr)
	{
		DeleteDC(aDC);
		return false;
	}

	WCHAR aWide = static_cast<WCHAR>(theChar);
	TEXTMETRICW aMetrics;
	GetTextMetricsW(aDC, &aMetrics);
	SIZE aSize = { 0, 0 };
	GetTextExtentPoint32W(aDC, &aWide, 1, &aSize);

	int aWidth = std::max<int>(1, aSize.cx);
	int aHeight = std::max<int>(1, std::max<int>(aSize.cy, aMetrics.tmHeight));

	BITMAPINFO aInfo;
	memset(&aInfo, 0, sizeof(aInfo));
	aInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	aInfo.bmiHeader.biWidth = aWidth;
	aInfo.bmiHeader.biHeight = -aHeight;   // 自上而下，行序与贴图一致
	aInfo.bmiHeader.biPlanes = 1;
	aInfo.bmiHeader.biBitCount = 32;
	aInfo.bmiHeader.biCompression = BI_RGB;

	void* aBits = nullptr;
	HBITMAP aBitmap = CreateDIBSection(aDC, &aInfo, DIB_RGB_COLORS, &aBits, nullptr, 0);
	if (aBitmap == nullptr || aBits == nullptr)
	{
		SelectObject(aDC, anOldFont);
		DeleteObject(aFont);
		DeleteDC(aDC);
		return false;
	}

	HGDIOBJ anOldBitmap = SelectObject(aDC, aBitmap);
	memset(aBits, 0, static_cast<size_t>(aWidth) * aHeight * 4);

	SetBkMode(aDC, TRANSPARENT);
	SetTextColor(aDC, RGB(255, 255, 255));
	TextOutW(aDC, 0, 0, &aWide, 1);

	// GDI 只写 RGB，alpha 要自己从亮度取（白字黑底 → 覆盖率）
	std::vector<uint32_t> aPixels(static_cast<size_t>(aWidth) * aHeight);
	const uint8_t* aSrc = static_cast<const uint8_t*>(aBits);
	for (int i = 0; i < aWidth * aHeight; i++)
	{
		uint8_t aBlue = aSrc[i * 4 + 0];
		uint8_t aGreen = aSrc[i * 4 + 1];
		uint8_t aRed = aSrc[i * 4 + 2];
		uint8_t anAlpha = std::max(aRed, std::max(aGreen, aBlue));
		aPixels[i] = (static_cast<uint32_t>(anAlpha) << 24) | 0x00FFFFFF;
	}

	theGlyph.mImage = new MemoryImage();
	theGlyph.mImage->mBits = new uint32_t[static_cast<size_t>(aWidth) * aHeight + 1];
	theGlyph.mImage->mWidth = aWidth;
	theGlyph.mImage->mHeight = aHeight;
	theGlyph.mImage->mHasTrans = true;
	theGlyph.mImage->mHasAlpha = true;
	memcpy(theGlyph.mImage->mBits, aPixels.data(), static_cast<size_t>(aWidth) * aHeight * 4);
	theGlyph.mImage->mBits[static_cast<size_t>(aWidth) * aHeight] = MEMORYCHECK_ID;
	theGlyph.mAdvance = aSize.cx;
	theGlyph.mAscent = aMetrics.tmAscent - FALLBACK_BASELINE_BIAS;   // 绘制时 y = 基线 - mAscent
	theGlyph.mHeight = aHeight;

	SelectObject(aDC, anOldBitmap);
	DeleteObject(aBitmap);
	SelectObject(aDC, anOldFont);
	DeleteObject(aFont);
	DeleteDC(aDC);

	return true;
#else
	(void)theChar;
	(void)thePixelHeight;
	(void)theGlyph;
	return false;
#endif
}
