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

#ifndef __FILTEREFFECT_H__
#define __FILTEREFFECT_H__

#include <cstdint>
#include <map>

namespace Sexy
{
    class Image;
    class MemoryImage;
}
using namespace Sexy;

enum FilterEffect : int32_t
{
    FILTER_EFFECT_NONE = -1,
    FILTER_EFFECT_WASHED_OUT,
    FILTER_EFFECT_LESS_WASHED_OUT,
    FILTER_EFFECT_WHITE,
    // 紫火豌豆：把原版橙红色火豌豆贴图整体换成"色相 5.03（0..6 扇区）/ 饱和度 ×1.47"的紫火。
    // 与 FILTER_EFFECT_WHITE 一样按 (原图, 滤镜) 全局缓存，因此每颗弹丸共用同一份结果，无额外开销。
    FILTER_EFFECT_FIREPEA_PURPLE,
    NUM_FILTER_EFFECTS
};

typedef std::map<Image*, Image*> ImageFilterMap;
extern ImageFilterMap gFilterMap[FilterEffect::NUM_FILTER_EFFECTS];

void                FilterEffectInitForApp();
void                FilterEffectDisposeForApp();
void                FilterEffectDoLumSat(MemoryImage* theImage, float theLum, float theSat);
// 直接把整张图的色相设成 theHueSector（0..6 扇区，与 RGB_to_HSL 的 h*6 同一单位），并缩放饱和度。
// theHueSector < 0 表示保留原色相（只调饱和度）。
void                FilterEffectDoHueSat(MemoryImage* theImage, float theHueSector, float theSatMul);
/*inline*/ void     FilterEffectDoWashedOut(MemoryImage* theImage);
/*inline*/ void     FilterEffectDoLessWashedOut(MemoryImage* theImage);
void                FilterEffectDoWhite(MemoryImage* theImage);
MemoryImage*        FilterEffectCreateImage(Image* theImage, FilterEffect theFilterEffect);
Image*              FilterEffectGetImage(Image* theImage, FilterEffect theFilterEffect);

#endif
