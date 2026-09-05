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

#ifndef __TRAVEL_H__
#define __TRAVEL_H__

#include "../ConstEnums.h"

// 旅行模式数据驱动子系统：关卡表 + 旅行专属植物表。
// 后续完整旅行模式（路线推进/进度存档）只扩展本文件表与查询，不改 Board/LawnApp 模式分支。

struct TravelLevelDef
{
    int        mTravelId;        // 旅行内部关卡号（将来 1..N 沿路线推进）
    GameMode   mGameMode;        // 映射的 GameMode
    int        mNumFlags;        // 旗帜波数
    int        mTotalWaves;      // 总波数
    bool       mFreeChooser;     // 自由选卡
    bool       mChooserPaged;    // 选卡器启用翻页（旅行专属）
    bool       mConveyorBelt;    // 传送带关卡（不选卡）
};

struct TravelPlantDef
{
    SeedType   mSeedType;        // 旅行专属植物
    bool       mUpgradeCard;     // 是紫卡升级卡
};

extern const TravelLevelDef gTravelLevelDefs[];
extern const int NUM_TRAVEL_LEVELS;
extern const TravelPlantDef gTravelPlantDefs[];
extern const int NUM_TRAVEL_PLANTS;

bool IsTravelLevel(GameMode theGameMode);
const TravelLevelDef& GetTravelLevelDef(GameMode theGameMode);
bool IsTravelOnlySeed(SeedType theSeedType);

#endif
