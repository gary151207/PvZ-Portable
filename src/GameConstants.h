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

#include "ConstEnums.h"
constexpr const double PI = 3.141592653589793;

// ============================================================
// Constants
// ============================================================
constexpr const int BOARD_WIDTH = 800;
constexpr const int BOARD_HEIGHT = 600;
constexpr const int WIDE_BOARD_WIDTH = 800;
constexpr const int BOARD_OFFSET = 220;
constexpr const int BOARD_EDGE = -100;
constexpr const int BOARD_IMAGE_WIDTH_OFFSET = 1180;
constexpr const int BOARD_ICE_START = 800;
constexpr const int LAWN_XMIN = 40;
constexpr const int LAWN_YMIN = 80;
constexpr const float GLOOM_KNOCKBACK = 35.0f;   // per-hit knockback distance for Gloom-shroom (pixels, +X)
constexpr const float UMBRELLA_KNOCKBACK        = 40.0f;  // per-hit knockback distance for Umbrella Leaf (pixels, +X)
constexpr const int   UMBRELLA_KNOCKBACK_DAMAGE = 50;     // HP the leaf loses per shove (self-exhaustion cost)
constexpr const int   UMBRELLA_KNOCKBACK_COOLDOWN = 90;   // frames of attack+cooldown per cycle (~0.9s @ 100fps)
constexpr const int   UMBRELLA_REGEN_AMOUNT      = 25;     // HP recovered per regen tick
constexpr const int   UMBRELLA_REGEN_COOLDOWN    = 500;    // frames between regen ticks (~5s @ 100fps)
constexpr const int   PUMPKIN_HEAL_AMOUNT        = 45;     // HP recovered per inner-plant heal tick
constexpr const int   PUMPKIN_HEAL_COOLDOWN      = 200;    // frames between heal ticks (~2s @ 100fps)
constexpr const int   PEATER_1_5_UPGRADE_SECONDS = 15;     // 1.5 发射手：种下后多少秒可手动点击升级为双发射手
constexpr const int   PEATER_1_5_UPGRADE_FPS     = 100;    // 逻辑帧率（Sexy.TodLib/Reanimator.h: SECONDS_PER_UPDATE = 0.01）
constexpr const int   PEATER_1_5_UPGRADE_DELAY   = PEATER_1_5_UPGRADE_FPS * PEATER_1_5_UPGRADE_SECONDS;
// 电能蓝：究极电能机枪射手（头部实例）与电能豌豆**共用同一个颜色**，保证两者看起来一致。
//   - 植物走"白色滤镜叠加绘制"：最终色 = 本颜色 * A/255 + 原色 * (1 - A/255)
//     （A=245 → 约等于本颜色，只掺进 4% 原色以保留极淡的五官与描边）。
//     必须用叠加而不是"额外加色绘制"：加色按原图像素成比例相加，头部贴图蓝通道只有 ~36，
//     再加也压不掉绿色（之前几版一直偏黄绿的根因）。
//   - 豌豆走"白色滤镜剪影 + 正常绘制上色"：最终色 = 本颜色（alpha 255），所以两者色调一致。
// 头盔轨道同时豁免加色与叠加 → 保持原色；底部叶/茎所在的 body 实例完全不着色。
// 调色只看这三个数：整体越接近 255,255,255 越白，压低 R/G 越蓝。
constexpr const int   ELECTRIC_BLUE_R = 200;
constexpr const int   ELECTRIC_BLUE_G = 240;
constexpr const int   ELECTRIC_BLUE_B = 255;
// 只影响植物的叠加强度（越小越能看见原本的明暗/五官，越大越接近纯色）。
constexpr const int   ELECTRIC_GATLING_TINT_A = 245;
// 究极电能机枪射手专用贴图（reanim/ElectricGatling_*.png）接入开关。
// 启用条件：这六张图必须是**带透明通道**的 PNG（背景不能烘焙成白底），且尺寸与机枪射手同名贴图一致
// （reanim 按帧索引，尺寸不一致会串帧）。程序里仍有兜底校验：任何一张不合格就整体回退到
// "机枪射手贴图 + 电能蓝叠加"。
constexpr const bool  ELECTRIC_GATLING_USE_CUSTOM_ART = true;
// 究极电能机枪射手 / 究极电能星星果共用的"电能伤害节奏"：
//   两只究极植物的弹丸都是持续接触型伤害（电能豌豆无限穿透、电能星星钉住目标），
//   但不再"每游戏刻（10ms）结算一次"，而是每 ELECTRIC_DAMAGE_INTERVAL_TICKS 刻
//   （15 刻 → 0.15 秒）对接触中的目标结算一次 30 点伤害。
constexpr const int   ELECTRIC_DAMAGE_INTERVAL_TICKS = 15;    // 两次电能伤害之间的游戏刻数（100fps → 0.15 秒）
// 究极电能星星果（旅行红卡 + 升级卡：由杨桃升级，300 阳光 / 30.01s 冷却）：
//   5 颗追踪的电能星星，命中后钉在该僵尸身上 2.5 秒，期间每 0.15 秒（与电能豌豆同一口径）
//   造成 30 点伤害；其余机制与杨桃完全一致。
constexpr const int   ELECTRIC_STAR_HIT_DAMAGE   = 30;    // 钉住期间每次电能伤害的数值
constexpr const int   ELECTRIC_STAR_LINGER_TICKS = 250;   // 钉住时长（游戏刻；100fps → 2.5 秒）
// 究极形态互换：把"另一种基础植物"种在究极形态上 = 原地变身，并返还这么多阳光。
//   杨桃(125)@究极电能机枪射手 → 究极电能星星果；机枪射手(250)@究极电能星星果 → 究极电能机枪射手。
constexpr const int   ELECTRIC_STARFRUIT_SWITCH_REFUND = 225;
// 机枪射手合成（寒冰射手 @ 机枪射手 → 寒冰机枪射手 / 火豌豆射手 @ 机枪射手 → 火焰机枪射手）的返阳光：
// 两张卡都是 175 阳光，合成时照常按卡价扣款，落地后**原样返还** → 净花费 0（等于"升级免费"，
// 但保留了正常的扣款/卡槽冷却流程）。与究极形态互换用同一套做法。
// 只在**真的扣过款**时返还：传送带关卡与免费种植（mEasyPlantingCheat）本来就不扣钱，
// 无条件返还等于白送阳光。
constexpr const int   GATLING_SYNTHESIS_REFUND = 175;
// 究极电能星星果专用贴图（reanim/Electric_Starfruit_*.png）接入开关。
// 与究极电能机枪射手同一套规则：三张图必须带透明通道且尺寸与同名原图一致，
// 任何一张不合格就整体回退到"杨桃贴图 + 电能蓝叠加"。
constexpr const bool  ELECTRIC_STARFRUIT_USE_CUSTOM_ART = true;
// 究极电能机枪射手 / 究极电能星星果的弹丸（电能豌豆、电能星星）共用的"链式闪电"：
//   弹丸在飞行/钉住期间，持续向周围半径 ELECTRIC_CHAIN_RADIUS 像素内**最近**的
//   至多 ELECTRIC_CHAIN_MAX_TARGETS 只僵尸放电，伤害与频率都只有弹丸本身的一半
//   （本身为每 15 刻 30 点 → 闪电为每 30 刻 15 点），因此链式闪电是纯粹的额外收益。
//   半径两格 = 80px/格 × 2 = 160px（纵向一格 85~100px，故用圆形半径统一衡量）。
//   伤害节奏只跟 mProjectileAge 对齐（不新增存档字段）。
//   电弧观感：每次结算后的 ELECTRIC_CHAIN_ARC_TICKS 刻内都画电弧，越接近下次结算越亮，
//   这样电弧"几乎一直在"（持续放电的观感），而且每帧重算目标 → 僵尸走动时电弧一路跟着走。
constexpr const int   ELECTRIC_CHAIN_INTERVAL_TICKS = 2 * ELECTRIC_DAMAGE_INTERVAL_TICKS;   // 30 刻 = 0.3 秒
constexpr const float ELECTRIC_CHAIN_RADIUS         = 160.0f;   // 两格
constexpr const int   ELECTRIC_CHAIN_MAX_TARGETS    = 5;
constexpr const int   ELECTRIC_CHAIN_ARC_TICKS      = 26;       // 每次结算后电弧可见的刻数（≈间隔-4，留一点熄灭间隙）

// ===== 火豌豆射手（旅行红卡 SEED_FIRE_PEASHOOTER）=====
// 射速：1.125 秒一发。100 逻辑帧/秒 → 112.5 帧，向上取整成 113；
// 实际间隔与豌豆射手同一口径，是 `mLaunchRate - Rand(15)`（99~113 帧 ≈ 0.99~1.13 秒）。
constexpr const int   FIRE_PEASHOOTER_LAUNCH_RATE     = 113;
// 叶子位置那两张火焰图（reanim/FirePeaShooter_fire1/fire2.png）的切换节奏：
// 每 8 逻辑帧（0.08 秒）在两张之间来回切，形成火苗跳动的观感。
constexpr const int   FIRE_PEASHOOTER_FIRE_FLIP_TICKS = 8;
// 命中效果：紫火豌豆命中后，该僵尸 4 秒（400 逻辑帧）内**受到的任何伤害 +40%**，
// 4 秒后自动恢复原样；期间被再次命中只把计时刷新为满 4 秒（不叠加倍率）。
// 群伤：紫火豌豆现在是溅射弹（命中点周围一片 + 相邻行都会吃到 1/3 伤害），
// 被溅射到的僵尸同样会被点着。
constexpr const int   FIRE_PEA_VULN_TICKS   = 400;
constexpr const int   FIRE_PEA_VULN_PERCENT = 40;
// 溅射范围：以命中点为中心的矩形宽度（高度沿用弹丸自身 40，行方向允许上下各 1 行）。
constexpr const int   FIRE_PEA_SPLASH_WIDTH = 100;
// "变红"用的**和"冰冻/减速"完全同一套画法**：动画的覆写色（正片叠底）+ 同色加色叠加。
// 冰冻那套是 (75,75,255)（蓝），这里把蓝色通道搬到红色通道上 → (255,75,75)。
// 想让红更淡就把 G/B 往 255 抬（越接近 255 越灰白），想更红就往下压。
constexpr const int   FIRE_PEA_VULN_R = 255;
constexpr const int   FIRE_PEA_VULN_G = 75;
constexpr const int   FIRE_PEA_VULN_B = 75;
// 火焰在脑袋后面那撮小叶子上的位置微调（屏幕像素；植物 1:1 绘制，所以 1 单位 = 1 像素）。
// 正数向右、正数向下。挂在轨道实例的 mShakeX 上（只挪这一条轨道，不动整株动画）。
constexpr const float FIRE_PEASHOOTER_FIRE_OFFSET_X = 10.0f;
constexpr const float FIRE_PEASHOOTER_FIRE_OFFSET_Y = 0.0f;
// 紫火豌豆的观感：把原版橙红色火豌豆贴图整体按 HSL 换色。
// 两个数值取自仓库根目录的「火豌豆射手的颜色.txt」：色相扇区 5.03（0..6 扇区 → 301.8°，紫）、饱和度 ×1.47。
// 走新增的缓存滤镜 FilterEffect::FILTER_EFFECT_FIREPEA_PURPLE，所以明暗层次（火球亮斑）都保留。
constexpr const float FIRE_PEA_PURPLE_HUE = 5.03f;
constexpr const float FIRE_PEA_PURPLE_SAT = 1.47f;

// ===== 火焰机枪射手（合成态 SEED_FIRE_GATLING_PEA：火豌豆射手 × 机枪射手）=====
// 合成方式与寒冰机枪射手完全同一条路径：把火豌豆射手卡种在已种下的机枪射手上升级，
// 消耗的仍是火豌豆射手卡（175 阳光），落地后变成这只隐藏植物，没有自己的种子卡。
//
// 普攻（4 连发）：与机枪射手同节奏（100 帧一轮、18/35/51/68 各一发），但 4 发全是紫火豌豆
//   （PROJECTILE_PURPLE_FIRE_PEA，与火豌豆射手同一发子弹：直击 65 + 群伤 + 命中易伤 4 秒）。
//   不做机枪射手那 3% 电能豌豆掷骰 —— 它是火属性，不是电能形态。
// 大招（散射）：与机枪射手同一套 mGatlingScatterCountdown / mGatlingScatterChance 机制，
//   散射期间每 2 帧发射 SCATTER_COUNT 颗 ±SCATTER_ANGLE 的扇形子弹，伤害沿用机枪射手散射的
//   mDamageOverride（200）。唯一区别是每颗子弹各自掷骰决定弹种：
//   FIRE_GATLING_SCATTER_PURPLE_PERCENT% 紫火豌豆，其余为普通火豌豆（PROJECTILE_FIREBALL）。
constexpr const int   FIRE_GATLING_SCATTER_PURPLE_PERCENT = 50;   // 大招里紫火豌豆的占比（%）

// ===== 投手类植物（卷心菜投手 / 玉米投手 / 西瓜投手 / 冰瓜投手）=====
// 投手"一次投掷"就是一轮 anim_shooting：原版对**本行每只僵尸各投一颗**，
// 所以一行堆满僵尸时一次能糊出去十几颗弹丸（配合西瓜/冰瓜的溅射会瞬间清场）。
// 这里给一轮投掷的弹丸总数封顶：最多前 MAX_PULT_PROJECTILES_PER_VOLLEY 只僵尸各吃一颗，
// 超出的僵尸本轮不再挨打（植物照常进入下一轮冷却）。
constexpr const int   MAX_PULT_PROJECTILES_PER_VOLLEY = 10;

constexpr const int HIGH_GROUND_HEIGHT = 30;

constexpr const int SEEDBANK_MAX = 10;
constexpr const int SEED_BANK_OFFSET_X = 0;
constexpr const int SEED_BANK_OFFSET_X_END = 10;
constexpr const int SEED_CHOOSER_OFFSET_Y = 516;
constexpr const int SEED_PACKET_WIDTH = 50;
constexpr const int SEED_PACKET_HEIGHT = 70;
constexpr const int IMITATER_DIALOG_WIDTH = 500;
constexpr const int IMITATER_DIALOG_HEIGHT = 600;

// ============================================================
// About levels
// ============================================================
constexpr const int ADVENTURE_AREAS = 6;
constexpr const int LEVELS_PER_AREA = 10;
constexpr const int NUM_LEVELS = ADVENTURE_AREAS * LEVELS_PER_AREA;
constexpr const int FINAL_LEVEL = NUM_LEVELS;
constexpr const int FLAG_RAISE_TIME = 100;
constexpr const int LAST_STAND_FLAGS = 5;
constexpr const int SNOWY_DAY_STAGES = 3;
constexpr const int SNOWY_DAY_WAVES_PER_STAGE = 50;
constexpr const int ZOMBIE_COUNTDOWN_FIRST_WAVE = 1800;
constexpr const int ZOMBIE_COUNTDOWN = 2500;
constexpr const int ZOMBIE_COUNTDOWN_RANGE = 600;
constexpr const int ZOMBIE_COUNTDOWN_BEFORE_FLAG = 4500;
constexpr const int ZOMBIE_COUNTDOWN_BEFORE_REPICK = 5499;
constexpr const int ZOMBIE_COUNTDOWN_MIN = 400;
constexpr const int FOG_BLOW_RETURN_TIME = 2000;
constexpr const int SUN_COUNTDOWN = 425;
constexpr const int SUN_COUNTDOWN_RANGE = 275;
constexpr const int SUN_COUNTDOWN_MAX = 950;
constexpr const int SURVIVAL_NORMAL_FLAGS = 5;
constexpr const int SURVIVAL_HARD_FLAGS = 10;

// ============================================================
// About the store screen layout
// ============================================================
constexpr const int STORESCREEN_ITEMOFFSET_1_X = 422;
constexpr const int STORESCREEN_ITEMOFFSET_1_Y = 206;
constexpr const int STORESCREEN_ITEMOFFSET_2_X = 372;
constexpr const int STORESCREEN_ITEMOFFSET_2_Y = 310;
constexpr const int STORESCREEN_ITEMSIZE = 74;
constexpr const int STORESCREEN_COINBANK_X = 650;
constexpr const int STORESCREEN_COINBANK_Y = 559;
constexpr const int STORESCREEN_PAGESTRING_X = 470;
constexpr const int STORESCREEN_PAGESTRING_Y = 500;
