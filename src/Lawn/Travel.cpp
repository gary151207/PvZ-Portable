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
#include "../Sexy.TodLib/TodCommon.h"
#include "../Sexy.TodLib/TodDebug.h"

const TravelLevelDef gTravelLevelDefs[] = {
    // mTravelId=1：体验关。夜间泳池 2 旗帜 6 波（第 1、6 波为旗帜波），传送带关卡
    { 1, GameMode::GAMEMODE_CHALLENGE_TRAVEL_1, 2, 6, false, false, true },
    // mTravelId=2：巨大坚果体验关。普通白天 3 旗帜 6 波，传送带（坚果/巨大坚果主题阵容）
    { 2, GameMode::GAMEMODE_CHALLENGE_TRAVEL_2, 3, 6, false, false, true },
    // mTravelId=3：11 轮完整旅行路线。每轮 10 波 2 旗帜，自由选卡（可翻页），非传送带
    { 3, GameMode::GAMEMODE_CHALLENGE_TRAVEL_JOURNEY, TRAVEL_JOURNEY_FLAGS_PER_ROUND, TRAVEL_JOURNEY_WAVES_PER_ROUND, true, true, false },
};
const int NUM_TRAVEL_LEVELS = sizeof(gTravelLevelDefs) / sizeof(gTravelLevelDefs[0]);

const TravelPlantDef gTravelPlantDefs[] = {
    { SeedType::SEED_FUMESHROOM_GROUP, true  },   // 大喷菇群（紫卡升级卡）
    { SeedType::SEED_GIANT_WALLNUT,    false },   // 巨大坚果（红卡：双坚果底座融合，见 2026-09-06-giant-wallnut）
    { SeedType::SEED_PEATER_1_5,       false },   // 1.5 发射手（红卡：无眉毛双发射手，可直接种下）
    { SeedType::SEED_ELECTRIC_GATLING_PEA, true }, // 究极电能机枪射手（红卡 + 升级卡：由机枪射手升级，200 阳光，100% 电能豌豆）
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

// ====================================================================================================
// 旅行模式（11 轮完整路线）
// ====================================================================================================

bool IsTravelJourneyLevel(GameMode theGameMode)
{
    return theGameMode == GameMode::GAMEMODE_CHALLENGE_TRAVEL_JOURNEY;
}

int TravelJourneyRound(int theSurvivalStage)
{
    return ClampInt(theSurvivalStage + 1, 1, TRAVEL_JOURNEY_ROUNDS);
}

int TravelJourneyMapForRound(int theRound)
{
    // 前五轮泳池（第 1-5 轮）→ 后五轮迷雾（第 6-10 轮）→ 第 11 轮回到泳池
    if (theRound >= 6 && theRound <= 10)
        return TRAVEL_JOURNEY_MAP_FOG;

    return TRAVEL_JOURNEY_MAP_POOL;
}

int TravelJourneySpawnMultiplier(int theRound)
{
    // 每过一轮出怪量翻倍：第 1 轮 ×1、第 2 轮 ×2 …… 第 11 轮 ×1024
    int aShift = ClampInt(theRound - 1, 0, 20);
    return 1 << aShift;
}

bool TravelJourneyIsBossRound(int theRound)
{
    return theRound >= TRAVEL_JOURNEY_ROUNDS;
}

bool TravelJourneyIsBossFlagWave(int theWaveIndex, int theWavesPerFlag, int theNumWaves)
{
    if (theWavesPerFlag <= 0 || theNumWaves <= 0)
        return false;

    // 旗帜波落在波号 theWavesPerFlag-1、2*theWavesPerFlag-1、……
    // 第 11 轮固定刷 BOSS 的只有"第一大波"与"第二大波"
    for (int aFlag = 0; aFlag < 2; aFlag++)
    {
        int aWave = (aFlag + 1) * theWavesPerFlag - 1;
        if (aWave >= theNumWaves)
            return false;
        if (theWaveIndex == aWave)
            return true;
    }

    return false;
}
