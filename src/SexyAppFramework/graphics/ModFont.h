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

#ifndef __MODFONT_H__
#define __MODFONT_H__

#include "Common.h"
#include "Font.h"

#include <memory>
#include <string>
#include <vector>

namespace Sexy
{

class ImageFont;
class TrueTypeFont;

// Replaces one of the game's original bitmap fonts with a TrueType font.
//
// The wrapper keeps the original bitmap font around, but only as a per-character
// fallback for codepoints the TrueType font does not cover; everything else is
// rasterized from the TrueType font. Metrics (ascent/height/line spacing) come
// from the TrueType font, so layout math stays self-consistent.
class ModFont : public _Font
{
public:
	ModFont(_Font* theOriginal, std::shared_ptr<TrueTypeFont> theTrueTypeFont);
	ModFont(const ModFont& theFont);
	virtual ~ModFont();

	virtual int		StringWidth(const std::string& theString);
	virtual int		CharWidth(char32_t theChar);
	virtual int		CharWidthKern(char32_t theChar, char32_t thePrevChar);
	virtual void	DrawString(Graphics* g, int theX, int theY, const std::string& theString, const Color& theColor, const Rect& theClipRect);
	virtual _Font*	Duplicate();

	_Font*			GetOriginalFont() const { return mOriginal; }
	TrueTypeFont*	GetTrueTypeFont() const { return mTrueTypeFont.get(); }

	// The original bitmap fonts can draw a glyph several times through different
	// layers (inset, shadow, ...), each with its own ColorMult/ColorAdd and offset.
	// Those layer styles are replicated for the replacement font.
	struct LayerStyle
	{
		Color	mColorMult;
		Color	mColorAdd;
		Point	mOffset;

		LayerStyle() : mColorMult(255, 255, 255, 255), mColorAdd(0, 0, 0, 0), mOffset(0, 0) {}
	};

	int				GetLayerStyleCount() const { return static_cast<int>(mLayerStyles.size()); }
	LayerStyle		GetLayerStyle(int theIndex) const;
	static Color	ApplyLayerColor(const Color& theColor, const LayerStyle& theStyle);

private:
	char32_t		MapChar(char32_t theChar) const;
	bool			GlyphInTrueTypeFont(char32_t theChar) const;

	_Font*							mOriginal;
	std::shared_ptr<TrueTypeFont>	mTrueTypeFont;
	ImageFont*						mOriginalImageFont;
	std::vector<LayerStyle>			mLayerStyles;
};

// Loads the font file that replaces the original bitmap fonts. Call once on the
// main thread before the game's fonts are created (LawnApp::InitHook). When no
// usable file is found, nothing is replaced and the game keeps its bitmap fonts.
bool		InitializeModFont(const std::string& theExplicitPath = std::string());

// True when a usable font file was loaded and font replacement is active.
bool		IsModFontEnabled();

// Wraps a freshly loaded bitmap font so that it renders with the replacement
// font. Takes ownership of theOriginal. Returns theOriginal unchanged when the
// replacement font is unavailable or the font is already wrapped.
_Font*		CreateModFont(_Font* theOriginal, const std::string& theResId);

// Path of the loaded font file ("" when disabled).
std::string	GetModFontPath();

// Human readable "resource id -> pixel size" mapping collected so far, for logs.
std::string	GetModFontSummary();

}

#endif //__MODFONT_H__
