# 月夜屋顶第六大关 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add adventure levels 51-60 as a night rooftop chapter using the existing Dr. Zomboss map, with level 60 as the new final boss.

**Architecture:** Extend the existing adventure constants and wave table, route levels 51-60 through `BACKGROUND_6_BOSS`, and replace hard-coded level-50 terminal checks with `FINAL_LEVEL` where they represent campaign completion. Keep zombie availability compatible by clamping post-50 table lookups to the existing level-50 data.

**Tech Stack:** C++20, CMake/Ninja, existing PvZ-Portable SDL2/OpenGL codebase.

**Spec:** `docs/superpowers/specs/2026-08-23-moonlit-roof-design.md`

## Global Constraints

- 第六大关使用现有 `BACKGROUND_6_BOSS` 资源，不新增资源文件。
- 6-5 只接入基础关卡流程，不加入特殊波次或独有玩法。
- 使用 TLV 存档格式，不修改既有存档字段布局。
- 不重排 `ConstEnums.h` 中已有枚举值。

---

### Task 1: Extend adventure level boundaries

**Files:**
- Modify: `src/GameConstants.h:61-64`
- Modify: `src/Lawn/Challenge.cpp:55-61`
- Modify: `src/Lawn/Board.cpp:641-657,2620-2635`

- [ ] Change `ADVENTURE_AREAS` from 5 to 6 and append ten wave counts for levels 51-60, matching the existing late-game 20/30-wave cadence.
- [ ] Change adventure wave lookup clamp from index 49 to `NUM_LEVELS - 1`.
- [ ] Keep zombie availability lookups safe for levels 51-60 by clamping their table index to the existing level-50 entry.
- [ ] Run a source-level assertion script that counts the wave initializer values and checks the constants.

### Task 2: Route chapter six to the Zomboss rooftop map

**Files:**
- Modify: `src/Lawn/Board.cpp:970-1000`
- Modify: `src/LawnApp.cpp:2395-2402`

- [ ] Make adventure levels 51-60 select `BACKGROUND_6_BOSS`, while preserving 41-49 as the ordinary roof and 50 as the legacy boss map.
- [ ] Extend the global adventure night predicate to include levels 51-60.
- [ ] Verify that existing `StageHasRoof()` and `StageIsNight()` return true for all chapter-six levels without changing enum values.

### Task 3: Make level 60 the campaign final boss

**Files:**
- Modify: `src/LawnApp.cpp:1415-1425,1527-1550`
- Modify: `src/Lawn/Board.cpp:805-816,2095-2110`
- Modify: `src/Lawn/Coin.cpp:1200-1208`

- [ ] Replace campaign-final checks that currently use level 50 with `FINAL_LEVEL`.
- [ ] Keep level 50's existing Boss-map presentation but stop treating it as the campaign completion point.
- [ ] Add level-60 final-wave Boss spawning and final Boss coin reward behavior through the existing final-wave path.
- [ ] Include level 59 in the regular chapter-award boundary list.
- [ ] Search all remaining adventure-specific level-50 checks and classify each as campaign-final or legacy level-50 behavior before changing it.

### Task 4: Update first-time level intro and rooftop setup bounds

**Files:**
- Modify: `src/Lawn/CutScene.cpp:370-380,1060-1085`
- Modify: `src/Lawn/Board.cpp:7650-7660`

- [ ] Extend first-time adventure preload and chapter-end UI checks to recognize level 59/60.
- [ ] Ensure chapter-six rooftop levels receive the default three-column flower-pot setup, while preserving the existing special setup for levels 41-50.
- [ ] Keep 6-5 free of special gameplay logic.

### Task 5: Verification

**Files:**
- Test: source-level checks issued from PowerShell; no automated test suite exists in the repository.

- [ ] Run the source-level checks for constants, wave count, background routing, night routing, and final-level checks.
- [ ] Configure/build with `cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release` when dependencies are available.
- [ ] Inspect `git diff --check` and review all hard-coded 50/49 references touched by the feature.

### Task 6: Add the 6-5 Lone Wolf minigame

**Files:**
- Modify: `src/LawnApp.h`, `src/LawnApp.cpp`
- Modify: `src/Lawn/Board.h`, `src/Lawn/Board.cpp`
- Modify: `src/Lawn/Plant.h`, `src/Lawn/Plant.cpp`
- Test: `scripts/check-lone-wolf.ps1`

- [ ] Add `IsLoneWolfLevel()` for adventure level 55 only.
- [ ] Route level 55 to `BACKGROUND_2_NIGHT`, return no seed selection and zero seed packets, and create exactly one `SEED_GATLING` at grid column 4, row 2.
- [ ] Handle W/A/S/D in `Board::KeyDown` as one-cell moves, clamped to columns 0-8 and rows 0-4; update `mX`, `mY`, `mPlantCol`, `mRow`, and `mRenderOrder`.
- [ ] Add source checks for level identification, black-night routing, no-card setup, single Gatling initialization, and four directional moves.
