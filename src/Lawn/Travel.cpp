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

#include "Travel.h"
#include "../Sexy.TodLib/TodDebug.h"

const TravelLevelDef gTravelLevelDefs[] = {
    // mTravelId=1：体验关。夜间泳池 2 旗帜 6 波（第 1、6 波为旗帜波），传送带关卡
    { 1, GameMode::GAMEMODE_CHALLENGE_TRAVEL_1, 2, 6, false, false, true },
};
const int NUM_TRAVEL_LEVELS = sizeof(gTravelLevelDefs) / sizeof(gTravelLevelDefs[0]);

const TravelPlantDef gTravelPlantDefs[] = {
    { SeedType::SEED_FUMESHROOM_GROUP, true  },   // 大喷菇群（紫卡升级卡）
    { SeedType::SEED_GIANT_WALLNUT,    false },   // 巨大坚果（红卡：双坚果底座融合，见 2026-09-06-giant-wallnut）
};
const int NUM_TRAVEL_PLANTS = sizeof(gTravelPlantDefs) / sizeof(gTravelPlantDefs[0]);

bool IsTravelLevel(GameMode theGameMode)
{
    for (int i = 0; i < NUM_TRAVEL_LEVELS; i++)
        if (gTravelLevelDefs[i].mGameMode == theGameMode)
            return true;
    return false;
}

const TravelLevelDef& GetTravelLevelDef(GameMode theGameMode)
{
    for (int i = 0; i < NUM_TRAVEL_LEVELS; i++)
        if (gTravelLevelDefs[i].mGameMode == theGameMode)
            return gTravelLevelDefs[i];
    TOD_ASSERT(false); // 非旅行关卡不应调用
    return gTravelLevelDefs[0];
}

bool IsTravelOnlySeed(SeedType theSeedType)
{
    for (int i = 0; i < NUM_TRAVEL_PLANTS; i++)
        if (gTravelPlantDefs[i].mSeedType == theSeedType)
            return true;
    return false;
}
