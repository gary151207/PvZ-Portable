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

#ifndef __SYSFONTFALLBACK_H__
#define __SYSFONTFALLBACK_H__

#include <cstdint>
#include <map>

namespace Sexy
{

class Image;
class MemoryImage;

// 系统字体兜底：
// 游戏里的文字用的是 pak 里那份位图字库（ImageFont），字表是固定的。
// 字库里没有的字（例如「杨」「巡」「菌」等中文）过去只能画成空白。
// 这里用操作系统自带的字体把缺字现渲染成一张带 alpha 的小图并按 (字高, 字符) 缓存，
// 绘制时当普通贴图用（白色字形 + alpha，交给 Graphics 按文字颜色着色），
// 于是缺字也能正常显示。
//
// 目前只有 Windows（GDI）实现；其它平台 Available() 返回 false，
// 调用方保持原行为（缺字仍画空白），不影响可移植性。
class SysFontFallback
{
public:
	struct Glyph
	{
		MemoryImage*	mImage;	// 白色字形 + alpha
		int		mAdvance;	// 步进宽度（像素）
		int		mAscent;	// 字形位图顶到基线的距离（用于对齐基线）
		int		mHeight;	// 位图高度
	};

public:
	static SysFontFallback*	Get();

	// 没有可用实现（非 Windows / 系统字体取不到）时返回 false
	bool					Available();

	// 取 theChar 在 thePixelHeight 像素高下的字形；该字不可渲染时返回 nullptr。
	// 返回的字形归缓存所有，进程内长期有效。
	Glyph*					GetGlyph(char32_t theChar, int thePixelHeight);

private:
	SysFontFallback();

	bool					RenderGlyph(char32_t theChar, int thePixelHeight, Glyph& theGlyph);

private:
	bool						mAvailable;
	std::map<uint64_t, Glyph>	mGlyphs;
};

}

#endif //__SYSFONTFALLBACK_H__
