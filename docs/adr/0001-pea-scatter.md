# 豌豆散射系统

Peashooter 类植物每次射击同时发射 11 颗豌豆呈 ±15° 扇形分布，替代原版单发直射。散射豌豆沿角度飞行，可跨行命中僵尸，经过 Torchwood 时同样被点燃。

## Considered Options

### 散射运动模型

- **新增 `MOTION_SCATTER`**：语义清晰，未来可扩展散射衰减/穿透等专属行为，但增加枚举值、污染多处 switch-case。
- **复用 `MOTION_STAR`**（已选）：零枚举改动，`MOTION_STAR` 已支持速度向量 + 动态行更新 + 上下边界消失。`PROJECTILE_PEA` 类型不会触发 Starfruit 专属碰撞特判（Digger 免伤、盾牌穿透），风险可控。成本：直射豌豆仍走 `MOTION_STRAIGHT`，碰撞检测需同时处理两种运动模式。

### 直射豌豆处理

- **统一切换到速度向量**：代码路径统一，但需改动 `PeaAboutToHitTorchwood()` 中的 `MOTION_STRAIGHT` 硬检查，且与 MOTION_STAR 共享速度向量逻辑后可能引入 Starfruit 的特判歧义。
- **保持 `MOTION_STRAIGHT`**（已选）：最小改动，直射行为完全不变（碰撞、Torchwood 点燃逻辑均不受影响）。

### Torchwood 点燃

- **移除行锁定 + 允许 MOTION_STAR**：散射豌豆飞行路径上经过 Torchwood 即点燃，即使不在 Torchwood 所在行。需同时修改 `UpdateTorchwood()` 的 `mRow` 检查和 `PeaAboutToHitTorchwood()` 的 `MOTION_STRAIGHT` 检查。

## Consequences

- 弹丸池容量从 1024 扩至 2048，`DataArray::DataArrayInitialize` 调用点需同步。
- `FindCollisionTarget()` 移除 `aZombie->mRow == mRow` 约束，碰撞检测遍历所有行而非仅当前行，性能影响需关注。
- Gatling Pea 峰值并发弹丸 ~44 颗/轮，多株叠加需确保池不溢出。
