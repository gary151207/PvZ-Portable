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
    // 究极电能星星果的电能星星：命中后"钉"在该僵尸上的剩余游戏刻（0 = 尚未命中，正常飞行）。
    // 追加在 SaveGame 的 SyncProjectileTailPortable 末尾，旧存档读不到时按 0 处理。
    int32_t                 mLingerCountdown = 0;
    // 电能星星当前是否正"钉"在僵尸身上（false 且 mLingerCountdown > 0 = 目标已死、正在改追下一个）
    bool                    mElectricStarStuck = false;
    // 电能星星的伤害节奏：距离下一次结算电能伤害还剩多少游戏刻。
    // 每帧在 Projectile::Update 里自减，归零才算一次伤害并把节奏重置为
    // ELECTRIC_DAMAGE_INTERVAL_TICKS（15 刻 = 0.15 秒）；中间那些帧只跟随目标、不掉血。
    // 0 = 本帧可以结算（刚钉住时先打一次，之后按 0.15 秒的间隔）。
    // 电能豌豆不用这个计数器：它直接按 mProjectileAge % ELECTRIC_DAMAGE_INTERVAL_TICKS 结算。
    int32_t                 mElectricDamageCountdown = 0;
    // 究极电能机枪射手 / 究极电能星星果的电能弹丸：链式闪电。
    // 节奏完全不新增存档字段，直接由 mProjectileAge 推导：
    //   - mProjectileAge % ELECTRIC_CHAIN_INTERVAL_TICKS == 0 → 本帧向周围最近 5 只僵尸放电
    //   - 同一次结算之后的 ELECTRIC_CHAIN_ARC_TICKS 刻内，Draw 里画出电弧
    //     （即 [k*INTERVAL, k*INTERVAL + ARC_TICKS) 为"有电弧帧"，越接近结算越亮）。
    // 该字段只是观感状态：每帧按它算电弧亮度，读档后按 0 = 本帧无电弧处理即可。
    int32_t                 mElectricChainFlash = 0;    // 本次放电的电弧还能亮几刻（每帧自减）
    // 射出这颗弹丸的植物（PLANTID_NULL = 不是植物射的，例如僵尸豌豆 / 火球转化）。
    // 链式闪电只认"究极电能机枪射手 / 究极电能星星果"，而电能豌豆（PROJECTILE_FIREPEA_RED）
    // 普通机枪射手也有 3% 概率打出来，光看弹丸类型区分不了，所以必须记来源。
    PlantID                 mSourcePlantID = PlantID::PLANTID_NULL;
    // 发射瞬间就定死的"这颗弹丸是不是究极电能形态射出来的"。
    // 为什么不能只靠 mSourcePlantID 现查：弹丸可以活得比植物久（电能星星要钉 2.5 秒），
    // 植物被吃掉后 mSourcePlantID 就查不到东西了，电弧会毫无道理地中途消失。
    // 所以这里缓存一份；反过来，如果 ID 查得到植物、但那株植物已经不是究极形态
    // （数组槽位被复用的极端情况），以现查结果为准并就地修正缓存，避免认错来源。
    bool                    mElectricChainSource = false;

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
    bool                    IsElectricChainSource();
    void                    UpdateElectricChainLightning();
    void                    DrawElectricChain(Graphics* g);
    static void             DrawAllElectricChains(Board* theBoard, Graphics* g);

};

#endif
