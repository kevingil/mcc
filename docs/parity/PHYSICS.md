# Physics

Java Edition runs a fixed tick at 20 Hz. OpenCraft currently integrates with `GetFrameTime()` in `UpdatePlayerPhysics`. The same machine at 30 fps and 240 fps does not match, and the constants do not match the tick model below.

Sources for the numbers: the Java Edition pages [Player](https://minecraft.wiki/w/Player), [Transportation](https://minecraft.wiki/w/Transportation), [Hitbox](https://minecraft.wiki/w/Hitbox), [Attribute](https://minecraft.wiki/w/Attribute), and [Fluid](https://minecraft.wiki/w/Fluid), as published for the current game. Where a 26.2 attribute has no stable published base in those pages, the slice measures it instead of inventing one.

## Tick

One game tick is 1/20 second. The client may render faster. It accumulates frame time and runs zero or more ticks, then draws once. Clamp the backlog so a stall cannot simulate hundreds of ticks. A headless test steps ticks with no window and no dependency on frame time.

Day length is 24,000 ticks. Redstone and fluids have their own schedules on top of this clock. They do not get a second clock.

## Player body

Position is the center of the feet.

| Pose | Width | Height | Eye height |
| --- | ---: | ---: | ---: |
| Standing | 0.6 | 1.8 | 1.62 |
| Sneaking | 0.6 | 1.5 | 1.27 |
| Swimming, gliding, crawling | 0.6 | 0.6 | 0.4 |
| Sleeping | 0.2 | 0.2 | |

`PLAYER_WIDTH` and `PLAYER_HEIGHT` already match the standing box. The camera offset `PLAYER_HEIGHT * 0.9` is 1.62. Sneak, swim, and sleep boxes are not implemented. Yaw and pitch do not rotate the box.

Default field of view is 70 degrees. The client already uses 70.

## Attributes the movement slice has to hit

These are the player bases the acceptance tests lock. Other attributes are listed in `registry/catalogs.md` and stay at "do not invent a number" until a slice owns them.

| Attribute | Player base | What the test sees |
| --- | ---: | --- |
| `movement_speed` | 0.1 | Walk 4.317 blocks/s on flat stone |
| sprint modifier | +30% acceleration | Sprint 5.612 blocks/s |
| `sneaking_speed` | 0.3 | Sneak about 1.295 blocks/s |
| `gravity` | 0.08 blocks/tick^2 | See the jump and fall formulas |
| `jump_strength` | 0.42 blocks/tick | Apex 1.2522 blocks |
| `step_height` | 0.6 | Walk up a 0.5 slab, do not walk up a full block |
| `safe_fall_distance` | 3.0 | No fall damage at 3 blocks, damage starts after that |
| `block_interaction_range` | 4.5 | Survival reach. Creative adds 0.5, so 5.0 |
| `entity_interaction_range` | 3.0 | Arm's reach for entities, once entities exist |
| `scale` | 1.0 | Hitbox multiplier |
| `max_health` | 20 | Ten hearts, once health exists |

`src/player.c` today uses walk 5.0, sprint 8.0, jump impulse 8.0 per second, gravity 20 per second squared, reach 5.0, and a terminal fall of 50. Those constants go away in the movement slice. Reach 5.0 is creative range, not survival.

Horizontal model, from the Player page. On a normal block the per-tick friction factor is 0.546. Walking acceleration is 0.098 blocks/tick^2. Sprinting uses 0.098 * 1.3. Each tick the acceleration is added, the player is moved, then horizontal velocity is multiplied by 0.546. Terminal speed is `a / (1 - 0.546)`.

```
0.098 / (1 - 0.546) = 0.2159 blocks/tick = 4.317 blocks/s
0.1274 / (1 - 0.546) = 0.2806 blocks/tick = 5.612 blocks/s
```

Ice and slime change the block friction. Soul sand and honey change it the other way. The first slice only has to match stone and air. Later block properties carry the friction value.

Airborne horizontal velocity is multiplied by 0.91 instead of 0.546.

Vertical model, from the Transportation page:

```
v(1) = 0.42 on the jump tick, or 0 on a fall
v(t) = 0.98 * (v(t - 1) - 0.08)
```

`v` is blocks per tick and is also the distance moved that tick. Summing while `v > 0` after a jump lands on 1.2522. Terminal fall approaches 3.92 blocks/tick (78.4 blocks/s). Velocities under 0.003 are zeroed.

The movement slice's test steps this recurrence directly and checks walk distance, sprint distance, sneak distance, and jump apex within a small epsilon. It does not read the frame clock.

Sprint is blocked once hunger exists and the food level is 6 or lower. Until the hunger slice, sprint is always allowed. Jumping spends hunger later. The movement slice does not add a hunger bar.

## Collision

Move Y, then X, then Z, which the client already does. A solid block occupies its collision shape. Cubes use the full 1x1x1 box. Water is not solid. The eight-corner test in `CheckCollision` misses voxels the box overlaps in the middle of a face. The movement slice uses the real AABB against the block shape.

Step-up: if a horizontal move is blocked and the rise is at most `step_height` (0.6), move up and forward. Sneaking does not step off a ledge.

## Fluids

Java Edition fluids:

| Fluid | Where | Horizontal period | Notes |
| --- | --- | --- | --- |
| Water | Overworld, End | 1 block / 5 ticks | Level falls off over 7 steps |
| Lava | Nether (`fast_lava`) | 1 block / 10 ticks | Wider spread |
| Lava | Overworld, End | 1 block / 30 ticks | Shorter spread |

Water already spreads seven steps in `src/water.c`, including a new source where two sources meet on solid ground. It is not on the 5-tick schedule, and it has no lava. In the Nether, water evaporates (`water_evaporates` on that dimension type).

Swimming and the water current are part of the fluid slice after the tick exists, not part of the first movement slice. Today's water sets vertical velocity to 4.5 while space is held and damps horizontal velocity by 0.55 per frame. That is a placeholder.

## Combat and damage, later

Fall damage uses `safe_fall_distance` and `fall_damage_multiplier`. Melee uses `attack_damage` and `attack_speed` (player base 4 attacks per second). Those land with the health and zombie slices, using the same attribute record as movement. Do not add a second damage path on the client.
