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

#ifndef __TRUETYPEFONT_H__
#define __TRUETYPEFONT_H__

#include "Common.h"
#include "Font.h"

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace Sexy
{

class Image;
class MemoryImage;

// A _Font implementation that rasterizes glyphs from a TrueType font file with
// stb_truetype. One instance covers exactly one pixel size; instances are meant
// to be shared through std::shared_ptr (see ModFont), because the glyph atlas is
// built lazily and reused by every wrapper that needs the same size.
//
// Glyphs are drawn the same way ImageFont draws its bitmap glyphs: through
// g->SetColorizeImages(true) + g->DrawImage(atlas, ...), with the coverage in
// the alpha channel of a runtime-created MemoryImage.
class TrueTypeFont : public _Font
{
public:
	TrueTypeFont(std::shared_ptr<const std::vector<uint8_t>> theFontData, int thePixelSize, bool theOutline);
	virtual ~TrueTypeFont();

	// Looks up (and rasterizes on first use) one glyph. Returns false when the
	// font has no glyph for theChar.
	bool	GetGlyph(char32_t theChar, Rect& theSrcRect, int& theAdvance, int& theOffsetX, int& theOffsetY);

	Image*	GetAtlasImage() const;
	int		GetPixelSize() const { return mPixelSize; }
	bool	IsValid() const;

	// Pixel size whose ascent matches theAscent of the original bitmap font, so that
	// text keeps sitting on the same baseline as before the font was replaced.
	// Falls back to theFallbackHeight when the font metrics are unusable.
	static int PixelSizeForAscent(const std::vector<uint8_t>& theFontData, int theAscent, int theFallbackHeight);

	// Draws theString and returns the advanced width. theY is the baseline, matching
	// the convention of ImageFont::DrawString (a glyph's ink top sits GetAscent() above it).
	int		DrawStringEx(Graphics* g, int theX, int theY, const std::string& theString, const Color& theColor);

	virtual int		StringWidth(const std::string& theString);
	virtual int		CharWidth(char32_t theChar);
	virtual int		CharWidthKern(char32_t theChar, char32_t thePrevChar);
	virtual void	DrawString(Graphics* g, int theX, int theY, const std::string& theString, const Color& theColor, const Rect& theClipRect);
	virtual _Font*	Duplicate();

private:
	struct Impl;

	struct GlyphInfo
	{
		Rect	mSrcRect;
		int		mAdvance;
		int		mOffsetX;
		int		mOffsetY;
		bool	mHasGlyph;

		GlyphInfo()
			: mSrcRect(0, 0, 0, 0), mAdvance(0), mOffsetX(0), mOffsetY(0), mHasGlyph(false) {}
	};

	std::unique_ptr<Impl>					mImpl;
	std::shared_ptr<const std::vector<uint8_t>>	mFontData;
	int										mPixelSize;
	bool									mOutline;
	MemoryImage*							mAtlasImage;
	int										mAtlasSize;
	int										mPenX;
	int										mPenY;
	int										mRowHeight;
	std::map<char32_t, GlyphInfo>			mGlyphs;

	bool	AllocateAtlas(int theSize);
	bool	GrowAtlas();
	bool	HasGlyph(char32_t theChar) const;
	bool	PlaceGlyph(char32_t theChar, GlyphInfo& theGlyph);
	void	ApplyOutlineDilation(uint8_t* theCoverage, int theWidth, int theHeight) const;
};

}

#endif //__TRUETYPEFONT_H__
