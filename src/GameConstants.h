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
// 究极电能机枪射手 / 究极电能杨桃共用的"电能伤害节奏"：
//   两只究极植物的弹丸都是持续接触型伤害（电能豌豆无限穿透、电能星星钉住目标），
//   但不再"每游戏刻（10ms）结算一次"，而是每 ELECTRIC_DAMAGE_INTERVAL_TICKS 刻
//   （15 刻 → 0.15 秒）对接触中的目标结算一次 30 点伤害。
constexpr const int   ELECTRIC_DAMAGE_INTERVAL_TICKS = 15;    // 两次电能伤害之间的游戏刻数（100fps → 0.15 秒）
// 究极电能杨桃（旅行红卡 + 升级卡：由杨桃升级，300 阳光 / 30.01s 冷却）：
//   5 颗追踪的电能星星，命中后钉在该僵尸身上 2.5 秒，期间每 0.15 秒（与电能豌豆同一口径）
//   造成 30 点伤害；其余机制与杨桃完全一致。
constexpr const int   ELECTRIC_STAR_HIT_DAMAGE   = 30;    // 钉住期间每次电能伤害的数值
constexpr const int   ELECTRIC_STAR_LINGER_TICKS = 250;   // 钉住时长（游戏刻；100fps → 2.5 秒）
// 究极形态互换：把"另一种基础植物"种在究极形态上 = 原地变身，并返还这么多阳光。
//   杨桃(125)@究极电能机枪射手 → 究极电能杨桃；机枪射手(250)@究极电能杨桃 → 究极电能机枪射手。
constexpr const int   ELECTRIC_STARFRUIT_SWITCH_REFUND = 225;
// 究极电能杨桃专用贴图（reanim/Electric_Starfruit_*.png）接入开关。
// 与究极电能机枪射手同一套规则：三张图必须带透明通道且尺寸与同名原图一致，
// 任何一张不合格就整体回退到"杨桃贴图 + 电能蓝叠加"。
constexpr const bool  ELECTRIC_STARFRUIT_USE_CUSTOM_ART = true;
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
