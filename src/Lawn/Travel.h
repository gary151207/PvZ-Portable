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
// 完整旅行模式（11 轮路线推进）只扩展本文件表与查询，不改 Board/LawnApp 模式分支。

struct TravelLevelDef
{
    int        mTravelId;        // 旅行内部关卡号（体验关 1..2，11 轮路线 3）
    GameMode   mGameMode;        // 映射的 GameMode
    int        mNumFlags;        // 旗帜波数
    int        mTotalWaves;      // 总波数
    bool       mFreeChooser;     // 自由选卡
    bool       mChooserPaged;    // 选卡器启用翻页（旅行专属）
    bool       mConveyorBelt;    // 传送带关卡（不选卡）
};

// ====================================================================================================
// 旅行模式（11 轮完整路线）
// 轮次存在 mChallenge->mSurvivalStage（0..10 → 第 1..11 轮），换轮复用生存模式的"植物保留 + 重新选卡"流程。
// ====================================================================================================

constexpr const int TRAVEL_JOURNEY_ROUNDS          = 11;   // 总轮数
constexpr const int TRAVEL_JOURNEY_WAVES_PER_ROUND = 10;   // 每轮波数
constexpr const int TRAVEL_JOURNEY_FLAGS_PER_ROUND = 2;    // 每轮大波（旗帜波）数

enum TravelJourneyMap : int32_t
{
    TRAVEL_JOURNEY_MAP_POOL,   // 白天泳池（BACKGROUND_3_POOL）
    TRAVEL_JOURNEY_MAP_FOG     // 夜晚迷雾泳池（BACKGROUND_4_FOG）
};

bool IsTravelJourneyLevel(GameMode theGameMode);
int  TravelJourneyRound(int theSurvivalStage);            // 1..11（越界钳制）
int  TravelJourneyMapForRound(int theRound);              // TRAVEL_JOURNEY_MAP_*
int  TravelJourneySpawnMultiplier(int theRound);          // 每过一轮出怪量翻倍：2^(轮次-1)
bool TravelJourneyIsBossRound(int theRound);              // 第 11 轮
// 该波是否为需要固定刷 BOSS 的大波（第 11 轮的第一大波与第二大波）
bool TravelJourneyIsBossFlagWave(int theWaveIndex, int theWavesPerFlag, int theNumWaves);

struct TravelPlantDef
{
    SeedType   mSeedType;        // 旅行专属植物
    bool       mUpgradeCard;     // 是紫卡升级卡
};

extern const TravelLevelDef gTravelLevelDefs[];
extern const int NUM_TRAVEL_LEVELS;
extern const TravelPlantDef gTravelPlantDefs[];
extern const int NUM_TRAVEL_PLANTS;

// 旅行专属僵尸：只在旅行模式里出场的类型（BOSS 路障射手僵尸等），
// 常规关卡（含斗蛐蛐）默认不出，需要时按名单取用，避免把"旅行专属"写死在各个调用点。
extern const ZombieType gTravelZombieDefs[];
extern const int NUM_TRAVEL_ZOMBIES;

bool IsTravelLevel(GameMode theGameMode);
const TravelLevelDef& GetTravelLevelDef(GameMode theGameMode);
bool IsTravelOnlySeed(SeedType theSeedType);
bool IsTravelOnlyZombie(ZombieType theZombieType);

#endif
