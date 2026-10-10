# World

Numbers below are the vanilla dimension types from the 26.4 Snapshot 3 data pack (the same misode/mcmeta `data` branch as the registries). 26.3 uses the same heights and coordinate scales.

## Dimensions

Three dimensions. `overworld_caves` is a dimension type used by terrain generation, not a place the player travels to.

| | Overworld | Nether | End |
| --- | --- | --- | --- |
| id | `overworld` | `the_nether` | `the_end` |
| `min_y` | -64 | 0 | 0 |
| `height` | 384 | 256 | 256 |
| Top block y | 319 | 255 | 255 |
| `logical_height` | 384 | 128 | 256 |
| `coordinate_scale` | 1 | 8 | 1 |
| Skylight | yes | no | yes |
| Ceiling | no | yes | no |
| Clock | runs | fixed | fixed |
| `ambient_light` | 0.0 | 0.1 | 0.25 |
| Beds | sleep at night, set spawn | explode, no spawn | explode, no spawn |
| Respawn anchor | no | yes | no |
| Water | stays | evaporates | stays |
| Lava | slow | fast | slow |
| Sky | `#78a7ff`, fog `#c0d8ff`, clouds at 192.33 | no sky, fog starts 10 ends 96 | sky `#000000`, fog `#181318` |

`logical_height` caps where a nether portal or chorus fruit may place the player. A portal already built above that still links. Nether portal x and z are overworld coordinates divided by 8.

OpenCraft has one overworld. Stored height is `WORLD_HEIGHT` 128, y from 0. `CHUNK_HEIGHT` 256 is not used for the array. The height slice changes storage to sections of 16 and migrates old region files. Old saves keep their blocks in the y range they had. They do not get stretched.

## Chunks

A chunk is 16 by 16 in x and z. Vertically it is a stack of 16-block sections. The overworld has 24 sections, from y=-64.

Generation fills a chunk the first time it is requested. Edited chunks stay in `region/r.<x>.<z>.mca`. That rule stays. The region codec learns sections in the height slice. Until then, do not write a second save format.

Render distance is `RENDER_DISTANCE` 8. That is a client setting, not a world property.

The horizontal border is the vanilla default: the playable area is about 60 million blocks across, centered on the origin. The first server can reject travel past 30,000,000. It does not need the full border animation yet.

## Light

Two channels, each 0 through 15.

- Sky light falls from the top of the column through transparent blocks, then spreads.
- Block light comes from sources (`torch` 14, `glowstone` 15, `lava` 15, and the rest of the light table on the block properties).
- A face renders at the max of the two channels. Night darkens sky light. The Nether has no sky light. The End has sky light and a fixed clock.

The client does not compute light. Meshes are fully lit. The light slice fills a nibble array per section and bakes the level into the mesh. Torches and sun have to agree with the debug screen before any shader work depends on it.

Monster spawning later reads `monster_spawn_light_level` and `monster_spawn_block_light_limit` from the dimension. The zombie slice is the first consumer. Do not spawn mobs in the light slice.

## Clock and weather

The overworld clock advances one tick per tick. Midnight is the vanilla night window the bed rule calls "when dark". The Nether and End do not advance time.

Rain and thunder are a later system. They change sky color and the fluid tick in some biomes. They are not required for the daylight slice, which only has to move the sun color from the overworld sky and fog values above.

## Biomes and structures

68 biome ids, all missing. `registry/biomes.md`. Generation today is hills, a water level, sand under water, and oak-style trees from `ShouldPlaceTree`.

The first biome slice is plains, forest, desert, and ocean. It replaces the single surface in those columns and leaves every other biome id as "not generated". Structures (52 ids in the catalog) stay missing until a village or stronghold slice is opened. Do not scatter structure stubs through the biome slice.

## Block states

Modern ids are already flattened: `oak_stairs` is one id, and facing, half, shape, and waterlogged are state. The cube stand-ins ignore state. The state slice stores a state word on the block and implements `oak_stairs` plus `oak_door` end to end, including collision that is not a full cube. Every later stair and door copies that path. Do not add 1,144 special cases in the mesher.
