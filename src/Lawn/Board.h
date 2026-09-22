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

#ifndef __BOARD_H__
#define __BOARD_H__

#include <cstdint>

#include "../ConstEnums.h"
#include "../Sexy.TodLib/DataArray.h"
#include "widget/Widget.h"
#include "widget/ButtonListener.h"

#include "Plant.h"
#include "Zombie.h"
#include "Projectile.h"
#include "Coin.h"
#include "LawnMower.h"
#include "GridItem.h"

using namespace Sexy;

#define MAX_GRID_SIZE_X 9
#define MAX_GRID_SIZE_Y 6
#define MAX_ZOMBIES_IN_WAVE 300
#define MAX_ZOMBIE_WAVES 100
#define MAX_GRAVE_STONES MAX_GRID_SIZE_X * MAX_GRID_SIZE_Y
#define MAX_POOL_GRID_SIZE 10
#define MAX_RENDER_ITEMS 4096
#define PROGRESS_METER_COUNTER 150

class LawnApp;
class CursorObject;
class CursorPreview;
class GameButton;
class MessageWidget;
class SeedBank;
class ToolTipWidget;
class CutScene;
class Challenge;
class Reanimation;
class DataSync;
class TodParticleSystem;
namespace Sexy
{
	class Graphics;
	class ButtonWidget;
	class WidgetManager;
	class Image;
	class MTRand;
}

class HitResult
{
public:
	void*							mObject;
	GameObjectType					mObjectType;
};

class RenderItem
{
public:
	RenderObjectType				mRenderObjectType;
	int								mZPos;
	union
	{
		GameObject*					mGameObject;
		Plant*						mPlant;
		Zombie*						mZombie;
		Coin*						mCoin;
		Projectile*					mProjectile;
		CursorPreview*				mCursorPreview;
		TodParticleSystem*			mParticleSytem;
		Reanimation*				mReanimation;
		GridItem*					mGridItem;
		LawnMower*					mMower;
		BossPart					mBossPart;
		int							mBoardGridY;
	};
};
bool RenderItemSortFunc(const RenderItem& theItem1, const RenderItem& theItem2);

struct ZombiePicker
{
	int								mZombieCount;
	int								mZombiePoints;
	int								mZombieTypeCount[NUM_ZOMBIE_TYPES];
	int								mAllWavesZombieTypeCount[NUM_ZOMBIE_TYPES];
};

/*inline*/ void						ZombiePickerInitForWave(ZombiePicker* theZombiePicker);
/*inline*/ void						ZombiePickerInit(ZombiePicker* theZombiePicker);

struct PlantsOnLawn
{
	Plant*							mUnderPlant;
	Plant*							mPumpkinPlant;
	Plant*							mFlyingPlant;
	Plant*							mNormalPlant;
};

struct BungeeDropGrid
{
	TodWeightedGridArray			mGridArray[MAX_GRID_SIZE_X * MAX_GRID_SIZE_Y];
	int								mGridArrayCount;
};

class Board : public Widget, public ButtonListener
{
public:
	bool							mEndlessAiManuallyDisabled;
	LawnApp*						mApp;
	DataArray<Zombie>				mZombies;
	DataArray<Plant>				mPlants;
	DataArray<Projectile>			mProjectiles;
	DataArray<Coin>					mCoins;
	DataArray<LawnMower>			mLawnMowers;
	DataArray<GridItem>				mGridItems;
	CursorObject*					mCursorObject;
	CursorPreview*					mCursorPreview;
	MessageWidget*					mAdvice;
	SeedBank*						mSeedBank;
	GameButton*						mMenuButton;
	GameButton*						mStoreButton;
	bool							mIgnoreMouseUp;
	ToolTipWidget*					mToolTip;
	//_Font*							mDebugFont;
	CutScene*						mCutScene;
	Challenge*						mChallenge;
	bool							mPaused;
	GridSquareType					mGridSquareType[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y];
	int32_t							mGridCelLook[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y];
	int32_t							mGridCelOffset[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y][2];
	int32_t							mGridCelFog[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y + 1];
	bool							mEnableGraveStones;
	int32_t							mSpecialGraveStoneX;
	int32_t							mSpecialGraveStoneY;
	float							mFogOffset;
	int32_t							mFogBlownCountDown;
	PlantRowType					mPlantRow[MAX_GRID_SIZE_Y];
	int32_t							mWaveRowGotLawnMowered[MAX_GRID_SIZE_Y];
	int32_t							mBonusLawnMowersRemaining;
	int32_t							mIceMinX[MAX_GRID_SIZE_Y];
	int32_t							mIceTimer[MAX_GRID_SIZE_Y];
	ParticleSystemID				mIceParticleID[MAX_GRID_SIZE_Y];
	TodSmoothArray					mRowPickingArray[MAX_GRID_SIZE_Y];
	ZombieType						mZombiesInWave[MAX_ZOMBIE_WAVES][MAX_ZOMBIES_IN_WAVE];
	bool							mZombieAllowed[100];
	int32_t							mSunCountDown;
	int32_t							mNumSunsFallen;
	int32_t							mShakeCounter;
	int32_t							mShakeAmountX;
	int32_t							mShakeAmountY;
	BackgroundType					mBackground;
	int32_t							mLevel;
	int32_t							mSodPosition;
	int32_t							mPrevMouseX;
	int32_t							mPrevMouseY;
	int32_t							mSunMoney;
	int32_t							mNumWaves;
	uint32_t						mMainCounter;
	uint32_t						mEffectCounter;
	uint32_t						mDrawCount;
	int32_t							mRiseFromGraveCounter;
	int32_t							mOutOfMoneyCounter;
	int32_t							mCurrentWave;
	int32_t							mTotalSpawnedWaves;
	TutorialState					mTutorialState;
	ParticleSystemID				mTutorialParticleID;
	int32_t							mTutorialTimer;
	int32_t							mLastBungeeWave;
	int32_t							mZombieHealthToNextWave;
	int32_t							mZombieHealthWaveStart;
	int32_t							mZombieCountDown;
	int32_t							mZombieCountDownStart;
	int32_t							mHugeWaveCountDown;
	bool							mHelpDisplayed[NUM_ADVICE_TYPES];
	AdviceType						mHelpIndex;
	bool							mFinalBossKilled;
	bool							mShowShovel;
	int32_t							mGloveCooldown;      // 关卡手套剩余冷却（厘秒；0 = 可用）
	int32_t							mCoinBankFadeCount;
	DebugTextMode					mDebugTextMode;
	bool							mLevelComplete;
	int32_t							mBoardFadeOutCounter;
	int32_t							mNextSurvivalStageCounter;
	int32_t							mAutoSaveCounter;   // 周期自动存档帧计数（约 30 秒一次）
	int32_t							mScoreNextMowerCounter;
	bool							mLevelAwardSpawned;
	int32_t							mProgressMeterWidth;
	int32_t							mFlagRaiseCounter;
	int32_t							mIceTrapCounter;
	int32_t							mBoardRandSeed;
	ParticleSystemID				mPoolSparklyParticleID;
	ReanimationID					mFwooshID[MAX_GRID_SIZE_Y][12];
	int32_t							mFwooshCountDown;
	int32_t							mTimeStopCounter;
	bool							mDroppedFirstCoin;
	int32_t							mFinalWaveSoundCounter;
	int32_t							mCobCannonCursorDelayCounter;
	int32_t							mCobCannonMouseX;
	int32_t							mCobCannonMouseY;
	bool							mKilledYeti;
	bool							mMustacheMode;
	bool							mSuperMowerMode;
	bool							mFutureMode;
	bool							mPinataMode;
	bool							mDanceMode;
	bool							mDaisyMode;
	bool							mSukhbirMode;
	int								mZombieMultiplier;
	BoardResult						mPrevBoardResult;
	int32_t							mTriggeredLawnMowers;
	uint32_t						mPlayTimeActiveLevel;
	uint32_t						mPlayTimeInactiveLevel;
	int32_t							mMaxSunPlants;
	int64_t							mStartDrawTime;
	int64_t							mIntervalDrawTime;
	uint32_t						mIntervalDrawCountStart;
	float							mMinFPS;
	int32_t							mPreloadTime;
	intptr_t						mGameID;
	uint32_t						mGravesCleared;
	uint32_t						mPlantsEaten;
	uint32_t						mPlantsShoveled;
	bool							mPeaShooterUsed;										//+GOTY @Patoke: 0x5784
	bool							mCatapultPlantsUsed;									//+GOTY @Patoke: 0x5785
	bool							mMushroomAndCoffeeBeansOnly;							//+GOTY @Patoke: 0x5790
	bool							mMushroomsUsed;											//+GOTY @Patoke: 0x5791
	uint32_t						mLevelCoinsCollected;									//+GOTY @Patoke: 0x5788
	uint32_t						mGargantuarsKillsByCornCob;								//+GOTY @Patoke: 0x578C
	uint32_t						mCoinsCollected;										//+GOTY @Patoke: 0x57C8
	uint32_t						mDiamondsCollected;										//+GOTY @Patoke: 0x57CC
	uint32_t						mPottedPlantsCollected;
	uint32_t						mChocolateCollected;

public:
	Board(LawnApp* theApp);
	virtual ~Board();

	void							DisposeBoard();
	int								CountSunBeingCollected();
	void							DrawGameObjects(Graphics* g);
	void							ClearCursor();
	/*inline*/ bool					AreEnemyZombiesOnScreen();
	LawnMower*						FindLawnMowerInRow(int theRow);
	bool							EndlessAiCanRun();
	bool							EndlessAiChooseSeeds(SeedType theSeeds[SEEDBANK_MAX], SeedType& theImitaterType);
	void							EndlessAiUpdate();
	bool							EndlessAiPlant(SeedType theSeedType, int theGridX, int theGridY);
	bool							EndlessAiFireCob(Plant* theCobCannon, int theTargetX, int theTargetY);
	void							EndlessAiDisableForManualInput();
//  inline bool						SyncState(DataSync& theDataSync) { /* 未发现 */return true; }
	/*inline*/ void					SaveGame(const std::string& theFileName);
	bool							LoadGame(const std::string& theFileName);
	void							InitLevel();
	void							DisplayAdvice(const std::string& theAdvice, MessageStyle theMessageStyle, AdviceType theHelpIndex);
	void							StartLevel();
	Plant*							AddPlant(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType = SeedType::SEED_NONE);
	Projectile*						AddProjectile(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theProjectileType);
	Coin*							AddCoin(int theX, int theY, CoinType theCoinType, CoinMotion theCoinMotion);
	void							RefreshSeedPacketFromCursor();
	ZombieType						PickGraveRisingZombieType();
	// 旅行模式禁出的僵尸（当前：投篮车/投石车）。出怪池、随机抽取、刷怪三处共用
	bool							IsTravelForbiddenZombie(ZombieType theZombieType);
	ZombieType						PickZombieType(int theZombiePoints, int theWaveIndex, ZombiePicker* theZombiePicker);
	int								PickRowForNewZombie(ZombieType theZombieType);
	/*inline*/ Zombie*				AddZombie(ZombieType theZombieType, int theFromWave);
	void							SpawnZombieWave();
	void							RemoveAllZombies();
	void							RemoveCutsceneZombies();
	void							SpawnZombiesFromGraves();
	PlantingReason					CanPlantAt(int theGridX, int theGridY, SeedType theSeedType);
	virtual void					MouseMove(int x, int y);
	virtual void					MouseDrag(int x, int y);
	virtual void					MouseDown(int x, int y, int theClickCount);
	virtual void					MouseUp(int x, int y, int theClickCount);
	virtual void					KeyChar(char theChar);
	virtual void					KeyUp(KeyCode) {}
	virtual void					KeyDown(KeyCode theKey);
	virtual void					MouseWheel(int theDelta);
	virtual void					Update();
	void							UpdateLayers();
	virtual void					Draw(Graphics* g);
	void							DrawBackdrop(Graphics* g);
	virtual void					ButtonPress  	(int){}
	virtual void					ButtonDepress	(int){}
	virtual void					ButtonDownTick	(int){}
	virtual void					ButtonMouseEnter(int){}
	virtual void					ButtonMouseLeave(int){}
	virtual void					ButtonMouseMove(int, int, int){}
	/*inline*/ void					AddSunMoney(int theAmount);
	bool							TakeSunMoney(int theAmount);
	/*inline*/ bool					CanTakeSunMoney(int theAmount);
	/*inline*/ void					Pause(bool thePause);
	inline bool						MakeEasyZombieType() { /* 未发现 */return false; }
	void							TryToSaveGame();
	void							AutoSaveGame();
	/*inline*/ bool					NeedSaveGame();
	/*inline*/ bool					RowCanHaveZombies(int theRow);
	void							ProcessDeleteQueue();
	bool							ChooseSeedsOnCurrentLevel();
	int								GetNumSeedsInBank();
	/*inline*/ bool					StageIsNight();
	/*inline*/ bool					StageHasPool();
	/*inline*/ bool					StageHas6Rows();
	/*inline*/ bool					StageHasFog();
	/*inline*/ bool					StageIsDayWithoutPool();
	/*inline*/ bool					StageIsDayWithPool();
	bool							StageHasGraveStones();
	int								PixelToGridX(int theX, int theY);
	int								PixelToGridY(int theX, int theY);
	/*inline*/ int					GridToPixelX(int theGridX, int theGridY);
	int								GridToPixelY(int theGridX, int theGridY);
	/*inline*/ int					PixelToGridXKeepOnBoard(int theX, int theY);
	/*inline*/ int					PixelToGridYKeepOnBoard(int theX, int theY);
	void							UpdateGameObjects();
	bool							MouseHitTest(int x, int y, HitResult* theHitResult);
	void							MouseDownWithPlant(int x, int y, int theClickCount);
	void							MouseDownWithTool(int x, int y, int theClickCount, CursorType theCursorType);
//	inline void						MouseDownNormal(int x, int y, int theClickCount) { /* 未发现 */; }
	bool							CanInteractWithBoardButtons();
	void							DrawProgressMeter(Graphics* g);
	void							UpdateToolTip();
	Plant*							GetTopPlantAt(int theGridX, int theGridY, PlantPriority thePriority);
	void							GetPlantsOnLawn(int theGridX, int theGridY, PlantsOnLawn* thePlantOnLawn);
	/*inline*/ int					CountSunFlowers();
	int								GetSeedPacketPositionX(int theIndex);
	void							InitLoneWolf();
	void							MoveLoneWolf(KeyCode theKey);
	void							AddGraveStones(int theGridX, int theCount, MTRand& theLevelRNG);
	int								GetGraveStoneCount();
	void							ZombiesWon(Zombie* theZombie = nullptr);
	void							DrawLevel(Graphics* g);
	void							DrawShovel(Graphics* g);
	void							UpdateZombieSpawning();
	void							UpdateSunSpawning();
	/*inline*/ void					ClearAdvice(AdviceType theHelpIndex);
	bool							RowCanHaveZombieType(int theRow, ZombieType theZombieType);
	/*inline*/ int					NumberZombiesInWave(int theWaveIndex);
	int								TotalZombiesHealthInWave(int theWaveIndex);
	void							DrawDebugText(Graphics* g);
	void							DrawUICoinBank(Graphics* g);
	/*inline*/ void					ShowCoinBank(int theDuration = 1000);
	void							FadeOutLevel();
	void							DrawFadeOut(Graphics* g);
	void							DrawIce(Graphics* g, int theGridY);
	bool							IsIceAt(int theGridX, int theGridY);
	/*inline*/ ZombieID				ZombieGetID(Zombie* theZombie);
	/*inline*/ Zombie*				ZombieGet(ZombieID theZombieID);
	/*inline*/ Zombie*				ZombieTryToGet(ZombieID theZombieID);

	// ============================================================
	// 冰冻关卡（GAMEMODE_CHALLENGE_ICE）沙盒玩法
	// 进关后通过“背包”按钮在关内自选任意植物/任意僵尸放到任意格，
	// 无限阳光、免费放置、不判定输赢；铲子挖植物、橡皮擦卡删除任意单位。
	// ============================================================
	/*inline*/ bool					IsIceSandboxLevel();
	Rect							GetIceBagButtonRect();
	/*inline*/ void					IceBagOpenToggle();
	void							DrawIceSandboxUI(Graphics* g);
	bool							IceSandboxHandleMouseDown(int x, int y, int theClickCount);
	void							IceSandboxArmPlant(SeedType theSeed);
	void							IceSandboxArmZombie(ZombieType theZombieType);
	void							IceSandboxArmEraser();
	void							IceSandboxDisarm();
	bool							IceSandboxPlaceArmedAt(int x, int y);
	bool							IceSandboxBagMouseDown(int x, int y, int theClickCount);
	void							IceSandboxBagDraw(Graphics* g);
	int								mIceBagPage;        // 0=植物页  1=僵尸页  2=旅行页  3=工具页
	int								mIceBagHover;       // 悬停卡槽（调试可选）
	bool							mIceBagOpen;
	int								mIceArmKind;        // 0=无 1=植物 2=僵尸 3=橡皮擦
	SeedType						mIceArmedSeed;
	ZombieType						mIceArmedZombie;
	void							DrawDebugObjectRects(Graphics* g);
	void							UpdateIce();
	/*inline*/ int					GetIceZPos(int theRow);
	/*inline*/ bool					CanAddBobSled();
	/*inline*/ void					ShakeBoard(int theShakeAmountX, int theShakeAmountY);
	int								CountUntriggerLawnMowers();
	bool							IterateZombies(Zombie*& theZombie);
	bool							IteratePlants(Plant*& thePlant);
	bool							IterateProjectiles(Projectile*& theProjectile);
	bool							IterateCoins(Coin*& theCoin);
	bool							IterateLawnMowers(LawnMower*& theLawnMower);
	bool							IterateParticles(TodParticleSystem*& theParticle);
	bool							IterateReanimations(Reanimation*& theReanimation);
	bool							IterateGridItems(GridItem*& theGridItem);
	/*inline*/ Zombie*				AddZombieInRow(ZombieType theZombieType, int theRow, int theFromWave);
	/*inline*/ bool					IsPoolSquare(int theGridX, int theGridY);
	void							PickZombieWaves();
	void							SetupCricketFight();
	void							RestartCricketMatch();
	void							RecordCricketMatchResult(bool thePlantsWon);
	void							DrawCricketStatsPanel(Graphics* g);
	SeedType					mCricketBattlePlants[5];
	ZombieType					mCricketBattleZombies[6];
	bool						mCricketMatchRecorded;   // 每场战斗只记录一次结果
	int							mCricketStatsPanel;   // 0=关, 1=植物, 2=僵尸
	int							mCricketStatsScroll;

	// ============================================================
	// 斗蛐蛐 2（GAMEMODE_CHALLENGE_CRICKET_2）录制沙盒
	// 5 行草坪：选卡后自由布阵（无限阳光），左下「出怪设置」自选僵尸种类与出怪倍率，
	// 底部中央「开始战斗」把选中的僵尸一次性全部刷出；纯沙盒，不判负、不通关。
	// 「开始战斗」用与坚不可摧同一套 DrawStoneButton 石头按钮画法，但点击由本类
	// 在 MouseDown 里自接管（可抢在种植之前消费点击，避免误种）。
	// ============================================================
	/*inline*/ bool					IsCricket2Level();
	Rect							GetCricket2PanelButtonRect();
	Rect							GetCricket2StartButtonRect();
	void							CricketFight2StartBattle();
	void							CricketFight2EndBattle();
	/*inline*/ void					CricketFight2OpenPanel();
	void							CricketFight2DrawPanel(Graphics* g);
	bool							CricketFight2PanelMouseDown(int x, int y, int theClickCount);
	bool							CricketFight2HandleMouseDown(int x, int y, int theClickCount);
	void							DrawCricket2UI(Graphics* g);
	int								Cricket2SelectedTypeCount();
	int								Cricket2ZombieTotal();
	bool						mCricket2Prep;             // true = 准备阶段（不出任何僵尸）
	bool						mCricket2ZombieEnabled[ZombieType::NUM_ZOMBIE_TYPES];
	int							mCricket2ZombieMultiplier; // 1..50：每种已选僵尸各出几只
	bool						mCricket2PanelOpen;
	void							StopAllZombieSounds();
	/*inline*/ bool					HasLevelAwardDropped();
	void							UpdateProgressMeter();
	void							DrawUIBottom(Graphics* g);
	void							DrawUITop(Graphics* g);
	Zombie*							ZombieHitTest(int theMouseX, int theMouseY);
	Plant*						PlantHitTest(int theMouseX, int theMouseY);
	void							KillAllPlantsInRadius(int theX, int theY, int theRadius);
	void							GiantWallnutShareDamage(int theTotalDamage, Plant* theSourcePlant);   // 巨大坚果：伤害全场分摊
	Plant*							FindGiantWallnutShield(int theGridX, int theGridY);   // 巨大坚果：为周围 3x4 范围植物承伤的守护者
	Plant*							GetPumpkinAt(int theGridX, int theGridY);
	Plant*							GetFlowerPotAt(int theGridX, int theGridY);
	static bool						CanZombieSpawnOnLevel(ZombieType theZombieType, int theLevel);
	bool							IsZombieWaveDistributionOk();
	void							PickBackground();
	void							InitZombieWaves();
	void							InitSurvivalStage();
	void							InitTravelJourneyRound();   // 旅行模式：切换本轮的泳池/迷雾背景
	static /*inline*/ int			MakeRenderOrder(RenderLayer theRenderLayer, int theRow, int theLayerOffset);
	void							UpdateGame();
	void							InitZombieWavesForLevel(int theForLevel);
	unsigned int					SeedNotRecommendedForLevel(SeedType theSeedType);
	void							DrawTopRightUI(Graphics* g);
	void							DrawFog(Graphics* g);
	void							UpdateFog();
	/*inline*/ int					LeftFogColumn();
	static /*inline*/ bool			IsZombieTypePoolOnly(ZombieType theZombieType);
	void							DropLootPiece(int thePosX, int thePosY, int theDropFactor);
	void							UpdateLevelEndSequence();
	LawnMower*						GetBottomLawnMower();
	bool							CanDropLoot();
	ZombieType						GetIntroducedZombieType();
	void							PickSpecialGraveStone();
	float							GetPosYBasedOnRow(float thePosX, int theRow);
	void							NextWaveComing();
	bool							BungeeIsTargetingCell(int theGridX, int theGridY);
	/*inline*/ int					PlantingPixelToGridX(int theX, int theY, SeedType theSeedType);
	/*inline*/ int					PlantingPixelToGridY(int theX, int theY, SeedType theSeedType);
	Plant*							FindUmbrellaPlant(int theGridX, int theGridY);
	void							SetTutorialState(TutorialState theTutorialState);
	void							DoFwoosh(int theRow);
	void							UpdateFwoosh();
	Plant*							SpecialPlantHitTest(int x, int y);
	void							UpdateMousePosition();
	/*inline*/ Plant*				ToolHitTestHelper(HitResult* theHitResult);
	/*inline*/ Plant*				ToolHitTest(int theX, int theY);
	bool							CanAddGraveStoneAt(int theGridX, int theGridY);
	void							UpdateGridItems();
	/*inline*/ GridItem*			AddAGraveStone(int theGridX, int theGridY);
	int								GetSurvivalFlagsCompleted();
	bool							HasProgressMeter();
	void							UpdateCursor();
	void							UpdateTutorial();
	SeedType						GetSeedTypeInCursor();
	/*inline*/ int					CountPlantByType(SeedType theSeedType);
	bool							PlantingRequirementsMet(SeedType theSeedType);
	bool							HasValidCobCannonSpot();
	bool							IsValidCobCannonSpot(int theGridX, int theGridY);
	bool							IsValidCobCannonSpotHelper(int theGridX, int theGridY);
	void							MouseDownCobcannonFire(int x, int y, int theClickCount);
	int								KillAllZombiesInRadius(int theRow, int theX, int theY, int theRadius, int theRowRange, bool theBurn, int theDamageRangeFlags); // @Patoke: modified function prototype
	/*inline*/ int					GetSeedBankExtraWidth();
	bool							IsFlagWave(int theWaveNumber);
	void							DrawHouseDoorTop(Graphics* g);
	void							DrawHouseDoorBottom(Graphics* g);
	Zombie*							GetBossZombie();
	bool							HasConveyorBeltSeedBank();
	/*inline*/ bool					StageHasRoof();
	bool							IsSnowyDayFinalStage();
	bool							IsSnowyDayStageWithRepick();
	void							SpawnZombiesFromPool();
	void							SpawnZombiesFromSky();
	void							PickUpTool(GameObjectType theObjectType);
	void							TutorialArrowShow(int theX, int theY);
	void							TutorialArrowRemove();
	int								CountCoinsBeingCollected();
	void							BungeeDropZombie(BungeeDropGrid* theBungeeDropGrid, ZombieType theZombieType);
	void							SetupBungeeDrop(BungeeDropGrid* theBungeeDropGrid);
	/*inline*/ void					PutZombieInWave(ZombieType theZombieType, int theWaveNumber, ZombiePicker* theZombiePicker);
	/*inline*/ void					PutInMissingZombies(int theWaveNumber, ZombiePicker* theZombiePicker);
	Rect							GetShovelButtonRect();
	void							GetZenButtonRect(GameObjectType theObjectType, Rect& theRect);

	// ============================================================
	// 关卡手套：在禅境花园商店买下「园艺手套」后，任意关卡都能用的搬植物道具。
	// 按钮画在铲子旁边；其他模式冷却 10 秒、所有旅行模式无冷却。
	// 禅境花园/智慧树保留原有的花园手套（GetZenButtonRect 那一套），不走这里。
	// ============================================================
	bool							CanUseLevelGlove();
	bool							IsGloveToolbarReady();
	int								GetGloveCooldownDuration();
	Rect							GetGloveButtonRect();
	void							DrawGloveButton(Graphics* g);
	void							PickUpPlantWithGlove(Plant* thePlant);
	Plant*							GetGlovePlant();
	bool							GloveCanMovePlantTo(Plant* thePlant, int theGridX, int theGridY);
	void							MovePlantWithGlove(Plant* thePlant, int theGridX, int theGridY);
	Plant*							NewPlant(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType = SeedType::SEED_NONE);
	void							DoPlantingEffects(int theGridX, int theGridY, Plant* thePlant);
	bool							IsFinalSurvivalStage();
	void							SurvivalSaveScore();
	int								CountZombiesOnScreen();
	int								GetLiveGargantuarCount(); // @Patoke: implemented
	/*inline*/ int					GetNumWavesPerSurvivalStage();
	int								GetLevelRandSeed();
	void							AddBossRenderItem(RenderItem* theRenderList, int& theCurRenderItem, Zombie* theBossZombie);
	/*inline*/ GridItem*			GetCraterAt(int theGridX, int theGridY);
	/*inline*/ GridItem*			GetGraveStoneAt(int theGridX, int theGridY);
	/*inline*/ GridItem*			GetLadderAt(int theGridX, int theGridY);
	/*inline*/ GridItem*			AddALadder(int theGridX, int theGridY);
	/*inline*/ GridItem*			AddACrater(int theGridX, int theGridY);
	void							InitLawnMowers();
	/*inline*/ bool					IsPlantInCursor();
	void							HighlightPlantsForMouse(int theMouseX, int theMouseY);
	void							ClearFogAroundPlant(Plant* thePlant, int theSize);
	/*inline*/ void					RemoveParticleByType(ParticleEffect theEffectType);
	/*inline*/ GridItem*			GetScaryPotAt(int theGridX, int theGridY);
	void							PuzzleSaveStreak();
	/*inline*/ void					ClearAdviceImmediately();
	/*inline*/ bool					IsFinalScaryPotterStage();
	/*inline*/ void					DisplayAdviceAgain(const std::string& theAdvice, MessageStyle theMessageStyle, AdviceType theHelpIndex);
	GridItem*						GetSquirrelAt(int theGridX, int theGridY);
	GridItem*						GetZenToolAt(int theGridX, int theGridY);
	bool							IsPlantInGoldWateringCanRange(int theMouseX, int theMouseY, Plant* thePlant);
	bool							StageHasZombieWalkInFromRight();
	void							PlaceRake();
	GridItem*						GetRake();
	/*inline*/ bool					IsScaryPotterDaveTalking();
	/*inline*/ Zombie*				GetWinningZombie();
	/*inline*/ void					ResetFPSStats();
	int								CountEmptyPotsOrLilies(SeedType theSeedType);
	GridItem*						GetGridItemAt(GridItemType theGridItemType, int theGridX, int theGridY);
	bool							ProgressMeterHasFlags();
	/*inline*/ bool					IsLastStandFinalStage();
	/*inline*/ int					GetNumWavesPerFlag();
	int								GetCurrentPlantCost(SeedType theSeedType, SeedType theImitaterType);
	/*inline*/ bool					PlantUsesAcceleratedPricing(SeedType theSeedType);
	void							FreezeEffectsForCutscene(bool theFreeze);
	void							LoadBackgroundImages();
	bool							CanUseGameObject(GameObjectType theGameObject);
	void							SetMustacheMode(bool theEnableMustache);
	int								CountCoinByType(CoinType theCoinType);
	void							SetSuperMowerMode(bool theEnableSuperMower);
	void							DrawZenWheelBarrowButton(Graphics* g, int theOffsetY);
	void							DrawZenButtons(Graphics* g);
	/*inline*/ void					OffsetYForPlanting(int& theY, SeedType theSeedType);
	void							SetDanceMode(bool theEnableDance);
	void							SetFutureMode(bool theEnableFuture);
	void							SetPinataMode(bool theEnablePinata);
	void							SetDaisyMode(bool theEnableDaisy);
	void							SetSukhbirMode(bool theEnableSukhbir);
	bool							MouseHitTestPlant(int x, int y, HitResult* theHitResult);
	
	/*inline*/ Reanimation*			CreateRakeReanim(float theRakeX, float theRakeY, int theRenderOrder);
	void							CompleteEndLevelSequenceForSaving();
	void							RemoveZombiesForRepick();
	int								GetGraveStonesCount();
	/*inline*/ bool					IsSurvivalStageWithRepick();
	/*inline*/ bool					IsLastStandStageWithRepick();
	void							DoTypingCheck(KeyCode theKey);
	int								CountZombieByType(ZombieType theZombieType);
	static /*inline*/ bool			IsZombieTypeSpawnedOnly(ZombieType theZombieType);
};
extern bool gShownMoreSunTutorial;

int									GetRectOverlap(const Rect& rect1, const Rect& rect2);
bool								GetCircleRectOverlap(int theCircleX, int theCircleY, int theRadius, const Rect& theRect);
/*inline*/ void						BoardInitForPlayer();

#endif // __BOARD_H__
