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

#ifndef __PROJECTILE_H__
#define __PROJECTILE_H__

#include <cstdint>
#include "../ConstEnums.h"
#include "GameObject.h"

class Plant;
class Zombie;
namespace Sexy
{
    class Graphics;
};
using namespace Sexy;

class ProjectileDefinition
{
public:
    ProjectileType          mProjectileType;
    int32_t                 mImageRow;
    int32_t                 mDamage;
};
extern ProjectileDefinition gProjectileDefinition[NUM_PROJECTILES];

class Projectile : public GameObject
{
public:
    int32_t                 mFrame;
    int32_t                 mNumFrames;
    int32_t                 mAnimCounter;
    float                   mPosX;
    float                   mPosY;
    float                   mPosZ;
    float                   mVelX;
    float                   mVelY;
    float                   mVelZ;
    float                   mAccZ;
    float                   mShadowY;
    float                   mBaseY;
    bool                    mDead;
    int32_t                 mAnimTicksPerFrame;
    ProjectileMotion        mMotionType;
    ProjectileType          mProjectileType;
    int32_t                 mProjectileAge;
    int32_t                 mClickBackoffCounter;
    float                   mRotation;
    float                   mRotationSpeed;
    bool                    mOnHighGround;
    int32_t                 mDamageRangeFlags;
    int32_t                 mHitTorchwoodGridX;
    AttachmentID            mAttachmentID;
    float                   mCobTargetX;
    int32_t                 mCobTargetRow;
    ZombieID                mTargetZombieID;
    int32_t                 mLastPortalX;
    int32_t                 mDamageOverride = 0;
    float                   mRenderScale = 1.0f;
    int32_t                 mPenetrations = 0;
    ZombieID                mLastHitZombieID = ZombieID::ZOMBIEID_NULL;
    // 究极电能杨桃的电能星星：命中后"钉"在该僵尸上的剩余游戏刻（0 = 尚未命中，正常飞行）。
    // 追加在 SaveGame 的 SyncProjectileTailPortable 末尾，旧存档读不到时按 0 处理。
    int32_t                 mLingerCountdown = 0;
    // 电能星星当前是否正"钉"在僵尸身上（false 且 mLingerCountdown > 0 = 目标已死、正在改追下一个）
    bool                    mElectricStarStuck = false;

public:
    Projectile();
    ~Projectile();

    void                    ProjectileInitialize(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theProjectileType);
    void                    Update();
    void                    Draw(Graphics* g);
    void                    DrawShadow(Graphics* g);
    void                    Die();
    void                    DoImpact(Zombie* theZombie);
    void                    UpdateMotion();
    void                    CheckForCollision();
    Zombie*                 FindCollisionTarget();
    void                    UpdateLobMotion();
    void                    CheckForHighGround();
    bool                    CantHitHighGround();
    void                    DoSplashDamage(Zombie* theZombie);
    ProjectileDefinition&   GetProjectileDef();
    unsigned int            GetDamageFlags(Zombie* theZombie/* = nullptr*/);
    Rect                    GetProjectileRect();
    void                    UpdateNormalMotion();
    Plant*                  FindCollisionTargetPlant();
    void                    ConvertToFireball(int theGridX);
    void                    ConvertToPea(int theGridX);
    bool                    IsSplashDamage(Zombie* theZombie/* = nullptr*/);
    void                    PlayImpactSound(Zombie* theZombie);
    bool                    IsZombieHitBySplash(Zombie* theZombie);
    bool                    PeaAboutToHitTorchwood();
    void                    FindNewHomingTarget();
    bool                    IsElectricStarStuck() const { return mElectricStarStuck; }
    void                    StartElectricStarLinger(Zombie* theZombie);
    void                    UpdateElectricStarLinger();
    bool                    RetargetElectricStar();

};

#endif
