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

#include "EndlessSlot.h"
#include "../../ConstEnums.h"
#include "../../LawnApp.h"
#include "../LawnCommon.h"
#include "../Widget/ChallengeScreen.h"
#include "../../Sexy.TodLib/TodStringFile.h"
#include "../../SexyAppFramework/Common.h"
#include "../../SexyAppFramework/SexyAppBase.h"

#include <cstdio>
#include <ctime>

bool IsEndlessGameMode(GameMode theGameMode)
{
    return LawnApp::IsSurvivalEndless(theGameMode) ||
        LawnApp::IsEndlessIZombie(theGameMode) ||
        LawnApp::IsEndlessScaryPotter(theGameMode);
}

std::string GetEndlessSaveName(GameMode theGameMode, int theProfileId, int theSlot)
{
    return GetAppDataPath(StrFormat("userdata/game%d_%d_s%d.v4", theProfileId, static_cast<int>(theGameMode), theSlot));
}

std::string GetEndlessMetaName(GameMode theGameMode, int theProfileId, int theSlot)
{
    return GetAppDataPath(StrFormat("userdata/game%d_%d_s%d.meta", theProfileId, static_cast<int>(theGameMode), theSlot));
}

std::string GetEndlessAutoName(GameMode theGameMode, int theSlot)
{
    int aChallengeIndex = static_cast<int>(theGameMode) - static_cast<int>(GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_1);
    std::string aModeName = TodStringTranslate(GetChallengeDefinition(aChallengeIndex).mChallengeName);
    return aModeName + StrFormat(" 存档%d", theSlot + 1);
}

bool EndlessSlotHasSave(GameMode theGameMode, int theProfileId, int theSlot)
{
    return gSexyAppBase->FileExists(GetEndlessSaveName(theGameMode, theProfileId, theSlot));
}

static void SetCurrentTimestamp(std::string& theTimestamp)
{
    time_t aNow = time(nullptr);
    char aBuf[64];
    strftime(aBuf, sizeof(aBuf), "%Y-%m-%d %H:%M", localtime(&aNow));
    theTimestamp = aBuf;
}

bool LoadEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, EndlessSlotMeta& theMeta)
{
    std::string aFilePath = GetEndlessMetaName(theGameMode, theProfileId, theSlot);
    FILE* aFile = fopen(aFilePath.c_str(), "rb");
    if (!aFile)
        return false;

    theMeta = EndlessSlotMeta();
    char aNameBuf[512];
    if (fgets(aNameBuf, sizeof(aNameBuf), aFile))
    {
        std::string aName = aNameBuf;
        if (!aName.empty() && (aName[aName.size() - 1] == '\n' || aName[aName.size() - 1] == '\r'))
            aName.resize(aName.size() - 1);
        if (!aName.empty() && aName[aName.size() - 1] == '\r')
            aName.resize(aName.size() - 1);
        theMeta.mName = aName;
    }

    char aLine[256];
    while (fgets(aLine, sizeof(aLine), aFile))
    {
        std::string aLineStr = aLine;
        if (!aLineStr.empty() && (aLineStr[aLineStr.size() - 1] == '\n' || aLineStr[aLineStr.size() - 1] == '\r'))
            aLineStr.resize(aLineStr.size() - 1);
        int aValue = 0;
        if (sscanf(aLineStr.c_str(), "flags=%d", &aValue) == 1)
            theMeta.mFlags = aValue;
        else if (sscanf(aLineStr.c_str(), "wave=%d", &aValue) == 1)
            theMeta.mWave = aValue;
        else if (sscanf(aLineStr.c_str(), "stage=%d", &aValue) == 1)
            theMeta.mStage = aValue;
        else if (aLineStr.compare(0, 10, "timestamp=") == 0)
            theMeta.mTimestamp = aLineStr.substr(10);
    }

    fclose(aFile);
    return true;
}

void SaveEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const EndlessSlotMeta& theMeta)
{
    MkDir(GetAppDataPath("userdata"));
    std::string aFilePath = GetEndlessMetaName(theGameMode, theProfileId, theSlot);
    FILE* aFile = fopen(aFilePath.c_str(), "wb");
    if (!aFile)
        return;
    fprintf(aFile, "%s\n", theMeta.mName.c_str());
    fprintf(aFile, "flags=%d\n", theMeta.mFlags);
    fprintf(aFile, "wave=%d\n", theMeta.mWave);
    fprintf(aFile, "stage=%d\n", theMeta.mStage);
    fprintf(aFile, "timestamp=%s\n", theMeta.mTimestamp.c_str());
    fclose(aFile);
}

void InitEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const std::string& theName)
{
    EndlessSlotMeta aMeta;
    aMeta.mName = theName;
    SetCurrentTimestamp(aMeta.mTimestamp);
    SaveEndlessSlotMeta(theGameMode, theProfileId, theSlot, aMeta);
}

void RefreshEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, int theFlags, int theWave, int theStage)
{
    EndlessSlotMeta aMeta;
    if (!LoadEndlessSlotMeta(theGameMode, theProfileId, theSlot, aMeta))
        aMeta.mName = GetEndlessAutoName(theGameMode, theSlot);
    aMeta.mFlags = theFlags;
    aMeta.mWave = theWave;
    aMeta.mStage = theStage;
    SetCurrentTimestamp(aMeta.mTimestamp);
    SaveEndlessSlotMeta(theGameMode, theProfileId, theSlot, aMeta);
}

void EraseEndlessSlot(GameMode theGameMode, int theProfileId, int theSlot)
{
    gSexyAppBase->EraseFile(GetEndlessSaveName(theGameMode, theProfileId, theSlot));
    gSexyAppBase->EraseFile(GetEndlessMetaName(theGameMode, theProfileId, theSlot));
}

void MigrateLegacyEndlessSave(GameMode theGameMode, int theProfileId)
{
    if (!IsEndlessGameMode(theGameMode))
        return;
    std::string aLegacyName = GetSavedGameName(theGameMode, theProfileId);
    std::string aSlot0Name = GetEndlessSaveName(theGameMode, theProfileId, 0);
    if (!gSexyAppBase->FileExists(aLegacyName) || gSexyAppBase->FileExists(aSlot0Name))
        return;

    // 读旧文件字节，写入槽位 0，再删旧文件（跨平台无 rename，用读写搬移）
    Buffer aBuffer;
    if (gSexyAppBase->ReadBufferFromFile(aLegacyName, &aBuffer, false) && aBuffer.GetDataLen() > 0)
    {
        FILE* aOut = fopen(aSlot0Name.c_str(), "wb");
        if (aOut)
        {
            fwrite(aBuffer.GetDataPtr(), 1, static_cast<size_t>(aBuffer.GetDataLen()), aOut);
            fclose(aOut);
            gSexyAppBase->EraseFile(aLegacyName);
        }
    }
}
