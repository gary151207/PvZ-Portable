# PvZ-Portable 架构总览

> 本文档面向新加入的开发者，帮助快速理解项目的模块划分与依赖关系。

---

## 项目概况

PvZ-Portable 是对《植物大战僵尸》GOTY 版的跨平台社区复刻（SDL2 + OpenGL ES 2.0），代码按**四层分层架构**组织，每层只依赖下层。

---

## 分层架构总览

```
main.cpp
  └── LawnApp::Init() → Start() → Shutdown()
        ├── SexyAppBase  (窗口/输入/主循环)
        │     ├── GLInterface         (OpenGL ES 2.0)
        │     ├── WidgetManager       (Widget 树)
        │     ├── SoundManager        (SDL_Mixer)
        │     └── PakInterface        (读 main.pak)
        ├── Sexy.TodLib  (动画/粒子/特效/定义解析)
        │     ├── Reanimator / EffectSystem / TodParticle
        │     ├── Definition          (解析 properties/*.xml)
        │     └── TodStringFile       (本地化)
        └── Lawn         (游戏逻辑)
              ├── PlayerInfo / ProfileMgr / SaveGame  (存档)
              ├── Music               (音乐)
              ├── Board               (核心战斗)
              │     ├── Plant / Zombie / Projectile / Coin / LawnMower / GridItem
              │     ├── Challenge     (特殊关卡)
              │     ├── CutScene      (过场)
              │     └── SeedBank / CursorObject
              └── 各 Screen/Widget    (UI)
```

---

## 第零层：平台适配层

```
src/SexyAppFramework/platform/{3ds, switch, emscripten, default}/
src/SexyAppFramework/glad/
```

- 各平台窗口创建、输入事件、OpenGL 上下文管理
- 编译时通过 `#ifdef NINTENDO_SWITCH` 等宏切换具体实现
- `glad/` 为 OpenGL ES 2.0 加载器

---

## 第一层：SexyAppFramework — 游戏引擎框架

| 子系统 | 路径 | 关键类 | 职责 |
|--------|------|--------|------|
| **应用骨架** | `SexyAppFramework/SexyAppBase.*` | `SexyAppBase` → `SexyApp` | 主循环、初始化、关闭、窗口管理、资源路径、注册表模拟 |
| **渲染** | `SexyAppFramework/graphics/` | `Graphics`, `Image`, `MemoryImage`, `GLImage`, `GLInterface`, `ImageFont`, `Font`, `Color` | OpenGL 绘制、图片加载/合成、字体渲染、九宫格/平铺 |
| **UI 控件** | `SexyAppFramework/widget/` | `Widget`, `WidgetManager`, `WidgetContainer`, `ButtonWidget`, `Dialog`, `Checkbox`, `EditWidget`, `ListWidget`, `Slider`, `Scrollbar` | 事件驱动 UI 系统 |
| **音频** | `SexyAppFramework/sound/` | `SoundManager`, `MusicInterface`, `SDLSoundManager`, `SDLMusicInterface`, `SoundInstance` | SDL_Mixer 音效 + 音乐播放，支持 MO3 格式 |
| **资源打包** | `SexyAppFramework/paklib/` | `PakInterface` | 读取 `main.pak` 压缩资源包 |
| **图片解码** | `SexyAppFramework/imagelib/` | | PNG 等格式解码 |
| **工具** | `SexyAppFramework/misc/` | `Buffer`, `Rect`, `Ratio`, `MTRand`, `CritSec` | 缓冲区、矩形、随机数、互斥锁 |

---

## 第二层：Sexy.TodLib — 游戏通用库

| 模块 | 路径 | 关键类 | 职责 |
|------|------|--------|------|
| **定义解析** | `Sexy.TodLib/Definition.*` | | 解析 `properties/` 下的 XML 游戏数据定义 |
| **骨骼动画** | `Sexy.TodLib/Reanimator.*`, `ReanimAtlas.*` | `Reanimation`, `ReanimatorCache` | 植物/僵尸所有帧动画 |
| **粒子特效** | `Sexy.TodLib/TodParticle.*` | `TodParticleSystem`, `TodParticleEmitter` | 爆炸、烟雾、闪光等 |
| **特效管理** | `Sexy.TodLib/EffectSystem.*` | `EffectSystem` | 统一调度 Reanim/Particle/Trail/Attachment |
| **附着特效** | `Sexy.TodLib/Attachment.*` | | 跟随其他对象的特效 |
| **拖尾特效** | `Sexy.TodLib/Trail.*` | | 拖尾渲染 |
| **屏幕滤镜** | `Sexy.TodLib/FilterEffect.*` | | 全屏滤镜效果 |
| **音效 API** | `Sexy.TodLib/TodFoley.*` | `TodFoley` | 音效播放接口 |
| **本地化** | `Sexy.TodLib/TodStringFile.*` | | 多语言字符串表 |
| **通用工具** | `Sexy.TodLib/TodCommon.*` | `TodWeightedGridArray`, `TodSmoothArray` | 通用数据结构与数学工具 |
| **对象容器** | `Sexy.TodLib/DataArray.h` | `DataArray<T>` | 模板化固定容量数组（游戏对象容器） |

---

## 第三层：Lawn — 游戏逻辑层

### 3a. 应用入口与场景管理

| 模块 | 路径 | 关键类 | 职责 |
|------|------|--------|------|
| **应用** | `src/LawnApp.*` | `LawnApp` | 继承 `SexyApp`，拥有所有子系统，驱动场景切换 |
| **主菜单** | `Lawn/Widget/TitleScreen.*` | `TitleScreen` | 开始、继续、选项、退出 |
| **关卡选择** | `Lawn/Widget/GameSelector.*` | `GameSelector` | 冒险模式地图，小游戏/解谜/生存模式入口 |
| **选卡界面** | `Lawn/Widget/SeedChooserScreen.*` | `SeedChooserScreen` | 关卡前的植物卡牌选择 |
| **商店** | `Lawn/Widget/StoreScreen.*` | `StoreScreen` | 疯狂戴夫商店（植物升级、禅境花园物品、卡槽扩展） |
| **图鉴** | `Lawn/Widget/AlmanacDialog.*` | `AlmanacDialog` | 植物与僵尸图鉴 |
| **奖杯界面** | `Lawn/Widget/AwardScreen.*` | `AwardScreen` | 通关后奖励展示 |
| **成就** | `Lawn/Widget/AchievementsScreen.*` | `AchievementsScreen` | 成就面板 |
| **挑战菜单** | `Lawn/Widget/ChallengeScreen.*` | `ChallengeScreen` | 生存模式/小游戏/益智模式选择 |
| **致谢** | `Lawn/Widget/CreditScreen.*` | `CreditScreen` | 制作人员名单 |
| **设置** | `Lawn/Widget/NewOptionsDialog.*` | `NewOptionsDialog` | 音效、音乐、全屏等设置 |
| **对话框** | `Lawn/Widget/LawnDialog.*`, `UserDialog.*`, `NewUserDialog.*`, `CheatDialog.*`, `ContinueDialog.*` | | 各类弹窗 |

### 3b. 核心战场 Board

`Board` 是最复杂的单文件（~290KB），是游戏主循环的载体。

```
Board
├── DataArray<Plant>          mPlants          ← 场上所有植物
├── DataArray<Zombie>         mZombies         ← 场上所有僵尸
├── DataArray<Projectile>     mProjectiles     ← 所有飞行子弹
├── DataArray<Coin>           mCoins           ← 阳光/金币/掉落物
├── DataArray<LawnMower>      mLawnMowers      ← 割草机
├── DataArray<GridItem>       mGridItems       ← 墓碑/弹坑/梯子/花盆/钉耙
├── CursorObject              mCursorObject    ← 光标（植物种植/铲子/工具）
├── SeedBank                  mSeedBank        ← 种子栏
├── Challenge                 mChallenge       ← 特殊关卡逻辑
├── CutScene                  mCutScene        ← 过场动画
├── MessageWidget             mAdvice          ← 教学提示
```

**核心游戏循环**：

1. `Board::Update()` → 更新所有对象（僵尸 AI、植物攻击、子弹飞行、掉落物收集）
2. `Board::Draw()` → 按 Z-order 排序所有 `RenderItem` 后统一绘制

**关键状态**：`mGridSquareType[9][6]`（格子类型）、`mZombiesInWave[100][50]`（每波僵尸阵容）、`mSunMoney`、`mCurrentWave`、`mLevel`、`mBackground`、`mTutorialState`

### 3c. 游戏实体（GameObject 体系）

基类 `GameObject`（`Lawn/GameObject.*`）持有 x/y/row/renderOrder 等通用属性。

| 实体 | 路径 | 说明 |
|------|------|------|
| `Plant` | `Lawn/Plant.*` (~186KB) | 植物状态机：就绪→攻击→特殊技能。按 `SeedType` 分支，非继承多态 |
| `Zombie` | `Lawn/Zombie.*` (~344KB) | 僵尸状态机：行走→攻击→死亡。护盾系统（路障/铁桶/铁门/梯子） |
| `Projectile` | `Lawn/Projectile.*` | 14 种弹道：豌豆/冰豌豆/卷心菜/西瓜/冰瓜/火焰/星星/刺/篮球/玉米粒/玉米炮/黄油/僵尸豌豆 |
| `Coin` | `Lawn/Coin.*` | 阳光/银币/金币/钻石/奖杯/铲子/种子包/花盆/钥匙等掉落物 |
| `LawnMower` | `Lawn/LawnMower.*` | 割草机：触发后沿行碾压僵尸 |
| `GridItem` | `Lawn/GridItem.*` | 墓碑/弹坑/梯子/恐怖罐子/钉耙 |

> **设计要点**：植物和僵尸并非每个品种一个子类，而是在 `Update()` 中按枚举值用 switch-case 分支实现不同行为（与原始 PvZ 设计一致）。

### 3d. 玩家数据系统

| 模块 | 路径 | 职责 |
|------|------|------|
| `PlayerInfo` | `Lawn/System/PlayerInfo.*` | 单个用户存档：冒险进度、金币、解锁植物、禅境花园状态、成就 |
| `ProfileMgr` | `Lawn/System/ProfileMgr.*` | 多用户管理，增删改用户 |
| `SaveGame` | `Lawn/System/SaveGame.*` | 存档读写（`user*.dat` 兼容原版 GOTY），`.v4` 中关存档 + YAML 导出 |
| `DataSync` | `Lawn/System/DataSync.*` | 运行时状态序列化，供存读档用 |
| `Music` | `Lawn/System/Music.*` | 背景音乐管理与播放 |
| `PoolEffect` | `Lawn/System/PoolEffect.*` | 泳池水面特效 |

### 3e. 禅境花园

| 模块 | 路径 | 职责 |
|------|------|------|
| `ZenGarden` | `Lawn/ZenGarden.*` | 独立于 Board 运行，管理盆栽植物的浇水/施肥/杀虫/听音乐 |

---

## 横切面定义文件

| 文件 | 行数 | 内容 |
|------|------|------|
| `src/ConstEnums.h` | ~1400 | 所有枚举常量：`SeedType`(49 种植物)、`ZombieType`(36 种僵尸)、`ProjectileType`、`GameMode`(60+ 种模式)、`BackgroundType`、`TutorialState`… — **这是项目的领域词汇表** |
| `src/GameConstants.h` | ~100 | 数值常量：`BOARD_WIDTH=800`、`MAX_GRID_SIZE_X=9`、`ZOMBIE_COUNTDOWN=2500`、`SUN_COUNTDOWN=425`… |
| `src/Resources.h/.cpp` | ~1900/~150KB | 资源 ID 映射表——将 `main.pak` 中数千个文件映射到整数 ID（图片、字体、声音、Reanim） |
| `src/Lawn/LawnCommon.*` | | 通用游戏工具函数、UI 辅助控件 |

---

## 游戏模式 (GameMode)

`GameMode` 枚举决定一切——同一套 Board/Plant/Zombie 代码，通过以下枚举值分支驱动 60+ 种模式：

| 分类 | 模式示例 |
|------|---------|
| 冒险模式 | `GAMEMODE_ADVENTURE`（5 大关 × 10 小关） |
| 生存模式 | `GAMEMODE_SURVIVAL_NORMAL/HARD/ENDLESS_STAGE_1~5` |
| 小游戏 | 植物大战(War and Peas)、坚果保龄球、老虎机、隐形食脑者、看星星… |
| 益智模式 | 花瓶终结者(Scary Potter)、我是僵尸(I, Zombie) |
| 特殊 | 禅境花园、智慧树 |

通过 `LawnApp::IsAdventureMode()`, `IsSurvivalMode()`, `IsIZombieLevel()` 等方法判断当前模式。

---

## 数据资产（需用户自行提供）

项目不包含受版权保护的资产。用户需从正版 PvZ GOTY 获取：

| 资产 | 说明 |
|------|------|
| `main.pak` | 压缩资源包（图片、音效、音乐、字体、Reanim 动画） |
| `properties/` | XML 游戏数据（植物属性、僵尸属性、关卡定义等） |

运行时数据路径：
- 只读资源：可执行文件所在目录
- 可写数据（存档/缓存/设置）：OS 推荐的应用数据目录（如 Windows `%APPDATA%\io.github.wszqkzqk\PvZPortable\`）
