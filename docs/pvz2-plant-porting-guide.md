# PvZ2 植物原生移植指南：孢子菇已验证案例

**状态：** 已实现并通过实机验收  
**验收日期：** 2026-09-26  
**参考实现：** `SEED_SPORESHROOM` / `PROJECTILE_SPORESHROOM`

本文记录将国服《植物大战僵尸2》4.2.4 的 1 级孢子菇移植为 PvZ-Portable
原生植物的完整方法。目标是为后续 PvZ2 植物移植提供一条可重复、可检查、
不破坏旧存档的工程路径，而不是要求其它植物照搬孢子菇的数值或战斗逻辑。
带持续状态、原生弹丸与多轮实机视觉调校的案例见
[一阶毒液豌豆射手移植复盘](poison-peashooter-port.md)。

## 1. 已验证的孢子菇口径

| 项目 | 实现 |
| --- | --- |
| 选卡入口 | 仅 `GAMEMODE_CHALLENGE_CRICKET_2` 第二选卡页 |
| 数值 | 150 阳光、500 tick 冷却、290 tick 发射间隔、300 生命 |
| 攻击 | 同行单目标抛射，每发 40 伤害，复用 `FOLEY_PUFF` |
| 繁殖 | 孢子对活着僵尸造成致死一击后，尝试在僵尸原格生成孢子菇 |
| 成长 | 新生植物完整播放 `anim_grow`，成长期间不攻击 |
| 日夜属性 | 属于蘑菇，但不属于白天休眠植物 |
| 素材 | 植物 75 张 PNG、弹丸 7 张 PNG、2 个原生 `.reanim` |
| 未实现 | 能量豆逻辑、升级等级、随机生成其它蘑菇 |

## 2. 移植原则

1. **不为植物新建子类。** 遵循项目的枚举分支架构，在 `Plant` / `Projectile` / `Board`
   现有状态机中接入。
2. **所有可序列化枚举只能追加。** 新值放在对应 `NUM_*` 哨兵之前，不能插入、
   重排或复用旧整数值。
3. **优先复用现有 TLV 字段。** 孢子菇仅使用已有的弹丸类型、`mSourcePlantID`、
   植物状态和 Reanimation 字段，没有提升 `.v4` 存档版本。
4. **运行时不依赖提取目录。** PAM JSON 和裁切图可以位于仓库外，但转换后的
   `.reanim` 和 PNG 必须作为完整运行时输入。
5. **功能入口与实体定义分离。** 植物可以被图鉴展示，但选卡权限必须由
   `LawnApp::HasSeedType` 和选卡页逻辑单独限定。
6. **素材来源需由提供者授权。** 提取目录不应被当作仓库或运行时依赖；
   引入其它素材前应再次确认项目的资源分发约定。

## 3. PAM 素材转换流程

### 3.1 先盘点源素材

孢子菇案例的源目录为 `/Users/fiture/Privates/pvz-2-crack/work/sporeshroom`，关键输入是：

- 植物 PAM JSON：`SPORESHROOM.json`，329 帧，30 FPS。
- 弹丸 PAM JSON：`SPORESHROOM_PROJECTILE.json`，88 帧，30 FPS。
- `sprites/images` 下的裁切 PNG。

新植物不应立即把全部素材搬入项目。先检查：

- 主时间轴的 label、帧数和 FPS。
- `image` 与 `sprite` 资源数量。
- 嵌套 sprite 是否有多帧内部时间轴。
- 是否存在装扮、高等级、能量豆、水面或其它变体层。
- 是否包含加色混合、RGB 变色、裁切矩形或 sprite seek 等 reanim 无法直接表达的特性。

### 3.2 使用确定性转换器

参考脚本是 `scripts/gen-spore-shroom-reanim.py`，它负责：

1. 验证 PAM JSON SHA-256、PAM 版本、FPS、帧数结构和 label 顺序。
2. 逐帧模拟 PAM display list 的 `append` / `change` / `remove`。
3. 递归展开嵌套 sprite，组合父子仿射矩阵和 alpha。
4. 为每个叶子路径生成稳定的 reanim 轨道名。
5. 把 PAM label 映射为项目使用的 `anim_*` marker 轨道。
6. 用稳定文件名复制所有裁切 PNG。
7. `--check` 重新计算输出并逐字节对比，确保已提交资源可重现。

生成与检查：

```bash
python3 scripts/gen-spore-shroom-reanim.py --source /path/to/sporeshroom
python3 scripts/gen-spore-shroom-reanim.py --source /path/to/sporeshroom --check
```

为下一株植物编写转换器时，应把以下内容参数化或锁定：

- 输入 JSON 相对路径与源哈希。
- 输出 reanim 名、PNG 前缀与预期图片数。
- 原点、整体缩放、label 映射。
- 需要排除的装扮/高等级 sprite 名称。

### 3.3 仿射矩阵的正确口径

这是本次最重要的踩坑记录。PAM 的 2、3、6 元变换先统一为：

```text
[ a c tx ]
[ b d ty ]
[ 0 0  1 ]
```

嵌套时使用 `parent × child × imageTransform`。写入 reanim 时：

- `sx = hypot(a, b)`，`sx` 与 `kx` 由第一列得到。
- `sy = hypot(c, d)`，`sy` 与 `ky` 由第二列得到。
- **`x/y` 必须使用组合矩阵的 `tx/ty`（变换后左上角），不能换成图片中心。**

`Reanimation::DrawTrack` 会先插入图片中心 pivot，引擎最终仍把 reanim `x/y` 解释为
变换后左上角。如果错用变换后中心，每张裁切件都会多偏移自身的半宽/半高：
单张图看似正常，合成后却会出现喷口肿大、帽盖错层、高光悬空等问题。

### 3.4 装扮不等于基础本体

孢子菇 PAM 中的 `_custom`、`custom_01`、`custom_02` 实际是纸袋和蓝色獠牙装扮。
如果无区别展开所有 sprite，移植后会变成装扮外观，而不是 1 级基础外观。

处理原则：

- 先按 sprite 名和它引用的裁切图做人工盘点。
- 在转换器中以显式名单排除，不要依赖脆弱的显示层序号。
- 输出检查要断言基础 reanim 不再引用装扮图片。
- 可以继续复制全部基础图集以保持输出清单稳定，但不应让装扮进入任何可播放轨道。

## 4. 动画资源接入

1. 在 `ReanimationType` 末尾追加植物和弹丸类型。
2. 在 `gLawnReanimationArray` 注册 `.reanim`。PvZ2 裁切件多且动态加载，
   孢子菇使用 `REANIM_NO_ATLAS`。
3. 在 `Plant::PreloadPlantResources` 预加载植物额外需要的弹丸/特效定义。
4. 待机、攻击、成长、飞行和命中动画应用 marker 轨道分段，不要在 C++ 中写死绝对帧区间。

孢子菇的 label 映射：

| PAM label | reanim marker |
| --- | --- |
| `idle` | `anim_idle` |
| `idle2` | `anim_idle2` |
| `attack` | `anim_shooting` |
| `plantfood` | `anim_plantfood`（保留素材，暂无逻辑入口） |
| `water` | `anim_water` |
| `grow` | `anim_grow` |
| `animation` | `anim_fly` |
| `hit` / `hit2` | `anim_hit` / `anim_hit2` |

## 5. 植物与弹丸接线

### 5.1 枚举和数值表

在各自哨兵之前追加：

- `SEED_SPORESHROOM`
- `PROJECTILE_SPORESHROOM`
- `REANIM_SPORESHROOM`
- `REANIM_SPORESHROOM_PROJECTILE`
- `STATE_SPORESHROOM_GROWING`

在 `gPlantDefs` 中追加完整定义，在 `gProjectileDefinition` 中追加伤害。如果植物使用
引擎的默认 300 生命，不要再建一个只为重复该默认值的字段。

### 5.2 攻击动画与动作点

PvZ2 的动作点必须映射到 PvZ-Portable 的 100 Hz 逻辑 tick。孢子菇 `use_action`
在攻击段第 29 帧，原动画为 30 FPS：

```text
29 / 30 × 100 ≈ 97 tick
```

因此攻击开始时设置 `mShootingCounter = 97`，计数到动作点才发射，动画结束后
让现有 `UpdateShooting` 收招逻辑恢复待机。后续植物应从 PAM command/label 提取动作点，
不要仅凭视觉猜测。

### 5.3 专用抛射弹丸

孢子菇需要抛物线，但不应成为 `IsPultPlant`：该分类在本项目中会走“遍历同行僵尸、
每只发一颗”的投手齐射逻辑。正确做法是：

- 保持 `SUBCLASS_SHOOTER` 和默认同行单目标索敌。
- 仅在 `Plant::Fire` 的弹道配置中把专用弹丸设为 `MOTION_LOBBED`。
- 弹丸自身携带专用 `ProjectileType`，命中时不需要回查发射者是否仍存活。

### 5.4 “该枚弹丸直接击杀”的判定

在 `Projectile::DoImpact` 中：

1. 伤害前记录目标非空且 `!IsDeadOrDying()`。
2. 调用正常 `TakeDamage`。
3. 伤害后仅在目标进入 `IsDeadOrDying()` 时请求繁殖。

这个前后状态门控保证：

- 已死目标不会重复繁殖。
- 只打破头盔/护盾但本体未死时不繁殖。
- 其它伤害源完成击杀时不繁殖。
- 发射者在弹丸命中前死亡不影响结果。

### 5.5 Board 级生成接口

把坐标换算、合法性判定和实体创建收敛到 `Board::TrySpawnSporeShroom`：

1. 用僵尸矩形中心和 `mRow` 计算原格。
2. 对该格严格调用 `CanPlantAt`。
3. 成功时用 `NewPlant` 免费创建。
4. 立即进入 `STATE_SPORESHROOM_GROWING` 并播放 `anim_grow`。

不搜索邻近格，不自动补花盆/睡莲，不在弹丸代码里手工复制种植规则。

## 6. 模式、选卡和图鉴

一株新植物是否能被选中，至少由三层逻辑决定：

1. `LawnApp::HasSeedType`：模式是否拥有该植物。
2. `SeedChooserScreen::SeedShownOnChooserPage`：当前页是否显示。
3. `GetSeedPositionInChooser`：在对应页内的独立网格位置。

孢子菇的隔离策略：

- 斗蛐蛐 2：在现有旅行植物之后追加到第二页。
- 其它模式：`HasSeedType` 显式返回 `false`。
- 不加入 `TravelPlantDef`，不加入冰面沙盒植物表。
- 图鉴扩展页可始终展示，但不用图鉴可见性反推选卡权限。

卡面使用 `SeedPacket.cpp` 中孢子菇专用缩放/偏移；战场体型由转换器的整体缩放
与原点控制，弹丸影子在 `Projectile::DrawShadow` 单独调整。不要为一株植物修改
所有植物的通用布局。

## 7. 本地化与资料页

至少提供：

- 名称：`[SPORE_SHROOM]`
- 选卡提示：`[SPORE_SHROOM_TOOLTIP]`
- 图鉴说明：`[SPORE_SHROOM_DESCRIPTION]`

项目当前的 `pvzp-strings.xml` 和 `pvzp-strings.zh-CN.xml` 都遵循现有中文资源约定；
后续如果恢复独立英文资源，再同步英文文案，不要只改其中一份。

## 8. 验证流程

### 8.1 自动检查

```bash
# 源素材、生成文件、标签、图片引用和输出一致性
python3 scripts/gen-spore-shroom-reanim.py --check

# Windows / PowerShell：枚举、数值、动画、模式隔离、繁殖门控、图鉴和文案
pwsh -File scripts/check-spore-shroom.ps1

# 完整构建
cmake --build build
```

非 PowerShell 环境下，应执行与 `.ps1` 相同的断言，不应因为解释器缺失而跳过检查。

### 8.2 离线动画预览

```bash
python3 tools/reanim-preview.py res/main/reanim/SporeShroom.reanim idle.png 0 3
python3 tools/reanim-preview.py res/main/reanim/SporeShroom.reanim attack.png 127 3
python3 tools/reanim-preview.py res/main/reanim/SporeShroom.reanim grow.png 315 3
python3 tools/reanim-preview.py res/main/reanim/SporeShroomProjectile.reanim fly.png 0 3
python3 tools/reanim-preview.py res/main/reanim/SporeShroomProjectile.reanim hit.png 55 3
python3 tools/reanim-preview.py res/main/reanim/SporeShroomProjectile.reanim hit2.png 75 3
```

预览时必须检查：

- 本体轮廓是否与 PvZ2 基础外观一致。
- 是否出现纸袋、帽子、皮肤等装扮层。
- 高光、面部、帽盖、喷口是否错层或悬空。
- 弹丸本体、尾迹和命中特效是否共用同一个视觉中心。
- 卡面缓存画布是否裁边。

### 8.3 实机验收矩阵

- [ ] 指定模式的指定页面能看到、选择和种植。
- [ ] 费用、冷却、生命、索敌、发射节奏和伤害正确。
- [ ] 发射点与动画动作点对齐，收招后恢复待机。
- [ ] 空格、占用格、障碍格、场外格和支撑物格子的生成结果正确。
- [ ] 非致命头盔/护盾伤害、已死目标、其它来源击杀不会误触发。
- [ ] 发射者提前死亡后，已发出弹丸仍能正常结算。
- [ ] 成长期间不攻击，成长完成后可继续繁殖。
- [ ] 所有非目标模式不能选到植物。
- [ ] 卡面、图鉴、战场、弹丸、影子与命中特效大小/偏移正确。
- [ ] 弹丸飞行中、特殊植物状态中、状态完成后的 `.v4` 存取档正常。
- [ ] 旧 `.v4` 存档仍可读取。

孢子菇案例已由用户于 2026-09-26 完成实机验收。

## 9. 下一株 PvZ2 植物的实施清单

### 素材

- [ ] 记录游戏版本、植物等级、PAM/PNG 来源和授权。
- [ ] 列出动画 label、帧数、FPS、图片数与嵌套 sprite。
- [ ] 区分基础本体、装扮、升级、能量豆、水面和特效层。
- [ ] 生成器固定源哈希、预期标签和输出清单，提供 `--check`。
- [ ] 用离线预览验证待机、攻击、特殊状态、弹丸和命中特效。

### 原生逻辑

- [ ] 在哨兵前追加所有枚举，确认旧值未改变。
- [ ] 在定义表中填写数值，不建植物/僵尸子类。
- [ ] 按 PAM 动作点接入攻击或特殊技能。
- [ ] 新弹丸明确伤害、轨迹、碰撞、护盾语义、视觉和消亡条件。
- [ ] 跨实体行为收敛到 Board 级接口，复用 `CanPlantAt` / `NewPlant` 等现有规则。
- [ ] 优先复用现有 TLV 字段；如必须新增字段，单独设计存档迁移。

### 产品入口

- [ ] 明确可用模式、选卡页、页内位置和非目标模式的显式禁用规则。
- [ ] 不通过不相关数据表“伪装”新植物身份。
- [ ] 加入名称、选卡提示、图鉴说明、卡面布局和图鉴卡位。
- [ ] 新增静态契约脚本，覆盖数值、资源注册、模式隔离和存档约束。
- [ ] 完成构建、离线视觉检查和实机验收，记录验收日期与未覆盖项。

## 10. 孢子菇参考文件

| 职责 | 文件 |
| --- | --- |
| PAM 转换与可重现检查 | `scripts/gen-spore-shroom-reanim.py` |
| 静态契约 | `scripts/check-spore-shroom.ps1` |
| 枚举 | `src/ConstEnums.h`、`src/Lawn/Plant.h` |
| 植物数值、攻击、分类与成长 | `src/Lawn/Plant.cpp` |
| 弹丸伤害、弹道、命中特效与致死门控 | `src/Lawn/Projectile.cpp` |
| 原格生成接口 | `src/Lawn/Board.h`、`src/Lawn/Board.cpp` |
| 模式拥有权 | `src/LawnApp.cpp` |
| 第二选卡页 | `src/Lawn/Widget/SeedChooserScreen.cpp` |
| 图鉴 | `src/Lawn/Widget/AlmanacDialog.h`、`src/Lawn/Widget/AlmanacDialog.cpp` |
| 卡面缩放/偏移 | `src/Lawn/SeedPacket.cpp` |
| 动画注册 | `src/Sexy.TodLib/Reanimator.cpp` |
| 文案 | `res/properties/pvzp-strings.xml`、`res/properties/pvzp-strings.zh-CN.xml` |
