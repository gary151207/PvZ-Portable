/*
 * Copyright (C) 2026 PvZ-Portable contributors
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

#include "ModFont.h"

#include "ImageFont.h"
#include "TrueTypeFont.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

namespace Sexy
{

// Per-font pixel height overrides for fine tuning the replacement font. Empty by
// default: the pixel height is derived from the original bitmap font's own
// rendered height, which keeps the text boxes roughly the same size.
struct ModFontSizeOverride
{
	const char*	mId;
	int			mPixelSize;
};

static const ModFontSizeOverride kFontSizeTable[] =
{
	// { "FONT_PICO129", 10 },
};

static std::shared_ptr<const std::vector<uint8_t>>	gModFontData;
static std::string									gModFontPath;
static std::string									gModFontSummary;
static bool											gModFontInitDone = false;
static std::mutex									gModFontMutex;
static std::map<std::pair<int, bool>, std::shared_ptr<TrueTypeFont>>	gModFontCache;

static bool IsTrueTypeSignature(const std::vector<uint8_t>& theData)
{
	if (theData.size() < 12)
		return false;

	uint32_t aTag = (static_cast<uint32_t>(theData[0]) << 24)
		| (static_cast<uint32_t>(theData[1]) << 16)
		| (static_cast<uint32_t>(theData[2]) << 8)
		| static_cast<uint32_t>(theData[3]);

	return aTag == 0x00010000u	// TrueType outlines
		|| aTag == 0x74727565u	// 'true'
		|| aTag == 0x4F54544Fu	// 'OTTO' (CFF outlines; stb_truetype handles the common cases)
		|| aTag == 0x74746366u;	// 'ttcf' (font collection)
}

static bool ReadWholeFile(const std::string& thePath, std::vector<uint8_t>& theOutData)
{
	std::ifstream aFile(PathFromU8(thePath), std::ios::binary);
	if (!aFile.is_open())
		return false;

	aFile.seekg(0, std::ios::end);
	std::streamoff aSize = aFile.tellg();
	if (aSize <= 0)
		return false;

	aFile.seekg(0, std::ios::beg);
	theOutData.resize(static_cast<size_t>(aSize));
	aFile.read(reinterpret_cast<char*>(theOutData.data()), aSize);
	return aFile.good() || aFile.eof();
}

bool InitializeModFont(const std::string& theExplicitPath)
{
	std::lock_guard<std::mutex> aLock(gModFontMutex);
	if (gModFontInitDone)
		return gModFontData != nullptr;

	gModFontInitDone = true;

	std::vector<std::string> aCandidates;
	if (!theExplicitPath.empty())
		aCandidates.push_back(theExplicitPath);
	aCandidates.push_back(GetResourcePath("properties/fzcq.ttf"));
	aCandidates.push_back(GetResourcePath("fonts/fzcq.ttf"));
	aCandidates.push_back(GetResourcePath("fzcq.ttf"));

	for (const std::string& aCandidate : aCandidates)
	{
		if (aCandidate.empty() || !FileExists(aCandidate))
			continue;

		std::vector<uint8_t> aData;
		if (!ReadWholeFile(aCandidate, aData))
			continue;

		if (!IsTrueTypeSignature(aData))
			continue;

		gModFontData = std::make_shared<const std::vector<uint8_t>>(std::move(aData));
		gModFontPath = aCandidate;
		break;
	}

	return gModFontData != nullptr;
}

bool IsModFontEnabled()
{
	return gModFontData != nullptr;
}

std::string GetModFontPath()
{
	std::lock_guard<std::mutex> aLock(gModFontMutex);
	return gModFontPath;
}

std::string GetModFontSummary()
{
	std::lock_guard<std::mutex> aLock(gModFontMutex);
	return gModFontSummary;
}

_Font* CreateModFont(_Font* theOriginal, const std::string& theResId)
{
	if (theOriginal == nullptr || !IsModFontEnabled())
		return theOriginal;

	if (dynamic_cast<ModFont*>(theOriginal) != nullptr)
		return theOriginal;

	int aPixelSize = 0;
	int anOriginalAscent = theOriginal->GetAscent();
	int anOriginalHeight = theOriginal->GetHeight();
	ImageFont* anImageFont = dynamic_cast<ImageFont*>(theOriginal);
	if (anImageFont != nullptr)
	{
		if (anOriginalAscent <= 0)
			anOriginalAscent = anImageFont->mPointSize;
		if (anOriginalHeight <= 0)
			anOriginalHeight = anImageFont->mPointSize;
	}

	// Size the replacement so that its ascent matches the original font's ascent:
	// that keeps the visual glyph height and the baseline where the game expects them.
	aPixelSize = TrueTypeFont::PixelSizeForAscent(*gModFontData, anOriginalAscent, anOriginalHeight);

	for (const ModFontSizeOverride& anOverride : kFontSizeTable)
	{
		if (theResId == anOverride.mId)
		{
			aPixelSize = anOverride.mPixelSize;
			break;
		}
	}

	bool anOutline = theResId.find("OUTLINE") != std::string::npos;

	std::shared_ptr<TrueTypeFont> aTrueTypeFont;
	{
		std::lock_guard<std::mutex> aLock(gModFontMutex);

		std::pair<int, bool> aKey(aPixelSize, anOutline);
		auto anItr = gModFontCache.find(aKey);
		if (anItr != gModFontCache.end())
			aTrueTypeFont = anItr->second;
		else
		{
			std::shared_ptr<TrueTypeFont> aNewFont = std::make_shared<TrueTypeFont>(gModFontData, aPixelSize, anOutline);
			if (aNewFont->IsValid())
			{
				gModFontCache[aKey] = aNewFont;
				aTrueTypeFont = aNewFont;

				std::string aLayerInfo = "no-imagefont";
				if (anImageFont != nullptr)
				{
					aLayerInfo = StrFormat("layers=%d", (int)anImageFont->mActiveLayerList.size());
					for (const ActiveFontLayer& anActive : anImageFont->mActiveLayerList)
					{
						if (anActive.mBaseFontLayer == nullptr)
							continue;
						aLayerInfo += StrFormat(" [mult=%d,%d,%d,%d add=%d,%d,%d,%d]",
							anActive.mBaseFontLayer->mColorMult.mRed, anActive.mBaseFontLayer->mColorMult.mGreen,
							anActive.mBaseFontLayer->mColorMult.mBlue, anActive.mBaseFontLayer->mColorMult.mAlpha,
							anActive.mBaseFontLayer->mColorAdd.mRed, anActive.mBaseFontLayer->mColorAdd.mGreen,
							anActive.mBaseFontLayer->mColorAdd.mBlue, anActive.mBaseFontLayer->mColorAdd.mAlpha);
					}
				}

				printf("[mod font] %s: %dpx (original ascent=%d height=%d point=%d) glyphAscent=%d glyphHeight=%d %s%s\n",
					theResId.c_str(), aPixelSize, anOriginalAscent, anOriginalHeight,
					anImageFont != nullptr ? anImageFont->mPointSize : 0,
					aNewFont->GetAscent(), aNewFont->GetHeight(), aLayerInfo.c_str(),
					anOutline ? " (outline)" : "");
			}
		}

		if (!theResId.empty() && aTrueTypeFont != nullptr)
		{
			if (!gModFontSummary.empty())
				gModFontSummary += ", ";
			gModFontSummary += theResId + " -> " + std::to_string(aPixelSize) + "px";
			if (anOutline)
				gModFontSummary += " (outline)";
		}
	}

	if (aTrueTypeFont == nullptr)
		return theOriginal;

	return new ModFont(theOriginal, aTrueTypeFont);
}

ModFont::ModFont(_Font* theOriginal, std::shared_ptr<TrueTypeFont> theTrueTypeFont)
	: mOriginal(theOriginal),
	  mTrueTypeFont(std::move(theTrueTypeFont)),
	  mOriginalImageFont(dynamic_cast<ImageFont*>(theOriginal))
{
	// Vertical metrics come from the original bitmap font: the game positions text
	// against the font's own ascent/height, so keeping them preserves every baseline
	// and line box that the layouts were tuned for. Both fonts share the convention
	// that the y passed to DrawString is the baseline.
	if (mOriginal != nullptr)
	{
		mAscent = mOriginal->GetAscent();
		mAscentPadding = mOriginal->GetAscentPadding();
		mHeight = mOriginal->GetHeight();
		mLineSpacingOffset = mOriginal->GetLineSpacingOffset();
	}
	else if (mTrueTypeFont != nullptr)
	{
		mAscent = mTrueTypeFont->GetAscent();
		mAscentPadding = mTrueTypeFont->GetAscentPadding();
		mHeight = mTrueTypeFont->GetHeight();
		mLineSpacingOffset = mTrueTypeFont->GetLineSpacingOffset();
	}

	// Replicate the original font's layer stack (shadow/inset layers included)
	if (mOriginalImageFont != nullptr)
	{
		for (const ActiveFontLayer& anActive : mOriginalImageFont->mActiveLayerList)
		{
			if (anActive.mBaseFontLayer == nullptr)
				continue;

			LayerStyle aStyle;
			aStyle.mColorMult = anActive.mBaseFontLayer->mColorMult;
			aStyle.mColorAdd = anActive.mBaseFontLayer->mColorAdd;
			aStyle.mOffset = anActive.mBaseFontLayer->mOffset;
			mLayerStyles.push_back(aStyle);
		}
	}

	if (mLayerStyles.empty())
		mLayerStyles.push_back(LayerStyle());
}

ModFont::ModFont(const ModFont& theFont)
	: _Font(theFont),
	  mOriginal(theFont.mOriginal != nullptr ? theFont.mOriginal->Duplicate() : nullptr),
	  mTrueTypeFont(theFont.mTrueTypeFont),
	  mOriginalImageFont(nullptr),
	  mLayerStyles(theFont.mLayerStyles)
{
	mOriginalImageFont = dynamic_cast<ImageFont*>(mOriginal);
}

ModFont::LayerStyle ModFont::GetLayerStyle(int theIndex) const
{
	if (theIndex < 0 || theIndex >= static_cast<int>(mLayerStyles.size()))
		return LayerStyle();

	return mLayerStyles[theIndex];
}

Color ModFont::ApplyLayerColor(const Color& theColor, const LayerStyle& theStyle)
{
	Color aColor;
	aColor.mRed = std::min(static_cast<int>(theColor.mRed) * theStyle.mColorMult.mRed / 255 + theStyle.mColorAdd.mRed, 255);
	aColor.mGreen = std::min(static_cast<int>(theColor.mGreen) * theStyle.mColorMult.mGreen / 255 + theStyle.mColorAdd.mGreen, 255);
	aColor.mBlue = std::min(static_cast<int>(theColor.mBlue) * theStyle.mColorMult.mBlue / 255 + theStyle.mColorAdd.mBlue, 255);
	aColor.mAlpha = std::min(static_cast<int>(theColor.mAlpha) * theStyle.mColorMult.mAlpha / 255 + theStyle.mColorAdd.mAlpha, 255);
	return aColor;
}

ModFont::~ModFont()
{
	delete mOriginal;
}

char32_t ModFont::MapChar(char32_t theChar) const
{
	if (mOriginalImageFont != nullptr)
		return mOriginalImageFont->GetMappedChar(theChar);

	return theChar;
}

bool ModFont::GlyphInTrueTypeFont(char32_t theChar) const
{
	if (mTrueTypeFont == nullptr)
		return false;

	Rect aSrcRect;
	int anAdvance = 0;
	int anOffsetX = 0;
	int anOffsetY = 0;
	return mTrueTypeFont->GetGlyph(MapChar(theChar), aSrcRect, anAdvance, anOffsetX, anOffsetY);
}

int ModFont::StringWidth(const std::string& theString)
{
	if (mTrueTypeFont == nullptr)
		return mOriginal != nullptr ? mOriginal->StringWidth(theString) : 0;

	int aWidth = 0;
	char32_t aPrevChar = 0;
	size_t anOffset = 0;
	char32_t aChar = 0;

	while (anOffset < theString.size())
	{
		size_t aCharStart = anOffset;
		if (!UTF8DecodeNext(theString, anOffset, aChar))
		{
			anOffset = aCharStart + 1;
			continue;
		}

		aWidth += CharWidthKern(aChar, aPrevChar);
		aPrevChar = aChar;
	}

	return aWidth;
}

int ModFont::CharWidth(char32_t theChar)
{
	return CharWidthKern(theChar, 0);
}

int ModFont::CharWidthKern(char32_t theChar, char32_t thePrevChar)
{
	if (mTrueTypeFont == nullptr)
		return mOriginal != nullptr ? mOriginal->CharWidthKern(theChar, thePrevChar) : 0;

	Rect aSrcRect;
	int anAdvance = 0;
	int anOffsetX = 0;
	int anOffsetY = 0;
	if (mTrueTypeFont->GetGlyph(MapChar(theChar), aSrcRect, anAdvance, anOffsetX, anOffsetY))
		return anAdvance;

	if (mOriginal == nullptr)
		return 0;

	// Characters the replacement font does not cover are measured with the
	// original bitmap font. Kerning against a character that came from the
	// replacement font is skipped, because the two fonts have unrelated metrics.
	bool aPrevInTrueType = (thePrevChar != 0) && GlyphInTrueTypeFont(thePrevChar);
	return mOriginal->CharWidthKern(theChar, aPrevInTrueType ? 0 : thePrevChar);
}

void ModFont::DrawString(Graphics* g, int theX, int theY, const std::string& theString, const Color& theColor, const Rect& theClipRect)
{
	if (mTrueTypeFont == nullptr)
	{
		if (mOriginal != nullptr)
			mOriginal->DrawString(g, theX, theY, theString, theColor, theClipRect);
		return;
	}

	int aPenX = theX;
	int aRunStart = -1;
	bool aRunInTrueType = false;

	auto aFlushRun = [&](size_t theEndOffset)
	{
		if (aRunStart < 0 || theEndOffset <= static_cast<size_t>(aRunStart))
		{
			aRunStart = -1;
			return;
		}

		std::string aRun = theString.substr(static_cast<size_t>(aRunStart), theEndOffset - static_cast<size_t>(aRunStart));
		if (aRunInTrueType)
		{
			// Draw once per original layer so shadow/inset layers survive the replacement.
			// theY is the baseline, exactly like ImageFont::DrawString.
			for (int aLayer = 0; aLayer < GetLayerStyleCount(); aLayer++)
			{
				const LayerStyle aStyle = GetLayerStyle(aLayer);
				mTrueTypeFont->DrawStringEx(g, aPenX + aStyle.mOffset.mX, theY + aStyle.mOffset.mY, aRun,
					ApplyLayerColor(theColor, aStyle));
			}

			aPenX += mTrueTypeFont->StringWidth(aRun);
		}
		else if (mOriginal != nullptr)
		{
			// Both fonts take the baseline as y, so no vertical adjustment is needed
			mOriginal->DrawString(g, aPenX, theY, aRun, theColor, theClipRect);
			aPenX += mOriginal->StringWidth(aRun);
		}

		aRunStart = -1;
	};

	size_t anOffset = 0;
	char32_t aChar = 0;
	while (anOffset < theString.size())
	{
		size_t aCharStart = anOffset;
		if (!UTF8DecodeNext(theString, anOffset, aChar))
		{
			// Invalid UTF-8 byte: nothing drawable, skip it
			anOffset = aCharStart + 1;
			continue;
		}

		bool aInTrueType = GlyphInTrueTypeFont(aChar);
		if (aRunStart >= 0 && aInTrueType != aRunInTrueType)
			aFlushRun(aCharStart);

		if (aRunStart < 0)
		{
			aRunStart = static_cast<int>(aCharStart);
			aRunInTrueType = aInTrueType;
		}
	}

	aFlushRun(theString.size());
}

_Font* ModFont::Duplicate()
{
	return new ModFont(*this);
}

}
