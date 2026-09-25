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

#pragma once

#include <cstdint>
#include <set>
#include <string>
#include "GameObject.h"

#define MAX_MAGNET_ITEMS 5

enum PlantSubClass : int32_t
{
    SUBCLASS_NORMAL = 0,
    SUBCLASS_SHOOTER = 1
};

enum PlantWeapon : int32_t
{
    WEAPON_PRIMARY,
    WEAPON_SECONDARY
};

enum PlantOnBungeeState : int32_t
{
    NOT_ON_BUNGEE,
    GETTING_GRABBED_BY_BUNGEE,
    RISING_WITH_BUNGEE
};

enum PlantState : int32_t
{
    STATE_NOTREADY,
    STATE_READY,
    STATE_DOINGSPECIAL,
    STATE_SQUASH_LOOK,
    STATE_SQUASH_PRE_LAUNCH,
    STATE_SQUASH_RISING,
    STATE_SQUASH_FALLING,
    STATE_SQUASH_DONE_FALLING,
    STATE_GRAVEBUSTER_LANDING,
    STATE_GRAVEBUSTER_EATING,
    STATE_CHOMPER_BITING,
    STATE_CHOMPER_BITING_GOT_ONE,
    STATE_CHOMPER_BITING_MISSED,
    STATE_CHOMPER_DIGESTING,
    STATE_CHOMPER_SWALLOWING,
    STATE_POTATO_RISING,
    STATE_POTATO_ARMED,
    STATE_POTATO_MASHED,
    STATE_SPIKEWEED_ATTACKING,
    STATE_SPIKEWEED_ATTACKING_2,
    STATE_SCAREDYSHROOM_LOWERING,
    STATE_SCAREDYSHROOM_SCARED,
    STATE_SCAREDYSHROOM_RAISING,
    STATE_SUNSHROOM_SMALL,
    STATE_SUNSHROOM_GROWING,
    STATE_SUNSHROOM_BIG,
    STATE_MAGNETSHROOM_SUCKING,
    STATE_MAGNETSHROOM_CHARGING,
    STATE_BOWLING_UP,
    STATE_BOWLING_DOWN,
    STATE_CACTUS_LOW,
    STATE_CACTUS_RISING,
    STATE_CACTUS_HIGH,
    STATE_CACTUS_LOWERING,
    STATE_TANGLEKELP_GRABBING,
    STATE_COBCANNON_ARMING,
    STATE_COBCANNON_LOADING,
    STATE_COBCANNON_READY,
    STATE_COBCANNON_FIRING,
    STATE_KERNELPULT_BUTTER,
    STATE_UMBRELLA_TRIGGERED,
    STATE_UMBRELLA_REFLECTING,
    STATE_IMITATER_MORPHING,
    STATE_ZEN_GARDEN_WATERED,
    STATE_ZEN_GARDEN_NEEDY,
    STATE_ZEN_GARDEN_HAPPY,
    STATE_MARIGOLD_ENDING,
    STATE_FLOWERPOT_INVULNERABLE,
    STATE_LILYPAD_INVULNERABLE,
    STATE_UMBRELLA_KNOCKING   // proximity shove in progress
};

enum PLANT_LAYER : int32_t
{
    PLANT_LAYER_BELOW = -1,
    PLANT_LAYER_MAIN,
    PLANT_LAYER_REANIM,
    PLANT_LAYER_REANIM_HEAD,
    PLANT_LAYER_REANIM_BLINK,
    PLANT_LAYER_ON_TOP,
    NUM_PLANT_LAYERS
};

enum PLANT_ORDER : int32_t
{
    PLANT_ORDER_LILYPAD,
    PLANT_ORDER_NORMAL,
    PLANT_ORDER_PUMPKIN,
    PLANT_ORDER_FLYER,
    PLANT_ORDER_CHERRYBOMB
};

enum MagnetItemType : int32_t
{
    MAGNET_ITEM_NONE,
    MAGNET_ITEM_PAIL_1,
    MAGNET_ITEM_PAIL_2,
    MAGNET_ITEM_PAIL_3,
    MAGNET_ITEM_FOOTBALL_HELMET_1,
    MAGNET_ITEM_FOOTBALL_HELMET_2,
    MAGNET_ITEM_FOOTBALL_HELMET_3,
    MAGNET_ITEM_DOOR_1,
    MAGNET_ITEM_DOOR_2,
    MAGNET_ITEM_DOOR_3,
    //MAGNET_ITEM_PROPELLER,
    MAGNET_ITEM_POGO_1,
    MAGNET_ITEM_POGO_2,
    MAGNET_ITEM_POGO_3,
    MAGNET_ITEM_JACK_IN_THE_BOX,
    MAGNET_ITEM_LADDER_1,
    MAGNET_ITEM_LADDER_2,
    MAGNET_ITEM_LADDER_3,
    MAGNET_ITEM_LADDER_PLACED,
    MAGNET_ITEM_SILVER_COIN,
    MAGNET_ITEM_GOLD_COIN,
    MAGNET_ITEM_DIAMOND,
    MAGNET_ITEM_PICK_AXE,
    MAGNET_ITEM_SUN,
    MAGNET_ITEM_SMALLSUN,
    MAGNET_ITEM_LARGESUN
};

class MagnetItem
{
public:
    float                   mPosX;
    float                   mPosY;
    float                   mDestOffsetX;
    float                   mDestOffsetY;
    MagnetItemType          mItemType;
    ReanimationID           mSunReanimID;
};

class Coin;
class Zombie;
class Reanimation;
class TodParticleSystem;

class Plant : public GameObject
{
public:
    SeedType                mSeedType;
    int32_t                 mPlantCol;
    int32_t                 mAnimCounter;
    int32_t                 mFrame;
    int32_t                 mFrameLength;
    int32_t                 mNumFrames;
    PlantState              mState;
    int32_t                 mPlantHealth;
    int32_t                 mPlantMaxHealth;
    int32_t                 mSubclass;
    int32_t                 mDisappearCountdown;
    int32_t                 mDoSpecialCountdown;
    int32_t                 mStateCountdown;
    int32_t                 mLaunchCounter;
    int32_t                 mTorchwoodPeaCount;   // 火炬树桩已影响的子弹计数（仅火炬使用）
    int32_t                 mLaunchRate;
    Rect                    mPlantRect;
    Rect                    mPlantAttackRect;
    int32_t                 mTargetX;
    int32_t                 mTargetY;
    int32_t                 mStartRow;
    ParticleSystemID        mParticleID;
    int32_t                 mShootingCounter;
    int32_t                 mGatlingScatterCountdown;
    int32_t                 mGatlingScatterChance;
    int32_t                 mScaredyShroomLaunchRate;
    ReanimationID           mBodyReanimID;
    ReanimationID           mHeadReanimID;
    ReanimationID           mHeadReanimID2;
    ReanimationID           mHeadReanimID3;
    ReanimationID           mBlinkReanimID;
    ReanimationID           mLightReanimID;
    ReanimationID           mSleepingReanimID;
    ReanimationID           mEliteSunReanimID;
    ReanimationID           mTravelPuffLReanimID;    // 大喷菇群：左侧小喷菇动画
    ReanimationID           mTravelPuffRReanimID;    // 大喷菇群：右侧小喷菇动画
    int32_t                 mPuffLShootCounter;      // 左头独立攻击前摇计数
    int32_t                 mPuffRShootCounter;      // 右头独立攻击前摇计数
    int32_t                 mBlinkCountdown;
    int32_t                 mRecentlyEatenCountdown;
    int32_t                 mTallnutCounterCooldown;
    int32_t                 mEatenFlashCountdown;
    int32_t                 mBeghouledFlashCountdown;
    int32_t                 mGiantRegenCountdown = 500;     // 巨大坚果：回血计时（500 tick = 5 秒，每次 +200）
    float                   mShakeOffsetX;
    float                   mShakeOffsetY;
    MagnetItem              mMagnetItems[MAX_MAGNET_ITEMS];
    ZombieID                mTargetZombieID;
    int32_t                 mWakeUpCounter;
    PlantOnBungeeState      mOnBungeeState;
    SeedType                mImitaterType;
    int32_t                 mPottedPlantIndex;
    bool                    mAnimPing;
    bool                    mDead;
    int32_t                 mUmbrellaRegenCountdown = 0;          // counts down to next regen tick (starts expired => first heal after 5s)
    int32_t                 mPumpkinRegenCountdown = 0;           // counts down to next inner-plant heal tick
    bool                    mSquished;
    bool                    mIsAsleep;
    bool                    mIsOnBoard;
    bool                    mHighlighted;
    bool                    mIsElite;
    bool                    mHasFiredFirstPea = false;
    bool                    mPeater15DoubleShot = false;   // 1.5 发射手：本轮攻击是否发射第二发（每轮开始时掷骰）
    int32_t                 mPeater15UpgradeCountdown = 0; // 1.5 发射手：距可点击免费升级为双发射手还剩多少帧（0 = 已可升级）

public:
    Plant();

    void                    PlantInitialize(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType);
    void                    Update();
    void                    Animate();
    void                    Draw(Graphics* g);
    void                    MouseDown(int x, int y, int theClickCount);
    void                    DoSpecial();
    void                    Fire(Zombie* theTargetZombie, int theRow, PlantWeapon thePlantWeapon = PlantWeapon::WEAPON_PRIMARY, int theYOffset = 0);
    Zombie*                 FindTargetZombie(int theRow, PlantWeapon thePlantWeapon = PlantWeapon::WEAPON_PRIMARY);
    void                    Die();
    void                    UpdateProductionPlant();
    void                    UpdateShooter();
    bool                    FindTargetAndFire(int theRow, PlantWeapon thePlantWeapon = PlantWeapon::WEAPON_PRIMARY);
    void                    LaunchThreepeater();
    // 三线机枪射手（SEED_THREE_GATLING_PEA）：每轮起手（3 个头播开火动画 + 4 连发计数 + 开大判定）；
    // 三个头的开火动画（theLoop = true 用于大招期间持续开火）；以及"向每一行各发一轮"的批量开火
    // （theJitterY = true 时每行那一发带上 ±THREE_GATLING_HEIGHT_JITTER 的高度浮动，只在大招里用）。
    void                    LaunchThreeGatling();
    void                    PlayThreeGatlingShootAnim(bool theLoop);
    void                    FireThreeGatlingVolley(bool theJitterY);
    // 激光豌豆（SEED_LASER_PEA）：开火动画放完时调用 —— 自己重新索敌（射程内任意一行、空中优先），
    // 然后把**目标僵尸**交给 Fire()，光束才会朝它斜着射出去。
    // 为什么不沿用 FindTargetAndFire：那是"起手"（决定开不开火、播动画、置 mShootingCounter），
    // 真正出弹是在动画末尾的 UpdateShooting 里，那时目标已经查不到了。
    void                    FireLaserPea();
    static Image*           GetImage(SeedType theSeedType);
    static int              GetCost(SeedType theSeedType, SeedType theImitaterType = SeedType::SEED_NONE);
    static std::string       GetNameString(SeedType theSeedType, SeedType theImitaterType = SeedType::SEED_NONE);
    static std::string       GetToolTip(SeedType theSeedType);
    static int              GetRefreshTime(SeedType theSeedType, SeedType theImitaterType = SeedType::SEED_NONE);
    static /*inline*/ bool  IsNocturnal(SeedType theSeedtype);
    static /*inline*/ bool  IsFungus(SeedType theSeedType);
    static /*inline*/ bool  IsAquatic(SeedType theSeedType);
    static /*inline*/ bool  IsFlying(SeedType theSeedtype);
    static /*inline*/ bool  IsUpgrade(SeedType theSeedtype);
    static /*inline*/ bool  IsRedCard(SeedType theSeedtype);   // 红卡（旅行高阶卡面；当前：巨大坚果、1.5 发射手）
    static /*inline*/ bool  IsPultPlant(SeedType theSeedType); // 投手类（卷心菜 / 玉米 / 西瓜 / 冰瓜），一轮投掷的弹丸数有上限
    void                    UpdateAbilities();
    void                    Squish();
    void                    DoRowAreaDamage(int theDamage, unsigned int theDamageFlags);
    int                     GetDamageRangeFlags(PlantWeapon thePlantWeapon = PlantWeapon::WEAPON_PRIMARY);
    Rect                    GetPlantRect();
    Rect                    GetPlantAttackRect(PlantWeapon thePlantWeapon = PlantWeapon::WEAPON_PRIMARY);
    Zombie*                 FindSquashTarget();
    void                    UpdateSquash();
    /*inline*/ bool         NotOnGround();
    void                    DoSquashDamage();
    void                    BurnRow(int theRow);
    void                    IceZombies();
    void                    BlowAwayFliers();
    void                    UpdateGraveBuster();
    TodParticleSystem*      AddAttachedParticle(int thePosX, int thePosY, int theRenderPosition, ParticleEffect theEffect);
    void                    GetPeaHeadOffset(int& theOffsetX, int& theOffsetY);
    /*inline*/ bool         MakesSun();
    void                    ProduceSunCoins();
    void                    ChainProduceSun(std::set<Plant*>& theVisited);
    static void             DrawSeedType(Graphics* g, SeedType theSeedType, SeedType theImitaterType, DrawVariation theDrawVariation, float thePosX, float thePosY);
    void                    KillAllPlantsNearDoom();
    bool                    IsOnHighGround();
    void                    UpdateTorchwood();
    void                    SpawnCharmedGatlingZombie();
    void                    LaunchStarFruit();
    bool                    FindStarFruitTarget();
    void                    UpdateChomper();
    void                    DoBlink();
    void                    UpdateBlink();
    void                    PlayBodyReanim(const char* theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate);
    void                    UpdateMagnetShroom();
    MagnetItem*             GetFreeMagnetItem();
    void                    DrawMagnetItems(Graphics* g);
    void                    UpdateDoomShroom();
    void                    UpdateIceShroom();
    void                    UpdatePotato();
    int                     CalcRenderOrder();
    void                    AnimateNuts();
    void                    SetSleeping(bool theIsAsleep);
    void                    UpdateShooting();
    void                    UpdateTravelPuffHeads();
    void                    UpdateTravelPuffHead(int& theCounter, ReanimationID theReanimID, int theYDirection, bool aHasTarget);
    /*inline*/ Reanimation* GetFumeGroupPuff(bool theLeft);
    void                    SyncFumeGroupPuffs(bool theLocalToPlant = false);
    void                    FireTravelPuff(int theYDirection);
    void                    PlayTravelPuffShoot(Reanimation* thePuffReanim);
    void                    PlayTravelPuffIdle(Reanimation* thePuffReanim);
    void                    DrawShadow(Graphics* g, float theOffsetX, float theOffsetY);
    void                    UpdateScaredyShroom();
    int                     DistanceToClosestZombie();
    void                    UpdateSpikeweed();
    void                    MagnetShroomAttactItem(Zombie* theZombie);
    void                    UpdateSunShroom();
    void                    UpdateBowling();
    void                    AnimatePumpkin();
    void                    UpdateBlover();
    void                    UpdateCactus();
    void                    StarFruitFire();
    void                    ElectricStarFruitFire();
    void                    UpdateTanglekelp();
    Reanimation*            AttachBlinkAnim(Reanimation* theReanimBody);
    void                    UpdateReanimColor();
    bool                    IsUpgradableTo(SeedType theUpgradedType);
    bool                    IsPartOfUpgradableTo(SeedType theUpgradedType);
    void                    UpdateCobCannon();
    void                    CobCannonFire(int theTargetX, int theTargetY);
    void                    UpdateGoldMagnetShroom();
    /*inline*/ bool         IsOnBoard();
    void                    RemoveEffects();
    void                    UpdateCoffeeBean();
    void                    UpdateUmbrella();
    void                    UpdatePumpkin();
    bool                    FindUmbrellaTarget() const;
    void                    DoUmbrellaKnockback();
    void                    EndBlink();
    void                    AnimateGarlic();
    Coin*                   FindGoldMagnetTarget();
    void                    SpikeweedAttack();
    void                    ImitaterMorph();
    void                    UpdateImitater();
    void                    UpdateReanim();
    void                    SpikeRockTakeDamage();
    bool                    IsSpiky();
    static /*inline*/ void  PreloadPlantResources(SeedType theSeedType);
    /*inline*/ bool         IsInPlay();
    void                    UpdateNeedsFood() { ; }
    void                    PlayIdleAnim(float theRate);
    void                    UpdateFlowerPot();
    void                    UpdateLilypad();
    void                    UpdateTallnut();
    void                    GoldMagnetFindTargets();
    bool                    IsAGoldMagnetAboutToSuck();
    bool                    DrawMagnetItemsOnTop();
};

float                       PlantDrawHeightOffset(Board* theBoard, Plant* thePlant, SeedType theSeedType, int theCol, int theRow);
float                       PlantFlowerPotHeightOffset(SeedType theSeedType, float theFlowerPotScale);

// 究极电能机枪射手专用贴图（打包在 main.pak 的 reanim/ 下：ElectricGatling_head/mouth/mouth_overlay/barrel、
// EletricGatling_blink1/blink2）。首次调用时把它们替换进 REANIM_ELECTRIC_GATLINGPEA 的定义里。
// 返回 false 表示贴图缺失、或没有真正的透明通道（背景被烘焙成白色，直接用会画成白方块），
// 此时调用方应回退到"机枪射手贴图 + 电能蓝叠加"。幂等，可随时调用。
bool                        ElectricGatlingHasCustomArt();

// 专用贴图是否真正启用（= 开关打开 且 贴图可用）。染色兜底与 reanim 类型选择都以它为准。
bool                        ElectricGatlingUsesCustomArt();

// 究极电能机枪射手实际使用的 reanim 类型：专用贴图可用时用 REANIM_ELECTRIC_GATLINGPEA（并换好贴图），
// 否则退回 REANIM_GATLINGPEA —— 与"机枪射手贴图 + 电能蓝叠加"的兜底方案完全一致，
// 同时避免为同一套贴图多建一份 Atlas。
ReanimationType             ElectricGatlingReanimType();

// 寒冰机枪射手专用贴图（reanim/SnowGatling_head/mouth/mouth_overlay/barrel/blink1/blink2/helmet.png）。
// 眉毛（PeaShooter_eyebrow）没有寒冰版本，仍复用原版；缺图、无透明通道或尺寸不符时返回 false
// 并退回普通机枪射手外观。
bool                        SnowGatlingHasCustomArt();
bool                        SnowGatlingUsesCustomArt();
ReanimationType             SnowGatlingReanimType();

// 火焰机枪射手专用贴图（reanim/FireGatling_head/mouth/mouth_overlay/blink1/blink2/helmet.png）。
// 与寒冰机枪射手同一套部件与同一套安全阀，区别是**没有** FireGatling_barrel.png ——
// 枪管继续用原版机枪射手的贴图（所以这里只替换确实存在的那六张，不会触发"缺一张就整体回退"）。
// 缺图、无透明通道或尺寸不符时返回 false，退回普通机枪射手外观。
bool                        FireGatlingHasCustomArt();
bool                        FireGatlingUsesCustomArt();
ReanimationType             FireGatlingReanimType();

// 火豌豆射手专用贴图（reanim/FirePeaShooter_head/mouth/blink1/blink2.png）。
// 与既有专用贴图同一套安全阀：首次调用时把四张图按"原图指针"替换进 REANIM_FIRE_PEASHOOTER
// 的定义里（尺寸必须与 PeaShooter 同名图一致、必须带透明通道），任何一张不合格就整体回退 ——
// 该 reanim 槽位会保持 PeaShooter 原版贴图，于是植物看起来就是普通豌豆射手。
// 注意：PeaShooter.reanim 并不引用 PeaShooter_Lips，所以 FirePeaShooter_lips.png 不参与替换。
// 幂等，可随时调用。
bool                        FirePeaShooterHasCustomArt();

// 叶子位置交替显示的两张火焰（0 = FirePeaShooter_fire1，1 = FirePeaShooter_fire2）。
// 专用贴图不可用时返回 nullptr。
Image*                      FirePeaShooterFireImage(int theIndex);

// 隐藏火豌豆射手不要的六条原版装饰轨道：PeaShooter_eyebrow（新头没眉毛）与
// 头顶后面那撮小叶子中除锚点（idle_headleaf_tip_top）以外的五支
// （整撮被火取代了，留着会戳在火里/盖在火上）。
// 场上植物（body + head 两个实例）与卡面/图鉴/光标预览都要调一次。
void                        FirePeaShooterHideTracks(Reanimation* theReanim);

// 把上面那张火焰覆盖到**头顶后面那撮小叶子最上面的一支**（idle_headleaf_tip_top）上 ——
// 这条轨道属于 head 实例、且排在 anim_face（脸）**之前**，所以火画在脑袋左上后方。
// 位置/缩放/摇摆全部沿用该轨道。theReanim 为空、轨道不存在或贴图不可用时什么都不做。
// 每帧调用即可实现两张图来回切换（场上植物传 head 实例）。
void                        FirePeaShooterApplyFireOverride(Reanimation* theReanim, int theFireIndex);

// 究极电能星星果专用贴图（打包在 main.pak 的 reanim/ 下：Electric_Starfruit_body/eyes1/eyes2）。
// 与究极电能机枪射手同一套机制与同一套安全阀：首次调用时把三张图替换进
// REANIM_ELECTRIC_STARFRUIT 的定义里；缺一张 / 无透明通道 / 尺寸不符都返回 false，
// 此时调用方应回退到"杨桃贴图 + 电能蓝叠加"。幂等，可随时调用。
bool                        ElectricStarfruitHasCustomArt();

// 专用贴图是否真正启用（= 开关打开 且 贴图可用）。染色兜底与 reanim 类型选择都以它为准。
bool                        ElectricStarfruitUsesCustomArt();

// 究极电能星星果实际使用的 reanim 类型：专用贴图可用时用 REANIM_ELECTRIC_STARFRUIT（并换好贴图），
// 否则退回 REANIM_STARFRUIT。
ReanimationType             ElectricStarfruitReanimType();

// 激光豌豆（旅行红卡）：与机枪射手同一套身体/头部/头盔，只把**枪管**换成
// reanim/LaserPea_barrel_small.png 一张图（与 GatlingPea_barrel.png 同为 43x27，
// 由 tools/make-laser-pea-barrel.py 从玩家的 500x500 原图裁切缩放而来）。
// 缺图 / 无透明通道 / 尺寸不符时返回 false，调用方应退回普通机枪射手外观（四根枪管）。
bool                        LaserPeaHasCustomArt();
bool                        LaserPeaUsesCustomArt();
ReanimationType             LaserPeaReanimType();

// 隐藏激光豌豆不要的四条机枪射手轨道：多出来的三根枪管（GatlingPea_barrel2/3/4）与
// 枪口叠加层（GatlingPea_mouth_overlay）——于是整株只画一根枪管、一张嘴。
// 场上植物（body + head 两个实例）与卡面/图鉴/光标预览都要调一次。
void                        LaserPeaHideExtraTracks(Reanimation* theReanim);

// 把留下的那根枪管（GatlingPea_barrel1）整体前移 LASER_PEA_BARREL_OFFSET_X 像素
// （走轨道实例的 mShakeX，与 Plant::Fire 里光束起点的同一个偏移量成对使用）。
// 传 **head 实例**（枪管与脸在同一层）。场上植物与卡面/图鉴/光标预览都要调一次。
void                        LaserPeaShiftBarrel(Reanimation* theReanim);

// 该植物射出的弹丸是否是"究极电能"弹丸（链式闪电只挂在这两只植物身上）：
//   - 究极电能机枪射手（100% 电能豌豆）
//   - 究极电能星星果（5 颗电能星星）
// 故意不认"普通机枪射手 3% 概率的那颗电能豌豆"：那颗是机枪射手打出来的，不属于究极形态。
bool                        PlantFiresElectricChainProjectile(const Plant* thePlant);

class PlantDefinition
{
public:
    SeedType                mSeedType;
    Image**                 mPlantImage;
    ReanimationType         mReanimationType;
    int                     mPacketIndex;
    int                     mSeedCost;
    int                     mRefreshTime;
    PlantSubClass           mSubClass;
    int                     mLaunchRate;
    const char*         mPlantName;
};
extern PlantDefinition gPlantDefs[SeedType::NUM_SEED_TYPES];

/*inline*/ PlantDefinition& GetPlantDefinition(SeedType theSeedType);
