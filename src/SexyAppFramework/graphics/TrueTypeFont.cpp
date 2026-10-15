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

#include "TrueTypeFont.h"

#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace Sexy
{

static const int kInitialAtlasSize = 512;
static const int kMaxAtlasSize = 2048;

// Linear coverage makes antialiased glyphs look thinner and greyer than the original
// crisp bitmap fonts, especially at the small sizes the game uses for numbers. This
// gamma darkens the coverage a little so the text keeps its original weight.
static const double kCoverageGamma = 0.75;

static const std::array<uint8_t, 256>& GetCoverageGammaTable()
{
	static const std::array<uint8_t, 256> aTable = []
	{
		std::array<uint8_t, 256> aResult{};
		for (int i = 0; i < 256; i++)
			aResult[i] = static_cast<uint8_t>(std::lround(std::pow(i / 255.0, kCoverageGamma) * 255.0));
		return aResult;
	}();

	return aTable;
}

struct TrueTypeFont::Impl
{
	stbtt_fontinfo	mFont;
	bool			mValid;

	Impl()
		: mValid(false)
	{
		memset(&mFont, 0, sizeof(mFont));
	}
};

TrueTypeFont::TrueTypeFont(std::shared_ptr<const std::vector<uint8_t>> theFontData, int thePixelSize, bool theOutline)
	: mImpl(new Impl()),
	  mFontData(std::move(theFontData)),
	  mPixelSize(std::max(1, thePixelSize)),
	  mOutline(theOutline),
	  mAtlasImage(nullptr),
	  mAtlasSize(0),
	  mPenX(0),
	  mPenY(0),
	  mRowHeight(0)
{
	mAscent = mPixelSize;
	mAscentPadding = 0;
	mHeight = mPixelSize;
	mLineSpacingOffset = 0;

	if (mFontData == nullptr || mFontData->empty())
		return;

	int anOffset = stbtt_GetFontOffsetForIndex(mFontData->data(), 0);
	if (anOffset < 0 || stbtt_InitFont(&mImpl->mFont, mFontData->data(), anOffset) == 0)
		return;

	mImpl->mValid = true;

	float aScale = stbtt_ScaleForPixelHeight(&mImpl->mFont, static_cast<float>(mPixelSize));
	int anAscent = 0;
	int aDescent = 0;
	int aLineGap = 0;
	stbtt_GetFontVMetrics(&mImpl->mFont, &anAscent, &aDescent, &aLineGap);

	mAscent = std::max(1, static_cast<int>(std::lround(anAscent * aScale)));
	mHeight = std::max(1, static_cast<int>(std::lround((anAscent - aDescent + aLineGap) * aScale)));

	AllocateAtlas(kInitialAtlasSize);
}

TrueTypeFont::~TrueTypeFont()
{
	delete mAtlasImage;
}

bool TrueTypeFont::IsValid() const
{
	return mImpl->mValid && mAtlasImage != nullptr;
}

Image* TrueTypeFont::GetAtlasImage() const
{
	return static_cast<Image*>(mAtlasImage);
}

bool TrueTypeFont::AllocateAtlas(int theSize)
{
	MemoryImage* aNewAtlas = new MemoryImage(gSexyAppBase);
	aNewAtlas->Create(theSize, theSize);

	uint32_t* aBits = aNewAtlas->GetBits();
	// MemoryImage::GetBits() allocates without zeroing; a fresh atlas must be fully transparent
	memset(aBits, 0, static_cast<size_t>(theSize) * static_cast<size_t>(theSize) * sizeof(uint32_t));
	aNewAtlas->BitsChanged();

	delete mAtlasImage;
	mAtlasImage = aNewAtlas;
	mAtlasSize = theSize;
	mPenX = 0;
	mPenY = 0;
	mRowHeight = 0;
	return true;
}

bool TrueTypeFont::GrowAtlas()
{
	if (mAtlasSize >= kMaxAtlasSize)
		return false;

	std::vector<char32_t> aCachedGlyphs;
	aCachedGlyphs.reserve(mGlyphs.size());
	for (const auto& aPair : mGlyphs)
	{
		if (aPair.second.mHasGlyph)
			aCachedGlyphs.push_back(aPair.first);
	}

	int aNewSize = mAtlasSize * 2;
	if (aNewSize > kMaxAtlasSize)
		aNewSize = kMaxAtlasSize;

	if (!AllocateAtlas(aNewSize))
		return false;

	mGlyphs.clear();
	for (char32_t aChar : aCachedGlyphs)
	{
		GlyphInfo aGlyph;
		if (PlaceGlyph(aChar, aGlyph))
			mGlyphs[aChar] = aGlyph;
	}

	return true;
}

void TrueTypeFont::ApplyOutlineDilation(uint8_t* theCoverage, int theWidth, int theHeight) const
{
	if (theWidth <= 0 || theHeight <= 0)
		return;

	std::vector<uint8_t> aSource(theCoverage, theCoverage + static_cast<size_t>(theWidth) * theHeight);
	for (int y = 0; y < theHeight; y++)
	{
		for (int x = 0; x < theWidth; x++)
		{
			uint8_t aMax = 0;
			for (int dy = -1; dy <= 1; dy++)
			{
				int aY = y + dy;
				if (aY < 0 || aY >= theHeight)
					continue;
				for (int dx = -1; dx <= 1; dx++)
				{
					int aX = x + dx;
					if (aX < 0 || aX >= theWidth)
						continue;
					aMax = std::max(aMax, aSource[static_cast<size_t>(aY) * theWidth + aX]);
				}
			}
			theCoverage[static_cast<size_t>(y) * theWidth + x] = aMax;
		}
	}
}

bool TrueTypeFont::HasGlyph(char32_t theChar) const
{
	if (!mImpl->mValid)
		return false;

	int aGlyphIndex = stbtt_FindGlyphIndex(&mImpl->mFont, static_cast<int>(theChar));
	return aGlyphIndex != 0 || theChar == 0;
}

bool TrueTypeFont::PlaceGlyph(char32_t theChar, GlyphInfo& theGlyph)
{
	if (!mImpl->mValid || mAtlasImage == nullptr)
		return false;

	int aGlyphIndex = stbtt_FindGlyphIndex(&mImpl->mFont, static_cast<int>(theChar));
	if (aGlyphIndex == 0 && theChar != 0)
		return false;

	float aScale = stbtt_ScaleForPixelHeight(&mImpl->mFont, static_cast<float>(mPixelSize));

	int anAdvance = 0;
	int aLeftSideBearing = 0;
	stbtt_GetGlyphHMetrics(&mImpl->mFont, aGlyphIndex, &anAdvance, &aLeftSideBearing);
	theGlyph.mAdvance = static_cast<int>(std::lround(anAdvance * aScale));

	int aWidth = 0;
	int aHeight = 0;
	int anOffsetX = 0;
	int anOffsetY = 0;
	unsigned char* aCoverage = stbtt_GetGlyphBitmap(&mImpl->mFont, aScale, aScale, aGlyphIndex,
		&aWidth, &aHeight, &anOffsetX, &anOffsetY);

	theGlyph.mOffsetX = anOffsetX;
	theGlyph.mOffsetY = anOffsetY;
	theGlyph.mHasGlyph = true;

	// Whitespace and other empty glyphs keep their advance but have no bitmap
	if (aCoverage == nullptr || aWidth <= 0 || aHeight <= 0)
	{
		if (aCoverage != nullptr)
			stbtt_FreeBitmap(aCoverage, nullptr);
		theGlyph.mSrcRect = Rect(0, 0, 0, 0);
		return true;
	}

	if (mOutline)
		ApplyOutlineDilation(aCoverage, aWidth, aHeight);

	// Match the weight of the original bitmap glyphs
	const std::array<uint8_t, 256>& aGammaTable = GetCoverageGammaTable();
	for (int aIndex = 0; aIndex < aWidth * aHeight; aIndex++)
		aCoverage[aIndex] = aGammaTable[aCoverage[aIndex]];

	// Shelf packing: move to the next row when the glyph does not fit
	if (mPenX + aWidth + 1 > mAtlasSize)
	{
		mPenX = 0;
		mPenY += mRowHeight + 1;
		mRowHeight = 0;
	}

	if (mPenY + aHeight + 1 > mAtlasSize)
	{
		stbtt_FreeBitmap(aCoverage, nullptr);
		return false;
	}

	uint32_t* aBits = mAtlasImage->GetBits();
	for (int aRow = 0; aRow < aHeight; aRow++)
	{
		const unsigned char* aSourceRow = aCoverage + static_cast<size_t>(aRow) * aWidth;
		uint32_t* aDestRow = aBits + static_cast<size_t>(mPenY + aRow) * mAtlasSize + mPenX;
		for (int aColumn = 0; aColumn < aWidth; aColumn++)
		{
			uint8_t aValue = aSourceRow[aColumn];
			// ARGB8888 with white RGB and the coverage in the alpha channel, so that
			// drawing with SetColorizeImages(true) tints the glyph with the text color
			aDestRow[aColumn] = (static_cast<uint32_t>(aValue) << 24) | 0x00FFFFFFu;
		}
	}

	mAtlasImage->BitsChanged();

	theGlyph.mSrcRect = Rect(mPenX, mPenY, aWidth, aHeight);
	mPenX += aWidth + 1;
	mRowHeight = std::max(mRowHeight, aHeight);

	stbtt_FreeBitmap(aCoverage, nullptr);
	return true;
}

bool TrueTypeFont::GetGlyph(char32_t theChar, Rect& theSrcRect, int& theAdvance, int& theOffsetX, int& theOffsetY)
{
	auto anItr = mGlyphs.find(theChar);
	if (anItr != mGlyphs.end())
	{
		if (!anItr->second.mHasGlyph)
			return false;

		theSrcRect = anItr->second.mSrcRect;
		theAdvance = anItr->second.mAdvance;
		theOffsetX = anItr->second.mOffsetX;
		theOffsetY = anItr->second.mOffsetY;
		return true;
	}

	if (!mImpl->mValid || mAtlasImage == nullptr)
		return false;

	// Missing glyphs never need atlas space, so they must not trigger a grow
	if (!HasGlyph(theChar))
	{
		mGlyphs[theChar] = GlyphInfo();
		return false;
	}

	GlyphInfo aGlyph;
	if (!PlaceGlyph(theChar, aGlyph))
	{
		// Either the atlas is full (grow and retry) or the font has no such glyph
		if (GrowAtlas() && PlaceGlyph(theChar, aGlyph))
		{
			// placed after growing
		}
		else
		{
			aGlyph = GlyphInfo();
			mGlyphs[theChar] = aGlyph;
			return false;
		}
	}

	mGlyphs[theChar] = aGlyph;
	theSrcRect = aGlyph.mSrcRect;
	theAdvance = aGlyph.mAdvance;
	theOffsetX = aGlyph.mOffsetX;
	theOffsetY = aGlyph.mOffsetY;
	return true;
}

int TrueTypeFont::PixelSizeForAscent(const std::vector<uint8_t>& theFontData, int theAscent, int theFallbackHeight)
{
	int aFallback = std::min(std::max(theFallbackHeight, 6), 96);

	if (theFontData.empty() || theAscent <= 0)
		return aFallback;

	stbtt_fontinfo aFont;
	int anOffset = stbtt_GetFontOffsetForIndex(theFontData.data(), 0);
	if (anOffset < 0 || stbtt_InitFont(&aFont, theFontData.data(), anOffset) == 0)
		return aFallback;

	int anAscentMetric = 0;
	int aDescentMetric = 0;
	int aLineGap = 0;
	stbtt_GetFontVMetrics(&aFont, &anAscentMetric, &aDescentMetric, &aLineGap);
	if (anAscentMetric <= 0 || anAscentMetric - aDescentMetric <= 0)
		return aFallback;

	// stbtt_ScaleForPixelHeight(H) yields scale = H / (ascent - descent), so the
	// resulting ascent in pixels is H * ascent / (ascent - descent)
	double aSize = static_cast<double>(theAscent) * (anAscentMetric - aDescentMetric) / static_cast<double>(anAscentMetric);
	return std::min(std::max(static_cast<int>(std::lround(aSize)), 6), 96);
}

int TrueTypeFont::DrawStringEx(Graphics* g, int theX, int theY, const std::string& theString, const Color& theColor)
{
	if (!IsValid())
		return 0;

	bool anOldColorize = g->GetColorizeImages();
	Color anOldColor = g->GetColor();
	g->SetColorizeImages(true);
	g->SetColor(theColor);

	int aPenX = theX;
	// ImageFont places glyphs relative to the baseline passed in as theY
	int aBaseline = theY;
	size_t anOffset = 0;
	char32_t aChar = 0;

	while (anOffset < theString.size())
	{
		size_t aCharStart = anOffset;
		if (!UTF8DecodeNext(theString, anOffset, aChar))
		{
			// Invalid UTF-8 byte: skip it (callers that iterate bytes themselves
			// cannot produce a drawable character here anyway)
			anOffset = aCharStart + 1;
			continue;
		}

		Rect aSrcRect;
		int anAdvance = 0;
		int anOffsetX = 0;
		int anOffsetY = 0;
		if (GetGlyph(aChar, aSrcRect, anAdvance, anOffsetX, anOffsetY))
		{
			if (aSrcRect.mWidth > 0 && aSrcRect.mHeight > 0)
				g->DrawImage(mAtlasImage, aPenX + anOffsetX, aBaseline + anOffsetY, aSrcRect);
			aPenX += anAdvance;
		}
	}

	g->SetColor(anOldColor);
	g->SetColorizeImages(anOldColorize);

	return aPenX - theX;
}

int TrueTypeFont::StringWidth(const std::string& theString)
{
	int aWidth = 0;
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
		aWidth += CharWidth(aChar);
	}

	return aWidth;
}

int TrueTypeFont::CharWidth(char32_t theChar)
{
	Rect aSrcRect;
	int anAdvance = 0;
	int anOffsetX = 0;
	int anOffsetY = 0;
	if (!GetGlyph(theChar, aSrcRect, anAdvance, anOffsetX, anOffsetY))
		return 0;

	return anAdvance;
}

int TrueTypeFont::CharWidthKern(char32_t theChar, char32_t thePrevChar)
{
	(void)thePrevChar;	// Kerning is intentionally not applied: predictable metrics first
	return CharWidth(theChar);
}

void TrueTypeFont::DrawString(Graphics* g, int theX, int theY, const std::string& theString, const Color& theColor, const Rect& theClipRect)
{
	(void)theClipRect;
	DrawStringEx(g, theX, theY, theString, theColor);
}

_Font* TrueTypeFont::Duplicate()
{
	// Wrappers share instances through std::shared_ptr, so this is only a safety net
	// for callers that duplicate a font on their own.
	return new TrueTypeFont(mFontData, mPixelSize, mOutline);
}

}
