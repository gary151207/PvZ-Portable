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

#ifndef __ENDLESSSLOT_H__
#define __ENDLESSSLOT_H__

#include "../../ConstEnums.h"
#include <string>

struct EndlessSlotMeta
{
    std::string mName;
    int mFlags = 0;   // 已过旗帜数（生存无尽）
    int mWave = 0;    // 当前波数（生存无尽）
    int mStage = 0;   // 阶段数（谜题无尽）
    std::string mTimestamp;
};

bool        IsEndlessGameMode(GameMode theGameMode);
std::string GetEndlessSaveName(GameMode theGameMode, int theProfileId, int theSlot);
std::string GetEndlessMetaName(GameMode theGameMode, int theProfileId, int theSlot);
std::string GetEndlessAutoName(GameMode theGameMode, int theSlot);
bool        EndlessSlotHasSave(GameMode theGameMode, int theProfileId, int theSlot);
bool        LoadEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, EndlessSlotMeta& theMeta);
void        SaveEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const EndlessSlotMeta& theMeta);
void        InitEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const std::string& theName);
void        RefreshEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, int theFlags, int theWave, int theStage);
void        EraseEndlessSlot(GameMode theGameMode, int theProfileId, int theSlot);
void        MigrateLegacyEndlessSave(GameMode theGameMode, int theProfileId);

#endif
