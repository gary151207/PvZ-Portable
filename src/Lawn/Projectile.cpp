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

#include "Board.h"
#include "Plant.h"
#include "Zombie.h"
#include "Cutscene.h"
#include "Projectile.h"
#include <algorithm>
#include <utility>
#include <vector>
#include "../LawnApp.h"
#include "../Resources.h"
#include "../GameConstants.h"
#include "../Sexy.TodLib/TodFoley.h"
#include "../Sexy.TodLib/TodCommon.h"
#include <climits>
#include "../Sexy.TodLib/TodDebug.h"
#include "../Sexy.TodLib/Reanimator.h"
#include "../Sexy.TodLib/FilterEffect.h"
#include "../Sexy.TodLib/Attachment.h"
#include "Widget/AchievementsScreen.h"

ProjectileDefinition gProjectileDefinition[] = {
	{ ProjectileType::PROJECTILE_PEA,           0,  20  },
	{ ProjectileType::PROJECTILE_SNOWPEA,       0,  20  },
	{ ProjectileType::PROJECTILE_CABBAGE,       0,  40  },
	{ ProjectileType::PROJECTILE_MELON,         0,  80  },
	{ ProjectileType::PROJECTILE_PUFF,          0,  20  },
	{ ProjectileType::PROJECTILE_WINTERMELON,   0,  80  },
	{ ProjectileType::PROJECTILE_FIREBALL,      0,  40  },
	{ ProjectileType::PROJECTILE_STAR,          0,  20  },
	{ ProjectileType::PROJECTILE_SPIKE,         0,  30  },
	{ ProjectileType::PROJECTILE_BASKETBALL,    0,  75  },
	{ ProjectileType::PROJECTILE_KERNEL,        0,  20  },
	{ ProjectileType::PROJECTILE_COBBIG,        0,  300 },
	{ ProjectileType::PROJECTILE_BUTTER,        0,  40  },
	{ ProjectileType::PROJECTILE_ZOMBIE_PEA,    0,  20  },
	{ ProjectileType::PROJECTILE_FIREPEA_RED,   0,  30  },
	{ ProjectileType::PROJECTILE_ELECTRIC_STAR, 0,  ELECTRIC_STAR_HIT_DAMAGE },
	{ ProjectileType::PROJECTILE_PURPLE_FIRE_PEA, 0, 65 },   // 紫火豌豆（火豌豆射手）：65 伤害 + 命中后僵尸易伤
	{ ProjectileType::PROJECTILE_LASER_PEA, 0, LASER_PEA_DAMAGE },   // 激光豌豆的贯穿光束：路径上每只僵尸 20 伤害
	{ ProjectileType::PROJECTILE_SPORESHROOM,     0, 40 }   // 孢子菇孢子：40 伤害 + 直接击杀后原格繁殖
};

Projectile::Projectile()
{
}

Projectile::~Projectile()
{
	AttachmentDie(mAttachmentID);
}

void Projectile::ProjectileInitialize(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theProjectileType)
{
	int aGridX = mBoard->PixelToGridXKeepOnBoard(theX, theY);
	mProjectileType = theProjectileType;
	mPosX = theX;
	mPosY = theY;
	mPosZ = 0.0f;
	mVelX = 0.0f;
	mVelY = 0.0f;
	mVelZ = 0.0f;
	mAccZ = 0.0f;
	mShadowY = mBoard->GridToPixelY(aGridX, theRow) + 67.0f;
	mBaseY = theY;
	mHitTorchwoodGridX = -1;
	mMotionType = ProjectileMotion::MOTION_STRAIGHT;
	mFrame = 0;
	mNumFrames = 1;
	mRow = theRow;
	mCobTargetX = 0.0f;
	mDamageRangeFlags = 0;
	mDead = false;
	mAttachmentID = AttachmentID::ATTACHMENTID_NULL;
	mCobTargetRow = 0;
	mTargetZombieID = ZombieID::ZOMBIEID_NULL;
	mOnHighGround = mBoard->mGridSquareType[aGridX][theRow] == GridSquareType::GRIDSQUARE_HIGH_GROUND;
	if (mBoard->StageHasRoof())
	{
		mShadowY -= 12.0f;
	}
	mRenderOrder = theRenderOrder;
	mRotation = 0.0f;
	mRotationSpeed = 0.0f;
	mWidth = 40;
	mHeight = 40;
	mProjectileAge = 0;
	mClickBackoffCounter = 0;
	mAnimTicksPerFrame = 0;
	mDamageOverride = 0;
	mRenderScale = 1.0f;

	mPenetrations = 0;
	mLastHitZombieID = ZombieID::ZOMBIEID_NULL;
	mLingerCountdown = 0;
	mElectricStarStuck = false;
	mElectricDamageCountdown = 0;
	mElectricChainFlash = 0;
	mSourcePlantID = PlantID::PLANTID_NULL;
	mElectricChainSource = false;
	mLaserPeaBeamCountdown = 0;
	if (mProjectileType == ProjectileType::PROJECTILE_SPIKE)
	{
		mPenetrations = 2;
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_LASER_PEA)
	{
		// 激光豌豆的光束：不移动（mVelX/Y/Z 全 0，由 Plant::Fire 再明确置零一次），
		// 只在出生那一帧结算一次伤害，然后作为纯视觉停留 LASER_PEA_BEAM_TICKS 帧。
		mLaserPeaBeamCountdown = LASER_PEA_BEAM_TICKS;
	}

	switch (mProjectileType)
	{
	case ProjectileType::PROJECTILE_CABBAGE:
	case ProjectileType::PROJECTILE_BUTTER:
		mRotation = -7 * PI / 25;  // DEG_TO_RAD(-50.4f);
		mRotationSpeed = RandRangeFloat(-0.08f, -0.02f);
		break;
	case ProjectileType::PROJECTILE_MELON:
	case ProjectileType::PROJECTILE_WINTERMELON:
		mRotation = -2 * PI / 5;  // DEG_TO_RAD(-72.0f);
		mRotationSpeed = RandRangeFloat(-0.08f, -0.02f);
		break;
	case ProjectileType::PROJECTILE_KERNEL:
		mRotation = 0.0f;
		mRotationSpeed = RandRangeFloat(-0.2f, -0.08f);
		break;
	case ProjectileType::PROJECTILE_SNOWPEA:
	{
		TodParticleSystem* aParticle = mApp->AddTodParticle(mPosX + 8.0f, mPosY + 13.0f, 400000, ParticleEffect::PARTICLE_SNOWPEA_TRAIL);
		AttachParticle(mAttachmentID, aParticle, 8.0f, 13.0f);
		break;
	}
	case ProjectileType::PROJECTILE_FIREBALL:
	{
		// 普通火豌豆（被火炬树桩点燃的那种）。原版这条路径**只会**走 Projectile::ConvertToFireball()：
		// 那里是"先有豌豆、飞过火炬树桩时再补挂火焰动画"，所以这里原本只剩一句断言
		// （"不该有人直接生成火球"）。现在火焰机枪射手的大招会**直接**生成这种弹丸
		// （散射的每颗子弹 50% 紫火豌豆 / 50% 普通火豌豆），于是这里补齐与紫火豌豆完全同款、
		// 只是**不带紫火滤镜**的橙红火球观感 —— 否则这颗弹丸 Draw 里 aImage 为 nullptr，
		// 会变成一颗只有影子、看不见火球的隐形弹丸（看起来就像"大招只出紫火"）。
		Reanimation* aFireReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_FIRE_PEA);
		aFireReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
		aFireReanim->mAnimRate = RandRangeFloat(50.0f, 80.0f);

		float aOffsetX = -25.0f;
		float aOffsetY = -25.0f;
		aFireReanim->SetPosition(mPosX + aOffsetX, mPosY + aOffsetY);
		AttachReanim(mAttachmentID, aFireReanim, aOffsetX, aOffsetY);
		break;
	}
	case ProjectileType::PROJECTILE_PURPLE_FIRE_PEA:
	{
		// 紫火豌豆（火豌豆射手）：观感直接复用原版"被火炬树桩点燃的豌豆"那套火球 reanim
		// （旋转火球 + 三条火舌 + 火花），整株过一层"紫火"滤镜换成紫色（色相 5.03 / 饱和度 ×1.47）。
		// 弹丸自身没有贴图，视觉全部由这个附属动画负责（与 PROJECTILE_FIREBALL 同一套画法）。
		Reanimation* aFireReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_FIRE_PEA);
		aFireReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
		aFireReanim->mAnimRate = RandRangeFloat(50.0f, 80.0f);
		aFireReanim->mFilterEffect = FilterEffect::FILTER_EFFECT_FIREPEA_PURPLE;

		float aOffsetX = -25.0f;
		float aOffsetY = -25.0f;
		aFireReanim->SetPosition(mPosX + aOffsetX, mPosY + aOffsetY);
		AttachReanim(mAttachmentID, aFireReanim, aOffsetX, aOffsetY);
		break;
	}
	case ProjectileType::PROJECTILE_SPORESHROOM:
	{
		Reanimation* aSporeReanim = mApp->AddReanimation(mPosX, mPosY, mRenderOrder + 1, ReanimationType::REANIM_SPORESHROOM_PROJECTILE);
		aSporeReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
		aSporeReanim->mAnimRate = 30.0f;
		aSporeReanim->SetFramesForLayer("anim_fly");
		AttachReanim(mAttachmentID, aSporeReanim, 0.0f, 0.0f);
		break;
	}
	case ProjectileType::PROJECTILE_COBBIG:
		mWidth = IMAGE_REANIM_COBCANNON_COB->GetWidth();
		mHeight = IMAGE_REANIM_COBCANNON_COB->GetHeight();
		mRotation = PI / 2;
		break;
	case ProjectileType::PROJECTILE_PUFF:
	{
		TodParticleSystem* aParticle = mApp->AddTodParticle(mPosX + 13.0f, mPosY + 13.0f, 400000, ParticleEffect::PARTICLE_PUFFSHROOM_TRAIL);
		AttachParticle(mAttachmentID, aParticle, 13.0f, 13.0f);
		break;
	}
	case ProjectileType::PROJECTILE_BASKETBALL:
		mRotation = RandRangeFloat(0.0f, 2 * PI);
		mRotationSpeed = RandRangeFloat(0.05f, 0.1f);
		break;
	case ProjectileType::PROJECTILE_STAR:
		mShadowY += 15.0f;
		mRotationSpeed = RandRangeFloat(0.05f, 0.1f);
		if (Rand(2) == 0)
		{
			mRotationSpeed = -mRotationSpeed;
		}
		break;
	default:
		break;
	}

	mAnimCounter = 0;
	mX = static_cast<int>(mPosX);
	mY = static_cast<int>(mPosY);
}

Plant* Projectile::FindCollisionTargetPlant()
{
	Rect aProjectileRect = GetProjectileRect();

	Plant* aPlant = nullptr;
	while (mBoard->IteratePlants(aPlant))
	{
		if (aPlant->mRow != mRow)
			continue;

		if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA)
		{
			if (aPlant->mSeedType == SeedType::SEED_PUFFSHROOM ||
				aPlant->mSeedType == SeedType::SEED_SUNSHROOM ||
				aPlant->mSeedType == SeedType::SEED_POTATOMINE ||
				aPlant->mSeedType == SeedType::SEED_SPIKEWEED ||
				aPlant->mSeedType == SeedType::SEED_SPIKEROCK ||
				aPlant->mSeedType == SeedType::SEED_LILYPAD)  // 僵尸豌豆不能击中低矮植物
				continue;
		}

		Rect aPlantRect = aPlant->GetPlantRect();
		if (GetRectOverlap(aProjectileRect, aPlantRect) > 8)
		{
			if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA)
			{
				return mBoard->GetTopPlantAt(aPlant->mPlantCol, aPlant->mRow, PlantPriority::TOPPLANT_EATING_ORDER);
			}
			else
			{
				return mBoard->GetTopPlantAt(aPlant->mPlantCol, aPlant->mRow, PlantPriority::TOPPLANT_CATAPULT_ORDER);
			}
		}
	}

	return nullptr;
}

bool Projectile::PeaAboutToHitTorchwood()
{
	if (mMotionType != ProjectileMotion::MOTION_STRAIGHT && mMotionType != ProjectileMotion::MOTION_STAR)
		return false;

	if (mProjectileType != ProjectileType::PROJECTILE_PEA && mProjectileType != ProjectileType::PROJECTILE_SNOWPEA)
		return false;

	Plant* aPlant = nullptr;
	while (mBoard->IteratePlants(aPlant))
	{
		if (aPlant->mSeedType == SeedType::SEED_TORCHWOOD && aPlant->mRow == mRow && !aPlant->NotOnGround() && mHitTorchwoodGridX != aPlant->mPlantCol)
		{
			Rect aPlantAttackRect = aPlant->GetPlantAttackRect(PlantWeapon::WEAPON_PRIMARY);
			Rect aProjectileRect = GetProjectileRect();
			aProjectileRect.mX += 40;

			if (GetRectOverlap(aPlantAttackRect, aProjectileRect) > 10)
			{
				return true;
			}
		}
	}

	return false;
}

Zombie* Projectile::FindCollisionTarget()
{
	if (PeaAboutToHitTorchwood())  // “卡火炬”的原理，这段代码在两版内测版中均不存在
		return nullptr;

	if (mLastHitZombieID != ZombieID::ZOMBIEID_NULL)
	{
		Zombie* aLastHitZombie = mBoard->ZombieTryToGet(mLastHitZombieID);
		if (!aLastHitZombie || aLastHitZombie->mDead)
		{
			mLastHitZombieID = ZombieID::ZOMBIEID_NULL;
		}
		else
		{
			Rect aProjectileRect = GetProjectileRect();
			Rect aZombieRect = aLastHitZombie->GetZombieRect();
			if (GetRectOverlap(aProjectileRect, aZombieRect) <= 0)
			{
				mLastHitZombieID = ZombieID::ZOMBIEID_NULL;
			}
		}
	}

	Rect aProjectileRect = GetProjectileRect();
	Zombie* aBestZombie = nullptr;
	int aMinX = 0;

	Zombie* aZombie = nullptr;
	while (mBoard->IterateZombies(aZombie))
	{
		if (mBoard->ZombieGetID(aZombie) == mLastHitZombieID)
			continue;

		if ((aZombie->mZombieType == ZombieType::ZOMBIE_BOSS || aZombie->mRow == mRow) && aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
		{
			if (aZombie->mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL && mPosZ >= 45.0f)
			{
				continue;
			}

			// 矿工僵尸的豁免只针对"还在土里"的那一段：出土站起来之后，星星与其它僵尸一视同仁。
			if (mProjectileType == ProjectileType::PROJECTILE_STAR && mProjectileAge < 25 && mVelX >= 0.0f &&
				aZombie->mZombieType == ZombieType::ZOMBIE_DIGGER && aZombie->IsUnderground())
			{
				continue;
			}

			Rect aZombieRect = aZombie->GetZombieRect();
			if (GetRectOverlap(aProjectileRect, aZombieRect) > 0)
			{
				if (aBestZombie == nullptr || aZombie->mX < aMinX)
				{
					aBestZombie = aZombie;
					aMinX = aZombie->mX;
				}
			}
		}
	}

	return aBestZombie;
}

void Projectile::FindNewHomingTarget()
{
	Zombie* aBestZombie = nullptr;
	int aMinX = INT32_MAX;

	Zombie* aZombie = nullptr;
	while (mBoard->IterateZombies(aZombie))
	{
		if (mBoard->ZombieGetID(aZombie) == mLastHitZombieID)
			continue;

		if ((aZombie->mZombieType == ZombieType::ZOMBIE_BOSS || aZombie->mRow == mRow) && aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
		{
			if (aZombie->mX < aMinX)
			{
				aMinX = aZombie->mX;
				aBestZombie = aZombie;
			}
		}
	}

	if (aBestZombie)
	{
		mTargetZombieID = mBoard->ZombieGetID(aBestZombie);
	}
}

// 究极电能星星果的电能星星：命中后不再消失，而是"钉"在该僵尸身上（总寿命见 UpdateElectricStarLinger）。
void Projectile::StartElectricStarLinger(Zombie* theZombie)
{
	if (theZombie == nullptr)
	{
		Die();
		return;
	}

	// 只有第一次命中才起算寿命（改追下一个目标后再次命中的不重新计时）
	if (mLingerCountdown <= 0)
	{
		mLingerCountdown = ELECTRIC_STAR_LINGER_TICKS;
	}

	mElectricStarStuck = true;
	mTargetZombieID = mBoard->ZombieGetID(theZombie);
	mVelX = 0.0f;
	mVelY = 0.0f;
	mRotationSpeed = 0.15f;   // 原地缓慢旋转，读起来像"电光钉"
	mRow = theZombie->mRow;
	mShadowY = mPosY + 67.0f;
}

// 钉住期间：每帧跟随目标僵尸 + 每 0.15 秒结算一次电能伤害；目标死亡则自动改追下一个。
void Projectile::UpdateElectricStarLinger()
{
	Zombie* aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
	if (aZombie != nullptr && aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
	{
		Rect aZombieRect = aZombie->GetZombieRect();
		mPosX = aZombie->mX + aZombieRect.mWidth / 2.0f - mWidth / 2.0f;
		mPosY = aZombieRect.mY + aZombieRect.mHeight / 2.0f - mHeight / 2.0f;
		mRow = aZombie->mRow;
		mShadowY = mPosY + 67.0f;
		mX = static_cast<int>(mPosX);
		mY = static_cast<int>(mPosY);

		// 伤害节奏与电能豌豆同一口径：计数归零才打一次（刚钉住时计数为 0 → 立刻打第一次），
		// 随后重置为 ELECTRIC_DAMAGE_INTERVAL_TICKS，中间那些帧只跟随不掉血。
		if (mElectricDamageCountdown <= 0)
		{
			mElectricDamageCountdown = ELECTRIC_DAMAGE_INTERVAL_TICKS;
			aZombie->TakeDamage(ELECTRIC_STAR_HIT_DAMAGE, GetDamageFlags(aZombie));
		}
	}
	else if (RetargetElectricStar())
	{
		// 目标已死：已退出钉住状态，下一帧起由 MOTION_STAR 的追踪逻辑飞向新目标
		return;
	}
	// 没有下一个目标时留在原地，等总寿命走完

	if (--mLingerCountdown <= 0)
	{
		Die();
	}
}

// 钉住的僵尸死亡（或不再可被伤害）后，改追下一个可被本星星伤害的僵尸
// （索敌规则与普通杨桃一致：取最靠左的一只）。找到就退出钉住状态并交还给追踪飞行。
bool Projectile::RetargetElectricStar()
{
	Zombie* aBestZombie = nullptr;
	int aMinX = INT32_MAX;

	Zombie* aZombie = nullptr;
	while (mBoard->IterateZombies(aZombie))
	{
		if (!aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
			continue;   // 已含"死亡/濒死僵尸不算目标"
		if (aZombie->mX < aMinX)
		{
			aMinX = aZombie->mX;
			aBestZombie = aZombie;
		}
	}

	if (aBestZombie == nullptr)
	{
		return false;   // 场上没有可追的目标
	}

	mElectricStarStuck = false;
	mTargetZombieID = mBoard->ZombieGetID(aBestZombie);
	return true;
}

// 究极电能机枪射手 / 究极电能星星果的弹丸共用的"链式闪电"结算：
// 弹丸是一个持续放电的电源，每 ELECTRIC_CHAIN_INTERVAL_TICKS 刻向**自身周围半径
// ELECTRIC_CHAIN_RADIUS 像素内最近的至多 ELECTRIC_CHAIN_MAX_TARGETS 只僵尸**各放一道电弧，
// 伤害只有弹丸本身的一半（30 → 15）；因为间隔也是本身的两倍（15 → 30 刻），
// 摊到每秒的 DPS 只有弹丸本身的四分之一，属于纯额外收益。
//
// 节奏不新增存档字段，直接跟 mProjectileAge 对齐：出膛那一帧（age 0）就先放一次电，
// 之后每 30 刻一次。这样读档回来哪怕 mElectricChainFlash 是 0，节奏也完全不会错位。
// 放电完成后把 mElectricChainFlash 置为 ELECTRIC_CHAIN_FLASH_TICKS，
// 由 DrawElectricChain() 在随后的几帧里把电弧画出来（电弧是"闪现"而不是长亮）。
//
// 注意：本函数在 UpdateMotion() 之前调用 —— 弹丸自己的接触伤害（电能豌豆的穿透伤害）
// 要等移动之后才结算，所以哪怕同一帧同时打中，也是先电击、后穿透，不会漏掉目标。
bool Projectile::IsElectricChainSource()
{
	// 只有究极形态自己射出的弹丸才带电弧（普通机枪射手那 3% 的电能豌豆不算）
	if (mProjectileType != ProjectileType::PROJECTILE_FIREPEA_RED &&
		mProjectileType != ProjectileType::PROJECTILE_ELECTRIC_STAR)
	{
		return false;
	}

	Plant* aSourcePlant = mBoard->mPlants.DataArrayTryToGet(static_cast<unsigned int>(mSourcePlantID));
	if (aSourcePlant != nullptr)
	{
		// 来源植物还在场上：以现查结果为准，顺手修正缓存的标记
		mElectricChainSource = PlantFiresElectricChainProjectile(aSourcePlant);
	}

	// 来源植物已经不在了（被吃掉/被铲掉）：用发射瞬间就定死的缓存，
	// 否则电能星星钉到一半会因为"植物死了"而停止放电。
	return mElectricChainSource;
}

void Projectile::UpdateElectricChainLightning()
{
	if (!IsElectricChainSource())
	{
		return;
	}

	// 电弧闪现倒计时：与结算解耦，每帧自减，与 mProjectileAge 的模运算共同决定"本帧画不画"
	if (mElectricChainFlash > 0)
	{
		mElectricChainFlash--;
	}

	if (mProjectileAge % ELECTRIC_CHAIN_INTERVAL_TICKS != 0)
	{
		return;
	}

	// 弹丸自己的伤害口径：优先 mDamageOverride（孤狼关卡 / 机枪散射的 200），否则查表
	const int aBaseDamage = mDamageOverride > 0 ? mDamageOverride : GetProjectileDef().mDamage;
	const int aChainDamage = aBaseDamage / 2;   // 伤害是本身的一半（30 → 15）
	if (aChainDamage <= 0)
	{
		return;
	}

	const float aCenterX = mPosX + mWidth * 0.5f;
	const float aCenterY = mPosY + mHeight * 0.5f;

	// 先按"到弹丸的距离"挑出半径内最近的至多 5 只僵尸，再统一结算伤害：
	// 半径是圆形（纵向 2 格 85~100px × 2 ≈ 170~200px 因此必然落在 160px 的圆内，
	// 横向 2 格正好 160px），故"两格内的僵尸"不会被漏掉，也不会多打到三格外的。
	std::vector<std::pair<float, Zombie*>> aCandidates;
	Zombie* aZombie = nullptr;
	while (mBoard->IterateZombies(aZombie))
	{
		if (!aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
			continue;   // 已含"死亡/濒死僵尸不算目标"

		Rect aZombieRect = aZombie->GetZombieRect();
		float aDistance = Distance2D(aCenterX, aCenterY,
			aZombieRect.mX + aZombieRect.mWidth * 0.5f, aZombieRect.mY + aZombieRect.mHeight * 0.5f);
		if (aDistance <= ELECTRIC_CHAIN_RADIUS)
		{
			aCandidates.push_back(std::make_pair(aDistance, aZombie));
		}
	}

	if (aCandidates.empty())
	{
		return;
	}

	if (static_cast<int>(aCandidates.size()) > ELECTRIC_CHAIN_MAX_TARGETS)
	{
		std::partial_sort(aCandidates.begin(), aCandidates.begin() + ELECTRIC_CHAIN_MAX_TARGETS, aCandidates.end(),
			[](const std::pair<float, Zombie*>& a, const std::pair<float, Zombie*>& b) { return a.first < b.first; });
		aCandidates.resize(ELECTRIC_CHAIN_MAX_TARGETS);
	}

	for (const std::pair<float, Zombie*>& aCandidate : aCandidates)
	{
		aCandidate.second->TakeDamage(aChainDamage, GetDamageFlags(aCandidate.second));
	}

	mElectricChainFlash = ELECTRIC_CHAIN_ARC_TICKS;
}

// 沿着"上一点 → 本点"画一条有粗细的线：Graphics 只有 1px 的 DrawLine，
// 所以沿垂直方向平移 strokeCount 条平行线凑出宽度。
static void DrawElectricStroke(Graphics* g, float theFromX, float theFromY, float theToX, float theToY, int theStrokeCount)
{
	float aDeltaX = theToX - theFromX;
	float aDeltaY = theToY - theFromY;
	float aLength = sqrt(aDeltaX * aDeltaX + aDeltaY * aDeltaY);
	if (aLength < 0.01f)
	{
		return;
	}

	// 单位法线（垂直方向），"第 i 条线"沿它偏移
	const float aNormalX = -aDeltaY / aLength;
	const float aNormalY = aDeltaX / aLength;
	const float aStart = -(theStrokeCount - 1) * 0.5f;

	for (int i = 0; i < theStrokeCount; i++)
	{
		float aOffset = aStart + i;
		g->DrawLineAA(static_cast<int>(theFromX + aNormalX * aOffset), static_cast<int>(theFromY + aNormalY * aOffset),
			static_cast<int>(theToX + aNormalX * aOffset), static_cast<int>(theToY + aNormalY * aOffset));
	}
}

// 把 mElectricChainFlash 仍然有效的电弧画出来：以弹丸为圆心，向半径两格内的最近至多 5 只僵尸各画一道闪电。
// 这里重算一遍目标（而不是把目标存进弹丸）是为了不给存档加字段：
// 电弧只是表现，取"当下仍在范围内且最近"的僵尸画，跟结算时的目标最多差几帧，观感上完全一致。
// 每帧调用意味着僵尸往前走时电弧也从弹丸一路跟到僵尸身上，而不是画在固定位置。
void Projectile::DrawElectricChain(Graphics* g)
{
	if (mElectricChainFlash <= 0)
	{
		return;
	}

	if (!IsElectricChainSource())
	{
		return;
	}

	const float aCenterX = mPosX + mWidth * 0.5f;
	const float aCenterY = mPosY + mHeight * 0.5f;

	std::vector<std::pair<float, Zombie*>> aCandidates;
	Zombie* aZombie = nullptr;
	while (mBoard->IterateZombies(aZombie))
	{
		if (!aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
			continue;

		Rect aZombieRect = aZombie->GetZombieRect();
		float aTargetX = aZombieRect.mX + aZombieRect.mWidth * 0.5f;
		float aTargetY = aZombieRect.mY + aZombieRect.mHeight * 0.5f;
		float aDistance = Distance2D(aCenterX, aCenterY, aTargetX, aTargetY);
		if (aDistance <= ELECTRIC_CHAIN_RADIUS)
		{
			aCandidates.push_back(std::make_pair(aDistance, aZombie));
		}
	}

	if (aCandidates.empty())
	{
		return;
	}

	if (static_cast<int>(aCandidates.size()) > ELECTRIC_CHAIN_MAX_TARGETS)
	{
		std::partial_sort(aCandidates.begin(), aCandidates.begin() + ELECTRIC_CHAIN_MAX_TARGETS, aCandidates.end(),
			[](const std::pair<float, Zombie*>& a, const std::pair<float, Zombie*>& b) { return a.first < b.first; });
		aCandidates.resize(ELECTRIC_CHAIN_MAX_TARGETS);
	}

	const float aScreenX = static_cast<float>(mBoard->mX);
	const float aScreenY = static_cast<float>(mBoard->mY);

	// 放电强度：刚结算完最亮，越接近下一次结算越暗 —— 观感上就是电弧一直在、并周期性"啪"一下。
	// 用倒计时而不是随机数，保证 100fps 下每次放电的观感完全一致。
	float aPulse = static_cast<float>(mElectricChainFlash) / ELECTRIC_CHAIN_ARC_TICKS;
	aPulse = ClampFloat(aPulse, 0.0f, 1.0f);

	// 闪电由"外圈淡蓝 + 内芯电能蓝"两层折线叠出来：
	// 外层 5 条线做辉光，内芯 3 条线做亮芯，合计差不多 5px 宽，在草坪上不会被花草吞掉。
	for (const std::pair<float, Zombie*>& aCandidate : aCandidates)
	{
		Rect aZombieRect = aCandidate.second->GetZombieRect();
		const float aTargetX = aZombieRect.mX + aZombieRect.mWidth * 0.5f;
		const float aTargetY = aZombieRect.mY + aZombieRect.mHeight * 0.5f;

		for (int aLayer = 0; aLayer < 2; aLayer++)
		{
			// 每层各自从弹丸起笔，走完一整条折线到僵尸身上
			float aPreviousX = aCenterX;
			float aPreviousY = aCenterY;

			g->SetColor(aLayer == 0
				? Color(110, 190, 255, ClampInt(static_cast<int>(120.0f * aPulse), 0, 255))
				: Color(ELECTRIC_BLUE_R, ELECTRIC_BLUE_G, ELECTRIC_BLUE_B, ClampInt(static_cast<int>(245.0f * aPulse), 0, 255)));

			const int aSegmentCount = 5;
			for (int aSegment = 1; aSegment <= aSegmentCount; aSegment++)
			{
				float aT = static_cast<float>(aSegment) / aSegmentCount;
				float aPointX = aCenterX + (aTargetX - aCenterX) * aT;
				float aPointY = aCenterY + (aTargetY - aCenterY) * aT;
				if (aSegment != aSegmentCount)
				{
					// 中段抖动：每帧重新掷，闪电才会"噼啪"作响（最后一节必须落在僵尸身上）
					aPointX += RandRangeFloat(-11.0f, 11.0f);
					aPointY += RandRangeFloat(-11.0f, 11.0f);
				}

				DrawElectricStroke(g, aPreviousX + aScreenX, aPreviousY + aScreenY,
					aPointX + aScreenX, aPointY + aScreenY, aLayer == 0 ? 5 : 3);

				aPreviousX = aPointX;
				aPreviousY = aPointY;
			}
		}
	}
}

// 所有究极电能弹丸的电弧，统一由 Board 在**所有渲染项画完之后**调用（见 Board::Draw），
// 这样闪电永远压在最顶层：僵尸、植物、雾、甚至屏幕渐隐都盖不住它。
void Projectile::DrawAllElectricChains(Board* theBoard, Graphics* g)
{
	Projectile* aProjectile = nullptr;
	while (theBoard->IterateProjectiles(aProjectile))
	{
		aProjectile->DrawElectricChain(g);
	}
}

// 线段与轴对齐矩形是否相交（slab 法）。
// 激光是一条有角度的线，所以命中判定不能像豌豆那样拿两个矩形求重叠 —— 必须做真正的线段/矩形相交，
// 否则"斜着打过去的光束"要么漏掉目标、要么把没扫到的僵尸也算进去。
static bool LaserBeamHitsRect(float theFromX, float theFromY, float theDirX, float theDirY,
	float theRange, const Rect& theRect)
{
	// 先把线段裁到矩形四条边的外侧区间（slab）：两个方向的参数区间只要有交集就相交
	float aMinT = 0.0f;
	float aMaxT = theRange;

	const float aOriginX = theFromX;
	const float aOriginY = theFromY;

	// X 方向
	if (FloatApproxEqual(theDirX, 0.0f))
	{
		if (aOriginX < theRect.mX || aOriginX > theRect.mX + theRect.mWidth)
			return false;
	}
	else
	{
		float aT1 = (theRect.mX - aOriginX) / theDirX;
		float aT2 = (theRect.mX + theRect.mWidth - aOriginX) / theDirX;
		if (aT1 > aT2)
		{
			float aSwap = aT1;
			aT1 = aT2;
			aT2 = aSwap;
		}
		aMinT = std::max(aMinT, aT1);
		aMaxT = std::min(aMaxT, aT2);
	}

	// Y 方向
	if (FloatApproxEqual(theDirY, 0.0f))
	{
		if (aOriginY < theRect.mY || aOriginY > theRect.mY + theRect.mHeight)
			return false;
	}
	else
	{
		float aT1 = (theRect.mY - aOriginY) / theDirY;
		float aT2 = (theRect.mY + theRect.mHeight - aOriginY) / theDirY;
		if (aT1 > aT2)
		{
			float aSwap = aT1;
			aT1 = aT2;
			aT2 = aSwap;
		}
		aMinT = std::max(aMinT, aT1);
		aMaxT = std::min(aMaxT, aT2);
	}

	return aMinT <= aMaxT;
}

// 激光豌豆的光束：从枪口沿发射时定下的方向射出去，路径上每只僵尸各吃 LASER_PEA_DAMAGE 点伤害。
//
// 为什么做成"不移动的弹丸"而不是一颗高速穿透弹丸：
//   1) 需求是"贯穿路径上所有僵尸"，一次结算最干净 —— 不需要给每颗子弹记一串已命中目标
//      （仓库里既有的穿透弹丸 PROJECTILE_SPIKE 是靠 mPenetrations 计数 + mLastHitZombieID
//      单目标回溯，那种做法对"无限穿透"只能靠 mProjectileAge 反复结算，容易出现同一只僵尸
//      在光束里反复掉血）；
//   2) 弹丸池本来就负责寿命管理（Board::mProjectiles），光束借用它就不必新造一套特效对象。
//
// 判定口径：
//   - **不限行**：光束是一条有角度的射线（方向存在 mVelX/mVelY 上，由 Plant::Fire 按目标算出来），
//     所以目标那一行以及这条线扫过的其它僵尸都会吃到伤害；
//   - 空中/地面僵尸共用这一道光束：掩码里同时提供地面与空中位，于是无论目标属性如何都会走到结算；
//     真正被排除的仍是原版那套（濒死、被魅惑、蹦极/雪橇免疫、矿工钻地、潜水、空投过程……）；
//   - 判定用**线段/矩形相交**（LaserBeamHitsRect），而不是两个矩形求重叠；
//   - 潜水中的僵尸沿用"电能豌豆"那条规避（mPosZ < 45 才算浮出水面可被打）。
void Projectile::UpdateLaserPeaBeam()
{
	// ⚠ 出生帧的判定必须用 mLaserPeaBeamCountdown 满值，**不能**用 mProjectileAge：
	//   Projectile::Update 一进来就 mProjectileAge++，所以这里的年龄永远 >= 1；
	//   而 Update 在游戏场景不是 SCENE_PLAYING 时还会提前 return（年龄照样在涨），
	//   于是"年龄 == 0"这种判定会永远不成立 —— 表现就是"激光看得见、但一点伤害都没有"。
	//   倒计时是这一帧才在 ProjectileInitialize 里置成满值的，用它当"首帧"标记最稳。
	if (mLaserPeaBeamCountdown == LASER_PEA_BEAM_TICKS)
	{
		const int aDamage = mDamageOverride > 0 ? mDamageOverride : LASER_PEA_DAMAGE;

		// 方向默认水平向右（理论上 Plant::Fire 一定会写进来；这里只是防止出现零向量把归一化搞坏）
		float aDirX = mVelX;
		float aDirY = mVelY;
		const float aLength = sqrt(aDirX * aDirX + aDirY * aDirY);
		if (aLength < 0.01f)
		{
			aDirX = 1.0f;
			aDirY = 0.0f;
		}
		else
		{
			aDirX /= aLength;
			aDirY /= aLength;
		}

		// 光束起点 = 枪口（与 DrawLaserPeaBeam 用的是同一个点）
		const float aFromX = mPosX + aDirX * LASER_PEA_MUZZLE_MARGIN;
		const float aFromY = mPosY + mHeight * 0.5f;

		const unsigned int aRangeFlags = (1U << static_cast<int>(DamageRangeFlags::DAMAGES_GROUND)) |
		                                 (1U << static_cast<int>(DamageRangeFlags::DAMAGES_FLYING));

		Zombie* aZombie = nullptr;
		while (mBoard->IterateZombies(aZombie))
		{
			if (mPosZ >= 45.0f && aZombie->mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL)
				continue;
			if (!aZombie->EffectedByDamage(aRangeFlags))
				continue;

			Rect aZombieRect = aZombie->GetZombieRect();
			if (!LaserBeamHitsRect(aFromX, aFromY, aDirX, aDirY, LASER_PEA_BEAM_RANGE, aZombieRect))
				continue;

			// 纯能量伤害：不绕盾、不吃冰冻、不加成（与普通豌豆同一套"不设任何伤害标志"的口径）。
			aZombie->TakeDamage(aDamage, 0U);
		}
	}

	// 首帧只结算伤害（倒计时保持满值）；从第 2 帧起每帧倒计时一次，归零就熄灭。
	if (mProjectileAge > 1 && mLaserPeaBeamCountdown > 0)
	{
		if (--mLaserPeaBeamCountdown <= 0)
		{
			Die();
		}
	}
}

// 光束由"外圈辉光 + 内芯亮芯"两层描边叠出来，越接近熄灭越暗。
// 光束是一条**有角度的直线**（方向来自 mVelX/mVelY，见 Plant::Fire），所以从枪口沿该方向
// 走 LASER_PEA_BEAM_RANGE 像素作为终点；DrawElectricStroke 本身按线段的垂直方向铺平行线，
// 所以斜着画也是均匀的粗细。
// 与链式闪电一样在 Board 的顶层统一绘制（见 DrawAllLaserBeams），坐标补 Board 的震屏偏移。
void Projectile::DrawLaserPeaBeam(Graphics* g)
{
	if (mLaserPeaBeamCountdown <= 0)
	{
		return;
	}

	float aDirX = mVelX;
	float aDirY = mVelY;
	const float aLength = sqrt(aDirX * aDirX + aDirY * aDirY);
	if (aLength < 0.01f)
	{
		aDirX = 1.0f;
		aDirY = 0.0f;
	}
	else
	{
		aDirX /= aLength;
		aDirY /= aLength;
	}

	// 越接近熄灭越暗
	float aPulse = static_cast<float>(mLaserPeaBeamCountdown) / LASER_PEA_BEAM_TICKS;
	aPulse = ClampFloat(aPulse, 0.0f, 1.0f);

	const float aScreenX = static_cast<float>(mBoard->mX);
	const float aScreenY = static_cast<float>(mBoard->mY);
	// 起点必须与 UpdateLaserPeaBeam 里那一帧用**同一个**枪口点（含 MUZZLE_MARGIN 沿方向的前移），
	// 否则会出现"看着打中了却没掉血"。
	const float aFromX = mPosX + aDirX * LASER_PEA_MUZZLE_MARGIN + aScreenX;
	const float aFromY = mPosY + mHeight * 0.5f + aScreenY;
	const float aToX = aFromX + aDirX * LASER_PEA_BEAM_RANGE;
	const float aToY = aFromY + aDirY * LASER_PEA_BEAM_RANGE;

	// 外圈辉光（先画宽的，再压上内芯，读起来才有"光晕包着亮芯"的层次）
	g->SetColor(Color(LASER_PEA_BEAM_GLOW_R, LASER_PEA_BEAM_GLOW_G, LASER_PEA_BEAM_GLOW_B,
		ClampInt(static_cast<int>(LASER_PEA_BEAM_GLOW_ALPHA * aPulse), 0, 255)));
	DrawElectricStroke(g, aFromX, aFromY, aToX, aToY, LASER_PEA_BEAM_GLOW_WIDTH);
	// 内芯
	g->SetColor(Color(LASER_PEA_BEAM_CORE_R, LASER_PEA_BEAM_CORE_G, LASER_PEA_BEAM_CORE_B,
		ClampInt(static_cast<int>(LASER_PEA_BEAM_CORE_ALPHA * aPulse), 0, 255)));
	DrawElectricStroke(g, aFromX, aFromY, aToX, aToY, LASER_PEA_BEAM_CORE_WIDTH);
}

void Projectile::DrawAllLaserBeams(Board* theBoard, Graphics* g)
{
	Projectile* aProjectile = nullptr;
	while (theBoard->IterateProjectiles(aProjectile))
	{
		if (aProjectile->IsLaserPeaBeam())
		{
			aProjectile->DrawLaserPeaBeam(g);
		}
	}
}

void Projectile::CheckForCollision()
{
	if (mMotionType == ProjectileMotion::MOTION_PUFF && mProjectileAge >= 75)
	{
		Die();
		return;
	}

	// 大喷菇群两侧孢子（PUFF + STAR 斜飞）：飞行距离与大喷菇烟雾射程一致（约 340px），到点消散
	if (mProjectileType == ProjectileType::PROJECTILE_PUFF && mMotionType == ProjectileMotion::MOTION_STAR && mProjectileAge >= 115)
	{
		Die();
		return;
	}

	if (mPosX > WIDE_BOARD_WIDTH || mPosX + mWidth < 0.0f)
	{
		Die();
		return;
	}

	if (mMotionType == ProjectileMotion::MOTION_HOMING)
	{
		if (mTargetZombieID == ZombieID::ZOMBIEID_NULL)
		{
			FindNewHomingTarget();
		}
		Zombie* aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
		if (aZombie && aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
		{
			Rect aProjectileRect = GetProjectileRect();
			Rect aZombieRect = aZombie->GetZombieRect();
			if (GetRectOverlap(aProjectileRect, aZombieRect) >= 0 && mPosY > aZombieRect.mY&& mPosY < aZombieRect.mY + aZombieRect.mHeight)
			{
				DoImpact(aZombie);
			}
		}
		return;
	}

	if ((mProjectileType == ProjectileType::PROJECTILE_STAR || mProjectileType == ProjectileType::PROJECTILE_ELECTRIC_STAR) && (mPosY > 600.0f || mPosY < 0.0f))
	{
		Die();
		return;
	}

	// 究极电能星星果的电能星星：命中即"钉住"（见 StartElectricStarLinger），不在这里消失。
	// 判定与普通杨桃星星一致（优先钉住当前追踪目标；目标失效时撞到谁就钉谁）。
	if (mProjectileType == ProjectileType::PROJECTILE_ELECTRIC_STAR)
	{
		Zombie* aZombie = nullptr;
		if (mTargetZombieID != ZombieID::ZOMBIEID_NULL)
		{
			Zombie* aTargetZombie = mBoard->ZombieTryToGet(mTargetZombieID);
			if (aTargetZombie && aTargetZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
			{
				Rect aProjectileRect = GetProjectileRect();
				Rect aZombieRect = aTargetZombie->GetZombieRect();
				if (GetRectOverlap(aProjectileRect, aZombieRect) >= 0 && mPosY > aZombieRect.mY && mPosY < aZombieRect.mY + aZombieRect.mHeight)
				{
					aZombie = aTargetZombie;
				}
			}
		}
		if (aZombie == nullptr)
		{
			aZombie = FindCollisionTarget();
		}

		if (aZombie)
		{
			StartElectricStarLinger(aZombie);
		}
		return;
	}

	if (mProjectileType == ProjectileType::PROJECTILE_STAR && mTargetZombieID != ZombieID::ZOMBIEID_NULL)
	{
		Zombie* aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
		if (aZombie && aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
		{
			Rect aProjectileRect = GetProjectileRect();
			Rect aZombieRect = aZombie->GetZombieRect();
			if (GetRectOverlap(aProjectileRect, aZombieRect) >= 0 && mPosY > aZombieRect.mY && mPosY < aZombieRect.mY + aZombieRect.mHeight)
			{
				DoImpact(aZombie);
			}
		}
		return;
	}

	if ((mProjectileType == ProjectileType::PROJECTILE_PEA || mProjectileType == ProjectileType::PROJECTILE_STAR) && mShadowY - mPosY > 90.0f)
	{
		return;
	}

	if (mMotionType == ProjectileMotion::MOTION_FLOAT_OVER)
	{
		return;
	}

	if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA)
	{
		Plant* aPlant = FindCollisionTargetPlant();
		if (aPlant)
		{
			// Reflect only when the pea actually hits the umbrella itself: the umbrella
			// opens (anim_block) and the pea is converted into a friendly pea flying
			// right, which damages zombies (and can pass through a Torchwood).
			if (aPlant->mSeedType == SeedType::SEED_UMBRELLA && !aPlant->NotOnGround())
			{
				if (aPlant->mState != PlantState::STATE_UMBRELLA_TRIGGERED &&
					aPlant->mState != PlantState::STATE_UMBRELLA_REFLECTING)
				{
					// Same presentation as deflecting a Bungee zombie (Zombie::BungeeLanding):
					// boing + umbrella foley + DoSpecial() opens the umbrella (anim_block).
					mApp->PlaySample(SOUND_BOING);
					mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA);
					aPlant->DoSpecial();
				}

				mProjectileType = ProjectileType::PROJECTILE_PEA;
				mMotionType = ProjectileMotion::MOTION_STRAIGHT;
				mHitTorchwoodGridX = -1;
				// Friendly peas need the GROUND damage-range bit (bit 0, value 1) or
				// EffectedByDamage() rejects every ground zombie and the pea passes through.
				mDamageRangeFlags = 1U << static_cast<int>(DamageRangeFlags::DAMAGES_GROUND);
				AttachmentDie(mAttachmentID);
				return;
			}

			const ProjectileDefinition& aProjectileDef = GetProjectileDef();
			if (Plant* aShieldNut = mBoard->FindGiantWallnutShield(aPlant->mPlantCol, aPlant->mRow); aShieldNut)
			{
				mBoard->GiantWallnutShareDamage(aProjectileDef.mDamage, aShieldNut);   // 巨大坚果为周围植物承伤
			}
			else if (aPlant->mSeedType == SeedType::SEED_GIANT_WALLNUT)
			{
				mBoard->GiantWallnutShareDamage(aProjectileDef.mDamage, aPlant);   // 巨大坚果：伤害全场分摊
			}
			else
			{
				aPlant->mPlantHealth -= aProjectileDef.mDamage;
			}
			aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 25);

			mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
			mApp->AddTodParticle(mPosX - 3.0f, mPosY + 17.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_PEA_SPLAT);
			Die();
		}
		return;
	}

	if (mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
	{
		// 电能伤害：每 0.15 秒（ELECTRIC_DAMAGE_INTERVAL_TICKS = 15 刻）对矩形重叠的每个僵尸
		// 造成 30 点伤害；未到结算刻的那几帧只是"接触但不掉血"。
		// 节奏直接跟 mProjectileAge 对齐（此弹丸永远不会在出生的同一刻打中僵尸），
		// 因此从出膛起就是稳定的 15 刻一次。无限穿透，永不因击中僵尸而消亡（出屏判定在上方，正常 Die）
		if (mProjectileAge % ELECTRIC_DAMAGE_INTERVAL_TICKS != 0)
		{
			return;
		}

		Rect aProjectileRect = GetProjectileRect();
		Zombie* aZombie = nullptr;
		while (mBoard->IterateZombies(aZombie))
		{
			if (aZombie->mZombieType != ZombieType::ZOMBIE_BOSS && aZombie->mRow != mRow)
				continue;
			if (!aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
				continue;
			if (aZombie->mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL && mPosZ >= 45.0f)
				continue;
			if (aZombie->mOnHighGround && CantHitHighGround())
				continue;

			Rect aZombieRect = aZombie->GetZombieRect();
			if (GetRectOverlap(aProjectileRect, aZombieRect) > 0)
			{
				aZombie->TakeDamage(30, GetDamageFlags(aZombie));
			}
		}
		return;  // 无限穿透
	}

	Zombie* aZombie = FindCollisionTarget();
	if (aZombie)
	{
		if (aZombie->mOnHighGround && CantHitHighGround())
		{
			return;
		}

		DoImpact(aZombie);
	}
}

bool Projectile::CantHitHighGround()
{
	if (mMotionType == ProjectileMotion::MOTION_BACKWARDS || mMotionType == ProjectileMotion::MOTION_HOMING)
		return false;

	return (
		mProjectileType == ProjectileType::PROJECTILE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_SNOWPEA ||
		mProjectileType == ProjectileType::PROJECTILE_STAR ||
		mProjectileType == ProjectileType::PROJECTILE_PUFF ||
		mProjectileType == ProjectileType::PROJECTILE_FIREBALL ||
		mProjectileType == ProjectileType::PROJECTILE_PURPLE_FIRE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED
		) && !mOnHighGround;
}

void Projectile::CheckForHighGround()
{
	float aShadowDelta = mShadowY - mPosY;

	if (mProjectileType == ProjectileType::PROJECTILE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_SNOWPEA ||
		mProjectileType == ProjectileType::PROJECTILE_FIREBALL ||
		mProjectileType == ProjectileType::PROJECTILE_SPIKE ||
		mProjectileType == ProjectileType::PROJECTILE_COBBIG ||
		mProjectileType == ProjectileType::PROJECTILE_PURPLE_FIRE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
	{
		if (aShadowDelta < 28.0f)
		{
			DoImpact(nullptr);
			return;
		}
	}

	if (mProjectileType == ProjectileType::PROJECTILE_PUFF && aShadowDelta < 0.0f)
	{
		DoImpact(nullptr);
		return;
	}

	if (mProjectileType == ProjectileType::PROJECTILE_STAR && aShadowDelta < 23.0f)
	{
		DoImpact(nullptr);
		return;
	}

	if (CantHitHighGround())
	{
		int aGridX = mBoard->PixelToGridXKeepOnBoard(mPosX + 30, mPosY);
		if (mBoard->mGridSquareType[aGridX][mRow] == GridSquareType::GRIDSQUARE_HIGH_GROUND)
		{
			DoImpact(nullptr);
		}
	}
}

bool Projectile::IsSplashDamage(Zombie* theZombie)
{
	if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL && theZombie && theZombie->IsFireResistant())
		return false;

	return
		mProjectileType == ProjectileType::PROJECTILE_MELON ||
		mProjectileType == ProjectileType::PROJECTILE_WINTERMELON ||
		// 紫火豌豆：群伤（命中点周围 + 相邻行都会吃到 1/3 伤害）
		mProjectileType == ProjectileType::PROJECTILE_PURPLE_FIRE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_FIREBALL;
}

unsigned int Projectile::GetDamageFlags(Zombie* theZombie)
{
	unsigned int aDamageFlags = 0U;

	if (IsSplashDamage(theZombie))
	{
		SetBit(aDamageFlags, static_cast<int>(DamageFlags::DAMAGE_HITS_SHIELD_AND_BODY), true);
	}
	else if (mMotionType == ProjectileMotion::MOTION_LOBBED || mMotionType == ProjectileMotion::MOTION_BACKWARDS)
	{
		SetBit(aDamageFlags, static_cast<int>(DamageFlags::DAMAGE_BYPASSES_SHIELD), true);
	}
	else if (mMotionType == ProjectileMotion::MOTION_STAR && (mVelX < 0.0f || mTargetZombieID != ZombieID::ZOMBIEID_NULL))
	{
		SetBit(aDamageFlags, static_cast<int>(DamageFlags::DAMAGE_BYPASSES_SHIELD), true);
	}

	if (mProjectileType == ProjectileType::PROJECTILE_SNOWPEA || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON)
	{
		SetBit(aDamageFlags, static_cast<int>(DamageFlags::DAMAGE_FREEZE), true);
	}

	return aDamageFlags;
}

bool Projectile::IsZombieHitBySplash(Zombie* theZombie)
{
	Rect aProjectileRect = GetProjectileRect();
	if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL)
	{
		aProjectileRect.mWidth = 100;
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_PURPLE_FIRE_PEA)
	{
		// 紫火豌豆：群伤范围比普通弹丸宽一圈；行方向允许上下各 1 行（走下面的通用分支）
		aProjectileRect.mWidth = FIRE_PEA_SPLASH_WIDTH;
	}

	int aRowDeviation = theZombie->mRow - mRow;
	Rect aZombieRect = theZombie->GetZombieRect();
	if (theZombie->IsFireResistant() && mProjectileType == ProjectileType::PROJECTILE_FIREBALL)
	{
		return false;
	}

	if (theZombie->mZombieType == ZombieType::ZOMBIE_BOSS)
	{
		aRowDeviation = 0;
	}
	if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL)
	{
		if (aRowDeviation != 0)
		{
			return false;
		}
	}
	else if (aRowDeviation > 1 || aRowDeviation < -1)
	{
		return false;
	}

	return theZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)) && GetRectOverlap(aProjectileRect, aZombieRect) >= 0;
}

void Projectile::DoSplashDamage(Zombie* theZombie)
{
	const ProjectileDefinition& aProjectileDef = GetProjectileDef();

	int aZombiesGetSplashed = 0;
	Zombie* aZombie = nullptr;
	while (mBoard->IterateZombies(aZombie))
	{
		if (aZombie != theZombie && IsZombieHitBySplash(aZombie))
		{
			aZombiesGetSplashed++;
		}
	}

	int aOriginalDamage = aProjectileDef.mDamage;
	int aSplashDamage = aProjectileDef.mDamage / 3;
	int aMaxSplashDamageAmount = aOriginalDamage * 7;
	if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL)
	{
		aMaxSplashDamageAmount = aOriginalDamage;
	}
	int aSplashDamageAmount = aSplashDamage * aZombiesGetSplashed;
	if (aSplashDamageAmount > aMaxSplashDamageAmount)
	{
		//aSplashDamage *= aMaxSplashDamageAmount / aSplashDamage;
		aSplashDamage = aOriginalDamage * aMaxSplashDamageAmount / (aSplashDamageAmount * 3);
		aSplashDamage = std::max(aSplashDamage, 1);
	}

	aZombie = nullptr;
	while (mBoard->IterateZombies(aZombie))
	{
		if (IsZombieHitBySplash(aZombie))
		{
			unsigned int aDamageFlags = GetDamageFlags(aZombie);
			if (aZombie == theZombie)
			{
				aZombie->TakeDamage(aOriginalDamage, aDamageFlags);
			}
			else
			{
				aZombie->TakeDamage(aSplashDamage, aDamageFlags);
			}
		}
	}

	// 紫火豌豆（群伤）：被溅射到的僵尸同样会被点着（4 秒内受到的伤害 +40%）。
	// 放在伤害结算**之后** —— 这一发本身吃不到自己的易伤加成。
	if (mProjectileType == ProjectileType::PROJECTILE_PURPLE_FIRE_PEA)
	{
		aZombie = nullptr;
		while (mBoard->IterateZombies(aZombie))
		{
			if (IsZombieHitBySplash(aZombie))
			{
				aZombie->ApplyFireVulnerability();
			}
		}
	}
}

// GOTY @Patoke: 0x471B41
void Projectile::UpdateLobMotion()
{
	if (mProjectileType == ProjectileType::PROJECTILE_COBBIG && mPosZ < -700.0f)
	{
		mVelZ = 8.0f;
		mRow = mCobTargetRow;
		mPosX = mCobTargetX;
		int aCobTargetCol = mBoard->PixelToGridXKeepOnBoard(mCobTargetX, 0);
		mPosY = mBoard->GridToPixelY(aCobTargetCol, mCobTargetRow);
		mShadowY = mPosY + 67.0f;
		mRotation = -PI / 2;
	}

	mVelZ += mAccZ;
	if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HIGH_GRAVITY)
	{
		mVelZ += mAccZ;
	}
	mPosX += mVelX;
	mPosY += mVelY;
	mPosZ += mVelZ;

	bool isRising = mVelZ < 0.0f;
	if (isRising && (mProjectileType == ProjectileType::PROJECTILE_BASKETBALL || mProjectileType == ProjectileType::PROJECTILE_COBBIG))
	{
		return;
	}
	if (mProjectileAge > 20)
	{
		if (isRising)
		{
			return;
		}

		float aMinCollisionZ = 0.0f;
		if (mProjectileType == ProjectileType::PROJECTILE_BUTTER)
		{
			aMinCollisionZ = -32.0f;
		}
		else if (mProjectileType == ProjectileType::PROJECTILE_BASKETBALL)
		{
			aMinCollisionZ = 60.0f;
		}
		else if (mProjectileType == ProjectileType::PROJECTILE_MELON || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON)
		{
			aMinCollisionZ = -35.0f;
		}
		else if (mProjectileType == ProjectileType::PROJECTILE_CABBAGE || mProjectileType == ProjectileType::PROJECTILE_KERNEL)
		{
			aMinCollisionZ = -30.0f;
		}
		else if (mProjectileType == ProjectileType::PROJECTILE_COBBIG)
		{
			aMinCollisionZ = -60.0f;
		}
		if (mBoard->mPlantRow[mRow] == PlantRowType::PLANTROW_POOL)
		{
			aMinCollisionZ += 40.0f;
		}

		if (mPosZ <= aMinCollisionZ)
		{
			return;
		}
	}

	Plant* aPlant = nullptr;
	Zombie* aZombie = nullptr;
	if (mProjectileType == ProjectileType::PROJECTILE_BASKETBALL || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA)
	{
		aPlant = FindCollisionTargetPlant();
	}
	else
	{
		aZombie = FindCollisionTarget();
	}

	float aGroundZ = 80.0f;
	if (mProjectileType == ProjectileType::PROJECTILE_COBBIG)
	{
		aGroundZ = -40.0f;
	}
	bool hitGround = mPosZ > aGroundZ;
	if (aZombie == nullptr && aPlant == nullptr && !hitGround)
	{
		return;
	}

	if (aPlant)
	{
		Plant* aUmbrellaPlant = mBoard->FindUmbrellaPlant(aPlant->mPlantCol, aPlant->mRow);
		if (aUmbrellaPlant)
		{
			if (aUmbrellaPlant->mState == PlantState::STATE_UMBRELLA_REFLECTING)
			{
				mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
				int aRenderPosition = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 1);
				mApp->AddTodParticle(mPosX + 20.0f, mPosY + 20.0f, aRenderPosition, ParticleEffect::PARTICLE_UMBRELLA_REFLECT);
				Die();
			}
			else if (aUmbrellaPlant->mState != PlantState::STATE_UMBRELLA_TRIGGERED)
			{
				mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA);
				aUmbrellaPlant->DoSpecial();
			}
		}
		else
		{
			int aProjDamage = GetProjectileDef().mDamage;
			if (Plant* aShieldNut = mBoard->FindGiantWallnutShield(aPlant->mPlantCol, aPlant->mRow); aShieldNut)
			{
				mBoard->GiantWallnutShareDamage(aProjDamage, aShieldNut);   // 巨大坚果为周围植物承伤
			}
			else if (aPlant->mSeedType == SeedType::SEED_GIANT_WALLNUT)
			{
				mBoard->GiantWallnutShareDamage(aProjDamage, aPlant);   // 巨大坚果：伤害全场分摊
			}
			else
			{
				aPlant->mPlantHealth -= aProjDamage;
			}
			aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 25);
			mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
			Die();
		}
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_COBBIG)
	{
		// @Patoke: implemented
		int aBeforeGargantuarCount = mBoard->GetLiveGargantuarCount();
		mBoard->KillAllZombiesInRadius(mRow, mPosX + 80, mPosY + 40, 115, 1, true, mDamageRangeFlags);
		int aAfterGargantuarCount = mBoard->GetLiveGargantuarCount();
		mBoard->mGargantuarsKillsByCornCob += aBeforeGargantuarCount - aAfterGargantuarCount;
		if (mBoard->mGargantuarsKillsByCornCob >= 2)
			ReportAchievement::GiveAchievement(mApp, PopcornParty, true);

		DoImpact(nullptr);
	}
	else
	{
		DoImpact(aZombie);
	}
}

void Projectile::UpdateNormalMotion()
{
	if (mMotionType == ProjectileMotion::MOTION_BACKWARDS)
	{
		mPosX -= 3.33f;
	}
	else if (mMotionType == ProjectileMotion::MOTION_HOMING)
	{
		if (mTargetZombieID == ZombieID::ZOMBIEID_NULL)
		{
			FindNewHomingTarget();
		}
		Zombie* aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
		if (aZombie && aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
		{
			Rect aZombieRect = aZombie->GetZombieRect();
			SexyVector2 aTargetCenter(aZombie->ZombieTargetLeadX(0.0f), aZombieRect.mY + aZombieRect.mHeight / 2);
			SexyVector2 aProjectileCenter(mPosX + mWidth / 2, mPosY + mHeight / 2);
			SexyVector2 aToTarget = (aTargetCenter - aProjectileCenter).Normalize();
			SexyVector2 aMotion(mVelX, mVelY);

			aMotion += aToTarget * (0.001f * mProjectileAge);
			aMotion = aMotion.Normalize();
			aMotion *= 2.0f;

			mVelX = aMotion.x;
			mVelY = aMotion.y;
			mRotation = -atan2(mVelY, mVelX);
		}

		mPosY += mVelY;
		mPosX += mVelX;
		mShadowY += mVelY;
		mRow = mBoard->PixelToGridYKeepOnBoard(mPosX, mPosY);
	}
	else if (mMotionType == ProjectileMotion::MOTION_STAR)
	{
		// 普通杨桃星星与究极电能星星果的电能星星共用同一套"飞 1 格后追踪"逻辑
		if ((mProjectileType == ProjectileType::PROJECTILE_STAR || mProjectileType == ProjectileType::PROJECTILE_ELECTRIC_STAR) && mProjectileAge >= 24)
		{
			bool aNeedNewTarget = (mTargetZombieID == ZombieID::ZOMBIEID_NULL);
			if (!aNeedNewTarget)
			{
				Zombie* aCurZombie = mBoard->ZombieTryToGet(mTargetZombieID);
				if (!aCurZombie || !aCurZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
				{
					mTargetZombieID = ZombieID::ZOMBIEID_NULL;
					aNeedNewTarget = true;
				}
			}
			if (aNeedNewTarget)
			{
				// 旧规则"只有向左飞的星星能锁矿工"只该作用在矿工真正钻地的期间：
				// 出土后他就是一个普通地面僵尸，5 颗星星都应当正常索敌。
				Zombie* aBestZombie = nullptr;
				int aMinX = INT32_MAX;
				Zombie* aZombie = nullptr;
				while (mBoard->IterateZombies(aZombie))
				{
					if (aZombie->mZombieType == ZombieType::ZOMBIE_DIGGER && aZombie->IsUnderground())
						continue;
					if (!aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
						continue;
					if (aZombie->mX < aMinX)
					{
						aMinX = aZombie->mX;
						aBestZombie = aZombie;
					}
				}
				if (aBestZombie)
				{
					mTargetZombieID = mBoard->ZombieGetID(aBestZombie);
				}
			}
		}

		if (mTargetZombieID != ZombieID::ZOMBIEID_NULL)
		{
			Zombie* aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
			if (aZombie && aZombie->EffectedByDamage(static_cast<unsigned int>(mDamageRangeFlags)))
			{
				Rect aZombieRect = aZombie->GetZombieRect();
				SexyVector2 aTargetCenter(aZombie->ZombieTargetLeadX(0.0f), aZombieRect.mY + aZombieRect.mHeight / 2);
				SexyVector2 aProjectileCenter(mPosX + mWidth / 2, mPosY + mHeight / 2);
				SexyVector2 aToTarget = (aTargetCenter - aProjectileCenter).Normalize();
				mVelX = aToTarget.x * 3.33f;
				mVelY = aToTarget.y * 3.33f;
				mRotation = -atan2(mVelY, mVelX);
			}
		}

		mPosY += mVelY;
		mPosX += mVelX;
		mShadowY += mVelY;
		if (mVelY != 0.0f || mTargetZombieID != ZombieID::ZOMBIEID_NULL)
		{
			mRow = mBoard->PixelToGridYKeepOnBoard(mPosX, mPosY);
		}
	}
	else if (mMotionType == ProjectileMotion::MOTION_BEE)
	{
		if (mProjectileAge < 60)
		{
			mPosY -= 0.5f;
		}
		mPosX += 3.33f;
	}
	else if (mMotionType == ProjectileMotion::MOTION_FLOAT_OVER)
	{
		if (mVelZ < 0.0f)
		{
			mVelZ += 0.002f;
			mVelZ = std::min(mVelZ, 0.0f);
			mPosY += mVelZ;
			mRotation = 0.3f - 0.7f * mVelZ * PI * 0.25f;
		}
		mPosX += 0.4f;
	}
	else if (mMotionType == ProjectileMotion::MOTION_BEE_BACKWARDS)
	{
		if (mProjectileAge < 60)
		{
			mPosY -= 0.5f;
		}
		mPosX -= 3.33f;
	}
	else if (mMotionType == ProjectileMotion::MOTION_WAVE)
	{
		mPosX += 3.33f;
		mPosY = mBaseY + sinf(mProjectileAge * 0.15f) * 25.0f;
	}
	else if (mMotionType == ProjectileMotion::MOTION_THREEPEATER)
	{
		mPosX += 3.33f;
		mPosY += mVelY;
		mVelY *= 0.97f;
		mShadowY += mVelY;
	}
	else
	{
		mPosX += 3.33f;
	}

	if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HIGH_GRAVITY)
	{
		if (mMotionType == ProjectileMotion::MOTION_FLOAT_OVER)
		{
			mVelZ += 0.004f;
		}
		else
		{
			mVelZ += 0.2f;
		}

		mPosY += mVelZ;
	}

	CheckForCollision();
	CheckForHighGround();
}

void Projectile::UpdateMotion()
{
	if (mAnimTicksPerFrame > 0)
	{
		mAnimCounter = (mAnimCounter + 1) % (mNumFrames * mAnimTicksPerFrame);
		mFrame = mAnimCounter / mAnimTicksPerFrame;
	}

	int aOldRow = mRow;
	float aOldY = mBoard->GetPosYBasedOnRow(mPosX, mRow);
	if (mMotionType == ProjectileMotion::MOTION_LOBBED)
	{
		UpdateLobMotion();
	}
	else
	{
		UpdateNormalMotion();
	}

	float aSlopeHeightChange = mBoard->GetPosYBasedOnRow(mPosX, aOldRow) - aOldY;
	if (mProjectileType == ProjectileType::PROJECTILE_COBBIG)
	{
		aSlopeHeightChange = 0.0f;  // Fix The Roof Offset Bug of Corn Cob
	}
	if (mMotionType == ProjectileMotion::MOTION_FLOAT_OVER)
	{
		mPosY += aSlopeHeightChange;
	}
	if (mMotionType == ProjectileMotion::MOTION_LOBBED)
	{
		mPosY += aSlopeHeightChange;
		mPosZ -= aSlopeHeightChange;
	}
	mShadowY += aSlopeHeightChange;
	mX = static_cast<int>(mPosX);
	mY = static_cast<int>(mPosY + mPosZ);
}

void Projectile::PlayImpactSound(Zombie* theZombie)
{
	bool aPlayHelmSound = true;
	bool aPlaySplatSound = true;
	if (mProjectileType == ProjectileType::PROJECTILE_KERNEL)
	{
		mApp->PlayFoley(FoleyType::FOLEY_KERNEL_SPLAT);
		aPlayHelmSound = false;
		aPlaySplatSound = false;
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_BUTTER)
	{
		mApp->PlayFoley(FoleyType::FOLEY_BUTTER);
		aPlaySplatSound = false;
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL && IsSplashDamage(theZombie))
	{
		mApp->PlayFoley(FoleyType::FOLEY_IGNITE);
		aPlayHelmSound = false;
		aPlaySplatSound = false;
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_MELON || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON)
	{
		mApp->PlayFoley(FoleyType::FOLEY_MELONIMPACT);
		aPlaySplatSound = false;
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_PURPLE_FIRE_PEA)
	{
		// 紫火豌豆：命中是一声"呼"的火焰声，不是豌豆的啪叽声
		mApp->PlayFoley(FoleyType::FOLEY_IGNITE);
		aPlaySplatSound = false;
	}

	if (aPlayHelmSound && theZombie)
	{
		if (theZombie->mHelmType == HELMTYPE_PAIL)
		{
			mApp->PlayFoley(FoleyType::FOLEY_SHIELD_HIT);
			aPlaySplatSound = false;
		}
		else if (theZombie->mHelmType == HELMTYPE_TRAFFIC_CONE || theZombie->mHelmType == HELMTYPE_DIGGER || theZombie->mHelmType == HELMTYPE_FOOTBALL)
		{
			mApp->PlayFoley(FoleyType::FOLEY_PLASTIC_HIT);
		}
	}

	if (aPlaySplatSound)
	{
		mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
	}
}

void Projectile::DoImpact(Zombie* theZombie)
{
	PlayImpactSound(theZombie);
	bool aSporeTargetWasAlive = mProjectileType == ProjectileType::PROJECTILE_SPORESHROOM &&
		theZombie != nullptr && !theZombie->IsDeadOrDying();

	if (IsSplashDamage(theZombie))
	{
		if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL && theZombie)
		{
			theZombie->RemoveColdEffects();
		}

		DoSplashDamage(theZombie);
	}
	else if (theZombie)
	{
		unsigned int aDamageFlags = GetDamageFlags(theZombie);
		int aDamage = mDamageOverride > 0 ? mDamageOverride : GetProjectileDef().mDamage;
		theZombie->TakeDamage(aDamage, aDamageFlags);
	}

	if (aSporeTargetWasAlive && theZombie->IsDeadOrDying())
	{
		mBoard->TrySpawnSporeShroom(theZombie);
	}

	// 紫火豌豆：把命中的僵尸点着（4 秒内受到的伤害 +40%）。
	// 必须在伤害结算**之后**施加 —— 这一发本身吃不到自己的易伤加成，
	// 加成是从下一发（以及这 4 秒内任何其它来源的伤害）才开始算的。
	// 群伤时溅射到的那些僵尸由 DoSplashDamage 负责（它跑在上面的分支里）。
	if (mProjectileType == ProjectileType::PROJECTILE_PURPLE_FIRE_PEA && theZombie && !IsSplashDamage(theZombie))
	{
		theZombie->ApplyFireVulnerability();
	}

	float aLastPosX = mPosX - mVelX;
	float aLastPosY = mPosY + mPosZ - mVelY - mVelZ;
	ParticleEffect aEffect = ParticleEffect::PARTICLE_NONE;
	float aSplatPosX = mPosX + 12.0f;
	float aSplatPosY = mPosY + 12.0f;
	if (mProjectileType == ProjectileType::PROJECTILE_SPORESHROOM)
	{
		Reanimation* aHitReanim = mApp->AddReanimation(mPosX, mPosY + mPosZ, mRenderOrder + 1, ReanimationType::REANIM_SPORESHROOM_PROJECTILE);
		aHitReanim->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE;
		aHitReanim->mAnimRate = 30.0f;
		aHitReanim->SetFramesForLayer(Rand(2) == 0 ? "anim_hit" : "anim_hit2");
	}
	switch (mProjectileType)
	{
	case ProjectileType::PROJECTILE_MELON:
		mApp->AddTodParticle(aLastPosX + 30.0f, aLastPosY + 30.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_MELONSPLASH);
		break;
	case ProjectileType::PROJECTILE_WINTERMELON:
		mApp->AddTodParticle(aLastPosX + 30.0f, aLastPosY + 30.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_WINTERMELON);
		break;
	case ProjectileType::PROJECTILE_COBBIG:
	{
		int aRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_GROUND, mCobTargetRow, 2);
		mApp->AddTodParticle(mPosX + 80.0f, mPosY + 40.0f, aRenderOrder, ParticleEffect::PARTICLE_BLASTMARK);
		mApp->AddTodParticle(mPosX + 80.0f, mPosY + 40.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_POPCORNSPLASH);
		mApp->PlaySample(SOUND_DOOMSHROOM);
		mBoard->ShakeBoard(3, -4);
		break;
	}
	case ProjectileType::PROJECTILE_PEA:
		aSplatPosX -= 15.0f;
		aEffect = ParticleEffect::PARTICLE_PEA_SPLAT;
		break;
	case ProjectileType::PROJECTILE_SNOWPEA:
		aSplatPosX -= 15.0f;
		aEffect = ParticleEffect::PARTICLE_SNOWPEA_SPLAT;
		break;
	case ProjectileType::PROJECTILE_PURPLE_FIRE_PEA:
		// 紫火豌豆：命中**不**炸火（原版 FireballDeath 是僵尸博士火球的爆燃粒子，
		// 20 颗 2 倍大小的火焰粒子炸开，糊一整屏），只留 FOLEY_IGNITE 的"呼"声。
		break;
	case ProjectileType::PROJECTILE_FIREBALL:
	{
		if (IsSplashDamage(theZombie))
		{
			Reanimation* aFireReanim = mApp->AddReanimation(mPosX + 38.0f, mPosY - 20.0f, mRenderOrder + 1, ReanimationType::REANIM_JALAPENO_FIRE);
			aFireReanim->mAnimTime = 0.25f;
			aFireReanim->mAnimRate = 24.0f;
			aFireReanim->OverrideScale(0.7f, 0.4f);
		}
		break;
	}
	case ProjectileType::PROJECTILE_STAR:
		aEffect = ParticleEffect::PARTICLE_STAR_SPLAT;
		break;
	case ProjectileType::PROJECTILE_PUFF:
		aSplatPosX -= 20.0f;
		aEffect = ParticleEffect::PARTICLE_PUFF_SPLAT;
		break;
	case ProjectileType::PROJECTILE_CABBAGE:
		aSplatPosX = aLastPosX - 38.0f;
		aSplatPosY = aLastPosY + 23.0f;
		aEffect = ParticleEffect::PARTICLE_CABBAGE_SPLAT;
		break;
	case ProjectileType::PROJECTILE_BUTTER:
		aSplatPosX = aLastPosX - 20.0f;
		aSplatPosY = aLastPosY + 63.0f;
		aEffect = ParticleEffect::PARTICLE_BUTTER_SPLAT;

		if (theZombie)
		{
			theZombie->ApplyButter();
		}
		break;
	default:
		break;
	}

	if (aEffect != ParticleEffect::PARTICLE_NONE)
	{
		if (theZombie)
		{
			float aPosX = aSplatPosX + 52.0f - theZombie->mX;
			float aPosY = aSplatPosY - theZombie->mY;
			if (theZombie->mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL || theZombie->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING_IN_POOL)
			{
				aPosY += 60.0f;
			}
			if (mMotionType == ProjectileMotion::MOTION_BACKWARDS)
			{
				aPosX -= 80.0f;
			}
			else if (mPosX > theZombie->mX + 40 && mMotionType != ProjectileMotion::MOTION_LOBBED)
			{
				aPosX -= 60.0f;
			}

			aPosY = ClampFloat(aPosY, 20.0f, 100.0f);
			theZombie->AddAttachedParticle(aPosX, aPosY, aEffect);
		}
		else
		{
			mApp->AddTodParticle(aSplatPosX, aSplatPosY, mRenderOrder + 1, aEffect);
		}
	}

	if (mPenetrations > 0 && theZombie)
	{
		mPenetrations--;
		mLastHitZombieID = mBoard->ZombieGetID(theZombie);
		mTargetZombieID = ZombieID::ZOMBIEID_NULL;
		return;
	}

	Die();
}

void Projectile::Update()
{
	mProjectileAge++;
	if (mApp->mGameScene != GameScenes::SCENE_PLAYING && !mBoard->mCutScene->ShouldRunUpsellBoard())
		return;

	int aTime = 20;
	if (mProjectileType == ProjectileType::PROJECTILE_PEA || 
		mProjectileType == ProjectileType::PROJECTILE_SNOWPEA || 
		mProjectileType == ProjectileType::PROJECTILE_CABBAGE || 
		mProjectileType == ProjectileType::PROJECTILE_MELON || 
		mProjectileType == ProjectileType::PROJECTILE_WINTERMELON || 
		mProjectileType == ProjectileType::PROJECTILE_KERNEL || 
		mProjectileType == ProjectileType::PROJECTILE_BUTTER || 
		mProjectileType == ProjectileType::PROJECTILE_COBBIG || 
		mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA || 
		mProjectileType == ProjectileType::PROJECTILE_SPIKE || 
		mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED ||
		mProjectileType == ProjectileType::PROJECTILE_PURPLE_FIRE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_ELECTRIC_STAR ||
		mProjectileType == ProjectileType::PROJECTILE_LASER_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_SPORESHROOM)
	{
		aTime = 0;
	}
	if (mProjectileAge > aTime)
	{
		mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PROJECTILE, mRow, 0);
	}

	if (mClickBackoffCounter > 0)
	{
		mClickBackoffCounter--;
	}
	mRotation += mRotationSpeed;

	// 电能星星的伤害节奏：每帧在这里自减一次（电能豌豆不用计数器，直接跟 mProjectileAge 对齐），
	// 实际结算在 UpdateElectricStarLinger 里按归零判定。
	if (mProjectileType == ProjectileType::PROJECTILE_ELECTRIC_STAR && mElectricDamageCountdown > 0)
	{
		mElectricDamageCountdown--;
	}

	// 究极电能弹丸（电能豌豆 / 电能星星）的链式闪电：在移动之前放电，
	// 保证同一帧里"先电击、后弹丸自身的接触伤害"，不会因为目标先被打死而漏掉电弧。
	// ⚠ 必须放在"电能星星钉住就 return"之前：钉住的星星位置本来就不动（只跟随僵尸），
	//   要是被那个 return 挡掉，钉住期间的 2.5 秒就完全不放闪电、电弧也不再跟随目标。
	UpdateElectricChainLightning();

	// 究极电能星星果的电能星星：钉住时只跟随 + 按节奏结算伤害，不再走飞行/碰撞
	if (mProjectileType == ProjectileType::PROJECTILE_ELECTRIC_STAR)
	{
		if (IsElectricStarStuck())
		{
			UpdateElectricStarLinger();
			return;
		}

		// 未钉住（首次飞行，或"目标已死、正在飞向下一个目标"）：总寿命照常流逝
		if (mLingerCountdown > 0)
		{
			mLingerCountdown--;
			if (mLingerCountdown <= 0)
			{
				Die();
				return;
			}
		}
	}

	// 激光豌豆的光束：不移动、不参与任何碰撞逻辑 ——
	// 只在出生那一帧结算一次本行的贯穿伤害，随后作为纯视觉停留数帧（见 UpdateLaserPeaBeam）。
	if (mProjectileType == ProjectileType::PROJECTILE_LASER_PEA)
	{
		UpdateLaserPeaBeam();
		return;
	}

	UpdateMotion();
	AttachmentUpdateAndMove(mAttachmentID, mPosX, mPosY + mPosZ);
}

void Projectile::Draw(Graphics* g)
{
	const ProjectileDefinition& aProjectileDef = GetProjectileDef();

	Image* aImage = nullptr;
	float aScale = 1.0f;
	switch (mProjectileType)
	{
	case ProjectileType::PROJECTILE_COBBIG:
		aImage = IMAGE_REANIM_COBCANNON_COB;
		aScale = 0.9f;
		break;
	case ProjectileType::PROJECTILE_PEA:
	case ProjectileType::PROJECTILE_ZOMBIE_PEA:
		aImage = IMAGE_PROJECTILEPEA;
		break;
	case ProjectileType::PROJECTILE_FIREPEA_RED:
		aImage = IMAGE_PROJECTILEPEA;
		break;
	case ProjectileType::PROJECTILE_PURPLE_FIRE_PEA:
		// 紫火豌豆：本体不画贴图，全部由 Initialize 里挂上的火球 reanim（紫色滤镜）负责
		aImage = nullptr;
		break;
	case ProjectileType::PROJECTILE_LASER_PEA:
		// 激光豌豆的光束：本体不画贴图。光束是"一道很长的线"，跟着弹丸渲染项画会被同层
		// 后面渲染的僵尸盖住，所以统一交给 Board::Draw 顶层的 DrawAllLaserBeams 画。
	case ProjectileType::PROJECTILE_SPORESHROOM:
		aImage = nullptr;
		break;
	case ProjectileType::PROJECTILE_SNOWPEA:
		aImage = IMAGE_PROJECTILESNOWPEA;
		break;
	case ProjectileType::PROJECTILE_FIREBALL:
		aImage = nullptr;
		break;
	case ProjectileType::PROJECTILE_SPIKE:
		aImage = IMAGE_PROJECTILECACTUS;
		break;
	case ProjectileType::PROJECTILE_STAR:
		aImage = IMAGE_PROJECTILE_STAR;
		break;
	case ProjectileType::PROJECTILE_ELECTRIC_STAR:
		// 究极电能星星果的星星：同一张星星图，绘制时过白色滤镜剪影 + 电能蓝上色（见下）
		aImage = IMAGE_PROJECTILE_STAR;
		break;
	case ProjectileType::PROJECTILE_PUFF:
		aImage = IMAGE_PUFFSHROOM_PUFF1;
		aScale = TodAnimateCurveFloat(0, 30, mProjectileAge, 0.3f, 1.0f, TodCurves::CURVE_LINEAR);
		break;
	case ProjectileType::PROJECTILE_BASKETBALL:
		aImage = IMAGE_REANIM_ZOMBIE_CATAPULT_BASKETBALL;
		aScale = 1.1f;
		break;
	case ProjectileType::PROJECTILE_CABBAGE:
		aImage = IMAGE_REANIM_CABBAGEPULT_CABBAGE;
		aScale = 1.0f;
		break;
	case ProjectileType::PROJECTILE_KERNEL:
		aImage = IMAGE_REANIM_CORNPULT_KERNAL;
		aScale = 0.95f;
		break;
	case ProjectileType::PROJECTILE_BUTTER:
		aImage = IMAGE_REANIM_CORNPULT_BUTTER;
		aScale = 0.8f;
		break;
	case ProjectileType::PROJECTILE_MELON:
		aImage = IMAGE_REANIM_MELONPULT_MELON;
		aScale = 1.0f;
		break;
	case ProjectileType::PROJECTILE_WINTERMELON:
		aImage = IMAGE_REANIM_WINTERMELON_PROJECTILE;
		aScale = 1.0f;
		break;
	default:
		TOD_ASSERT(false);
		break;
	}

	aScale *= mRenderScale;
	bool aMirror = false;
	if (mMotionType == ProjectileMotion::MOTION_BEE_BACKWARDS)
	{
		aMirror = true;
	}

	if (aImage)
	{
		TOD_ASSERT(aProjectileDef.mImageRow < aImage->mNumRows);
		TOD_ASSERT(mFrame < aImage->mNumCols);

		int aCelWidth = aImage->GetCelWidth();
		int aCelHeight = aImage->GetCelHeight();
		Rect aSrcRect(aCelWidth * mFrame, aCelHeight * aProjectileDef.mImageRow, aCelWidth, aCelHeight);
		if (mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED || mProjectileType == ProjectileType::PROJECTILE_ELECTRIC_STAR)
		{
			// 电能豌豆（蓝）：先把绿色豌豆贴图过白色滤镜取剪影（RGB=255、alpha 不变），
			// 再用正常绘制按 ELECTRIC_BLUE_* 上色 —— 只有"换颜色"能得到干净的蓝。
			// 这里用的就是植物那套电能蓝，保证豌豆与究极电能机枪射手同色。
			// 究极电能星星果的电能星星走同一条路径（黄色星星图 → 电光蓝星星）。
			// 不能用加色叠加：加色按原图像素成比例相加，绿色通道永远压不下去
			// （旧版用 4 次白色加色堆出"纯白"，实际暗边仍是绿的）。
			float aOffsetX = mPosX + aCelWidth * 0.5f;
			float aOffsetY = mPosZ + mPosY + aCelHeight * 0.5f;
			SexyTransform2D aTransform;
			TodScaleRotateTransformMatrix(aTransform, aOffsetX + mBoard->mX, aOffsetY + mBoard->mY, mRotation, aScale, aScale);
			Image* aSilhouette = FilterEffectGetImage(aImage, FilterEffect::FILTER_EFFECT_WHITE);
			TodBltMatrix(g, aSilhouette, aTransform, g->mClipRect,
				Color(ELECTRIC_BLUE_R, ELECTRIC_BLUE_G, ELECTRIC_BLUE_B, 255), g->mDrawMode, aSrcRect);
		}
		else if (FloatApproxEqual(mRotation, 0.0f) && FloatApproxEqual(aScale, 1.0f))
		{
			Rect aDestRect(0, 0, aCelWidth, aCelHeight);
			g->DrawImageMirror(aImage, aDestRect, aSrcRect, aMirror);
		}
		else
		{
			float aOffsetX = mPosX + aCelWidth * 0.5f;
			float aOffsetY = mPosZ + mPosY + aCelHeight * 0.5f;
			SexyTransform2D aTransform;
			TodScaleRotateTransformMatrix(aTransform, aOffsetX + mBoard->mX, aOffsetY + mBoard->mY, mRotation, aScale, aScale);
			TodBltMatrix(g, aImage, aTransform, g->mClipRect, Color::White, g->mDrawMode, aSrcRect);
		}
	}

	if (mAttachmentID != AttachmentID::ATTACHMENTID_NULL)
	{
		Graphics theParticleGraphics(*g);
		MakeParentGraphicsFrame(&theParticleGraphics);
		AttachmentDraw(mAttachmentID, &theParticleGraphics, false);
	}

	// 链式闪电不在这里画：它要求"永远在最顶层"，所以由 Board::Draw 在所有渲染项之后统一调用
	// DrawAllElectricChains()，画在弹丸身上会被同层后面渲染的僵尸盖住。
}

void Projectile::DrawShadow(Graphics* g)
{
	int aCelCol = 0;
	float aScale = 1.0f;
	float aStretch = 1.0f;
	float aOffsetX = mPosX - mX;
	float aOffsetY = mPosY - mY;

	int aGridX = mBoard->PixelToGridXKeepOnBoard(mX, mY);
	bool isHighGround = false;
	if (mBoard->mGridSquareType[aGridX][mRow] == GridSquareType::GRIDSQUARE_HIGH_GROUND)
	{
		isHighGround = true;
	}
	if (mOnHighGround && !isHighGround)
	{
		aOffsetY += HIGH_GROUND_HEIGHT;
	}
	else if (!mOnHighGround && isHighGround)
	{
		aOffsetY -= HIGH_GROUND_HEIGHT;
	}

	if (mBoard->StageIsNight())
	{
		aCelCol = 1;
	}

	switch (mProjectileType)
	{
	case ProjectileType::PROJECTILE_PEA:
	case ProjectileType::PROJECTILE_ZOMBIE_PEA:
	case ProjectileType::PROJECTILE_FIREPEA_RED:
		aOffsetX += 3.0f;
		break;

	case ProjectileType::PROJECTILE_SNOWPEA:
		aOffsetX += -1.0f;
		aScale = 1.3f;
		break;

	case ProjectileType::PROJECTILE_STAR:
		aOffsetX += 7.0f;
		break;

	case ProjectileType::PROJECTILE_CABBAGE:
	case ProjectileType::PROJECTILE_KERNEL:
	case ProjectileType::PROJECTILE_BUTTER:
	case ProjectileType::PROJECTILE_MELON:
	case ProjectileType::PROJECTILE_WINTERMELON:
		aOffsetX += 3.0f;
		aOffsetY += 10.0f;
		aScale = 1.6f;
		break;

	case ProjectileType::PROJECTILE_PUFF:
		return;

	case ProjectileType::PROJECTILE_ELECTRIC_STAR:
		// 钉在僵尸身上（或正在追踪）的电能星星不画地面影子
		return;

	case ProjectileType::PROJECTILE_LASER_PEA:
		// 激光光束是空中的一道射线，没有落在地面上的实体，不画影子
		return;
		
	case ProjectileType::PROJECTILE_COBBIG:
		aScale = 1.0f;
		aStretch = 3.0f;
		aOffsetX += 57.0f;
		break;

	case ProjectileType::PROJECTILE_FIREBALL:
		aScale = 1.4f;
		break;
	case ProjectileType::PROJECTILE_PURPLE_FIRE_PEA:
		// 与火球同一套体型（视觉是火球 reanim，比豌豆本体大一圈）
		aScale = 1.4f;
		break;
	case ProjectileType::PROJECTILE_SPORESHROOM:
		aScale = 0.9f;
		break;
	default:
		break;
	}

	if (mMotionType == ProjectileMotion::MOTION_LOBBED)
	{
		float aHeight = ClampFloat(-mPosZ, 0.0f, 200.0f);
		aScale *= 200.0f / (aHeight + 200.0f);
	}

	TodDrawImageCelScaledF(g, IMAGE_PEA_SHADOWS, aOffsetX, (mShadowY - mPosY + aOffsetY), aCelCol, 0, aScale * aStretch, aScale);
}

void Projectile::Die()
{
	mDead = true;

	if (mProjectileType == ProjectileType::PROJECTILE_PUFF || mProjectileType == ProjectileType::PROJECTILE_SNOWPEA)
	{
		AttachmentCrossFade(mAttachmentID, "FadeOut");
		AttachmentDetach(mAttachmentID);
	}
	else
	{
		AttachmentDie(mAttachmentID);
	}
}

Rect Projectile::GetProjectileRect()
{
	if (mProjectileType == ProjectileType::PROJECTILE_PEA || 
		mProjectileType == ProjectileType::PROJECTILE_SNOWPEA ||
		mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_PURPLE_FIRE_PEA ||
		mProjectileType == ProjectileType::PROJECTILE_FIREPEA_RED)
	{
		return Rect(mX - 15, mY, mWidth + 15, mHeight);
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_COBBIG)
	{
		return Rect(mX + mWidth / 2 - 115, mY + mHeight / 2 - 115, 230, 230);
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_MELON || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON)
	{
		return Rect(mX + 20, mY, 60, mHeight);
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL)
	{
		return Rect(mX, mY, mWidth - 10, mHeight);
	}
	else if (mProjectileType == ProjectileType::PROJECTILE_SPIKE)
	{
		return Rect(mX - 25, mY, mWidth + 25, mHeight);
	}
	else
	{
		return Rect(mX, mY, mWidth, mHeight);
	}
}

void Projectile::ConvertToFireball(int theGridX)
{
	if (mHitTorchwoodGridX == theGridX)
		return;

	mProjectileType = ProjectileType::PROJECTILE_FIREBALL;
	mHitTorchwoodGridX = theGridX;
	mApp->PlayFoley(FoleyType::FOLEY_FIREPEA);

	float aOffsetX = -25.0f;
	float aOffsetY = -25.0f;
	Reanimation* aFirePeaReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_FIRE_PEA);
	if (mMotionType == ProjectileMotion::MOTION_BACKWARDS)
	{
		aFirePeaReanim->OverrideScale(-1.0f, 1.0f);
		aOffsetX += 80.0f;
	}

	aFirePeaReanim->SetPosition(mPosX + aOffsetX, mPosY + aOffsetY);
	aFirePeaReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
	aFirePeaReanim->mAnimRate = RandRangeFloat(50.0f, 80.0f);
	AttachReanim(mAttachmentID, aFirePeaReanim, aOffsetX, aOffsetY);
}

void Projectile::ConvertToPea(int theGridX)
{
	if (mHitTorchwoodGridX == theGridX)
		return;

	AttachmentDie(mAttachmentID);
	mProjectileType = ProjectileType::PROJECTILE_PEA;
	mHitTorchwoodGridX = theGridX;
	mApp->PlayFoley(FoleyType::FOLEY_THROW);
}

ProjectileDefinition& Projectile::GetProjectileDef()
{
	ProjectileDefinition& aProjectileDef = gProjectileDefinition[mProjectileType];
	TOD_ASSERT(aProjectileDef.mProjectileType == mProjectileType);

	return aProjectileDef;
}
